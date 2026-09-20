#include <juce_gui_basics/juce_gui_basics.h>

#include "DarkLookAndFeel.h"
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
        juce::LookAndFeel::setDefaultLookAndFeel (&darkLookAndFeel);
        content = std::make_unique<daw::MainComponent>();
        mainWindow = std::make_unique<daw::MainWindow> (*content);
    }

    void shutdown() override
    {
        mainWindow.reset();
        content.reset();
        juce::LookAndFeel::setDefaultLookAndFeel (nullptr);
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
    daw::DarkLookAndFeel darkLookAndFeel;
    std::unique_ptr<daw::MainComponent> content;
    std::unique_ptr<daw::MainWindow> mainWindow;
};
} // namespace

START_JUCE_APPLICATION (LinuxDawApplication)
