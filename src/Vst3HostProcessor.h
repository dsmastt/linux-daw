#pragma once

#include <atomic>
#include <memory>
#include <vector>

#include <juce_core/juce_core.h>
#include <juce_audio_processors/juce_audio_processors.h>

#include "AudioProcessor.h"

namespace daw
{
class Vst3HostProcessor final : public AudioProcessor
{
public:
    juce::String getName() const override { return "VST3 Plugin"; }
    juce::String loadPlugin (const juce::PluginDescription& description, double sampleRate, int bufferSize);
    void unloadPlugin() noexcept;

    bool isLoaded() const noexcept { return loaded.load(); }
    juce::String getPluginName() const noexcept
    {
        const juce::SpinLock::ScopedLockType lock (instanceLock);
        return pluginName;
    }

    juce::PluginDescription getPluginDescription() const noexcept
    {
        const juce::SpinLock::ScopedLockType lock (instanceLock);
        return pluginDescription;
    }
    bool hasFailed() const noexcept { return failed.load(); }

    juce::AudioPluginInstance* getInstance() const noexcept { return instance.get(); }

    void prepare (double sampleRate, int bufferSize) noexcept override;
    void process (float* const* channels, int numChannels, int numSamples) noexcept override;
    void processWithMidi (float* const* channels, int numChannels, int numSamples, juce::MidiBuffer& midiEvents) noexcept override;
    void reset() noexcept override;

private:
    std::atomic<bool> loaded { false };
    std::atomic<bool> failed { false };
    juce::String pluginName;
    juce::PluginDescription pluginDescription;
    std::unique_ptr<juce::AudioPluginInstance> instance;

    // Guards instance against the audio thread while a plugin is being
    // loaded or unloaded. The audio thread only try-locks, so it never
    // blocks; it simply bypasses the plugin for that block.
    mutable juce::SpinLock instanceLock;

    juce::AudioBuffer<float> pluginBuffer;
    juce::MidiBuffer midiBuffer;
    double sampleRate = 0.0;
    int bufferSize = 0;
};

std::vector<juce::PluginDescription> scanVst3Plugins (const juce::StringArray& paths,
                                                      const juce::File& deadMansPedalFile = {});
} // namespace daw
