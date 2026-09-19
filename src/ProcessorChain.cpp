#include "ProcessorChain.h"

namespace daw
{
void ProcessorChain::append (std::unique_ptr<AudioProcessor> processor)
{
    processors.push_back (std::move (processor));
}

void ProcessorChain::clear()
{
    processors.clear();
}

void ProcessorChain::prepare (double sampleRate, int bufferSize) noexcept
{
    for (const auto& processor : processors)
        processor->prepare (sampleRate, bufferSize);
}

void ProcessorChain::process (float* const* channels, int numChannels, int numSamples) noexcept
{
    for (const auto& processor : processors)
        processor->process (channels, numChannels, numSamples);
}

void ProcessorChain::processWithMidi (float* const* channels, int numChannels, int numSamples, juce::MidiBuffer& midiEvents) noexcept
{
    for (const auto& processor : processors)
        processor->processWithMidi (channels, numChannels, numSamples, midiEvents);
}

void ProcessorChain::reset() noexcept
{
    for (const auto& processor : processors)
        processor->reset();
}
} // namespace daw
