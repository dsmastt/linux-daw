#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "AudioGraph.h"
#include "UndoManager.h"

namespace daw
{
class ChannelStripComponent final : public juce::Component
{
public:
    ChannelStripComponent (Track& trackToUse, const juce::String& nameToUse, AudioGraph& graphToUse, UndoManager& undoToUse);

    void resized() override;
    void refreshPluginStatus();
    void mouseDown (const juce::MouseEvent& event) override;

private:
    Track& track;
    AudioGraph& graph;
    UndoManager& undo;
    juce::String name;
    juce::Label nameLabel;
    juce::TextEditor pluginStatus;
    juce::Label volumeLabel;
    juce::Slider volumeSlider;
    juce::Label panLabel;
    juce::Slider panSlider;
    juce::ToggleButton muteButton;
    juce::ToggleButton soloButton;
};

class MixerComponent final : public juce::Component,
                             public juce::Timer
{
public:
    explicit MixerComponent (AudioGraph& graphToUse, UndoManager& undoToUse);
    ~MixerComponent() override;

    void resized() override;
    void timerCallback() override;
    void refreshFromGraph();

private:
    AudioGraph& graph;
    UndoManager& undo;
    std::vector<std::unique_ptr<ChannelStripComponent>> channelStrips;
};
} // namespace daw
