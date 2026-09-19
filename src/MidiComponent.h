#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "AudioEngine.h"

namespace daw
{
class MidiComponent final : public juce::Component,
                            public juce::Timer
{
public:
    explicit MidiComponent (AudioEngine& engineToUse);
    ~MidiComponent() override;

    void resized() override;
    void timerCallback() override;

private:
    void refreshDevices();
    void openSelectedDevice();
    void closeDevice();

    AudioEngine& engine;

    juce::TextButton refreshButton;
    juce::ComboBox deviceCombo;
    juce::TextButton openButton;
    juce::TextButton closeButton;
    juce::TextEditor statusText;
};
} // namespace daw
