#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace daw
{
class DarkLookAndFeel final : public juce::LookAndFeel_V4
{
public:
    DarkLookAndFeel();

    void drawRotarySlider (juce::Graphics& g,
                           int x, int y, int width, int height,
                           float sliderPosProportional,
                           float rotaryStartAngle,
                           float rotaryEndAngle,
                           juce::Slider& slider) override;

    juce::Slider::SliderLayout getSliderLayout (juce::Slider& slider) override;

    void drawButtonBackground (juce::Graphics& g,
                               juce::Button& button,
                               const juce::Colour& backgroundColour,
                               bool shouldDrawButtonAsHighlighted,
                               bool shouldDrawButtonAsDown) override;

    void drawToggleButton (juce::Graphics& g,
                           juce::ToggleButton& button,
                           bool shouldDrawButtonAsHighlighted,
                           bool shouldDrawButtonAsDown) override;

    void drawComboBox (juce::Graphics& g,
                       int width, int height,
                       bool isButtonDown,
                       int buttonX, int buttonY,
                       int buttonW, int buttonH,
                       juce::ComboBox& comboBox) override;

    void drawPopupMenuBackground (juce::Graphics& g, int width, int height) override;

    void drawTabButton (juce::TabBarButton& button,
                        juce::Graphics& g,
                        bool isMouseOver,
                        bool isMouseDown) override;

    void drawGroupComponentOutline (juce::Graphics& g, int width, int height,
                                    const juce::String& text,
                                    const juce::Justification& position,
                                    juce::GroupComponent& group) override;

    void fillTextEditorBackground (juce::Graphics& g, int width, int height, juce::TextEditor& textEditor) override;

    void drawTextEditorOutline (juce::Graphics& g, int width, int height, juce::TextEditor& textEditor) override;

    static juce::Colour accent()      { return juce::Colour (0xff34d399); }
    static juce::Colour basePanel()   { return juce::Colour (0xff14171c); }
    static juce::Colour baseWindow()  { return juce::Colour (0xff0d0f12); }
    static juce::Colour baseOutline() { return juce::Colour (0xff262b33); }
    static juce::Colour baseText()    { return juce::Colour (0xffdbe3ec); }
};
} // namespace daw
