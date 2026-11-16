#pragma once

#include <vector>

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_core/juce_core.h>

namespace daw
{
struct NeuralPresetEntry
{
    juce::File file;
    juce::String displayName;
    juce::String relativePath;
};

class NeuralPreset
{
public:
    static std::vector<NeuralPresetEntry> findPresets (const juce::String& pluginName);
    static juce::String load (const juce::File& file,
                              juce::AudioPluginInstance& instance,
                              int& matchedParameters,
                              int& totalParameters);
};
} // namespace daw
