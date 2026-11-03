#include "AudioGraph.h"

#include <algorithm>

#include <juce_audio_basics/juce_audio_basics.h>

#include "PlaybackProcessor.h"

namespace daw
{
AudioGraph::AudioGraph()
{
    for (int i = 0; i < numTracks; ++i)
        tracks.push_back (std::make_unique<Track>());

    playbackProcessor = std::make_unique<PlaybackProcessor>();
}

AudioGraph::~AudioGraph() = default;

void AudioGraph::setGainDb (float gainDb) noexcept { getTrack (0).setGainDb (gainDb); }
float AudioGraph::getGainDb() const noexcept { return getTrack (0).getGainDb(); }

void AudioGraph::setGateThresholdDb (float thresholdDb) noexcept { getTrack (0).setGateThresholdDb (thresholdDb); }
float AudioGraph::getGateThresholdDb() const noexcept { return getTrack (0).getGateThresholdDb(); }
void AudioGraph::setGateAttackMs (float attackMs) noexcept { getTrack (0).setGateAttackMs (attackMs); }
float AudioGraph::getGateAttackMs() const noexcept { return getTrack (0).getGateAttackMs(); }
void AudioGraph::setGateReleaseMs (float releaseMs) noexcept { getTrack (0).setGateReleaseMs (releaseMs); }
float AudioGraph::getGateReleaseMs() const noexcept { return getTrack (0).getGateReleaseMs(); }

void AudioGraph::setEqBandParameters (int band, double frequencyHz, float gainDb, float q) noexcept { getTrack (0).setEqBandParameters (band, frequencyHz, gainDb, q); }
int AudioGraph::getNumEqBands() const noexcept { return getTrack (0).getNumEqBands(); }
double AudioGraph::getEqBandFrequency (int band) const noexcept { return getTrack (0).getEqBandFrequency (band); }
float AudioGraph::getEqBandGain (int band) const noexcept { return getTrack (0).getEqBandGain (band); }
float AudioGraph::getEqBandQ (int band) const noexcept { return getTrack (0).getEqBandQ (band); }

void AudioGraph::setCompressorThresholdDb (float thresholdDb) noexcept { getTrack (0).setCompressorThresholdDb (thresholdDb); }
void AudioGraph::setCompressorRatio (float ratio) noexcept { getTrack (0).setCompressorRatio (ratio); }
void AudioGraph::setCompressorAttackMs (float attackMs) noexcept { getTrack (0).setCompressorAttackMs (attackMs); }
void AudioGraph::setCompressorReleaseMs (float releaseMs) noexcept { getTrack (0).setCompressorReleaseMs (releaseMs); }
void AudioGraph::setCompressorMakeupGainDb (float makeupGainDb) noexcept { getTrack (0).setCompressorMakeupGainDb (makeupGainDb); }

void AudioGraph::setDistortionDriveDb (float driveDb) noexcept { getTrack (0).setDistortionDriveDb (driveDb); }
void AudioGraph::setDistortionTone (float tone) noexcept { getTrack (0).setDistortionTone (tone); }
void AudioGraph::setDistortionMode (DistortionMode mode) noexcept { getTrack (0).setDistortionMode (mode); }

float AudioGraph::getCompressorThresholdDb() const noexcept { return getTrack (0).getCompressorThresholdDb(); }
float AudioGraph::getCompressorRatio() const noexcept { return getTrack (0).getCompressorRatio(); }
float AudioGraph::getCompressorAttackMs() const noexcept { return getTrack (0).getCompressorAttackMs(); }
float AudioGraph::getCompressorReleaseMs() const noexcept { return getTrack (0).getCompressorReleaseMs(); }
float AudioGraph::getCompressorMakeupGainDb() const noexcept { return getTrack (0).getCompressorMakeupGainDb(); }
float AudioGraph::getDistortionDriveDb() const noexcept { return getTrack (0).getDistortionDriveDb(); }
float AudioGraph::getDistortionTone() const noexcept { return getTrack (0).getDistortionTone(); }
DistortionMode AudioGraph::getDistortionMode() const noexcept { return getTrack (0).getDistortionMode(); }

juce::String AudioGraph::loadImpulseResponse (const juce::File& file) { return getTrack (0).loadImpulseResponse (file); }
void AudioGraph::clearImpulseResponse() noexcept { getTrack (0).clearImpulseResponse(); }
bool AudioGraph::hasImpulseResponse() const noexcept { return getTrack (0).hasImpulseResponse(); }
int AudioGraph::getImpulseResponseLength() const noexcept { return getTrack (0).getImpulseResponseLength(); }

void AudioGraph::setReverbWetDry (float wetDry) noexcept { getTrack (0).setReverbWetDry (wetDry); }
void AudioGraph::setReverbDecay (float decay) noexcept { getTrack (0).setReverbDecay (decay); }
void AudioGraph::setReverbSize (float size) noexcept { getTrack (0).setReverbSize (size); }

float AudioGraph::getReverbWetDry() const noexcept { return getTrack (0).getReverbWetDry(); }
float AudioGraph::getReverbDecay() const noexcept { return getTrack (0).getReverbDecay(); }
float AudioGraph::getReverbSize() const noexcept { return getTrack (0).getReverbSize(); }

juce::String AudioGraph::loadVst3Plugin (const juce::PluginDescription& description, double sampleRate, int bufferSize)
{
    return getTrack (0).loadVst3Plugin (description, sampleRate, bufferSize);
}

void AudioGraph::unloadVst3Plugin() noexcept { getTrack (0).unloadVst3Plugin(); }
bool AudioGraph::isVst3PluginLoaded() const noexcept { return getTrack (0).isVst3PluginLoaded(); }
juce::String AudioGraph::getVst3PluginName() const noexcept { return getTrack (0).getVst3PluginName(); }
void AudioGraph::setVst3PluginBypassed (bool shouldBeBypassed) noexcept { getTrack (0).setVst3PluginBypassed (shouldBeBypassed); }
bool AudioGraph::isVst3PluginBypassed() const noexcept { return getTrack (0).isVst3PluginBypassed(); }
juce::AudioPluginInstance* AudioGraph::getVst3PluginInstance() const noexcept { return getTrack (0).getVst3PluginInstance(); }

void AudioGraph::setProcessorBypassed (ProcessorSlot slot, bool shouldBeBypassed) noexcept { getTrack (0).setProcessorBypassed (slot, shouldBeBypassed); }
bool AudioGraph::isProcessorBypassed (ProcessorSlot slot) const noexcept { return getTrack (0).isProcessorBypassed (slot); }

juce::String AudioGraph::loadPlaybackFile (const juce::File& file)
{
    return playbackProcessor->loadFile (file);
}

juce::String AudioGraph::getPlaybackFilePath() const noexcept
{
    return playbackProcessor->getLoadedFilePath();
}

void AudioGraph::playPlayback() noexcept { playbackProcessor->play(); }
void AudioGraph::pausePlayback() noexcept { playbackProcessor->pause(); }
void AudioGraph::stopPlayback() noexcept { playbackProcessor->stop(); }
bool AudioGraph::isPlaybackPlaying() const noexcept { return playbackProcessor->isPlaying(); }
double AudioGraph::getPlaybackPositionSeconds() const noexcept { return playbackProcessor->getPositionSeconds(); }
double AudioGraph::getPlaybackLengthSeconds() const noexcept { return playbackProcessor->getLengthSeconds(); }
std::int64_t AudioGraph::getPlaybackLengthFrames() const noexcept { return playbackProcessor->getPreparedFrames(); }

const float* AudioGraph::getPlaybackSourceData() const noexcept { return playbackProcessor->getSourceData(); }
std::int64_t AudioGraph::getPlaybackSourceFrames() const noexcept { return playbackProcessor->getSourceFrames(); }
int AudioGraph::getPlaybackSourceChannels() const noexcept { return playbackProcessor->getSourceChannels(); }
void AudioGraph::setPlaybackPositionSeconds (double seconds) noexcept { playbackProcessor->setPositionSeconds (seconds); }

void AudioGraph::playTransport() noexcept
{
    transport.play();
}

void AudioGraph::pauseTransport() noexcept
{
    transport.pause();
}

void AudioGraph::stopTransport() noexcept
{
    transport.stop();
}

int AudioGraph::getNumProcessors() const noexcept
{
    return getTrack (0).getNumProcessors();
}

void AudioGraph::resetToDefaults() noexcept
{
    stopTransport();
    playbackProcessor->clearFile();

    for (auto& track : tracks)
        track->resetParameters();
}

void AudioGraph::prepare (double sampleRate, int bufferSize) noexcept
{
    preparedSampleRate = sampleRate;
    preparedBufferSize = bufferSize;

    for (auto& track : tracks)
        track->prepare (sampleRate, bufferSize);

    for (auto& busBuffer : busBuffers)
        busBuffer.assign (static_cast<size_t> (maxChannels), std::vector<float> (maxBlockFrames, 0.0f));

    playbackProcessor->prepare (sampleRate, bufferSize);
    metronome.prepare (sampleRate);
}

void AudioGraph::reset() noexcept
{
    for (auto& track : tracks)
        track->reset();

    for (auto& busBuffer : busBuffers)
        for (auto& channel : busBuffer)
            std::fill (channel.begin(), channel.end(), 0.0f);

    playbackProcessor->reset();
    metronome.reset();
}

void AudioGraph::processWithInput (const float* const* inputChannels,
                                   float* const* outputChannels,
                                   int numChannels,
                                   int numSamples,
                                   juce::MidiBuffer& midiEvents) noexcept
{
    const auto transportPosition = transport.getPositionSamples();

    if (transport.isPlaying())
        transport.advance (numSamples);

    for (int channel = 0; channel < numChannels; ++channel)
        if (outputChannels[channel] != nullptr)
            juce::FloatVectorOperations::clear (outputChannels[channel], numSamples);

    bool anyTrackSoloed = false;

    for (const auto& track : tracks)
        anyTrackSoloed = anyTrackSoloed || track->isSoloed();

    const bool transportPlaying = transport.isPlaying();
    const bool monoInputForced = forceMonoInput.load (std::memory_order_relaxed);

    RoutingBuffers routing {};
    routing.master[0] = outputChannels[0];
    routing.master[1] = numChannels > 1 ? outputChannels[1] : nullptr;

    float* busPointers[numBuses][maxChannels] = {};

    for (int bus = 0; bus < numBuses; ++bus)
    {
        for (int channel = 0; channel < maxChannels && channel < numChannels; ++channel)
        {
            auto& busBuffer = busBuffers[bus][static_cast<size_t> (channel)];
            std::fill (busBuffer.begin(), busBuffer.begin() + numSamples, 0.0f);
            busPointers[bus][channel] = busBuffer.data();
            routing.buses[bus][channel] = busBuffer.data();
        }
    }

    for (int i = 0; i < numTracks; ++i)
        tracks[static_cast<size_t> (i)]->process (i == 0 ? inputChannels : nullptr,
                                                  outputChannels,
                                                  numChannels,
                                                  numSamples,
                                                  anyTrackSoloed,
                                                  transportPosition,
                                                  ClipSource { playbackProcessor->getPreparedData(),
                                                               playbackProcessor->getPreparedFrames(),
                                                               playbackProcessor->getPreparedChannels() },
                                                  midiEvents,
                                                  transportPlaying,
                                                  routing,
                                                  monoInputForced);

    for (int bus = 0; bus < numBuses; ++bus)
        for (int channel = 0; channel < numChannels; ++channel)
            if (busPointers[bus][channel] != nullptr && outputChannels[channel] != nullptr)
                juce::FloatVectorOperations::add (outputChannels[channel],
                                                  busPointers[bus][channel],
                                                  numSamples);

    playbackProcessor->process (outputChannels, numChannels, numSamples);

    metronome.process (outputChannels,
                       numChannels,
                       numSamples,
                       transportPosition,
                       transportPlaying,
                       transport.getTempoBpm());
}
} // namespace daw
