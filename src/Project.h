#pragma once

#include <juce_core/juce_core.h>

#include "AudioGraph.h"

namespace daw
{
class Project
{
public:
    static juce::String save (const juce::File& file, AudioGraph& graph);
    static juce::String load (const juce::File& file, AudioGraph& graph);

    static juce::var captureState (const AudioGraph& graph);
    static juce::String restoreState (const juce::var& state, AudioGraph& graph, const juce::File& projectFile);

private:
    static juce::var graphToVar (const juce::File& projectFile, const AudioGraph& graph);
    static juce::String applyVar (const juce::File& projectFile, const juce::var& root, AudioGraph& graph);

    static juce::var trackToVar (const Track& track);
    static juce::String applyTrack (const juce::var& trackVar, Track& track, double sampleRate, int bufferSize);
};
} // namespace daw
