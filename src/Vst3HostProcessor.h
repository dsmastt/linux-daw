#pragma once

#include <atomic>
#include <memory>
#include <vector>

#include <juce_audio_processors/juce_audio_processors.h>

#include "AudioProcessor.h"

namespace daw
{
class Vst3HostProcessor final : public AudioProcessor
{
public:
    juce::String loadPlugin (const juce::PluginDescription& description, double sampleRate, int bufferSize);
    void unloadPlugin() noexcept;

    bool isLoaded() const noexcept { return loaded.load(); }
    juce::String getPluginName() const noexcept { return pluginName; }

    void setBypassed (bool shouldBeBypassed) noexcept { bypassed.store (shouldBeBypassed); }
    bool isBypassed() const noexcept { return bypassed.load(); }

    juce::AudioPluginInstance* getInstance() const noexcept { return instance.get(); }

    void prepare (double sampleRate, int bufferSize) noexcept override;
    void process (float* const* channels, int numChannels, int numSamples) noexcept override;
    void reset() noexcept override;

private:
    std::atomic<bool> loaded { false };
    std::atomic<bool> bypassed { false };
    juce::String pluginName;
    std::unique_ptr<juce::AudioPluginInstance> instance;
    juce::AudioBuffer<float> pluginBuffer;
    juce::MidiBuffer midiBuffer;
    double sampleRate = 0.0;
    int bufferSize = 0;
};

std::vector<juce::PluginDescription> scanVst3Plugins (const juce::StringArray& paths);
} // namespace daw
