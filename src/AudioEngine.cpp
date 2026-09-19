#include "AudioEngine.h"

#include <algorithm>
#include <chrono>

namespace daw
{
namespace
{
void updateMax (std::atomic<double>& target, double value)
{
    auto current = target.load (std::memory_order_relaxed);

    while (value > current && !target.compare_exchange_weak (current, value, std::memory_order_relaxed))
    {
    }
}
} // namespace

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
}

void AudioEngine::audioDeviceIOCallbackWithContext (const float* const* inputChannelData,
                                                    int numInputChannelsIn,
                                                    float* const* outputChannelData,
                                                    int numOutputChannelsIn,
                                                    int numSamples,
                                                    const juce::AudioIODeviceCallbackContext&)
{
    const auto startTime = std::chrono::steady_clock::now();

    const int channelsToCopy = std::min (numInputChannelsIn, numOutputChannelsIn);

    for (int i = 0; i < channelsToCopy; ++i)
        if (inputChannelData[i] != nullptr && outputChannelData[i] != nullptr)
            juce::FloatVectorOperations::copy (outputChannelData[i], inputChannelData[i], numSamples);

    for (int i = channelsToCopy; i < numOutputChannelsIn; ++i)
        if (outputChannelData[i] != nullptr)
            juce::FloatVectorOperations::clear (outputChannelData[i], numSamples);

    const auto elapsedMs = std::chrono::duration<double, std::milli> (std::chrono::steady_clock::now() - startTime).count();

    numCallbacks.fetch_add (1, std::memory_order_relaxed);
    totalCallbackTimeMs.fetch_add (elapsedMs, std::memory_order_relaxed);
    updateMax (maxCallbackTimeMs, elapsedMs);

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
}

void AudioEngine::audioDeviceStopped()
{
    currentDevice = nullptr;
    running.store (false);
}
} // namespace daw
