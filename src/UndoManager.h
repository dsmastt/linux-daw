#pragma once

#include <vector>

#include <juce_core/juce_core.h>

#include "AudioGraph.h"
#include "Project.h"

namespace daw
{
class AudioGraph;

class UndoManager
{
public:
    void captureUndoState (const AudioGraph& graph);

    juce::String undo (AudioGraph& graph);
    juce::String redo (AudioGraph& graph);

    bool canUndo() const noexcept { return !undoStack.empty(); }
    bool canRedo() const noexcept { return !redoStack.empty(); }

    void clear() noexcept;

private:
    static constexpr size_t maxStackDepth = 100;

    std::vector<juce::var> undoStack;
    std::vector<juce::var> redoStack;
};
} // namespace daw
