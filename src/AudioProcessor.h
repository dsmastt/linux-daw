#pragma once

#include <atomic>

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_core/juce_core.h>

namespace daw
{
class AudioProcessor
{
public:
    virtual ~AudioProcessor() = default;

    virtual juce::String getName() const { return "Processor"; }

    virtual void prepare (double sampleRate, int bufferSize) noexcept = 0;
    virtual void process (float* const* channels, int numChannels, int numSamples) noexcept = 0;
    virtual void reset() noexcept = 0;

    void setBypassed (bool shouldBeBypassed) noexcept
    {
        bypassed.store (shouldBeBypassed, std::memory_order_relaxed);
    }

    bool isBypassed() const noexcept
    {
        return bypassed.load (std::memory_order_relaxed);
    }

    virtual void processWithMidi (float* const* channels,
                                  int numChannels,
                                  int numSamples,
                                  juce::MidiBuffer& midiEvents) noexcept
    {
        juce::ignoreUnused (midiEvents);
        process (channels, numChannels, numSamples);
    }

protected:
    std::atomic<bool> bypassed { false };
};
} // namespace daw
