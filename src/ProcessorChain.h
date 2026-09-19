#pragma once

#include <atomic>
#include <memory>
#include <vector>

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_core/juce_core.h>

#include "AudioProcessor.h"

namespace daw
{
class ProcessorChain final : public AudioProcessor
{
public:
    static constexpr int maxMeasuredProcessors = 16;

    void append (std::unique_ptr<AudioProcessor> processor);
    void clear();

    int size() const noexcept { return static_cast<int> (processors.size()); }

    void prepare (double sampleRate, int bufferSize) noexcept override;
    void process (float* const* channels, int numChannels, int numSamples) noexcept override;
    void processWithMidi (float* const* channels, int numChannels, int numSamples, juce::MidiBuffer& midiEvents) noexcept override;
    void reset() noexcept override;

    int getMeasuredProcessorCount() const noexcept { return measuredCount; }
    juce::String getProcessorName (int index) const noexcept;
    double getProcessorTotalTimeMs (int index) const noexcept;
    double getProcessorMaxTimeMs (int index) const noexcept;

private:
    std::vector<std::unique_ptr<AudioProcessor>> processors;

    std::atomic<double> processorTotalTimeMs[maxMeasuredProcessors] = {};
    std::atomic<double> processorMaxTimeMs[maxMeasuredProcessors] = {};
    int measuredCount = 0;
};
} // namespace daw
