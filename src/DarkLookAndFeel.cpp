#include "DarkLookAndFeel.h"

namespace daw
{
DarkLookAndFeel::DarkLookAndFeel()
{
    setColourScheme ({
        baseWindow(),   // windowBackground
        basePanel(),    // widgetBackground
        basePanel(),    // menuBackground
        baseOutline(),  // outline
        baseText(),     // defaultText
        accent(),       // defaultFill
        accent(),       // highlightedText
        accent(),       // highlightedFill
        baseText(),     // menuText
    });

    setColour (juce::TextEditor::backgroundColourId, basePanel());
    setColour (juce::TextEditor::textColourId, baseText());
    setColour (juce::TextEditor::highlightColourId, accent());
    setColour (juce::TextEditor::highlightedTextColourId, juce::Colours::black);
    setColour (juce::TextEditor::focusedOutlineColourId, accent());
    setColour (juce::CaretComponent::caretColourId, accent());
    setColour (juce::Label::textColourId, baseText());
    setColour (juce::Label::textWhenEditingColourId, baseText());
    setColour (juce::ComboBox::textColourId, baseText());
    setColour (juce::ComboBox::arrowColourId, accent());
    setColour (juce::Slider::thumbColourId, accent());
    setColour (juce::Slider::trackColourId, baseOutline());
    setColour (juce::Slider::rotarySliderFillColourId, accent());
    setColour (juce::Slider::rotarySliderOutlineColourId, baseOutline());
    setColour (juce::Slider::textBoxTextColourId, baseText());
    setColour (juce::Slider::textBoxBackgroundColourId, basePanel());
    setColour (juce::Slider::textBoxOutlineColourId, baseOutline());
    setColour (juce::ToggleButton::textColourId, baseText());
    setColour (juce::TextButton::textColourOnId, accent());
    setColour (juce::TextButton::textColourOffId, baseText());
    setColour (juce::TabbedComponent::outlineColourId, baseOutline());
    setColour (juce::TabbedComponent::backgroundColourId, baseWindow());
    setColour (juce::ScrollBar::thumbColourId, baseOutline());
    setColour (juce::ScrollBar::trackColourId, basePanel());
    setColour (juce::PopupMenu::backgroundColourId, basePanel());
    setColour (juce::PopupMenu::textColourId, baseText());
    setColour (juce::PopupMenu::highlightedBackgroundColourId, accent());
    setColour (juce::PopupMenu::highlightedTextColourId, juce::Colours::black);
    setColour (juce::ResizableWindow::backgroundColourId, baseWindow());
    setColour (juce::DocumentWindow::textColourId, baseText());
    setColour (juce::GroupComponent::outlineColourId, baseOutline());
    setColour (juce::GroupComponent::textColourId, accent());
}

void DarkLookAndFeel::drawRotarySlider (juce::Graphics& g,
                                        int x, int y, int width, int height,
                                        float sliderPos,
                                        float rotaryStartAngle,
                                        float rotaryEndAngle,
                                        juce::Slider& slider)
{
    const auto radius = juce::jmin (width / 2, height / 2) - 4.0f;
    const auto centreX = x + width * 0.5f;
    const auto centreY = y + height * 0.5f;
    const auto rx = centreX - radius;
    const auto ry = centreY - radius;
    const auto rw = radius * 2.0f;
    const auto angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);

    // knob body
    g.setColour (basePanel());
    g.fillEllipse (rx, ry, rw, rw);

    g.setColour (baseOutline());
    g.drawEllipse (rx, ry, rw, rw, 1.5f);

    // value arc
    juce::Path arc;
    arc.addCentredArc (centreX, centreY, radius, radius, 0.0f,
                       rotaryStartAngle, rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle),
                       true);

    g.setColour (slider.isEnabled() ? accent() : baseOutline());
    g.strokePath (arc, juce::PathStrokeType (3.0f));

    // pointer
    juce::Line<float> pointer (centreX, centreY,
                               centreX + radius * 0.75f * std::sin (angle),
                               centreY - radius * 0.75f * std::cos (angle));

    g.setColour (baseText());
    g.drawLine (pointer, 2.5f);

    // centre dot
    g.setColour (baseOutline());
    g.fillEllipse (centreX - 3.0f, centreY - 3.0f, 6.0f, 6.0f);
}

juce::Slider::SliderLayout DarkLookAndFeel::getSliderLayout (juce::Slider& slider)
{
    auto layout = juce::LookAndFeel_V4::getSliderLayout (slider);

    if (slider.getSliderStyle() == juce::Slider::RotaryHorizontalVerticalDrag)
        layout.textBoxBounds = slider.getLocalBounds().removeFromBottom (22);

    return layout;
}

void DarkLookAndFeel::drawButtonBackground (juce::Graphics& g,
                                            juce::Button& button,
                                            const juce::Colour&,
                                            bool isMouseOverButton,
                                            bool isButtonDown)
{
    const auto bounds = button.getLocalBounds().toFloat().reduced (1.0f);

    auto background = basePanel();

    if (button.getToggleState())
        background = accent().withAlpha (0.22f);
    else if (isButtonDown)
        background = accent().withAlpha (0.30f);
    else if (isMouseOverButton)
        background = baseOutline();

    g.setColour (background);
    g.fillRoundedRectangle (bounds, 6.0f);

    g.setColour (button.getToggleState() ? accent() : baseOutline());
    g.drawRoundedRectangle (bounds, 6.0f, 1.0f);
}

void DarkLookAndFeel::drawToggleButton (juce::Graphics& g,
                                        juce::ToggleButton& button,
                                        bool isMouseOverButton,
                                        bool isButtonDown)
{
    const auto bounds = button.getLocalBounds().toFloat();
    const auto box = juce::Rectangle<float> (bounds.getX() + 2.0f, bounds.getY() + 4.0f, 18.0f, 18.0f);

    g.setColour (button.getToggleState() ? accent() : basePanel());
    g.fillRoundedRectangle (box, 4.0f);

    g.setColour (button.getToggleState() ? accent() : baseOutline());
    g.drawRoundedRectangle (box, 4.0f, 1.0f);

    if (button.getToggleState())
    {
        g.setColour (juce::Colours::black);
        g.drawLine (box.getX() + 4.0f, box.getCentreY(), box.getX() + 7.5f, box.getBottom() - 4.0f, 2.0f);
        g.drawLine (box.getX() + 7.5f, box.getBottom() - 4.0f, box.getRight() - 3.0f, box.getY() + 4.5f, 2.0f);
    }

    g.setColour (isMouseOverButton && !isButtonDown ? accent() : baseText());
    g.setFont (juce::FontOptions{}.withHeight (14.0f));
    g.drawText (button.getButtonText(),
                juce::Rectangle<int> (static_cast<int> (box.getRight() + 6.0f), static_cast<int> (bounds.getY()),
                                      static_cast<int> (bounds.getWidth() - box.getRight() - 6.0f), static_cast<int> (bounds.getHeight())),
                juce::Justification::centredLeft, false);
}

void DarkLookAndFeel::drawComboBox (juce::Graphics& g,
                                    int width, int height,
                                    bool isButtonDown,
                                    int, int, int buttonW, int buttonH,
                                    juce::ComboBox&)
{
    g.setColour (basePanel());
    g.fillRoundedRectangle (0.0f, 0.0f, static_cast<float> (width), static_cast<float> (height), 6.0f);

    g.setColour (isButtonDown ? accent() : baseOutline());
    g.drawRoundedRectangle (1.0f, 1.0f, static_cast<float> (width - 2), static_cast<float> (height - 2), 6.0f, 1.0f);

    g.setColour (accent());
    g.drawLine (static_cast<float> (width - buttonW + buttonW / 4), static_cast<float> (height / 3),
                static_cast<float> (width - buttonW / 2), static_cast<float> (height * 2 / 3), 2.0f);
    g.drawLine (static_cast<float> (width - buttonW / 2), static_cast<float> (height * 2 / 3),
                static_cast<float> (width - buttonW / 4), static_cast<float> (height / 3), 2.0f);

    juce::ignoreUnused (buttonH);
}

void DarkLookAndFeel::drawPopupMenuBackground (juce::Graphics& g, int width, int height)
{
    g.setColour (basePanel());
    g.fillRoundedRectangle (0.0f, 0.0f, static_cast<float> (width), static_cast<float> (height), 8.0f);
    g.setColour (baseOutline());
    g.drawRoundedRectangle (0.5f, 0.5f, static_cast<float> (width - 1), static_cast<float> (height - 1), 8.0f, 1.0f);
}

void DarkLookAndFeel::drawTabButton (juce::TabBarButton& button,
                                     juce::Graphics& g,
                                     bool isMouseOver,
                                     bool isMouseDown)
{
    auto bounds = button.getActiveArea().toFloat();

    g.setColour (button.isFrontTab() ? baseWindow() : basePanel());
    g.fillRect (bounds);

    if (button.isFrontTab())
    {
        g.setColour (accent());
        g.fillRect (bounds.removeFromBottom (3.0f));
    }
    else
    {
        g.setColour (baseOutline());
        g.drawLine (bounds.getX(), bounds.getBottom(), bounds.getRight(), bounds.getBottom());
    }

    g.setColour (button.isFrontTab() || isMouseOver || isMouseDown ? accent() : baseText());
    g.setFont (juce::FontOptions{}.withHeight (14.0f));
    g.drawText (button.getButtonText(), button.getActiveArea().reduced (6, 4), juce::Justification::centred, false);
}

void DarkLookAndFeel::drawGroupComponentOutline (juce::Graphics& g, int width, int height,
                                                 const juce::String& text,
                                                 const juce::Justification& position,
                                                 juce::GroupComponent& group)
{
    const float textH = 16.0f;
    const auto bounds = juce::Rectangle<float> (0.0f, textH * 0.5f,
                                                static_cast<float> (width), static_cast<float> (height) - textH * 0.5f);

    g.setColour (baseOutline());
    g.drawRoundedRectangle (bounds, 8.0f, 1.0f);

    g.setColour (accent());
    g.setFont (juce::FontOptions{}.withHeight (13.0f).withStyle ("Bold"));
    g.drawText (text, 8, 0, width - 16, static_cast<int> (textH), position, false);

    juce::ignoreUnused (group);
}

void DarkLookAndFeel::fillTextEditorBackground (juce::Graphics& g, int width, int height, juce::TextEditor& textEditor)
{
    if (textEditor.isReadOnly())
        g.setColour (basePanel());
    else
        g.setColour (baseWindow());

    g.fillRoundedRectangle (0.0f, 0.0f, static_cast<float> (width), static_cast<float> (height), 6.0f);
}

void DarkLookAndFeel::drawTextEditorOutline (juce::Graphics& g, int width, int height, juce::TextEditor& textEditor)
{
    if (textEditor.isEnabled())
    {
        g.setColour (textEditor.hasKeyboardFocus (true) ? accent() : baseOutline());
        g.drawRoundedRectangle (0.5f, 0.5f, static_cast<float> (width - 1), static_cast<float> (height - 1), 6.0f, 1.0f);
    }
}
} // namespace daw
