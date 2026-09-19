#pragma once

#include <atomic>
#include <cstdint>

#include <juce_audio_devices/juce_audio_devices.h>

#include "AudioGraph.h"
#include "AudioRecorder.h"
#include "MidiInputHandler.h"

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
    int getInputLatencySamples() const noexcept { return inputLatencySamples.load(); }
    int getOutputLatencySamples() const noexcept { return outputLatencySamples.load(); }

    int getNumInputChannels() const noexcept { return numInputChannels.load(); }
    int getNumOutputChannels() const noexcept { return numOutputChannels.load(); }
    std::uint64_t getNumCallbacks() const noexcept { return numCallbacks.load(); }
    double getTotalCallbackTimeMs() const noexcept { return totalCallbackTimeMs.load(); }
    double getMaxCallbackTimeMs() const noexcept { return maxCallbackTimeMs.load(); }
    int getXRunCount() const noexcept { return xruns.load(); }

    float getInputPeak() const noexcept { return inputPeak.load(); }
    float getOutputPeak() const noexcept { return outputPeak.load(); }
    float consumeInputPeak() noexcept { return inputPeak.exchange (0.0f); }
    float consumeOutputPeak() noexcept { return outputPeak.exchange (0.0f); }
    int getClipCount() const noexcept { return clipCounter.load(); }

    AudioGraph& getAudioGraph() noexcept { return audioGraph; }

    juce::String startRecording (const juce::File& file);
    void stopRecording();
    bool isRecording() const noexcept { return recorder.isRecording(); }
    std::uint64_t getFramesWritten() const noexcept { return recorder.getFramesWritten(); }
    int getDroppedRecordingBlocks() const noexcept { return recorder.getDroppedBlocks(); }

    juce::String openMidiDevice (const juce::String& identifier);
    juce::String openVirtualMidiDevice (const juce::String& name);
    void closeMidiDevice() noexcept;
    bool isMidiDeviceOpen() const noexcept { return midiHandler.isOpen(); }
    MidiInputHandler& getMidiHandler() noexcept { return midiHandler; }

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
    std::atomic<float> inputPeak { 0.0f };
    std::atomic<float> outputPeak { 0.0f };
    std::atomic<int> clipCounter { 0 };

    juce::AudioIODevice* currentDevice = nullptr;
    int callbacksSinceXRunCheck = 0;
    AudioGraph audioGraph;
    AudioRecorder recorder;
    MidiInputHandler midiHandler;
    juce::MidiBuffer midiBuffer;
};
} // namespace daw
