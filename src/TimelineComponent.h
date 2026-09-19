#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "AudioEngine.h"

namespace daw
{
class TimelineComponent final : public juce::Component,
                                public juce::Timer
{
public:
    explicit TimelineComponent (AudioEngine& engineToUse);
    ~TimelineComponent() override;

    void paint (juce::Graphics& g) override;
    void resized() override;
    void timerCallback() override;

    void mouseDown (const juce::MouseEvent& event) override;
    void mouseDrag (const juce::MouseEvent& event) override;
    void mouseUp (const juce::MouseEvent& event) override;

private:
    static constexpr float pixelsPerSecond = 40.0f;
    static constexpr int gutterWidth = 64;
    static constexpr int rowHeight = 48;

    double getPixelsPerSample() const noexcept;

    void loadClipFile();
    void addClipToSelectedTrack();
    void deleteSelectedClip();

    int trackIndexAtY (int y) const noexcept;
    int hitTestClip (int trackIndex, int x, int y, bool& onResizeEdge) const noexcept;

    AudioEngine& engine;
    AudioGraph& graph;

    juce::TextButton loadFileButton;
    juce::ComboBox trackCombo;
    juce::TextButton addClipButton;
    juce::TextButton deleteClipButton;
    juce::TextEditor statusText;

    int selectedTrack = -1;
    int selectedClip = -1;
    bool dragging = false;
    bool resizing = false;
    int dragOffsetPixels = 0;
};
} // namespace daw
