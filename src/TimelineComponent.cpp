#include "TimelineComponent.h"

#include <algorithm>

namespace daw
{
TimelineComponent::TimelineComponent (AudioEngine& engineToUse)
    : engine (engineToUse),
      graph (engineToUse.getAudioGraph())
{
    loadFileButton.setButtonText ("Load Clip File...");
    loadFileButton.onClick = [this] { loadClipFile(); };
    addAndMakeVisible (loadFileButton);

    for (int i = 0; i < AudioGraph::numTracks; ++i)
        trackCombo.addItem ("Track " + juce::String (i + 1), i + 1);

    trackCombo.setSelectedItemIndex (0);
    addAndMakeVisible (trackCombo);

    addClipButton.setButtonText ("Add Clip");
    addClipButton.onClick = [this] { addClipToSelectedTrack(); };
    addAndMakeVisible (addClipButton);

    deleteClipButton.setButtonText ("Delete Clip");
    deleteClipButton.setEnabled (false);
    deleteClipButton.onClick = [this] { deleteSelectedClip(); };
    addAndMakeVisible (deleteClipButton);

    statusText.setMultiLine (false, false);
    statusText.setReadOnly (true);
    statusText.setCaretVisible (false);
    statusText.setText ("No clip file loaded", juce::dontSendNotification);
    addAndMakeVisible (statusText);

    startTimerHz (10);
}

TimelineComponent::~TimelineComponent()
{
    stopTimer();
}

double TimelineComponent::getPixelsPerSample() const noexcept
{
    const double rate = engine.isRunning() && engine.getSampleRate() > 0.0 ? engine.getSampleRate() : 48000.0;
    return pixelsPerSecond / rate;
}

void TimelineComponent::resized()
{
    auto bounds = getLocalBounds().reduced (8);

    auto controlRow = bounds.removeFromTop (32);
    loadFileButton.setBounds (controlRow.removeFromLeft (130));
    controlRow.removeFromLeft (8);
    trackCombo.setBounds (controlRow.removeFromLeft (90));
    controlRow.removeFromLeft (8);
    addClipButton.setBounds (controlRow.removeFromLeft (90));
    controlRow.removeFromLeft (8);
    deleteClipButton.setBounds (controlRow.removeFromLeft (90));
    controlRow.removeFromLeft (8);
    statusText.setBounds (controlRow);
}

void TimelineComponent::timerCallback()
{
    repaint();
}

void TimelineComponent::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds();
    bounds.removeFromTop (40);

    const auto canvasBounds = bounds.reduced (8).toFloat();
    const float canvasLeft = canvasBounds.getX() + gutterWidth;
    const float canvasWidth = canvasBounds.getWidth() - gutterWidth;

    g.setColour (juce::Colours::darkgrey.darker());
    g.fillRect (canvasBounds);

    const double pixelsPerSample = getPixelsPerSample();

    g.setColour (juce::Colours::black);

    for (int track = 0; track < AudioGraph::numTracks; ++track)
    {
        const auto rowTop = canvasBounds.getY() + track * rowHeight;
        const auto rowBottom = std::min (rowTop + rowHeight, canvasBounds.getBottom());

        g.setColour (juce::Colours::darkgrey);
        g.fillRect (canvasBounds.getX(), rowTop, static_cast<float> (gutterWidth), rowBottom - rowTop);

        g.setColour (juce::Colours::white);
        g.setFont (juce::FontOptions{}.withHeight (14.0f));
        g.drawText ("Track " + juce::String (track + 1),
                    juce::Rectangle<int> (static_cast<int> (canvasBounds.getX()), static_cast<int> (rowTop), gutterWidth, rowHeight),
                    juce::Justification::centred, false);

        g.setColour (juce::Colours::grey);
        g.drawRect (canvasBounds.getX(), rowTop, canvasBounds.getWidth(), rowBottom - rowTop);

        const auto& currentTrack = graph.getTrack (track);
        const auto numClips = currentTrack.getNumClips();

        for (int clipIndex = 0; clipIndex < numClips; ++clipIndex)
        {
            const auto& clip = currentTrack.getClip (clipIndex);

            const float clipX = canvasLeft + static_cast<float> (clip.startSamples.load() * pixelsPerSample);
            const float clipWidth = static_cast<float> (clip.lengthSamples.load() * pixelsPerSample);

            if (clipX + clipWidth < canvasLeft || clipX > canvasLeft + canvasWidth)
                continue;

            const bool isSelected = track == selectedTrack && clipIndex == selectedClip;

            g.setColour (isSelected ? juce::Colours::lightblue : juce::Colours::steelblue);
            g.fillRect (clipX, rowTop + 4.0f, clipWidth, static_cast<float> (rowHeight - 8));

            g.setColour (isSelected ? juce::Colours::white : juce::Colours::black);
            g.drawRect (clipX, rowTop + 4.0f, clipWidth, static_cast<float> (rowHeight - 8));
        }
    }

    for (int second = 0; second < 30; ++second)
    {
        const float x = canvasLeft + second * pixelsPerSecond;

        if (x > canvasLeft + canvasWidth)
            break;

        g.setColour (juce::Colours::grey.withAlpha (0.3f));
        g.drawVerticalLine (static_cast<int> (x), canvasBounds.getY(), canvasBounds.getBottom());

        g.setColour (juce::Colours::white);
        g.setFont (juce::FontOptions{}.withHeight (12.0f));
        g.drawText (juce::String (second) + "s", static_cast<int> (x) + 2, static_cast<int> (canvasBounds.getY()) - 14, 40, 14, juce::Justification::left, false);
    }

    const float playheadX = canvasLeft + static_cast<float> (graph.getTransport().getPositionSamples() * pixelsPerSample);

    if (playheadX >= canvasLeft && playheadX <= canvasLeft + canvasWidth)
    {
        g.setColour (juce::Colours::red);
        g.drawVerticalLine (static_cast<int> (playheadX), canvasBounds.getY(), canvasBounds.getBottom());
    }
}

int TimelineComponent::trackIndexAtY (int y) const noexcept
{
    auto bounds = getLocalBounds();
    bounds.removeFromTop (40);

    const auto canvasBounds = bounds.reduced (8);
    const int relativeY = y - canvasBounds.getY();

    if (relativeY < 0)
        return -1;

    const int track = relativeY / rowHeight;

    return track < AudioGraph::numTracks ? track : -1;
}

int TimelineComponent::hitTestClip (int trackIndex, int x, int, bool& onResizeEdge) const noexcept
{
    onResizeEdge = false;

    if (trackIndex < 0)
        return -1;

    const double pixelsPerSample = getPixelsPerSample();

    auto bounds = getLocalBounds();
    bounds.removeFromTop (40);

    const auto canvasBounds = bounds.reduced (8);
    const float canvasLeft = canvasBounds.getX() + gutterWidth;

    const auto& track = graph.getTrack (trackIndex);
    const auto numClips = track.getNumClips();

    for (int clipIndex = numClips - 1; clipIndex >= 0; --clipIndex)
    {
        const auto& clip = track.getClip (clipIndex);

        const float clipX = canvasLeft + static_cast<float> (clip.startSamples.load() * pixelsPerSample);
        const float clipWidth = static_cast<float> (clip.lengthSamples.load() * pixelsPerSample);

        if (x >= clipX && x < clipX + clipWidth)
        {
            onResizeEdge = x >= clipX + clipWidth - 8.0f;
            return clipIndex;
        }
    }

    return -1;
}

void TimelineComponent::mouseDown (const juce::MouseEvent& event)
{
    const int track = trackIndexAtY (event.getPosition().getY());

    bool onResizeEdge = false;
    const int clip = hitTestClip (track, event.getPosition().getX(), event.getPosition().getY(), onResizeEdge);

    selectedTrack = track;
    selectedClip = clip;
    deleteClipButton.setEnabled (clip >= 0);

    if (clip >= 0)
    {
        dragging = true;
        resizing = onResizeEdge;

        const double pixelsPerSample = getPixelsPerSample();
        auto bounds = getLocalBounds();
        bounds.removeFromTop (40);
        const auto canvasBounds = bounds.reduced (8);
        const float canvasLeft = canvasBounds.getX() + gutterWidth;

        const auto& currentClip = graph.getTrack (track).getClip (clip);
        const float clipX = canvasLeft + static_cast<float> (currentClip.startSamples.load() * pixelsPerSample);
        dragOffsetPixels = event.getPosition().getX() - static_cast<int> (clipX);
    }
}

void TimelineComponent::mouseDrag (const juce::MouseEvent& event)
{
    if (!dragging || selectedTrack < 0 || selectedClip < 0)
        return;

    const double pixelsPerSample = getPixelsPerSample();

    auto bounds = getLocalBounds();
    bounds.removeFromTop (40);
    const auto canvasBounds = bounds.reduced (8);
    const float canvasLeft = canvasBounds.getX() + gutterWidth;

    auto& track = graph.getTrack (selectedTrack);
    auto& clip = track.getClip (selectedClip);

    const float clipX = canvasLeft + static_cast<float> (clip.startSamples.load() * pixelsPerSample);

    if (resizing)
    {
        const auto newWidthPixels = event.getPosition().getX() - static_cast<int> (clipX);
        const auto newLength = std::max<std::int64_t> (1, static_cast<std::int64_t> (newWidthPixels / pixelsPerSample));
        track.resizeClip (selectedClip, newLength);
    }
    else
    {
        const float newX = static_cast<float> (event.getPosition().getX() - dragOffsetPixels);
        const auto newStart = std::max<std::int64_t> (0, static_cast<std::int64_t> ((newX - canvasLeft) / pixelsPerSample));
        track.moveClip (selectedClip, newStart);
    }
}

void TimelineComponent::mouseUp (const juce::MouseEvent&)
{
    dragging = false;
    resizing = false;
}

void TimelineComponent::loadClipFile()
{
    auto chooser = std::make_shared<juce::FileChooser> ("Load Clip Audio File", juce::File(), "*.wav");

    chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                          [this] (const juce::FileChooser& fc)
                          {
                              const auto result = fc.getResult();

                              juce::MessageManager::callAsync ([this, result]
                              {
                                  if (result == juce::File())
                                      return;

                                  const auto error = graph.loadPlaybackFile (result);

                                  if (error.isNotEmpty())
                                  {
                                      statusText.setText ("Error: " + error, juce::dontSendNotification);
                                      return;
                                  }

                                  statusText.setText ("Loaded: " + result.getFileName(), juce::dontSendNotification);
                              });
                          });
}

void TimelineComponent::addClipToSelectedTrack()
{
    const auto frames = graph.getPlaybackLengthFrames();

    if (frames <= 0)
    {
        statusText.setText ("No clip file loaded", juce::dontSendNotification);
        return;
    }

    const int trackIndex = trackCombo.getSelectedItemIndex();

    if (trackIndex < 0)
        return;

    auto& track = graph.getTrack (trackIndex);

    const auto position = graph.getTransport().getPositionSamples();

    track.addClip (position, frames, 0);

    statusText.setText ("Clip added to track " + juce::String (trackIndex + 1), juce::dontSendNotification);
}

void TimelineComponent::deleteSelectedClip()
{
    if (selectedTrack < 0 || selectedClip < 0)
        return;

    graph.getTrack (selectedTrack).removeClip (selectedClip);

    selectedClip = -1;
    deleteClipButton.setEnabled (false);
}
} // namespace daw
