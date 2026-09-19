#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "MainComponent.h"

namespace daw
{
class MainWindow final : public juce::DocumentWindow,
                         public juce::MenuBarModel
{
public:
    explicit MainWindow (MainComponent& contentToOwn);
    ~MainWindow() override;

    void closeButtonPressed() override;

    juce::StringArray getMenuBarNames() override;
    juce::PopupMenu getMenuForIndex (int menuIndex, const juce::String& menuName) override;
    void menuItemSelected (int menuItemId, int topLevelMenuIndex) override;

    bool keyPressed (const juce::KeyPress& key) override;

private:
    enum MenuIds
    {
        menuNewProject = 1,
        menuOpenProject,
        menuSaveProject,
        menuSaveProjectAs,
    };

    void newProject();
    void openProject();
    void saveProject();
    void saveProjectAs();
    void undo();
    void redo();

    void showError (const juce::String& title, const juce::String& message);

    MainComponent& content;
    juce::File currentProjectFile;
};
} // namespace daw
