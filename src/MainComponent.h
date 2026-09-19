#pragma once

#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include "AudioEngine.h"
#include "MeterComponent.h"
#include "MidiComponent.h"
#include "MixerComponent.h"
#include "ProcessorControlsComponent.h"
#include "TimelineComponent.h"

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
    juce::TextButton transportPlayButton;
    juce::TextButton transportPauseButton;
    juce::TextButton transportStopButton;
    juce::Label tempoLabel;
    juce::Slider tempoSlider;
    juce::TextEditor transportStatus;
    juce::TabbedComponent viewTabs;
    std::unique_ptr<ProcessorControlsComponent> processorControls;
    std::unique_ptr<MixerComponent> mixer;
    std::unique_ptr<TimelineComponent> timeline;
    std::unique_ptr<MidiComponent> midi;
    MeterComponent inputMeter;
    MeterComponent outputMeter;
    juce::TextEditor statusLabel;

    float displayedInputLevel = 0.0f;
    float displayedOutputLevel = 0.0f;
    int lastClipCount = 0;
    juce::Time lastClipTime;

    juce::AudioDeviceManager deviceManager;
    juce::OwnedArray<juce::AudioIODeviceType> deviceTypes;
    AudioEngine engine;
};
} // namespace daw
