#pragma once

#include <juce_core/juce_core.h>

#include "AudioGraph.h"

namespace daw
{
class Preset
{
public:
    static juce::File getPresetDirectory();
    static juce::Array<juce::File> listPresets();

    static juce::String save (const juce::File& file, const AudioGraph& graph);
    static juce::String load (const juce::File& file, AudioGraph& graph);
};
} // namespace daw
