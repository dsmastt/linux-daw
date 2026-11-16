#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_core/juce_core.h>

namespace daw
{
class NamPreset
{
public:
    static juce::String loadModel (juce::AudioPluginInstance& instance,
                                   const juce::String& pluginVersion,
                                   const juce::File& modelFile,
                                   const juce::String& currentIrWindowsPath,
                                   juce::String& newModelWindowsPath);

    static juce::String loadIr (juce::AudioPluginInstance& instance,
                                const juce::String& pluginVersion,
                                const juce::File& irFile,
                                const juce::String& currentModelWindowsPath,
                                juce::String& newIrWindowsPath);
};
} // namespace daw
