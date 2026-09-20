#pragma once

#include <atomic>
#include <memory>
#include <vector>

#include <juce_core/juce_core.h>

#include "Automation.h"
#include "Clip.h"
#include "DistortionShaper.h"
#include "MidiClip.h"
#include "ProcessorChain.h"

namespace juce
{
class AudioPluginInstance;
struct PluginDescription;
} // namespace juce

namespace daw
{
constexpr int numBuses = 2;
constexpr int maxChannels = 2;
constexpr int maxBlockFrames = 2048;

struct RoutingBuffers
{
    float* master[maxChannels] = {};
    float* buses[numBuses][maxChannels] = {};
};

class CompressorProcessor;
class ConvolutionProcessor;
class DistortionProcessor;
class GainProcessor;
class NoiseGateProcessor;
class ParametricEqProcessor;
class ReverbProcessor;
class Vst3HostProcessor;

class Track
{
public:
    Track();
    ~Track();

    void setVolumeDb (float volumeDb) noexcept;
    float getVolumeDb() const noexcept;
    void setPan (float pan) noexcept;
    float getPan() const noexcept;
    void setMuted (bool shouldBeMuted) noexcept;
    bool isMuted() const noexcept;
    void setSoloed (bool shouldBeSoloed) noexcept;
    bool isSoloed() const noexcept;

    void setRouteTarget (int busIndex) noexcept;
    int getRouteTarget() const noexcept;
    void setSendTarget (int busIndex) noexcept;
    int getSendTarget() const noexcept;
    void setSendAmount (float amount) noexcept;
    float getSendAmount() const noexcept;

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
    double getEqBandFrequency (int band) const noexcept;
    float getEqBandGain (int band) const noexcept;
    float getEqBandQ (int band) const noexcept;

    void setCompressorThresholdDb (float thresholdDb) noexcept;
    void setCompressorRatio (float ratio) noexcept;
    void setCompressorAttackMs (float attackMs) noexcept;
    void setCompressorReleaseMs (float releaseMs) noexcept;
    void setCompressorMakeupGainDb (float makeupGainDb) noexcept;

    float getCompressorThresholdDb() const noexcept;
    float getCompressorRatio() const noexcept;
    float getCompressorAttackMs() const noexcept;
    float getCompressorReleaseMs() const noexcept;
    float getCompressorMakeupGainDb() const noexcept;

    void setDistortionDriveDb (float driveDb) noexcept;
    void setDistortionTone (float tone) noexcept;
    void setDistortionMode (DistortionMode mode) noexcept;

    float getDistortionDriveDb() const noexcept;
    float getDistortionTone() const noexcept;
    DistortionMode getDistortionMode() const noexcept;

    juce::String loadImpulseResponse (const juce::File& file);
    void clearImpulseResponse() noexcept;
    bool hasImpulseResponse() const noexcept;
    int getImpulseResponseLength() const noexcept;
    juce::String getImpulseResponseFilePath() const noexcept;

    void setReverbWetDry (float wetDry) noexcept;
    void setReverbDecay (float decay) noexcept;
    void setReverbSize (float size) noexcept;

    float getReverbWetDry() const noexcept;
    float getReverbDecay() const noexcept;
    float getReverbSize() const noexcept;

    juce::String loadVst3Plugin (const juce::PluginDescription& description, double sampleRate, int bufferSize);
    void unloadVst3Plugin() noexcept;
    bool isVst3PluginLoaded() const noexcept;
    bool isVst3PluginFailed() const noexcept;
    juce::String getVst3PluginName() const noexcept;
    juce::PluginDescription getVst3PluginDescription() const noexcept;
    void setVst3PluginBypassed (bool shouldBeBypassed) noexcept;
    bool isVst3PluginBypassed() const noexcept;
    juce::AudioPluginInstance* getVst3PluginInstance() const noexcept;

    int getNumProcessors() const noexcept { return chain.size(); }

    int getMeasuredProcessorCount() const noexcept { return chain.getMeasuredProcessorCount(); }
    juce::String getProcessorName (int index) const noexcept { return chain.getProcessorName (index); }
    double getProcessorTotalTimeMs (int index) const noexcept { return chain.getProcessorTotalTimeMs (index); }
    double getProcessorMaxTimeMs (int index) const noexcept { return chain.getProcessorMaxTimeMs (index); }

    void resetParameters() noexcept;

    int addClip (std::int64_t startSamples, std::int64_t lengthSamples, std::int64_t sourceOffsetSamples = 0);
    void removeClip (int clipIndex) noexcept;
    void moveClip (int clipIndex, std::int64_t newStartSamples) noexcept;
    void resizeClip (int clipIndex, std::int64_t newLengthSamples) noexcept;
    int getNumClips() const noexcept { return clipCount.load(); }
    const Clip& getClip (int clipIndex) const noexcept { return clips[static_cast<size_t> (clipIndex)]; }

    int addMidiEvent (std::int64_t timeSamples, const std::uint8_t* data, int numBytes);
    void clearMidiEvents() noexcept;
    int getNumMidiEvents() const noexcept { return midiEventCount.load(); }
    const MidiEventRecord& getMidiEvent (int eventIndex) const noexcept { return midiEvents[static_cast<size_t> (eventIndex)]; }

    void addAutomationPoint (AutomationParameter parameter, int pluginParameterIndex,
                             std::int64_t timeSamples, float value) noexcept;
    void clearAutomation (AutomationParameter parameter, int pluginParameterIndex = -1) noexcept;
    int getNumAutomationCurves() const noexcept { return automationCurveCount.load(); }
    const AutomationCurve& getAutomationCurve (int curveIndex) const noexcept { return automationCurves[static_cast<size_t> (curveIndex)]; }

    void prepare (double sampleRate, int bufferSize) noexcept;
    void reset() noexcept;

    void process (const float* const* input,
                  float* const* output,
                  int numChannels,
                  int numSamples,
                  bool anyTrackSoloed,
                  std::int64_t transportPositionSamples,
                  const ClipSource& clipSource,
                  juce::MidiBuffer& midiEvents,
                  bool transportPlaying,
                  const RoutingBuffers& routing,
                  bool forceMonoInput) noexcept;

private:
    void applyAutomation (std::int64_t transportPositionSamples) noexcept;

    ProcessorChain chain;
    GainProcessor* gainProcessor = nullptr;
    NoiseGateProcessor* gateProcessor = nullptr;
    ParametricEqProcessor* eqProcessor = nullptr;
    CompressorProcessor* compressorProcessor = nullptr;
    DistortionProcessor* distortionProcessor = nullptr;
    Vst3HostProcessor* vst3Processor = nullptr;
    ConvolutionProcessor* convolutionProcessor = nullptr;
    ReverbProcessor* reverbProcessor = nullptr;

    std::atomic<float> volumeLinear { 1.0f };
    float volumeDb = 0.0f;
    std::atomic<float> panValue { 0.0f };
    std::atomic<bool> muted { false };
    std::atomic<bool> soloed { false };

    std::atomic<int> routeTarget { -1 };
    std::atomic<int> sendTarget { -1 };
    std::atomic<float> sendAmount { 0.0f };

    std::vector<std::vector<float>> scratch;
    int numChannels = 0;

    Clip clips[maxClipsPerTrack];
    std::atomic<int> clipCount { 0 };

    MidiEventRecord midiEvents[maxMidiEventsPerTrack];
    std::atomic<int> midiEventCount { 0 };

    AutomationCurve automationCurves[maxAutomationCurvesPerTrack];
    std::atomic<int> automationCurveCount { 0 };
    int automationSearchIndex[maxAutomationCurvesPerTrack] = {};

    float volumeAutomationDb = 0.0f;
    float panAutomation = 0.0f;
    float gainAutomationDb = 0.0f;
    float gainDbBase = 0.0f;
};
} // namespace daw
