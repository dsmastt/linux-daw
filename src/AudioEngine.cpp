#include "AudioEngine.h"

#include <algorithm>
#include <chrono>

#include "CompressorProcessor.h"
#include "ConvolutionProcessor.h"
#include "DistortionProcessor.h"
#include "GainProcessor.h"
#include "NoiseGateProcessor.h"
#include "ParametricEqProcessor.h"
#include "ReverbProcessor.h"

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

AudioEngine::AudioEngine()
{
    auto gain = std::make_unique<GainProcessor>();
    gainProcessor = gain.get();
    processorChain.append (std::move (gain));

    auto gate = std::make_unique<NoiseGateProcessor>();
    gateProcessor = gate.get();
    processorChain.append (std::move (gate));

    auto eq = std::make_unique<ParametricEqProcessor>();
    eqProcessor = eq.get();
    processorChain.append (std::move (eq));

    auto compressor = std::make_unique<CompressorProcessor>();
    compressorProcessor = compressor.get();
    processorChain.append (std::move (compressor));

    auto distortion = std::make_unique<DistortionProcessor>();
    distortionProcessor = distortion.get();
    processorChain.append (std::move (distortion));

    auto convolution = std::make_unique<ConvolutionProcessor>();
    convolutionProcessor = convolution.get();
    processorChain.append (std::move (convolution));

    auto reverb = std::make_unique<ReverbProcessor>();
    reverbProcessor = reverb.get();
    processorChain.append (std::move (reverb));
}

AudioEngine::~AudioEngine()
{
    stop();
}

void AudioEngine::setGainDb (float gainDb) noexcept
{
    if (gainProcessor != nullptr)
        gainProcessor->setGainDb (gainDb);
}

float AudioEngine::getGainDb() const noexcept
{
    return gainProcessor != nullptr ? gainProcessor->getGainDb() : 0.0f;
}

void AudioEngine::setGateThresholdDb (float thresholdDb) noexcept
{
    if (gateProcessor != nullptr)
        gateProcessor->setThresholdDb (thresholdDb);
}

float AudioEngine::getGateThresholdDb() const noexcept
{
    return gateProcessor != nullptr ? gateProcessor->getThresholdDb() : -60.0f;
}

void AudioEngine::setGateAttackMs (float attackMs) noexcept
{
    if (gateProcessor != nullptr)
        gateProcessor->setAttackMs (attackMs);
}

float AudioEngine::getGateAttackMs() const noexcept
{
    return gateProcessor != nullptr ? gateProcessor->getAttackMs() : 1.0f;
}

void AudioEngine::setGateReleaseMs (float releaseMs) noexcept
{
    if (gateProcessor != nullptr)
        gateProcessor->setReleaseMs (releaseMs);
}

float AudioEngine::getGateReleaseMs() const noexcept
{
    return gateProcessor != nullptr ? gateProcessor->getReleaseMs() : 100.0f;
}

void AudioEngine::setEqBandParameters (int band, double frequencyHz, float gainDb, float q) noexcept
{
    if (eqProcessor != nullptr)
        eqProcessor->setBandParameters (band, frequencyHz, gainDb, q);
}

int AudioEngine::getNumEqBands() const noexcept
{
    return ParametricEqProcessor::maxBands;
}

void AudioEngine::setCompressorThresholdDb (float thresholdDb) noexcept
{
    if (compressorProcessor != nullptr)
        compressorProcessor->setThresholdDb (thresholdDb);
}

void AudioEngine::setCompressorRatio (float ratio) noexcept
{
    if (compressorProcessor != nullptr)
        compressorProcessor->setRatio (ratio);
}

void AudioEngine::setCompressorAttackMs (float attackMs) noexcept
{
    if (compressorProcessor != nullptr)
        compressorProcessor->setAttackMs (attackMs);
}

void AudioEngine::setCompressorReleaseMs (float releaseMs) noexcept
{
    if (compressorProcessor != nullptr)
        compressorProcessor->setReleaseMs (releaseMs);
}

void AudioEngine::setCompressorMakeupGainDb (float makeupGainDb) noexcept
{
    if (compressorProcessor != nullptr)
        compressorProcessor->setMakeupGainDb (makeupGainDb);
}

void AudioEngine::setDistortionDriveDb (float driveDb) noexcept
{
    if (distortionProcessor != nullptr)
        distortionProcessor->setDriveDb (driveDb);
}

void AudioEngine::setDistortionTone (float tone) noexcept
{
    if (distortionProcessor != nullptr)
        distortionProcessor->setTone (tone);
}

void AudioEngine::setDistortionMode (DistortionMode mode) noexcept
{
    if (distortionProcessor != nullptr)
        distortionProcessor->setMode (mode);
}

juce::String AudioEngine::loadImpulseResponse (const juce::File& file)
{
    if (convolutionProcessor == nullptr)
        return "Convolution processor unavailable";

    return convolutionProcessor->loadImpulseResponse (file);
}

void AudioEngine::clearImpulseResponse() noexcept
{
    if (convolutionProcessor != nullptr)
        convolutionProcessor->clearImpulseResponse();
}

bool AudioEngine::hasImpulseResponse() const noexcept
{
    return convolutionProcessor != nullptr && convolutionProcessor->hasImpulseResponse();
}

int AudioEngine::getImpulseResponseLength() const noexcept
{
    return convolutionProcessor != nullptr ? convolutionProcessor->getImpulseResponseLength() : 0;
}

void AudioEngine::setReverbWetDry (float wetDry) noexcept
{
    if (reverbProcessor != nullptr)
        reverbProcessor->setWetDry (wetDry);
}

void AudioEngine::setReverbDecay (float decay) noexcept
{
    if (reverbProcessor != nullptr)
        reverbProcessor->setDecay (decay);
}

void AudioEngine::setReverbSize (float size) noexcept
{
    if (reverbProcessor != nullptr)
        reverbProcessor->setSize (size);
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

    processorChain.process (outputChannelData, numOutputChannelsIn, numSamples);

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

    processorChain.prepare (device->getCurrentSampleRate(), device->getCurrentBufferSizeSamples());
    processorChain.reset();
}

void AudioEngine::audioDeviceStopped()
{
    currentDevice = nullptr;
    running.store (false);
}
} // namespace daw
