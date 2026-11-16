#pragma once

#include <functional>

#include <juce_gui_basics/juce_gui_basics.h>

#include "AudioEngine.h"
#include "Preset.h"
#include "UndoManager.h"

namespace daw
{
class PresetComponent final : public juce::Component
{
public:
    PresetComponent (AudioEngine& engineToUse,
                     UndoManager& undoToUse,
                     std::function<void()> onPresetLoaded,
                     std::function<void()> onPresetBeforeLoad);
    ~PresetComponent() override = default;

    void resized() override;

    void refreshPresets();

private:
    void loadSelectedPreset();
    void savePresetAs();
    void deleteSelectedPreset();
    void updateButtons();

    AudioEngine& engine;
    UndoManager& undo;
    std::function<void()> onLoaded;
    std::function<void()> onBeforeLoad;

    juce::Label titleLabel;
    juce::Label hintLabel;
    juce::ComboBox presetCombo;
    juce::TextButton loadButton;
    juce::TextButton saveButton;
    juce::TextButton deleteButton;
    juce::TextButton refreshButton;
    juce::TextEditor statusText;

    juce::Array<juce::File> presetFiles;
};
} // namespace daw
