#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "AudioGraph.h"

namespace daw
{
class ProcessorControlsComponent final : public juce::Component
{
public:
    class SliderRow final : public juce::Component
    {
    public:
        SliderRow (const juce::String& name,
                   double minValue, double maxValue, double step, double initialValue,
                   std::function<void (double)> onChange);
        ~SliderRow() override;

        void resized() override;

        juce::Slider slider;

    private:
        juce::Label label;
    };

    explicit ProcessorControlsComponent (AudioGraph& graph);
    ~ProcessorControlsComponent() override;

    void resized() override;

private:
    static void layoutRows (juce::Component& parent, const std::vector<std::unique_ptr<SliderRow>>& rows);

    class GainTab final : public juce::Component
    {
    public:
        explicit GainTab (AudioGraph& graph);
        void resized() override;

    private:
        std::vector<std::unique_ptr<SliderRow>> rows;
    };

    class GateTab final : public juce::Component
    {
    public:
        explicit GateTab (AudioGraph& graph);
        void resized() override;

    private:
        std::vector<std::unique_ptr<SliderRow>> rows;
    };

    class EqTab final : public juce::Component
    {
    public:
        explicit EqTab (AudioGraph& graph);
        void resized() override;

    private:
        std::vector<std::unique_ptr<SliderRow>> rows;
    };

    class CompressorTab final : public juce::Component
    {
    public:
        explicit CompressorTab (AudioGraph& graph);
        void resized() override;

    private:
        std::vector<std::unique_ptr<SliderRow>> rows;
    };

    class DistortionTab final : public juce::Component
    {
    public:
        explicit DistortionTab (AudioGraph& graph);
        void resized() override;

    private:
        std::vector<std::unique_ptr<SliderRow>> rows;
        juce::Label modeLabel;
        juce::ComboBox modeCombo;
    };

    class CabinetTab final : public juce::Component
    {
    public:
        explicit CabinetTab (AudioGraph& graph);
        void resized() override;

    private:
        AudioGraph& graph;
        juce::TextButton loadButton;
        juce::TextButton clearButton;
        juce::TextEditor statusText;
    };

    class ReverbTab final : public juce::Component
    {
    public:
        explicit ReverbTab (AudioGraph& graph);
        void resized() override;

    private:
        std::vector<std::unique_ptr<SliderRow>> rows;
    };

    juce::TabbedComponent tabs;
    GainTab gainTab;
    GateTab gateTab;
    EqTab eqTab;
    CompressorTab compressorTab;
    DistortionTab distortionTab;
    CabinetTab cabinetTab;
    ReverbTab reverbTab;
};
} // namespace daw
