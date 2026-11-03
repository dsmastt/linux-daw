#include "TunerComponent.h"

#include <cmath>

#include "DarkLookAndFeel.h"

namespace daw
{
TunerComponent::TunerComponent (AudioEngine& engineToUse)
    : engine (engineToUse)
{
    startTimerHz (15);
}

void TunerComponent::resized()
{
    repaint();
}

void TunerComponent::timerCallback()
{
    if (! isShowing())
        return;

    if (! engine.isRunning())
    {
        hasReading = false;
        displayedFrequency = 0.0f;
        holdTicks = 0;
        repaint();
        return;
    }

    const auto reading = engine.getTuner().analyse (engine.getSampleRate());

    if (reading.hasSignal && reading.confidence >= 0.3f)
    {
        if (displayedFrequency <= 0.0f
            || std::abs (reading.frequencyHz - displayedFrequency) > displayedFrequency * 0.12f)
        {
            displayedFrequency = reading.frequencyHz;
        }
        else
        {
            displayedFrequency = displayedFrequency * 0.7f + reading.frequencyHz * 0.3f;
        }

        holdTicks = 20;
        hasReading = true;
    }
    else if (holdTicks > 0)
    {
        --holdTicks;
    }
    else
    {
        hasReading = false;
        displayedFrequency = 0.0f;
    }

    if (hasReading)
        currentNote = Tuner::getNoteName (displayedFrequency, currentCents, currentOctave);

    repaint();
}

void TunerComponent::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().reduced (24);

    g.setColour (DarkLookAndFeel::basePanel());
    g.fillRoundedRectangle (bounds.toFloat(), 8.0f);
    g.setColour (DarkLookAndFeel::baseOutline());
    g.drawRoundedRectangle (bounds.toFloat(), 8.0f, 1.0f);

    bounds.reduce (24, 18);

    g.setColour (DarkLookAndFeel::accent());
    g.setFont (juce::FontOptions{}.withHeight (18.0f).withStyle ("Bold"));
    g.drawText ("TUNER", bounds.removeFromTop (26), juce::Justification::centredLeft, false);

    bounds.removeFromTop (8);

    if (! engine.isRunning())
    {
        g.setColour (DarkLookAndFeel::baseText().withAlpha (0.7f));
        g.setFont (juce::FontOptions{}.withHeight (22.0f));
        g.drawText ("Start audio to tune", bounds, juce::Justification::centred, false);
        return;
    }

    if (! hasReading)
    {
        g.setColour (DarkLookAndFeel::baseText().withAlpha (0.6f));
        g.setFont (juce::FontOptions{}.withHeight (48.0f));
        g.drawText ("-", bounds.removeFromTop (140), juce::Justification::centred, false);

        g.setFont (juce::FontOptions{}.withHeight (20.0f));
        g.drawText ("Play a note", bounds.removeFromTop (40), juce::Justification::centred, false);
        return;
    }

    const auto noteText = currentNote + juce::String (currentOctave);

    g.setColour (DarkLookAndFeel::baseText());
    g.setFont (juce::FontOptions{}.withHeight (86.0f).withStyle ("Bold"));
    g.drawText (noteText, bounds.removeFromTop (110), juce::Justification::centred, false);

    g.setFont (juce::FontOptions{}.withHeight (18.0f));
    g.setColour (DarkLookAndFeel::accent());
    g.drawText (juce::String (displayedFrequency, 1) + " Hz",
                bounds.removeFromTop (24), juce::Justification::centred, false);

    g.setColour (std::abs (currentCents) <= 3.0f
                     ? DarkLookAndFeel::accent()
                     : (std::abs (currentCents) <= 15.0f ? juce::Colours::orange : juce::Colours::tomato));
    g.setFont (juce::FontOptions{}.withHeight (20.0f));
    g.drawText ((currentCents >= 0.0f ? "+" : "") + juce::String (currentCents, 1) + " cents",
                bounds.removeFromTop (28), juce::Justification::centred, false);

    auto bar = bounds.removeFromTop (54).withSizeKeepingCentre (juce::jmin (600, bounds.getWidth()), 34);
    bar = bar.withY (bar.getY() + 10);

    g.setColour (DarkLookAndFeel::baseOutline());
    g.fillRoundedRectangle (bar.toFloat(), 6.0f);

    const float centreX = static_cast<float> (bar.getCentreX());
    const float halfWidth = static_cast<float> (bar.getWidth()) * 0.5f - 10.0f;

    g.setColour (DarkLookAndFeel::baseText().withAlpha (0.35f));

    for (int tick = -50; tick <= 50; tick += 10)
    {
        const float x = centreX + (static_cast<float> (tick) / 50.0f) * halfWidth;
        const float height = tick == 0 ? 18.0f : 8.0f;
        g.drawVerticalLine (juce::roundToInt (x),
                            static_cast<float> (bar.getCentreY()) - height * 0.5f,
                            static_cast<float> (bar.getCentreY()) + height * 0.5f);
    }

    const float clampedCents = juce::jlimit (-50.0f, 50.0f, currentCents);
    const float needleX = centreX + (clampedCents / 50.0f) * halfWidth;

    g.setColour (std::abs (currentCents) <= 3.0f
                     ? DarkLookAndFeel::accent()
                     : (std::abs (currentCents) <= 15.0f ? juce::Colours::orange : juce::Colours::tomato));
    g.fillRoundedRectangle (needleX - 2.0f, static_cast<float> (bar.getY()) - 5.0f,
                            4.0f, static_cast<float> (bar.getHeight()) + 10.0f, 2.0f);
}
} // namespace daw
