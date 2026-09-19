#pragma once

#include <juce_core/juce_core.h>

#include "AudioProcessor.h"
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
class ReverbProcessor;

class AudioGraph final : public AudioProcessor
{
public:
    AudioGraph();
    ~AudioGraph() override;

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

    void setReverbWetDry (float wetDry) noexcept;
    void setReverbDecay (float decay) noexcept;
    void setReverbSize (float size) noexcept;

    int getNumProcessors() const noexcept { return chain.size(); }

    void prepare (double sampleRate, int bufferSize) noexcept override;
    void process (float* const* channels, int numChannels, int numSamples) noexcept override;
    void reset() noexcept override;

private:
    ProcessorChain chain;
    GainProcessor* gainProcessor = nullptr;
    NoiseGateProcessor* gateProcessor = nullptr;
    ParametricEqProcessor* eqProcessor = nullptr;
    CompressorProcessor* compressorProcessor = nullptr;
    DistortionProcessor* distortionProcessor = nullptr;
    ConvolutionProcessor* convolutionProcessor = nullptr;
    ReverbProcessor* reverbProcessor = nullptr;
};
} // namespace daw
