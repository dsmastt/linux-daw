#include "AudioEngine.h"

#include <algorithm>

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
}

void AudioEngine::audioDeviceIOCallbackWithContext (const float* const* inputChannelData,
                                                    int numInputChannels,
                                                    float* const* outputChannelData,
                                                    int numOutputChannels,
                                                    int numSamples,
                                                    const juce::AudioIODeviceCallbackContext&)
{
    const int channelsToCopy = std::min (numInputChannels, numOutputChannels);

    for (int i = 0; i < channelsToCopy; ++i)
        if (inputChannelData[i] != nullptr && outputChannelData[i] != nullptr)
            juce::FloatVectorOperations::copy (outputChannelData[i], inputChannelData[i], numSamples);

    for (int i = channelsToCopy; i < numOutputChannels; ++i)
        if (outputChannelData[i] != nullptr)
            juce::FloatVectorOperations::clear (outputChannelData[i], numSamples);
}

void AudioEngine::audioDeviceAboutToStart (juce::AudioIODevice* device)
{
    sampleRate.store (device->getCurrentSampleRate());
    bufferSize.store (device->getCurrentBufferSizeSamples());
    inputLatencySamples.store (device->getInputLatencyInSamples());
    outputLatencySamples.store (device->getOutputLatencyInSamples());
}

void AudioEngine::audioDeviceStopped()
{
    running.store (false);
}
} // namespace daw
