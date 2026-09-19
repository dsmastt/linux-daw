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
    gainDbBase = gainDb;
    gainAutomationDb = 0.0f;

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

float Track::getCompressorThresholdDb() const noexcept
{
    return compressorProcessor != nullptr ? compressorProcessor->getThresholdDb() : -10.0f;
}

float Track::getCompressorRatio() const noexcept
{
    return compressorProcessor != nullptr ? compressorProcessor->getRatio() : 4.0f;
}

float Track::getCompressorAttackMs() const noexcept
{
    return compressorProcessor != nullptr ? compressorProcessor->getAttackMs() : 5.0f;
}

float Track::getCompressorReleaseMs() const noexcept
{
    return compressorProcessor != nullptr ? compressorProcessor->getReleaseMs() : 100.0f;
}

float Track::getCompressorMakeupGainDb() const noexcept
{
    return compressorProcessor != nullptr ? compressorProcessor->getMakeupGainDb() : 0.0f;
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

float Track::getDistortionDriveDb() const noexcept
{
    return distortionProcessor != nullptr ? distortionProcessor->getDriveDb() : 12.0f;
}

float Track::getDistortionTone() const noexcept
{
    return distortionProcessor != nullptr ? distortionProcessor->getTone() : 0.5f;
}

DistortionMode Track::getDistortionMode() const noexcept
{
    return distortionProcessor != nullptr ? distortionProcessor->getMode() : DistortionMode::soft;
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

juce::String Track::getImpulseResponseFilePath() const noexcept
{
    return convolutionProcessor != nullptr ? convolutionProcessor->getImpulseResponseFilePath() : juce::String();
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

float Track::getReverbWetDry() const noexcept
{
    return reverbProcessor != nullptr ? reverbProcessor->getWetDry() : 0.2f;
}

float Track::getReverbDecay() const noexcept
{
    return reverbProcessor != nullptr ? reverbProcessor->getDecay() : 0.5f;
}

float Track::getReverbSize() const noexcept
{
    return reverbProcessor != nullptr ? reverbProcessor->getSize() : 0.5f;
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

juce::PluginDescription Track::getVst3PluginDescription() const noexcept
{
    return vst3Processor != nullptr ? vst3Processor->getPluginDescription() : juce::PluginDescription();
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

int Track::addClip (std::int64_t startSamples, std::int64_t lengthSamples, std::int64_t sourceOffsetSamples)
{
    const int index = clipCount.load (std::memory_order_relaxed);

    if (index >= maxClipsPerTrack)
        return -1;

    auto& clip = clips[static_cast<size_t> (index)];
    clip.startSamples.store (startSamples, std::memory_order_relaxed);
    clip.lengthSamples.store (lengthSamples, std::memory_order_relaxed);
    clip.sourceOffsetSamples.store (sourceOffsetSamples, std::memory_order_relaxed);

    clipCount.store (index + 1, std::memory_order_release);

    return index;
}

void Track::removeClip (int clipIndex) noexcept
{
    const int count = clipCount.load (std::memory_order_relaxed);

    if (clipIndex < 0 || clipIndex >= count)
        return;

    for (int i = clipIndex; i < count - 1; ++i)
    {
        clips[static_cast<size_t> (i)].startSamples.store (clips[static_cast<size_t> (i + 1)].startSamples.load(), std::memory_order_relaxed);
        clips[static_cast<size_t> (i)].lengthSamples.store (clips[static_cast<size_t> (i + 1)].lengthSamples.load(), std::memory_order_relaxed);
        clips[static_cast<size_t> (i)].sourceOffsetSamples.store (clips[static_cast<size_t> (i + 1)].sourceOffsetSamples.load(), std::memory_order_relaxed);
    }

    clipCount.store (count - 1, std::memory_order_release);
}

void Track::moveClip (int clipIndex, std::int64_t newStartSamples) noexcept
{
    if (clipIndex < 0 || clipIndex >= clipCount.load())
        return;

    clips[static_cast<size_t> (clipIndex)].startSamples.store (newStartSamples, std::memory_order_relaxed);
}

void Track::resizeClip (int clipIndex, std::int64_t newLengthSamples) noexcept
{
    if (clipIndex < 0 || clipIndex >= clipCount.load())
        return;

    clips[static_cast<size_t> (clipIndex)].lengthSamples.store (std::max<std::int64_t> (1, newLengthSamples), std::memory_order_relaxed);
}

int Track::addMidiEvent (std::int64_t timeSamples, const std::uint8_t* data, int numBytes)
{
    const int index = midiEventCount.load (std::memory_order_relaxed);

    if (index >= maxMidiEventsPerTrack)
        return -1;

    auto& event = midiEvents[static_cast<size_t> (index)];
    event.timeSamples.store (timeSamples, std::memory_order_relaxed);

    const int bytesToStore = std::min (numBytes, 4);

    for (int i = 0; i < bytesToStore; ++i)
        event.bytes[static_cast<size_t> (i)] = data[i];

    event.numBytes = bytesToStore;

    midiEventCount.store (index + 1, std::memory_order_release);

    return index;
}

void Track::clearMidiEvents() noexcept
{
    midiEventCount.store (0, std::memory_order_release);
}

void Track::addAutomationPoint (AutomationParameter parameter, int pluginParameterIndex,
                                std::int64_t timeSamples, float value) noexcept
{
    const int count = automationCurveCount.load (std::memory_order_relaxed);

    int curveIndex = -1;

    for (int i = 0; i < count; ++i)
    {
        const auto& curve = automationCurves[static_cast<size_t> (i)];

        if (curve.parameter == parameter && curve.pluginParameterIndex == pluginParameterIndex)
        {
            curveIndex = i;
            break;
        }
    }

    if (curveIndex < 0)
    {
        if (count >= maxAutomationCurvesPerTrack)
            return;

        curveIndex = count;

        auto& curve = automationCurves[static_cast<size_t> (curveIndex)];
        curve.parameter = parameter;
        curve.pluginParameterIndex = pluginParameterIndex;
        curve.pointCount.store (0, std::memory_order_relaxed);

        automationCurveCount.store (count + 1, std::memory_order_release);
    }

    auto& curve = automationCurves[static_cast<size_t> (curveIndex)];
    const int pointCount = curve.pointCount.load (std::memory_order_relaxed);

    if (pointCount >= maxAutomationPointsPerCurve)
        return;

    int insertAt = pointCount;

    for (int i = 0; i < pointCount; ++i)
    {
        if (curve.points[static_cast<size_t> (i)].timeSamples.load (std::memory_order_relaxed) > timeSamples)
        {
            insertAt = i;
            break;
        }
    }

    for (int i = pointCount; i > insertAt; --i)
    {
        curve.points[static_cast<size_t> (i)].timeSamples.store (curve.points[static_cast<size_t> (i - 1)].timeSamples.load(), std::memory_order_relaxed);
        curve.points[static_cast<size_t> (i)].value.store (curve.points[static_cast<size_t> (i - 1)].value.load(), std::memory_order_relaxed);
    }

    curve.points[static_cast<size_t> (insertAt)].timeSamples.store (timeSamples, std::memory_order_relaxed);
    curve.points[static_cast<size_t> (insertAt)].value.store (value, std::memory_order_relaxed);

    curve.pointCount.store (pointCount + 1, std::memory_order_release);
}

void Track::clearAutomation (AutomationParameter parameter, int pluginParameterIndex) noexcept
{
    const int count = automationCurveCount.load (std::memory_order_relaxed);

    for (int i = 0; i < count; ++i)
    {
        const auto& curve = automationCurves[static_cast<size_t> (i)];

        if (curve.parameter != parameter || curve.pluginParameterIndex != pluginParameterIndex)
            continue;

        for (int j = i; j < count - 1; ++j)
        {
            auto& target = automationCurves[static_cast<size_t> (j)];
            const auto& source = automationCurves[static_cast<size_t> (j + 1)];

            target.parameter = source.parameter;
            target.pluginParameterIndex = source.pluginParameterIndex;
            target.pointCount.store (source.pointCount.load(), std::memory_order_relaxed);

            const int sourcePoints = source.pointCount.load (std::memory_order_relaxed);

            for (int k = 0; k < sourcePoints; ++k)
            {
                target.points[static_cast<size_t> (k)].timeSamples.store (source.points[static_cast<size_t> (k)].timeSamples.load(), std::memory_order_relaxed);
                target.points[static_cast<size_t> (k)].value.store (source.points[static_cast<size_t> (k)].value.load(), std::memory_order_relaxed);
            }
        }

        automationCurveCount.store (count - 1, std::memory_order_release);
        return;
    }
}

void Track::applyAutomation (std::int64_t transportPositionSamples) noexcept
{
    const int numCurves = automationCurveCount.load (std::memory_order_acquire);

    for (int curveIndex = 0; curveIndex < numCurves; ++curveIndex)
    {
        auto& curve = automationCurves[static_cast<size_t> (curveIndex)];
        const int count = curve.pointCount.load (std::memory_order_acquire);

        if (count == 0)
            continue;

        float value = 0.0f;

        if (count == 1)
        {
            value = curve.points[0].value.load (std::memory_order_relaxed);
        }
        else
        {
            int& hint = automationSearchIndex[curveIndex];

            if (hint > count - 2)
                hint = count - 2;

            if (hint < 0)
                hint = 0;

            while (hint > 0 && curve.points[static_cast<size_t> (hint)].timeSamples.load (std::memory_order_relaxed) > transportPositionSamples)
                --hint;

            while (hint < count - 2 && curve.points[static_cast<size_t> (hint + 1)].timeSamples.load (std::memory_order_relaxed) < transportPositionSamples)
                ++hint;

            const auto t0 = curve.points[static_cast<size_t> (hint)].timeSamples.load (std::memory_order_relaxed);
            const auto t1 = curve.points[static_cast<size_t> (hint + 1)].timeSamples.load (std::memory_order_relaxed);
            const auto v0 = curve.points[static_cast<size_t> (hint)].value.load (std::memory_order_relaxed);
            const auto v1 = curve.points[static_cast<size_t> (hint + 1)].value.load (std::memory_order_relaxed);

            if (transportPositionSamples <= t0)
                value = v0;
            else if (transportPositionSamples >= t1)
                value = v1;
            else
                value = v0 + (v1 - v0) * static_cast<float> (transportPositionSamples - t0) / static_cast<float> (t1 - t0);
        }

        switch (curve.parameter)
        {
            case AutomationParameter::trackVolumeDb:
                volumeAutomationDb = value;
                break;

            case AutomationParameter::trackPan:
                panAutomation = value;
                break;

            case AutomationParameter::gainDb:
                gainAutomationDb = value;

                if (gainProcessor != nullptr)
                    gainProcessor->setGainDb (gainDbBase + value);
                break;

            case AutomationParameter::pluginParameter:
            {
                auto* instance = getVst3PluginInstance();

                if (instance != nullptr && curve.pluginParameterIndex >= 0
                    && curve.pluginParameterIndex < instance->getParameters().size())
                {
                    instance->getParameters().getUnchecked (curve.pluginParameterIndex)->setValue (value);
                }
                break;
            }
        }
    }
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
                     bool anyTrackSoloed,
                     std::int64_t transportPositionSamples,
                     const ClipSource& clipSource,
                     juce::MidiBuffer& midiEventsOut,
                     bool transportPlaying) noexcept
{
    if (isMuted() || (anyTrackSoloed && !isSoloed()))
        return;

    const int channels = std::min (numChannelsIn, maxChannels);
    const int frames = std::min (numSamples, maxBlockFrames);

    if (transportPlaying)
        applyAutomation (transportPositionSamples);

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

    if (transportPlaying)
    {
        const int numMidiEvents = midiEventCount.load (std::memory_order_acquire);

        for (int eventIndex = 0; eventIndex < numMidiEvents; ++eventIndex)
        {
            const auto& event = midiEvents[static_cast<size_t> (eventIndex)];
            const auto eventTime = event.timeSamples.load (std::memory_order_relaxed);

            if (eventTime < transportPositionSamples || eventTime >= transportPositionSamples + frames)
                continue;

            midiEventsOut.addEvent (event.bytes,
                                    event.numBytes,
                                    static_cast<int> (eventTime - transportPositionSamples));
        }
    }

    chain.processWithMidi (scratchPointers, channels, frames, midiEventsOut);

    const int numClips = clipCount.load (std::memory_order_acquire);

    if (numClips > 0 && clipSource.data != nullptr && clipSource.frames > 0)
    {
        const int sourceChannels = clipSource.channels;

        for (int clipIndex = 0; clipIndex < numClips; ++clipIndex)
        {
            const auto& clip = clips[static_cast<size_t> (clipIndex)];
            const auto clipStart = clip.startSamples.load (std::memory_order_relaxed);
            const auto clipLength = clip.lengthSamples.load (std::memory_order_relaxed);
            const auto sourceOffset = clip.sourceOffsetSamples.load (std::memory_order_relaxed);

            for (int frame = 0; frame < frames; ++frame)
            {
                const auto absolutePosition = transportPositionSamples + frame;

                if (absolutePosition < clipStart || absolutePosition >= clipStart + clipLength)
                    continue;

                const auto sourcePosition = sourceOffset + (absolutePosition - clipStart);

                if (sourcePosition < 0 || sourcePosition >= clipSource.frames)
                    continue;

                const auto* sourceSample = clipSource.data + sourcePosition * sourceChannels;

                for (int channel = 0; channel < channels; ++channel)
                {
                    const int sourceChannel = std::min (channel, sourceChannels - 1);
                    scratchPointers[channel][frame] += sourceSample[sourceChannel];
                }
            }
        }
    }

    const float volume = volumeLinear.load (std::memory_order_relaxed)
                           * std::pow (10.0f, volumeAutomationDb * 0.05f);
    const float pan = std::clamp (panValue.load (std::memory_order_relaxed) + panAutomation, -1.0f, 1.0f);

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
