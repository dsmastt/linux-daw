#include "MainWindow.h"

namespace daw
{
MainWindow::MainWindow()
    : juce::DocumentWindow ("Linux DAW",
                            juce::Desktop::getInstance().getDefaultLookAndFeel()
                                .findColour (juce::ResizableWindow::backgroundColourId),
                            juce::DocumentWindow::allButtons)
{
    setUsingNativeTitleBar (true);
    setContentOwned (new MainComponent(), true);
    setResizable (true, true);
    centreWithSize (560, 420);
    setVisible (true);
}

void MainWindow::closeButtonPressed()
{
    juce::JUCEApplicationBase::quit();
}
} // namespace daw
