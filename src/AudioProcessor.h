#pragma once

namespace daw
{
class AudioProcessor
{
public:
    virtual ~AudioProcessor() = default;

    virtual void prepare (double sampleRate, int bufferSize) noexcept = 0;
    virtual void process (float* const* channels, int numChannels, int numSamples) noexcept = 0;
    virtual void reset() noexcept = 0;
};
} // namespace daw
