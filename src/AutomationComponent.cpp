#include "AutomationComponent.h"

#include <algorithm>
#include <cmath>

namespace daw
{
AutomationComponent::AutomationComponent (AudioEngine& engineToUse, UndoManager& undoToUse)
    : engine (engineToUse),
      graph (engineToUse.getAudioGraph()),
      undo (undoToUse)
{
    for (int i = 0; i < AudioGraph::numTracks; ++i)
        trackCombo.addItem ("Track " + juce::String (i + 1), i + 1);

    trackCombo.setSelectedItemIndex (0);
    trackCombo.onChange = [this]
    {
        selectedTrackIndex = trackCombo.getSelectedItemIndex();
        selectedPoint = -1;
    };
    addAndMakeVisible (trackCombo);

    parameterCombo.addItem ("Volume (dB)", 1);
    parameterCombo.addItem ("Pan", 2);
    parameterCombo.addItem ("Gain (dB)", 3);
    parameterCombo.addItem ("Plugin Parameter", 4);
    parameterCombo.setSelectedItemIndex (0);
    parameterCombo.onChange = [this]
    {
        selectedKind = static_cast<ParameterKind> (parameterCombo.getSelectedItemIndex());
        selectedPoint = -1;
    };
    addAndMakeVisible (parameterCombo);

    clearButton.setButtonText ("Clear Curve");
    clearButton.onClick = [this]
    {
        undo.captureUndoState (graph);
        clearCurrentCurve();
    };
    addAndMakeVisible (clearButton);

    statusText.setMultiLine (false, false);
    statusText.setReadOnly (true);
    statusText.setCaretVisible (false);
    statusText.setText ("Click to add automation points, drag to move them", juce::dontSendNotification);
    addAndMakeVisible (statusText);

    startTimerHz (10);
}

AutomationComponent::~AutomationComponent()
{
    stopTimer();
}

double AutomationComponent::getPixelsPerSample() const noexcept
{
    const double rate = engine.isRunning() && engine.getSampleRate() > 0.0 ? engine.getSampleRate() : 48000.0;
    return pixelsPerSecond / rate;
}

float AutomationComponent::valueAtY (int y) const noexcept
{
    auto bounds = getLocalBounds();
    bounds.removeFromTop (40);

    const auto canvasBounds = bounds.reduced (8);
    const float relative = 1.0f - static_cast<float> (y - canvasBounds.getY()) / static_cast<float> (canvasBounds.getHeight());

    return std::clamp (relative, 0.0f, 1.0f);
}

int AutomationComponent::yForValue (float value) const noexcept
{
    auto bounds = getLocalBounds();
    bounds.removeFromTop (40);

    const auto canvasBounds = bounds.reduced (8);
    const auto clamped = std::clamp (value, 0.0f, 1.0f);

    return canvasBounds.getBottom() - static_cast<int> (clamped * canvasBounds.getHeight());
}

AutomationParameter AutomationComponent::parameterForKind (ParameterKind kind) const noexcept
{
    switch (kind)
    {
        case ParameterKind::volume: return AutomationParameter::trackVolumeDb;
        case ParameterKind::pan:    return AutomationParameter::trackPan;
        case ParameterKind::gain:   return AutomationParameter::gainDb;
        case ParameterKind::plugin: return AutomationParameter::pluginParameter;
    }

    return AutomationParameter::trackVolumeDb;
}

void AutomationComponent::resized()
{
    auto bounds = getLocalBounds().reduced (8);

    auto controlRow = bounds.removeFromTop (32);
    trackCombo.setBounds (controlRow.removeFromLeft (90));
    controlRow.removeFromLeft (8);
    parameterCombo.setBounds (controlRow.removeFromLeft (130));
    controlRow.removeFromLeft (8);
    clearButton.setBounds (controlRow.removeFromLeft (100));
    controlRow.removeFromLeft (8);
    statusText.setBounds (controlRow);
}

void AutomationComponent::timerCallback()
{
    repaint();
}

void AutomationComponent::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds();
    bounds.removeFromTop (40);

    const auto canvasBounds = bounds.reduced (8).toFloat();
    const float canvasLeft = canvasBounds.getX() + gutterWidth;
    const float canvasWidth = canvasBounds.getWidth() - gutterWidth;

    g.setColour (juce::Colours::darkgrey.darker());
    g.fillRect (canvasBounds);

    const double pixelsPerSample = getPixelsPerSample();

    g.setColour (juce::Colours::darkgrey);
    g.fillRect (canvasBounds.getX(), canvasBounds.getY(), static_cast<float> (gutterWidth), canvasBounds.getHeight());

    g.setColour (juce::Colours::white);
    g.setFont (juce::FontOptions{}.withHeight (12.0f));
    g.drawText ("1.0", static_cast<int> (canvasBounds.getX()), static_cast<int> (canvasBounds.getY()) - 8, 40, 14, juce::Justification::left, false);
    g.drawText ("0.0", static_cast<int> (canvasBounds.getX()), static_cast<int> (canvasBounds.getBottom()) - 6, 40, 14, juce::Justification::left, false);

    for (int second = 0; second < 30; ++second)
    {
        const float x = canvasLeft + second * pixelsPerSecond;

        if (x > canvasLeft + canvasWidth)
            break;

        g.setColour (juce::Colours::grey.withAlpha (0.3f));
        g.drawVerticalLine (static_cast<int> (x), canvasBounds.getY(), canvasBounds.getBottom());
    }

    if (selectedTrackIndex < 0 || selectedTrackIndex >= AudioGraph::numTracks)
        return;

    auto& track = graph.getTrack (selectedTrackIndex);
    const auto parameter = parameterForKind (selectedKind);
    const int pluginParameterIndex = selectedKind == ParameterKind::plugin ? 0 : -1;

    const int numCurves = track.getNumAutomationCurves();
    const AutomationCurve* curve = nullptr;

    for (int i = 0; i < numCurves; ++i)
    {
        const auto& candidate = track.getAutomationCurve (i);

        if (candidate.parameter == parameter && candidate.pluginParameterIndex == pluginParameterIndex)
        {
            curve = &candidate;
            break;
        }
    }

    if (curve == nullptr)
        return;

    const int pointCount = curve->pointCount.load();

    juce::Path path;
    bool pathStarted = false;

    for (int i = 0; i < pointCount; ++i)
    {
        const auto& point = curve->points[static_cast<size_t> (i)];

        const float x = canvasLeft + static_cast<float> (point.timeSamples.load() * pixelsPerSample);
        const float y = static_cast<float> (yForValue (point.value.load()));

        if (pointCount == 1)
        {
            g.setColour (juce::Colours::orange);
            g.fillEllipse (x - 4.0f, y - 4.0f, 8.0f, 8.0f);
            return;
        }

        if (!pathStarted)
        {
            path.startNewSubPath (x, y);
            pathStarted = true;
        }
        else
        {
            path.lineTo (x, y);
        }

        const bool isSelected = i == selectedPoint;

        g.setColour (isSelected ? juce::Colours::white : juce::Colours::orange);
        g.fillEllipse (x - 4.0f, y - 4.0f, 8.0f, 8.0f);
    }

    g.setColour (juce::Colours::orange);
    g.strokePath (path, juce::PathStrokeType (2.0f));

    const float playheadX = canvasLeft + static_cast<float> (graph.getTransport().getPositionSamples() * pixelsPerSample);

    if (playheadX >= canvasLeft && playheadX <= canvasLeft + canvasWidth)
    {
        g.setColour (juce::Colours::red);
        g.drawVerticalLine (static_cast<int> (playheadX), canvasBounds.getY(), canvasBounds.getBottom());
    }
}

int AutomationComponent::findPointAt (int x, int y) const noexcept
{
    if (selectedTrackIndex < 0 || selectedTrackIndex >= AudioGraph::numTracks)
        return -1;

    auto& track = graph.getTrack (selectedTrackIndex);
    const auto parameter = parameterForKind (selectedKind);
    const int pluginParameterIndex = selectedKind == ParameterKind::plugin ? 0 : -1;

    const int numCurves = track.getNumAutomationCurves();

    for (int i = 0; i < numCurves; ++i)
    {
        const auto& candidate = track.getAutomationCurve (i);

        if (candidate.parameter != parameter || candidate.pluginParameterIndex != pluginParameterIndex)
            continue;

        const int pointCount = candidate.pointCount.load();
        const double pixelsPerSample = getPixelsPerSample();

        auto bounds = getLocalBounds();
        bounds.removeFromTop (40);
        const auto canvasBounds = bounds.reduced (8);
        const float canvasLeft = canvasBounds.getX() + gutterWidth;

        for (int p = 0; p < pointCount; ++p)
        {
            const auto& point = candidate.points[static_cast<size_t> (p)];

            const float pointX = canvasLeft + static_cast<float> (point.timeSamples.load() * pixelsPerSample);
            const float pointY = static_cast<float> (yForValue (point.value.load()));

            if (std::abs (x - static_cast<int> (pointX)) < 8 && std::abs (y - static_cast<int> (pointY)) < 8)
                return p;
        }
    }

    return -1;
}

void AutomationComponent::mouseDown (const juce::MouseEvent& event)
{
    undo.captureUndoState (graph);

    selectedPoint = findPointAt (event.getPosition().getX(), event.getPosition().getY());

    if (selectedPoint < 0)
    {
        if (selectedTrackIndex < 0 || selectedTrackIndex >= AudioGraph::numTracks)
            return;

        auto bounds = getLocalBounds();
        bounds.removeFromTop (40);
        const auto canvasBounds = bounds.reduced (8);
        const float canvasLeft = canvasBounds.getX() + gutterWidth;
        const float canvasRight = canvasBounds.getRight();

        if (event.getPosition().getX() < canvasLeft || event.getPosition().getX() > canvasRight)
            return;

        const double pixelsPerSample = getPixelsPerSample();
        const auto timeSamples = static_cast<std::int64_t> ((event.getPosition().getX() - canvasLeft) / pixelsPerSample);

        auto& track = graph.getTrack (selectedTrackIndex);
        const auto parameter = parameterForKind (selectedKind);
        const int pluginParameterIndex = selectedKind == ParameterKind::plugin ? 0 : -1;

        float value = valueAtY (event.getPosition().getY());

        if (selectedKind == ParameterKind::volume)
            value = (value - 0.5f) * 120.0f;
        else if (selectedKind == ParameterKind::gain)
            value = (value - 0.5f) * 120.0f;
        else if (selectedKind == ParameterKind::pan)
            value = (value - 0.5f) * 2.0f;

        track.addAutomationPoint (parameter, pluginParameterIndex, timeSamples, value);

        selectedPoint = findPointAt (event.getPosition().getX(), event.getPosition().getY());

        dragging = true;
    }
    else
    {
        dragging = true;
    }
}

void AutomationComponent::mouseDrag (const juce::MouseEvent& event)
{
    if (!dragging || selectedPoint < 0 || selectedTrackIndex < 0 || selectedTrackIndex >= AudioGraph::numTracks)
        return;

    auto bounds = getLocalBounds();
    bounds.removeFromTop (40);
    const auto canvasBounds = bounds.reduced (8);
    const float canvasLeft = canvasBounds.getX() + gutterWidth;

    const double pixelsPerSample = getPixelsPerSample();
    const auto timeSamples = std::max<std::int64_t> (0, static_cast<std::int64_t> ((event.getPosition().getX() - canvasLeft) / pixelsPerSample));

    float value = valueAtY (event.getPosition().getY());

    if (selectedKind == ParameterKind::volume || selectedKind == ParameterKind::gain)
        value = (value - 0.5f) * 120.0f;
    else if (selectedKind == ParameterKind::pan)
        value = (value - 0.5f) * 2.0f;

    auto& track = graph.getTrack (selectedTrackIndex);
    const auto parameter = parameterForKind (selectedKind);
    const int pluginParameterIndex = selectedKind == ParameterKind::plugin ? 0 : -1;

    const int numCurves = track.getNumAutomationCurves();

    for (int i = 0; i < numCurves; ++i)
    {
        const auto& candidate = track.getAutomationCurve (i);

        if (candidate.parameter != parameter || candidate.pluginParameterIndex != pluginParameterIndex)
            continue;

        auto& editableCurve = const_cast<AutomationCurve&> (candidate);

        if (selectedPoint < editableCurve.pointCount.load())
        {
            editableCurve.points[static_cast<size_t> (selectedPoint)].timeSamples.store (timeSamples, std::memory_order_relaxed);
            editableCurve.points[static_cast<size_t> (selectedPoint)].value.store (value, std::memory_order_relaxed);
        }

        break;
    }
}

void AutomationComponent::mouseUp (const juce::MouseEvent&)
{
    dragging = false;
}

void AutomationComponent::clearCurrentCurve()
{
    if (selectedTrackIndex < 0 || selectedTrackIndex >= AudioGraph::numTracks)
        return;

    auto& track = graph.getTrack (selectedTrackIndex);
    const auto parameter = parameterForKind (selectedKind);
    const int pluginParameterIndex = selectedKind == ParameterKind::plugin ? 0 : -1;

    track.clearAutomation (parameter, pluginParameterIndex);

    selectedPoint = -1;
}
} // namespace daw
