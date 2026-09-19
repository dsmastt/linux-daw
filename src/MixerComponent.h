#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "AudioGraph.h"

namespace daw
{
class ChannelStripComponent final : public juce::Component
{
public:
    ChannelStripComponent (Track& trackToUse, const juce::String& nameToUse);

    void resized() override;
    void refreshPluginStatus();

private:
    Track& track;
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
    explicit MixerComponent (AudioGraph& graphToUse);
    ~MixerComponent() override;

    void resized() override;
    void timerCallback() override;

private:
    AudioGraph& graph;
    std::vector<std::unique_ptr<ChannelStripComponent>> channelStrips;
};
} // namespace daw
