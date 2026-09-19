#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "MainComponent.h"

namespace daw
{
class MainWindow final : public juce::DocumentWindow
{
public:
    MainWindow();

    void closeButtonPressed() override;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainWindow)
};
} // namespace daw
