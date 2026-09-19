#include "Track.h"

#include <algorithm>
#include <cmath>

#include "CompressorProcessor.h"
#include "ConvolutionProcessor.h"
#include "DistortionProcessor.h"
#include "GainProcessor.h"
#include "NoiseGateProcessor.h"
#include "ParametricEqProcessor.h"
#include "ReverbProcessor.h"
#include "Vst3HostProcessor.h"

namespace daw
{
Track::Track()
{
    auto gain = std::make_unique<GainProcessor>();
    gainProcessor = gain.get();
    chain.append (std::move (gain));

    auto gate = std::make_unique<NoiseGateProcessor>();
    gateProcessor = gate.get();
    chain.append (std::move (gate));

    auto eq = std::make_unique<ParametricEqProcessor>();
    eqProcessor = eq.get();
    chain.append (std::move (eq));

    auto compressor = std::make_unique<CompressorProcessor>();
    compressorProcessor = compressor.get();
    chain.append (std::move (compressor));

    auto distortion = std::make_unique<DistortionProcessor>();
    distortionProcessor = distortion.get();
    chain.append (std::move (distortion));

    auto vst3 = std::make_unique<Vst3HostProcessor>();
    vst3Processor = vst3.get();
    chain.append (std::move (vst3));

    auto convolution = std::make_unique<ConvolutionProcessor>();
    convolutionProcessor = convolution.get();
    chain.append (std::move (convolution));

    auto reverb = std::make_unique<ReverbProcessor>();
    reverbProcessor = reverb.get();
    chain.append (std::move (reverb));
}

Track::~Track() = default;

void Track::setVolumeDb (float newVolumeDb) noexcept
{
    volumeDb = newVolumeDb;
    volumeLinear.store (std::pow (10.0f, newVolumeDb * 0.05f), std::memory_order_relaxed);
}

float Track::getVolumeDb() const noexcept
{
    return volumeDb;
}

void Track::setPan (float newPan) noexcept
{
    panValue.store (std::clamp (newPan, -1.0f, 1.0f), std::memory_order_relaxed);
}

float Track::getPan() const noexcept
{
    return panValue.load();
}

void Track::setMuted (bool shouldBeMuted) noexcept
{
    muted.store (shouldBeMuted, std::memory_order_relaxed);
}

bool Track::isMuted() const noexcept
{
    return muted.load();
}

void Track::setSoloed (bool shouldBeSoloed) noexcept
{
    soloed.store (shouldBeSoloed, std::memory_order_relaxed);
}

bool Track::isSoloed() const noexcept
{
    return soloed.load();
}

void Track::setGainDb (float gainDb) noexcept
{
    if (gainProcessor != nullptr)
        gainProcessor->setGainDb (gainDb);
}

float Track::getGainDb() const noexcept
{
    return gainProcessor != nullptr ? gainProcessor->getGainDb() : 0.0f;
}

void Track::setGateThresholdDb (float thresholdDb) noexcept
{
    if (gateProcessor != nullptr)
        gateProcessor->setThresholdDb (thresholdDb);
}

float Track::getGateThresholdDb() const noexcept
{
    return gateProcessor != nullptr ? gateProcessor->getThresholdDb() : -60.0f;
}

void Track::setGateAttackMs (float attackMs) noexcept
{
    if (gateProcessor != nullptr)
        gateProcessor->setAttackMs (attackMs);
}

float Track::getGateAttackMs() const noexcept
{
    return gateProcessor != nullptr ? gateProcessor->getAttackMs() : 1.0f;
}

void Track::setGateReleaseMs (float releaseMs) noexcept
{
    if (gateProcessor != nullptr)
        gateProcessor->setReleaseMs (releaseMs);
}

float Track::getGateReleaseMs() const noexcept
{
    return gateProcessor != nullptr ? gateProcessor->getReleaseMs() : 100.0f;
}

void Track::setEqBandParameters (int band, double frequencyHz, float gainDb, float q) noexcept
{
    if (eqProcessor != nullptr)
        eqProcessor->setBandParameters (band, frequencyHz, gainDb, q);
}

int Track::getNumEqBands() const noexcept
{
    return ParametricEqProcessor::maxBands;
}

double Track::getEqBandFrequency (int band) const noexcept
{
    return eqProcessor != nullptr ? eqProcessor->getBandFrequency (band) : 0.0;
}

float Track::getEqBandGain (int band) const noexcept
{
    return eqProcessor != nullptr ? eqProcessor->getBandGainDb (band) : 0.0f;
}

float Track::getEqBandQ (int band) const noexcept
{
    return eqProcessor != nullptr ? eqProcessor->getBandQ (band) : 0.0f;
}

void Track::setCompressorThresholdDb (float thresholdDb) noexcept
{
    if (compressorProcessor != nullptr)
        compressorProcessor->setThresholdDb (thresholdDb);
}

void Track::setCompressorRatio (float ratio) noexcept
{
    if (compressorProcessor != nullptr)
        compressorProcessor->setRatio (ratio);
}

void Track::setCompressorAttackMs (float attackMs) noexcept
{
    if (compressorProcessor != nullptr)
        compressorProcessor->setAttackMs (attackMs);
}

void Track::setCompressorReleaseMs (float releaseMs) noexcept
{
    if (compressorProcessor != nullptr)
        compressorProcessor->setReleaseMs (releaseMs);
}

void Track::setCompressorMakeupGainDb (float makeupGainDb) noexcept
{
    if (compressorProcessor != nullptr)
        compressorProcessor->setMakeupGainDb (makeupGainDb);
}

void Track::setDistortionDriveDb (float driveDb) noexcept
{
    if (distortionProcessor != nullptr)
        distortionProcessor->setDriveDb (driveDb);
}

void Track::setDistortionTone (float tone) noexcept
{
    if (distortionProcessor != nullptr)
        distortionProcessor->setTone (tone);
}

void Track::setDistortionMode (DistortionMode mode) noexcept
{
    if (distortionProcessor != nullptr)
        distortionProcessor->setMode (mode);
}

juce::String Track::loadImpulseResponse (const juce::File& file)
{
    if (convolutionProcessor == nullptr)
        return "Convolution processor unavailable";

    return convolutionProcessor->loadImpulseResponse (file);
}

void Track::clearImpulseResponse() noexcept
{
    if (convolutionProcessor != nullptr)
        convolutionProcessor->clearImpulseResponse();
}

bool Track::hasImpulseResponse() const noexcept
{
    return convolutionProcessor != nullptr && convolutionProcessor->hasImpulseResponse();
}

int Track::getImpulseResponseLength() const noexcept
{
    return convolutionProcessor != nullptr ? convolutionProcessor->getImpulseResponseLength() : 0;
}

void Track::setReverbWetDry (float wetDry) noexcept
{
    if (reverbProcessor != nullptr)
        reverbProcessor->setWetDry (wetDry);
}

void Track::setReverbDecay (float decay) noexcept
{
    if (reverbProcessor != nullptr)
        reverbProcessor->setDecay (decay);
}

void Track::setReverbSize (float size) noexcept
{
    if (reverbProcessor != nullptr)
        reverbProcessor->setSize (size);
}

juce::String Track::loadVst3Plugin (const juce::PluginDescription& description, double sampleRate, int bufferSize)
{
    if (vst3Processor == nullptr)
        return "VST3 processor unavailable";

    return vst3Processor->loadPlugin (description, sampleRate, bufferSize);
}

void Track::unloadVst3Plugin() noexcept
{
    if (vst3Processor != nullptr)
        vst3Processor->unloadPlugin();
}

bool Track::isVst3PluginLoaded() const noexcept
{
    return vst3Processor != nullptr && vst3Processor->isLoaded();
}

juce::String Track::getVst3PluginName() const noexcept
{
    return vst3Processor != nullptr ? vst3Processor->getPluginName() : juce::String();
}

void Track::setVst3PluginBypassed (bool shouldBeBypassed) noexcept
{
    if (vst3Processor != nullptr)
        vst3Processor->setBypassed (shouldBeBypassed);
}

bool Track::isVst3PluginBypassed() const noexcept
{
    return vst3Processor != nullptr && vst3Processor->isBypassed();
}

juce::AudioPluginInstance* Track::getVst3PluginInstance() const noexcept
{
    return vst3Processor != nullptr ? vst3Processor->getInstance() : nullptr;
}

void Track::prepare (double sampleRate, int bufferSize) noexcept
{
    numChannels = maxChannels;

    scratch.assign (static_cast<size_t> (maxChannels), std::vector<float> (maxBlockFrames, 0.0f));

    chain.prepare (sampleRate, bufferSize);
}

void Track::reset() noexcept
{
    chain.reset();
}

void Track::process (const float* const* input,
                     float* const* output,
                     int numChannelsIn,
                     int numSamples,
                     bool anyTrackSoloed) noexcept
{
    if (isMuted() || (anyTrackSoloed && !isSoloed()))
        return;

    const int channels = std::min (numChannelsIn, maxChannels);
    const int frames = std::min (numSamples, maxBlockFrames);

    for (int channel = 0; channel < channels; ++channel)
    {
        auto* scratchChannel = scratch[static_cast<size_t> (channel)].data();

        if (input != nullptr && input[channel] != nullptr)
            juce::FloatVectorOperations::copy (scratchChannel, input[channel], frames);
        else
            juce::FloatVectorOperations::clear (scratchChannel, frames);
    }

    float* scratchPointers[maxChannels] = {};
    scratchPointers[0] = scratch[0].data();
    scratchPointers[1] = scratch[1].data();

    chain.process (scratchPointers, channels, frames);

    const float volume = volumeLinear.load (std::memory_order_relaxed);
    const float pan = panValue.load (std::memory_order_relaxed);

    constexpr float piOverFour = 3.14159265358979323846f / 4.0f;
    const float leftGain = volume * std::cos ((pan + 1.0f) * piOverFour);
    const float rightGain = volume * std::sin ((pan + 1.0f) * piOverFour);

    if (channels >= 2)
    {
        for (int frame = 0; frame < frames; ++frame)
        {
            output[0][frame] += scratchPointers[0][frame] * leftGain;
            output[1][frame] += scratchPointers[1][frame] * rightGain;
        }
    }
    else if (channels == 1)
    {
        for (int frame = 0; frame < frames; ++frame)
        {
            const float sample = scratchPointers[0][frame] * volume;
            output[0][frame] += sample * (1.0f - pan) * 0.5f;
            output[1][frame] += sample * (pan + 1.0f) * 0.5f;
        }
    }
}
} // namespace daw
