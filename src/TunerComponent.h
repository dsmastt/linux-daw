#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "AudioEngine.h"

namespace daw
{
class TunerComponent final : public juce::Component,
                             private juce::Timer
{
public:
    explicit TunerComponent (AudioEngine& engineToUse);
    ~TunerComponent() override = default;

    void paint (juce::Graphics& g) override;
    void resized() override;

private:
    void timerCallback() override;

    AudioEngine& engine;

    float displayedFrequency = 0.0f;
    float currentCents = 0.0f;
    int currentOctave = 0;
    juce::String currentNote = "-";
    int holdTicks = 0;
    bool hasReading = false;
};
} // namespace daw
