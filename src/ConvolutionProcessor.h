#pragma once

#include <atomic>
#include <memory>
#include <vector>

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_core/juce_core.h>
#include <juce_dsp/juce_dsp.h>

#include "AudioProcessor.h"

namespace daw
{
class ConvolutionProcessor final : public AudioProcessor
{
public:
    juce::String getName() const override { return "Convolution"; }
    juce::String loadImpulseResponse (const juce::File& file);
    void clearImpulseResponse();

    bool hasImpulseResponse() const noexcept { return hasImpulseResponse_.load(); }
    int getImpulseResponseLength() const noexcept;
    juce::String getImpulseResponseFilePath() const noexcept { return impulseResponseFilePath; }

    void prepare (double sampleRate, int bufferSize) noexcept override;
    void process (float* const* channels, int numChannels, int numSamples) noexcept override;
    void reset() noexcept override;

private:
    static constexpr int maxChannels = 8;

    void buildPartitions (double sampleRate, int bufferSize) noexcept;
    void updateChannelHistories (const float* const* channels, int numChannels, int numSamples) noexcept;
    void processWithDirectConvolution (float* const* channels, int numChannels, int numSamples) noexcept;

    std::atomic<bool> hasImpulseResponse_ { false };
    std::vector<float> impulseResponse;
    double impulseResponseSampleRate = 0.0;
    juce::String impulseResponseFilePath;

    std::vector<float> processedImpulseResponse;
    std::vector<float> irSpectra;
    std::vector<float> inputHistory;
    std::vector<float> fftBuffer;
    std::vector<float> accumulator;
    std::vector<float> channelHistories[maxChannels];

    std::unique_ptr<juce::dsp::FFT> fft;

    int fftSize = 0;
    int partitionSize = 0;
    int numPartitions = 0;
    bool partitionsReady = false;
    int numChannels = 0;
};
} // namespace daw
