#include "MixerComponent.h"

namespace daw
{
//==============================================================================
ChannelStripComponent::ChannelStripComponent (Track& trackToUse, const juce::String& nameToUse, AudioGraph& graphToUse, UndoManager& undoToUse)
    : track (trackToUse),
      graph (graphToUse),
      undo (undoToUse),
      name (nameToUse)
{
    nameLabel.setText (name, juce::dontSendNotification);
    nameLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (nameLabel);

    pluginStatus.setMultiLine (true, false);
    pluginStatus.setReadOnly (true);
    pluginStatus.setCaretVisible (false);
    pluginStatus.setColour (juce::TextEditor::backgroundColourId, juce::Colours::darkgrey);
    pluginStatus.setColour (juce::TextEditor::textColourId, juce::Colours::white);
    pluginStatus.setFont (juce::FontOptions{}.withHeight (12.0f));
    addAndMakeVisible (pluginStatus);

    refreshPluginStatus();

    volumeLabel.setText ("Volume", juce::dontSendNotification);
    volumeLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (volumeLabel);

    volumeSlider.setSliderStyle (juce::Slider::LinearVertical);
    volumeSlider.setRange (-60.0, 12.0, 0.5);
    volumeSlider.setValue (track.getVolumeDb(), juce::dontSendNotification);
    volumeSlider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 64, 20);
    volumeSlider.onDragStart = [this]
    {
        undo.captureUndoState (graph);
    };

    volumeSlider.onValueChange = [this]
    {
        track.setVolumeDb (static_cast<float> (volumeSlider.getValue()));
    };
    addAndMakeVisible (volumeSlider);

    panLabel.setText ("Pan", juce::dontSendNotification);
    panLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (panLabel);

    panSlider.setSliderStyle (juce::Slider::LinearHorizontal);
    panSlider.setRange (-1.0, 1.0, 0.01);
    panSlider.setValue (track.getPan(), juce::dontSendNotification);
    panSlider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 56, 20);
    panSlider.onDragStart = [this]
    {
        undo.captureUndoState (graph);
    };

    panSlider.onValueChange = [this]
    {
        track.setPan (static_cast<float> (panSlider.getValue()));
    };
    addAndMakeVisible (panSlider);

    routeLabel.setText ("Route", juce::dontSendNotification);
    routeLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (routeLabel);

    routeCombo.addItem ("Master", 1);
    routeCombo.addItem ("Bus 1", 2);
    routeCombo.addItem ("Bus 2", 3);
    routeCombo.setSelectedItemIndex (track.getRouteTarget() < 0 ? 0 : track.getRouteTarget() + 1);
    routeCombo.onChange = [this]
    {
        undo.captureUndoState (graph);
        track.setRouteTarget (routeCombo.getSelectedItemIndex() - 1);
    };
    addAndMakeVisible (routeCombo);

    sendLabel.setText ("Send", juce::dontSendNotification);
    sendLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (sendLabel);

    sendCombo.addItem ("None", 1);
    sendCombo.addItem ("Bus 1", 2);
    sendCombo.addItem ("Bus 2", 3);
    sendCombo.setSelectedItemIndex (track.getSendTarget() < 0 ? 0 : track.getSendTarget() + 1);
    sendCombo.onChange = [this]
    {
        undo.captureUndoState (graph);
        track.setSendTarget (sendCombo.getSelectedItemIndex() - 1);
    };
    addAndMakeVisible (sendCombo);

    sendSlider.setSliderStyle (juce::Slider::LinearHorizontal);
    sendSlider.setRange (0.0, 1.0, 0.01);
    sendSlider.setValue (track.getSendAmount(), juce::dontSendNotification);
    sendSlider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 56, 20);
    sendSlider.onDragStart = [this]
    {
        undo.captureUndoState (graph);
    };
    sendSlider.onValueChange = [this]
    {
        track.setSendAmount (static_cast<float> (sendSlider.getValue()));
    };
    addAndMakeVisible (sendSlider);

    muteButton.setButtonText ("M");
    muteButton.setToggleState (track.isMuted(), juce::dontSendNotification);
    muteButton.onClick = [this]
    {
        track.setMuted (muteButton.getToggleState());
    };
    addAndMakeVisible (muteButton);

    soloButton.setButtonText ("S");
    soloButton.setToggleState (track.isSoloed(), juce::dontSendNotification);
    soloButton.onClick = [this]
    {
        track.setSoloed (soloButton.getToggleState());
    };
    addAndMakeVisible (soloButton);
}

void ChannelStripComponent::resized()
{
    auto bounds = getLocalBounds().reduced (6);

    nameLabel.setBounds (bounds.removeFromTop (24));
    bounds.removeFromTop (4);

    pluginStatus.setBounds (bounds.removeFromTop (64));
    bounds.removeFromTop (6);

    volumeLabel.setBounds (bounds.removeFromTop (20));
    volumeSlider.setBounds (bounds.removeFromTop (bounds.getHeight() - 236));

    panLabel.setBounds (bounds.removeFromTop (20));
    panSlider.setBounds (bounds.removeFromTop (40));

    bounds.removeFromTop (6);

    routeLabel.setBounds (bounds.removeFromTop (20));
    routeCombo.setBounds (bounds.removeFromTop (28));

    bounds.removeFromTop (4);

    sendLabel.setBounds (bounds.removeFromTop (20));
    sendCombo.setBounds (bounds.removeFromTop (28));
    sendSlider.setBounds (bounds.removeFromTop (40));

    bounds.removeFromTop (6);

    auto buttons = bounds.removeFromTop (28);
    muteButton.setBounds (buttons.removeFromLeft (buttons.getWidth() / 2).reduced (2, 0));
    soloButton.setBounds (buttons.reduced (2, 0));
}

void ChannelStripComponent::mouseDown (const juce::MouseEvent& event)
{
    if (event.eventComponent == &muteButton || event.eventComponent == &soloButton)
        undo.captureUndoState (graph);
}

void ChannelStripComponent::refreshPluginStatus()
{
    if (track.isVst3PluginFailed())
    {
        pluginStatus.setText ("plugin failed\nremove to continue", juce::dontSendNotification);
        return;
    }

    const auto pluginName = track.getVst3PluginName();

    if (pluginName.isEmpty())
    {
        pluginStatus.setText ("no plugin", juce::dontSendNotification);
        return;
    }

    juce::String status = pluginName;

    if (track.isVst3PluginBypassed())
        status << "\n[bypassed]";

    pluginStatus.setText (status, juce::dontSendNotification);
}

//==============================================================================
MixerComponent::MixerComponent (AudioGraph& graphToUse, UndoManager& undoToUse)
    : graph (graphToUse),
      undo (undoToUse)
{
    for (int i = 0; i < AudioGraph::numTracks; ++i)
    {
        auto strip = std::make_unique<ChannelStripComponent> (graph.getTrack (i), "Track " + juce::String (i + 1), graph, undo);
        addAndMakeVisible (*strip);
        channelStrips.push_back (std::move (strip));
    }

    startTimerHz (2);
}

MixerComponent::~MixerComponent()
{
    stopTimer();
}

void MixerComponent::resized()
{
    auto bounds = getLocalBounds().reduced (6);

    const int stripWidth = bounds.getWidth() / AudioGraph::numTracks;

    for (auto& strip : channelStrips)
        strip->setBounds (bounds.removeFromLeft (stripWidth));
}

void MixerComponent::timerCallback()
{
    for (auto& strip : channelStrips)
        strip->refreshPluginStatus();
}

void MixerComponent::refreshFromGraph()
{
    repaint();
}
} // namespace daw
