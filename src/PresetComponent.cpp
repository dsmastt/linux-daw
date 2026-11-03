#include "PresetComponent.h"

#include "DarkLookAndFeel.h"

namespace daw
{
PresetComponent::PresetComponent (AudioEngine& engineToUse,
                                  UndoManager& undoToUse,
                                  std::function<void()> onPresetLoaded)
    : engine (engineToUse),
      undo (undoToUse),
      onLoaded (std::move (onPresetLoaded))
{
    titleLabel.setText ("DSP Presets", juce::dontSendNotification);
    titleLabel.setFont (juce::FontOptions{}.withHeight (20.0f).withStyle ("Bold"));
    titleLabel.setColour (juce::Label::textColourId, DarkLookAndFeel::accent());
    addAndMakeVisible (titleLabel);

    hintLabel.setText ("Save and recall the complete input effect chain, including bypass states and the amp plugin.",
                       juce::dontSendNotification);
    hintLabel.setColour (juce::Label::textColourId, DarkLookAndFeel::baseText().withAlpha (0.65f));
    addAndMakeVisible (hintLabel);

    presetCombo.onChange = [this] { updateButtons(); };
    addAndMakeVisible (presetCombo);

    loadButton.setButtonText ("Load");
    loadButton.onClick = [this] { loadSelectedPreset(); };
    addAndMakeVisible (loadButton);

    saveButton.setButtonText ("Save As...");
    saveButton.onClick = [this] { savePresetAs(); };
    addAndMakeVisible (saveButton);

    deleteButton.setButtonText ("Delete");
    deleteButton.onClick = [this] { deleteSelectedPreset(); };
    addAndMakeVisible (deleteButton);

    refreshButton.setButtonText ("Refresh");
    refreshButton.onClick = [this] { refreshPresets(); };
    addAndMakeVisible (refreshButton);

    statusText.setMultiLine (true, false);
    statusText.setReadOnly (true);
    statusText.setCaretVisible (false);
    statusText.setColour (juce::TextEditor::backgroundColourId, DarkLookAndFeel::basePanel());
    statusText.setColour (juce::TextEditor::textColourId, DarkLookAndFeel::baseText());
    statusText.setColour (juce::TextEditor::outlineColourId, DarkLookAndFeel::baseOutline());
    statusText.setText ("Presets are stored in " + Preset::getPresetDirectory().getFullPathName(),
                        juce::dontSendNotification);
    addAndMakeVisible (statusText);

    refreshPresets();
}

void PresetComponent::resized()
{
    auto bounds = getLocalBounds().reduced (12);

    titleLabel.setBounds (bounds.removeFromTop (30));
    hintLabel.setBounds (bounds.removeFromTop (24));
    bounds.removeFromTop (8);

    auto comboRow = bounds.removeFromTop (30);
    refreshButton.setBounds (comboRow.removeFromRight (80));
    comboRow.removeFromRight (8);
    presetCombo.setBounds (comboRow);

    bounds.removeFromTop (8);

    auto buttonRow = bounds.removeFromTop (32);
    loadButton.setBounds (buttonRow.removeFromLeft (90));
    buttonRow.removeFromLeft (8);
    saveButton.setBounds (buttonRow.removeFromLeft (110));
    buttonRow.removeFromLeft (8);
    deleteButton.setBounds (buttonRow.removeFromLeft (90));

    bounds.removeFromTop (10);
    statusText.setBounds (bounds.removeFromTop (juce::jmax (80, bounds.getHeight() / 2)));
}

void PresetComponent::refreshPresets()
{
    const auto selectedFile = presetCombo.getSelectedItemIndex() >= 0
                                  && presetCombo.getSelectedItemIndex() < presetFiles.size()
                                ? presetFiles[presetCombo.getSelectedItemIndex()]
                                : juce::File();

    presetFiles = Preset::listPresets();
    presetCombo.clear();

    for (const auto& file : presetFiles)
        presetCombo.addItem (file.getFileNameWithoutExtension(), presetCombo.getNumItems() + 1);

    if (presetFiles.isEmpty())
    {
        presetCombo.addItem ("No presets found", 1);
        presetCombo.setSelectedItemIndex (0, juce::dontSendNotification);
        statusText.setText ("No presets yet. Use Save As... to create one.",
                            juce::dontSendNotification);
        updateButtons();
        return;
    }

    int indexToSelect = 0;

    for (int i = 0; i < presetFiles.size(); ++i)
    {
        if (presetFiles[i] == selectedFile)
        {
            indexToSelect = i;
            break;
        }
    }

    presetCombo.setSelectedItemIndex (indexToSelect, juce::dontSendNotification);
    updateButtons();
}

void PresetComponent::updateButtons()
{
    const bool hasPreset = presetCombo.getSelectedItemIndex() >= 0
                           && presetCombo.getSelectedItemIndex() < presetFiles.size()
                           && ! presetFiles.isEmpty();

    loadButton.setEnabled (hasPreset);
    deleteButton.setEnabled (hasPreset);
}

void PresetComponent::loadSelectedPreset()
{
    const int index = presetCombo.getSelectedItemIndex();

    if (index < 0 || index >= presetFiles.size())
        return;

    undo.captureUndoState (engine.getAudioGraph());

    const auto error = Preset::load (presetFiles[index], engine.getAudioGraph());

    if (error.isNotEmpty())
    {
        statusText.setText ("Error: " + error, juce::dontSendNotification);
        return;
    }

    if (onLoaded != nullptr)
        onLoaded();

    statusText.setText ("Loaded preset: " + presetFiles[index].getFileNameWithoutExtension(),
                        juce::dontSendNotification);
}

void PresetComponent::savePresetAs()
{
    const auto selectedIndex = presetCombo.getSelectedItemIndex();
    const auto suggestedName = selectedIndex >= 0 && selectedIndex < presetFiles.size()
                                 ? presetFiles[selectedIndex].getFileName()
                                 : juce::String ("My Preset.ldpreset");

    auto chooser = std::make_shared<juce::FileChooser> ("Save DSP Preset",
                                                        Preset::getPresetDirectory().getChildFile (suggestedName),
                                                        "*.ldpreset");

    juce::Component::SafePointer<PresetComponent> safeThis (this);

    chooser->launchAsync (juce::FileBrowserComponent::saveMode
                              | juce::FileBrowserComponent::canSelectFiles
                              | juce::FileBrowserComponent::warnAboutOverwriting,
                          [safeThis, chooser] (const juce::FileChooser& fileChooser)
    {
        if (safeThis == nullptr)
            return;

        auto file = fileChooser.getResult();

        if (file == juce::File())
            return;

        if (! file.hasFileExtension ("ldpreset"))
            file = file.withFileExtension ("ldpreset");

        const auto error = Preset::save (file, safeThis->engine.getAudioGraph());

        if (error.isNotEmpty())
        {
            safeThis->statusText.setText ("Error: " + error, juce::dontSendNotification);
            return;
        }

        safeThis->refreshPresets();

        for (int i = 0; i < safeThis->presetFiles.size(); ++i)
        {
            if (safeThis->presetFiles[i] == file)
            {
                safeThis->presetCombo.setSelectedItemIndex (i, juce::dontSendNotification);
                break;
            }
        }

        safeThis->updateButtons();
        safeThis->statusText.setText ("Saved preset: " + file.getFileNameWithoutExtension(),
                                      juce::dontSendNotification);
    });
}

void PresetComponent::deleteSelectedPreset()
{
    const int index = presetCombo.getSelectedItemIndex();

    if (index < 0 || index >= presetFiles.size())
        return;

    const auto file = presetFiles[index];

    juce::Component::SafePointer<PresetComponent> safeThis (this);

    juce::AlertWindow::showOkCancelBox (juce::MessageBoxIconType::QuestionIcon,
                                        "Delete preset",
                                        "Delete \"" + file.getFileNameWithoutExtension() + "\"?",
                                        "Delete",
                                        "Cancel",
                                        nullptr,
                                        juce::ModalCallbackFunction::create ([safeThis, file] (int result)
                                        {
                                            if (safeThis == nullptr || result == 0)
                                                return;

                                            if (file.deleteFile())
                                            {
                                                safeThis->refreshPresets();
                                                safeThis->statusText.setText ("Deleted preset: "
                                                                                  + file.getFileNameWithoutExtension(),
                                                                              juce::dontSendNotification);
                                            }
                                            else
                                            {
                                                safeThis->statusText.setText ("Could not delete preset file",
                                                                              juce::dontSendNotification);
                                            }
                                        }));
}
} // namespace daw
