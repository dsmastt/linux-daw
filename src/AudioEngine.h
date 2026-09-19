#pragma once

#include <atomic>
#include <cstdint>

#include <juce_audio_devices/juce_audio_devices.h>

#include "DistortionShaper.h"
#include "ProcessorChain.h"

namespace daw
{
class CompressorProcessor;
class ConvolutionProcessor;
class DistortionProcessor;
class GainProcessor;
class NoiseGateProcessor;
class ParametricEqProcessor;

class AudioEngine final : public juce::AudioIODeviceCallback
{
public:
    AudioEngine();
    ~AudioEngine() override;

    juce::String start (const juce::String& inputDeviceName,
                        const juce::String& outputDeviceName,
                        double sampleRate,
                        int bufferSize);

    void stop();

    void setGainDb (float gainDb) noexcept;
    float getGainDb() const noexcept;

    void setGateThresholdDb (float thresholdDb) noexcept;
    float getGateThresholdDb() const noexcept;
    void setGateAttackMs (float attackMs) noexcept;
    float getGateAttackMs() const noexcept;
    void setGateReleaseMs (float releaseMs) noexcept;
    float getGateReleaseMs() const noexcept;

    void setEqBandParameters (int band, double frequencyHz, float gainDb, float q) noexcept;
    int getNumEqBands() const noexcept;

    void setCompressorThresholdDb (float thresholdDb) noexcept;
    void setCompressorRatio (float ratio) noexcept;
    void setCompressorAttackMs (float attackMs) noexcept;
    void setCompressorReleaseMs (float releaseMs) noexcept;
    void setCompressorMakeupGainDb (float makeupGainDb) noexcept;

    void setDistortionDriveDb (float driveDb) noexcept;
    void setDistortionTone (float tone) noexcept;
    void setDistortionMode (DistortionMode mode) noexcept;

    juce::String loadImpulseResponse (const juce::File& file);
    void clearImpulseResponse() noexcept;
    bool hasImpulseResponse() const noexcept;
    int getImpulseResponseLength() const noexcept;

    int getNumProcessors() const noexcept { return processorChain.size(); }

    bool isRunning() const noexcept { return running.load(); }
    double getSampleRate() const noexcept { return sampleRate.load(); }
    int getBufferSize() const noexcept { return bufferSize.load(); }
    int getInputLatencySamples() const noexcept { return inputLatencySamples.load(); }
    int getOutputLatencySamples() const noexcept { return outputLatencySamples.load(); }

    int getNumInputChannels() const noexcept { return numInputChannels.load(); }
    int getNumOutputChannels() const noexcept { return numOutputChannels.load(); }
    std::uint64_t getNumCallbacks() const noexcept { return numCallbacks.load(); }
    double getTotalCallbackTimeMs() const noexcept { return totalCallbackTimeMs.load(); }
    double getMaxCallbackTimeMs() const noexcept { return maxCallbackTimeMs.load(); }
    int getXRunCount() const noexcept { return xruns.load(); }

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
    std::atomic<int> inputLatencySamples { 0 };
    std::atomic<int> outputLatencySamples { 0 };
    std::atomic<int> numInputChannels { 0 };
    std::atomic<int> numOutputChannels { 0 };
    std::atomic<std::uint64_t> numCallbacks { 0 };
    std::atomic<double> totalCallbackTimeMs { 0.0 };
    std::atomic<double> maxCallbackTimeMs { 0.0 };
    std::atomic<int> xruns { 0 };

    juce::AudioIODevice* currentDevice = nullptr;
    int callbacksSinceXRunCheck = 0;
    ProcessorChain processorChain;
    GainProcessor* gainProcessor = nullptr;
    NoiseGateProcessor* gateProcessor = nullptr;
    ParametricEqProcessor* eqProcessor = nullptr;
    CompressorProcessor* compressorProcessor = nullptr;
    DistortionProcessor* distortionProcessor = nullptr;
    ConvolutionProcessor* convolutionProcessor = nullptr;
};
} // namespace daw
