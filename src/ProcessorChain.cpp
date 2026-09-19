#include "ProcessorChain.h"

#include <chrono>

#include "AtomicHelpers.h"

namespace daw
{

void ProcessorChain::append (std::unique_ptr<AudioProcessor> processor)
{
    processors.push_back (std::move (processor));

    if (measuredCount < maxMeasuredProcessors)
    {
        processorTotalTimeMs[measuredCount].store (0.0, std::memory_order_relaxed);
        processorMaxTimeMs[measuredCount].store (0.0, std::memory_order_relaxed);
        ++measuredCount;
    }
}

void ProcessorChain::clear()
{
    processors.clear();
    measuredCount = 0;
}

juce::String ProcessorChain::getProcessorName (int index) const noexcept
{
    return index >= 0 && index < static_cast<int> (processors.size())
             ? processors[static_cast<size_t> (index)]->getName()
             : juce::String();
}

double ProcessorChain::getProcessorTotalTimeMs (int index) const noexcept
{
    return index >= 0 && index < measuredCount ? processorTotalTimeMs[index].load() : 0.0;
}

double ProcessorChain::getProcessorMaxTimeMs (int index) const noexcept
{
    return index >= 0 && index < measuredCount ? processorMaxTimeMs[index].load() : 0.0;
}

void ProcessorChain::prepare (double sampleRate, int bufferSize) noexcept
{
    for (const auto& processor : processors)
        processor->prepare (sampleRate, bufferSize);
}

void ProcessorChain::process (float* const* channels, int numChannels, int numSamples) noexcept
{
    for (size_t i = 0; i < processors.size(); ++i)
    {
        const auto startTime = std::chrono::steady_clock::now();

        processors[i]->process (channels, numChannels, numSamples);

        const auto elapsedMs = std::chrono::duration<double, std::milli> (std::chrono::steady_clock::now() - startTime).count();

        if (i < static_cast<size_t> (measuredCount))
        {
            processorTotalTimeMs[i].fetch_add (elapsedMs, std::memory_order_relaxed);
            updateAtomicMax (processorMaxTimeMs[i], elapsedMs);
        }
    }
}

void ProcessorChain::processWithMidi (float* const* channels, int numChannels, int numSamples, juce::MidiBuffer& midiEvents) noexcept
{
    for (size_t i = 0; i < processors.size(); ++i)
    {
        const auto startTime = std::chrono::steady_clock::now();

        processors[i]->processWithMidi (channels, numChannels, numSamples, midiEvents);

        const auto elapsedMs = std::chrono::duration<double, std::milli> (std::chrono::steady_clock::now() - startTime).count();

        if (i < static_cast<size_t> (measuredCount))
        {
            processorTotalTimeMs[i].fetch_add (elapsedMs, std::memory_order_relaxed);
            updateAtomicMax (processorMaxTimeMs[i], elapsedMs);
        }
    }
}

void ProcessorChain::reset() noexcept
{
    for (const auto& processor : processors)
        processor->reset();
}
} // namespace daw
