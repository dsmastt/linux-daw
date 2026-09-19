#include "MeterComponent.h"

#include <algorithm>

namespace daw
{
MeterComponent::MeterComponent (const juce::String& nameToUse)
    : name (nameToUse)
{
}

void MeterComponent::setLevel (float newLevel)
{
    level = std::clamp (newLevel, 0.0f, 1.0f);
    repaint();
}

void MeterComponent::setClipping (bool shouldShowClip)
{
    if (clipLit == shouldShowClip)
        return;

    clipLit = shouldShowClip;
    repaint();
}

void MeterComponent::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds();
    const auto labelHeight = 20;
    auto meterBounds = bounds.removeFromTop (bounds.getHeight() - labelHeight);

    g.setColour (juce::Colours::black);
    g.fillRect (meterBounds);

    const float meterHeight = static_cast<float> (meterBounds.getHeight());
    const float filledHeight = meterHeight * level;

    auto fillBounds = meterBounds.toFloat();
    fillBounds.removeFromTop (meterHeight - filledHeight);

    if (level > 0.0f)
    {
        juce::ColourGradient gradient (juce::Colours::limegreen, fillBounds.getX(), meterBounds.getBottom(),
                                       juce::Colours::orange, fillBounds.getX(), meterBounds.getY(), false);

        if (level > 0.9f)
            gradient = juce::ColourGradient (juce::Colours::red, fillBounds.getX(), meterBounds.getBottom(),
                                             juce::Colours::orange, fillBounds.getX(), meterBounds.getY(), false);

        g.setGradientFill (gradient);
        g.fillRect (fillBounds);
    }

    if (clipLit)
    {
        g.setColour (juce::Colours::red);
        g.fillRect (meterBounds.removeFromTop (6));
    }

    g.setColour (juce::Colours::grey);
    g.drawRect (meterBounds, 1.0f);

    g.setColour (juce::Colours::white);
    g.setFont (juce::FontOptions{}.withHeight (14.0f));
    g.drawText (name, bounds.removeFromTop (labelHeight), juce::Justification::centred, false);
}
} // namespace daw
