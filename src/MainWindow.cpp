#include "MainWindow.h"

#include "Project.h"

namespace daw
{
MainWindow::MainWindow (MainComponent& contentToOwn)
    : juce::DocumentWindow ("Linux DAW",
                            juce::Desktop::getInstance().getDefaultLookAndFeel()
                                .findColour (juce::ResizableWindow::backgroundColourId),
                            juce::DocumentWindow::allButtons),
      content (contentToOwn)
{
    setUsingNativeTitleBar (true);
    setContentNonOwned (&contentToOwn, false);
    setResizable (true, true);
    setMenuBar (this);
    centreWithSize (800, 900);
    setVisible (true);

}

MainWindow::~MainWindow()
{
    setMenuBar (nullptr);
}

void MainWindow::closeButtonPressed()
{
    juce::JUCEApplicationBase::quit();
}

juce::StringArray MainWindow::getMenuBarNames()
{
    return { "File", "Edit" };
}

juce::PopupMenu MainWindow::getMenuForIndex (int menuIndex, const juce::String&)
{
    if (menuIndex == 0)
    {
        juce::PopupMenu menu;

        menu.addItem (menuNewProject, "New Project");
        menu.addItem (menuOpenProject, "Open Project...");
        menu.addSeparator();
        menu.addItem (menuSaveProject, "Save Project");
        menu.addItem (menuSaveProjectAs, "Save Project As...");

        return menu;
    }

    juce::PopupMenu menu;
    menu.addItem (1, "Undo");
    menu.addItem (2, "Redo");

    return menu;
}

void MainWindow::menuItemSelected (int menuItemId, int topLevelMenuIndex)
{
    if (topLevelMenuIndex == 1)
    {
        if (menuItemId == 1)
            undo();
        else if (menuItemId == 2)
            redo();

        return;
    }

    switch (menuItemId)
    {
        case menuNewProject:    newProject(); break;
        case menuOpenProject:   openProject(); break;
        case menuSaveProject:   saveProject(); break;
        case menuSaveProjectAs: saveProjectAs(); break;
        default: break;
    }
}

bool MainWindow::keyPressed (const juce::KeyPress& key)
{
    if (key == juce::KeyPress ('z', juce::ModifierKeys::ctrlModifier, 0))
    {
        undo();
        return true;
    }

    if (key == juce::KeyPress ('z', juce::ModifierKeys::ctrlModifier | juce::ModifierKeys::shiftModifier, 0)
        || key == juce::KeyPress ('y', juce::ModifierKeys::ctrlModifier, 0))
    {
        redo();
        return true;
    }

    if (key == juce::KeyPress ('n', juce::ModifierKeys::ctrlModifier, 0))
    {
        newProject();
        return true;
    }

    if (key == juce::KeyPress ('o', juce::ModifierKeys::ctrlModifier, 0))
    {
        openProject();
        return true;
    }

    if (key == juce::KeyPress ('s', juce::ModifierKeys::ctrlModifier | juce::ModifierKeys::shiftModifier, 0))
    {
        saveProjectAs();
        return true;
    }

    if (key == juce::KeyPress ('s', juce::ModifierKeys::ctrlModifier, 0))
    {
        saveProject();
        return true;
    }

    return false;
}

void MainWindow::undo()
{
    auto& undoManager = content.getUndoManager();
    const auto error = undoManager.undo (content.getEngine().getAudioGraph());

    if (error.isNotEmpty())
    {
        showError ("Undo", error);
        return;
    }

    content.refreshAfterUndoRedo();
}

void MainWindow::redo()
{
    auto& undoManager = content.getUndoManager();
    const auto error = undoManager.redo (content.getEngine().getAudioGraph());

    if (error.isNotEmpty())
    {
        showError ("Redo", error);
        return;
    }

    content.refreshAfterUndoRedo();
}

void MainWindow::newProject()
{
    content.getEngine().getAudioGraph().resetToDefaults();
    currentProjectFile = juce::File();
    setName ("Linux DAW");
}

void MainWindow::openProject()
{
    auto chooser = std::make_shared<juce::FileChooser> ("Open Project", juce::File(), "*.ldaw");

    chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                          [this] (const juce::FileChooser& fc)
                          {
                              const auto result = fc.getResult();

                              juce::MessageManager::callAsync ([this, result]
                              {
                                  if (result == juce::File())
                                      return;

                                  const auto error = Project::load (result, content.getEngine().getAudioGraph());

                                  if (error.isNotEmpty())
                                  {
                                      showError ("Open Project", error);
                                      return;
                                  }

                                  currentProjectFile = result;
                                  setName ("Linux DAW - " + result.getFileName());
                              });
                          });
}

void MainWindow::saveProject()
{
    if (currentProjectFile == juce::File())
    {
        saveProjectAs();
        return;
    }

    const auto error = Project::save (currentProjectFile, content.getEngine().getAudioGraph());

    if (error.isNotEmpty())
        showError ("Save Project", error);
}

void MainWindow::saveProjectAs()
{
    auto chooser = std::make_shared<juce::FileChooser> ("Save Project As",
                                                        currentProjectFile == juce::File()
                                                            ? juce::File ("untitled.ldaw")
                                                            : currentProjectFile,
                                                        "*.ldaw");

    chooser->launchAsync (juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::warnAboutOverwriting,
                          [this] (const juce::FileChooser& fc)
                          {
                              auto result = fc.getResult();

                              if (result.getFileExtension().isEmpty())
                                  result = result.withFileExtension ("ldaw");

                              juce::MessageManager::callAsync ([this, result]
                              {
                                  if (result == juce::File())
                                      return;

                                  const auto error = Project::save (result, content.getEngine().getAudioGraph());

                                  if (error.isNotEmpty())
                                  {
                                      showError ("Save Project", error);
                                      return;
                                  }

                                  currentProjectFile = result;
                                  setName ("Linux DAW - " + result.getFileName());
                              });
                          });
}

void MainWindow::showError (const juce::String& title, const juce::String& message)
{
    juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::WarningIcon,
                                            title,
                                            message);
}
} // namespace daw
