#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "AudioEngine.h"
#include "UndoManager.h"

namespace daw
{
class AutomationComponent final : public juce::Component,
                                  public juce::Timer
{
public:
    explicit AutomationComponent (AudioEngine& engineToUse, UndoManager& undoToUse);
    ~AutomationComponent() override;

    void paint (juce::Graphics& g) override;
    void resized() override;
    void timerCallback() override;

    void mouseDown (const juce::MouseEvent& event) override;
    void mouseDrag (const juce::MouseEvent& event) override;
    void mouseUp (const juce::MouseEvent& event) override;

private:
    static constexpr float pixelsPerSecond = 40.0f;
    static constexpr int gutterWidth = 40;
    static constexpr int rowHeight = 100;

    enum class ParameterKind
    {
        volume = 0,
        pan = 1,
        gain = 2,
        plugin = 3,
    };

    double getPixelsPerSample() const noexcept;
    float valueAtY (int y) const noexcept;
    int yForValue (float value) const noexcept;

    AutomationParameter parameterForKind (ParameterKind kind) const noexcept;

    void clearCurrentCurve();
    int findPointAt (int x, int y) const noexcept;

    AudioEngine& engine;
    AudioGraph& graph;
    UndoManager& undo;

    juce::ComboBox trackCombo;
    juce::ComboBox parameterCombo;
    juce::TextButton clearButton;
    juce::TextEditor statusText;

    int selectedPoint = -1;
    bool dragging = false;
    int selectedTrackIndex = 0;
    ParameterKind selectedKind = ParameterKind::volume;
};
} // namespace daw
