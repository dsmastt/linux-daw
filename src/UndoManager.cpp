#include "UndoManager.h"

namespace daw
{
void UndoManager::captureUndoState (const AudioGraph& graph)
{
    undoStack.push_back (Project::captureState (graph));

    if (undoStack.size() > maxStackDepth)
        undoStack.erase (undoStack.begin());

    redoStack.clear();
}

juce::String UndoManager::undo (AudioGraph& graph)
{
    if (undoStack.empty())
        return {};

    redoStack.push_back (Project::captureState (graph));

    const auto state = undoStack.back();
    undoStack.pop_back();

    return Project::restoreState (state, graph, {});
}

juce::String UndoManager::redo (AudioGraph& graph)
{
    if (redoStack.empty())
        return {};

    undoStack.push_back (Project::captureState (graph));

    const auto state = redoStack.back();
    redoStack.pop_back();

    return Project::restoreState (state, graph, {});
}

void UndoManager::clear() noexcept
{
    undoStack.clear();
    redoStack.clear();
}
} // namespace daw
