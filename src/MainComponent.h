#pragma once

#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include "AudioEngine.h"

namespace daw
{
class MainComponent final : public juce::Component,
                            public juce::Timer
{
public:
    MainComponent();
    ~MainComponent() override;

    void resized() override;
    void timerCallback() override;

private:
    void refreshDevices();
    void startAudio();
    void stopAudio();
    void updateStatus();

    juce::Label titleLabel;
    juce::Label inputLabel;
    juce::ComboBox inputCombo;
    juce::Label outputLabel;
    juce::ComboBox outputCombo;
    juce::Label sampleRateLabel;
    juce::ComboBox sampleRateCombo;
    juce::Label bufferLabel;
    juce::ComboBox bufferCombo;
    juce::TextButton startButton;
    juce::TextButton stopButton;
    juce::TextEditor statusLabel;

    juce::AudioDeviceManager deviceManager;
    juce::OwnedArray<juce::AudioIODeviceType> deviceTypes;
    AudioEngine engine;
};
} // namespace daw
