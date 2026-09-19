#include "AudioEngine.h"

#include <algorithm>
#include <chrono>

#include "AtomicHelpers.h"

namespace daw
{

AudioEngine::~AudioEngine()
{
    stop();
}

juce::String AudioEngine::start (const juce::String& inputDeviceName,
                                 const juce::String& outputDeviceName,
                                 double sampleRateToUse,
                                 int bufferSizeToUse)
{
    stop();

    juce::AudioDeviceManager::AudioDeviceSetup setup;
    setup.inputDeviceName  = inputDeviceName;
    setup.outputDeviceName = outputDeviceName;
    setup.sampleRate       = sampleRateToUse;
    setup.bufferSize       = bufferSizeToUse;

    const auto error = deviceManager.initialise (2, 2, nullptr, false, {}, &setup);

    if (error.isNotEmpty())
        return error;

    deviceManager.addAudioCallback (this);
    running.store (true);
    return {};
}

void AudioEngine::stop()
{
    if (deviceManager.getCurrentAudioDevice() != nullptr)
    {
        deviceManager.removeAudioCallback (this);
        deviceManager.closeAudioDevice();
    }

    running.store (false);
    sampleRate.store (0.0);
    bufferSize.store (0);
    inputLatencySamples.store (0);
    outputLatencySamples.store (0);
    numInputChannels.store (0);
    numOutputChannels.store (0);

    recorder.stopRecording();
}

juce::String AudioEngine::startRecording (const juce::File& file)
{
    const auto rate = sampleRate.load();

    if (rate <= 0.0)
        return "Audio engine is not running";

    return recorder.startRecording (file, rate, numInputChannels.load());
}

void AudioEngine::stopRecording()
{
    recorder.stopRecording();
}

juce::String AudioEngine::openMidiDevice (const juce::String& identifier)
{
    return midiHandler.openDevice (identifier);
}

juce::String AudioEngine::openVirtualMidiDevice (const juce::String& name)
{
    return midiHandler.openVirtualDevice (name);
}

void AudioEngine::closeMidiDevice() noexcept
{
    midiHandler.closeDevice();
}

void AudioEngine::audioDeviceIOCallbackWithContext (const float* const* inputChannelData,
                                                    int numInputChannelsIn,
                                                    float* const* outputChannelData,
                                                    int numOutputChannelsIn,
                                                    int numSamples,
                                                    const juce::AudioIODeviceCallbackContext&)
{
    const auto startTime = std::chrono::steady_clock::now();

    recorder.recordBlock (inputChannelData, numInputChannelsIn, numSamples);

    float inputPeakThisBlock = 0.0f;

    for (int i = 0; i < numInputChannelsIn; ++i)
        if (inputChannelData[i] != nullptr)
            inputPeakThisBlock = std::max (inputPeakThisBlock,
                                           juce::FloatVectorOperations::findMinAndMax (inputChannelData[i], numSamples).getEnd());

    updateAtomicMax (inputPeak, inputPeakThisBlock);

    midiBuffer.clear();
    midiHandler.drainInto (midiBuffer);

    audioGraph.processWithInput (inputChannelData, outputChannelData, numOutputChannelsIn, numSamples, midiBuffer);

    float outputPeakThisBlock = 0.0f;

    for (int i = 0; i < numOutputChannelsIn; ++i)
        if (outputChannelData[i] != nullptr)
            outputPeakThisBlock = std::max (outputPeakThisBlock,
                                            juce::FloatVectorOperations::findMinAndMax (outputChannelData[i], numSamples).getEnd());

    updateAtomicMax (outputPeak, outputPeakThisBlock);

    if (outputPeakThisBlock >= 0.999f)
        clipCounter.fetch_add (1, std::memory_order_relaxed);

    const auto elapsedMs = std::chrono::duration<double, std::milli> (std::chrono::steady_clock::now() - startTime).count();

    numCallbacks.fetch_add (1, std::memory_order_relaxed);
    totalCallbackTimeMs.fetch_add (elapsedMs, std::memory_order_relaxed);
    updateAtomicMax (maxCallbackTimeMs, elapsedMs);

    if (++callbacksSinceXRunCheck >= 128)
    {
        callbacksSinceXRunCheck = 0;

        if (currentDevice != nullptr)
            xruns.store (currentDevice->getXRunCount(), std::memory_order_relaxed);
    }
}

void AudioEngine::audioDeviceAboutToStart (juce::AudioIODevice* device)
{
    currentDevice = device;
    sampleRate.store (device->getCurrentSampleRate());
    bufferSize.store (device->getCurrentBufferSizeSamples());
    inputLatencySamples.store (device->getInputLatencyInSamples());
    outputLatencySamples.store (device->getOutputLatencyInSamples());
    numInputChannels.store (device->getActiveInputChannels().countNumberOfSetBits());
    numOutputChannels.store (device->getActiveOutputChannels().countNumberOfSetBits());

    audioGraph.prepare (device->getCurrentSampleRate(), device->getCurrentBufferSizeSamples());
    audioGraph.reset();

    midiBuffer.ensureSize (2048);
}

void AudioEngine::audioDeviceStopped()
{
    currentDevice = nullptr;
    running.store (false);
}
} // namespace daw
