#include "AudioGraph.h"

#include <memory>

#include "CompressorProcessor.h"
#include "ConvolutionProcessor.h"
#include "DistortionProcessor.h"
#include "GainProcessor.h"
#include "NoiseGateProcessor.h"
#include "ParametricEqProcessor.h"
#include "PlaybackProcessor.h"
#include "ReverbProcessor.h"

namespace daw
{
AudioGraph::AudioGraph()
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

    auto convolution = std::make_unique<ConvolutionProcessor>();
    convolutionProcessor = convolution.get();
    chain.append (std::move (convolution));

    auto reverb = std::make_unique<ReverbProcessor>();
    reverbProcessor = reverb.get();
    chain.append (std::move (reverb));

    auto playback = std::make_unique<PlaybackProcessor>();
    playbackProcessor = playback.get();
    chain.append (std::move (playback));
}

AudioGraph::~AudioGraph() = default;

void AudioGraph::setGainDb (float gainDb) noexcept
{
    if (gainProcessor != nullptr)
        gainProcessor->setGainDb (gainDb);
}

float AudioGraph::getGainDb() const noexcept
{
    return gainProcessor != nullptr ? gainProcessor->getGainDb() : 0.0f;
}

void AudioGraph::setGateThresholdDb (float thresholdDb) noexcept
{
    if (gateProcessor != nullptr)
        gateProcessor->setThresholdDb (thresholdDb);
}

float AudioGraph::getGateThresholdDb() const noexcept
{
    return gateProcessor != nullptr ? gateProcessor->getThresholdDb() : -60.0f;
}

void AudioGraph::setGateAttackMs (float attackMs) noexcept
{
    if (gateProcessor != nullptr)
        gateProcessor->setAttackMs (attackMs);
}

float AudioGraph::getGateAttackMs() const noexcept
{
    return gateProcessor != nullptr ? gateProcessor->getAttackMs() : 1.0f;
}

void AudioGraph::setGateReleaseMs (float releaseMs) noexcept
{
    if (gateProcessor != nullptr)
        gateProcessor->setReleaseMs (releaseMs);
}

float AudioGraph::getGateReleaseMs() const noexcept
{
    return gateProcessor != nullptr ? gateProcessor->getReleaseMs() : 100.0f;
}

void AudioGraph::setEqBandParameters (int band, double frequencyHz, float gainDb, float q) noexcept
{
    if (eqProcessor != nullptr)
        eqProcessor->setBandParameters (band, frequencyHz, gainDb, q);
}

int AudioGraph::getNumEqBands() const noexcept
{
    return ParametricEqProcessor::maxBands;
}

double AudioGraph::getEqBandFrequency (int band) const noexcept
{
    return eqProcessor != nullptr ? eqProcessor->getBandFrequency (band) : 0.0;
}

float AudioGraph::getEqBandGain (int band) const noexcept
{
    return eqProcessor != nullptr ? eqProcessor->getBandGainDb (band) : 0.0f;
}

float AudioGraph::getEqBandQ (int band) const noexcept
{
    return eqProcessor != nullptr ? eqProcessor->getBandQ (band) : 0.0f;
}

void AudioGraph::setCompressorThresholdDb (float thresholdDb) noexcept
{
    if (compressorProcessor != nullptr)
        compressorProcessor->setThresholdDb (thresholdDb);
}

void AudioGraph::setCompressorRatio (float ratio) noexcept
{
    if (compressorProcessor != nullptr)
        compressorProcessor->setRatio (ratio);
}

void AudioGraph::setCompressorAttackMs (float attackMs) noexcept
{
    if (compressorProcessor != nullptr)
        compressorProcessor->setAttackMs (attackMs);
}

void AudioGraph::setCompressorReleaseMs (float releaseMs) noexcept
{
    if (compressorProcessor != nullptr)
        compressorProcessor->setReleaseMs (releaseMs);
}

void AudioGraph::setCompressorMakeupGainDb (float makeupGainDb) noexcept
{
    if (compressorProcessor != nullptr)
        compressorProcessor->setMakeupGainDb (makeupGainDb);
}

void AudioGraph::setDistortionDriveDb (float driveDb) noexcept
{
    if (distortionProcessor != nullptr)
        distortionProcessor->setDriveDb (driveDb);
}

void AudioGraph::setDistortionTone (float tone) noexcept
{
    if (distortionProcessor != nullptr)
        distortionProcessor->setTone (tone);
}

void AudioGraph::setDistortionMode (DistortionMode mode) noexcept
{
    if (distortionProcessor != nullptr)
        distortionProcessor->setMode (mode);
}

juce::String AudioGraph::loadImpulseResponse (const juce::File& file)
{
    if (convolutionProcessor == nullptr)
        return "Convolution processor unavailable";

    return convolutionProcessor->loadImpulseResponse (file);
}

void AudioGraph::clearImpulseResponse() noexcept
{
    if (convolutionProcessor != nullptr)
        convolutionProcessor->clearImpulseResponse();
}

bool AudioGraph::hasImpulseResponse() const noexcept
{
    return convolutionProcessor != nullptr && convolutionProcessor->hasImpulseResponse();
}

int AudioGraph::getImpulseResponseLength() const noexcept
{
    return convolutionProcessor != nullptr ? convolutionProcessor->getImpulseResponseLength() : 0;
}

void AudioGraph::setReverbWetDry (float wetDry) noexcept
{
    if (reverbProcessor != nullptr)
        reverbProcessor->setWetDry (wetDry);
}

void AudioGraph::setReverbDecay (float decay) noexcept
{
    if (reverbProcessor != nullptr)
        reverbProcessor->setDecay (decay);
}

void AudioGraph::setReverbSize (float size) noexcept
{
    if (reverbProcessor != nullptr)
        reverbProcessor->setSize (size);
}

juce::String AudioGraph::loadPlaybackFile (const juce::File& file)
{
    if (playbackProcessor == nullptr)
        return "Playback processor unavailable";

    return playbackProcessor->loadFile (file);
}

void AudioGraph::playPlayback() noexcept
{
    if (playbackProcessor != nullptr)
        playbackProcessor->play();
}

void AudioGraph::pausePlayback() noexcept
{
    if (playbackProcessor != nullptr)
        playbackProcessor->pause();
}

void AudioGraph::stopPlayback() noexcept
{
    if (playbackProcessor != nullptr)
        playbackProcessor->stop();
}

bool AudioGraph::isPlaybackPlaying() const noexcept
{
    return playbackProcessor != nullptr && playbackProcessor->isPlaying();
}

double AudioGraph::getPlaybackPositionSeconds() const noexcept
{
    return playbackProcessor != nullptr ? playbackProcessor->getPositionSeconds() : 0.0;
}

double AudioGraph::getPlaybackLengthSeconds() const noexcept
{
    return playbackProcessor != nullptr ? playbackProcessor->getLengthSeconds() : 0.0;
}

void AudioGraph::setPlaybackPositionSeconds (double seconds) noexcept
{
    if (playbackProcessor != nullptr)
        playbackProcessor->setPositionSeconds (seconds);
}

void AudioGraph::prepare (double sampleRate, int bufferSize) noexcept
{
    chain.prepare (sampleRate, bufferSize);
}

void AudioGraph::process (float* const* channels, int numChannels, int numSamples) noexcept
{
    chain.process (channels, numChannels, numSamples);
}

void AudioGraph::reset() noexcept
{
    chain.reset();
}
} // namespace daw
