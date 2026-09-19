#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace daw
{
class MeterComponent final : public juce::Component
{
public:
    explicit MeterComponent (const juce::String& nameToUse);

    void setLevel (float newLevel);
    void setClipping (bool shouldShowClip);

    void paint (juce::Graphics& g) override;

private:
    juce::String name;
    float level = 0.0f;
    bool clipLit = false;
};
} // namespace daw
