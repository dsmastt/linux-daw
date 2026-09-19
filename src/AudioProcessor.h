#pragma once

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

    virtual void processWithMidi (float* const* channels,
                                  int numChannels,
                                  int numSamples,
                                  juce::MidiBuffer& midiEvents) noexcept
    {
        juce::ignoreUnused (midiEvents);
        process (channels, numChannels, numSamples);
    }
};
} // namespace daw
