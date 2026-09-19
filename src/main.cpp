#include <juce_gui_basics/juce_gui_basics.h>

#include "MainWindow.h"

namespace
{
class LinuxDawApplication final : public juce::JUCEApplication
{
public:
    LinuxDawApplication() = default;

    const juce::String getApplicationName() override { return "Linux DAW"; }
    const juce::String getApplicationVersion() override { return "0.1.0"; }
    bool moreThanOneInstanceAllowed() override { return true; }

    void initialise (const juce::String& commandLine) override
    {
        juce::ignoreUnused (commandLine);
        mainWindow = std::make_unique<daw::MainWindow>();
    }

    void shutdown() override
    {
        mainWindow.reset();
    }

    void systemRequestedQuit() override
    {
        quit();
    }

    void anotherInstanceStarted (const juce::String& commandLine) override
    {
        juce::ignoreUnused (commandLine);
    }

private:
    std::unique_ptr<daw::MainWindow> mainWindow;
};
} // namespace

START_JUCE_APPLICATION (LinuxDawApplication)
