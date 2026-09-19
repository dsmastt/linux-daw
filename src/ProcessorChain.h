#pragma once

#include <memory>
#include <vector>

#include "AudioProcessor.h"

namespace daw
{
class ProcessorChain final : public AudioProcessor
{
public:
    void append (std::unique_ptr<AudioProcessor> processor);
    void clear();

    int size() const noexcept { return static_cast<int> (processors.size()); }

    void prepare (double sampleRate, int bufferSize) noexcept override;
    void process (float* const* channels, int numChannels, int numSamples) noexcept override;
    void reset() noexcept override;

private:
    std::vector<std::unique_ptr<AudioProcessor>> processors;
};
} // namespace daw
