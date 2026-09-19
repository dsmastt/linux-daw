#pragma once

#include <atomic>

#include <juce_audio_devices/juce_audio_devices.h>

namespace daw
{
class AudioEngine final : public juce::AudioIODeviceCallback
{
public:
    AudioEngine() = default;
    ~AudioEngine() override;

    juce::String start (const juce::String& inputDeviceName,
                        const juce::String& outputDeviceName,
                        double sampleRate,
                        int bufferSize);

    void stop();

    bool isRunning() const noexcept { return running.load(); }
    double getSampleRate() const noexcept { return sampleRate.load(); }
    int getBufferSize() const noexcept { return bufferSize.load(); }

private:
    void audioDeviceIOCallbackWithContext (const float* const* inputChannelData,
                                           int numInputChannels,
                                           float* const* outputChannelData,
                                           int numOutputChannels,
                                           int numSamples,
                                           const juce::AudioIODeviceCallbackContext& context) override;

    void audioDeviceAboutToStart (juce::AudioIODevice* device) override;
    void audioDeviceStopped() override;

    juce::AudioDeviceManager deviceManager;
    std::atomic<bool> running { false };
    std::atomic<double> sampleRate { 0.0 };
    std::atomic<int> bufferSize { 0 };
};
} // namespace daw
