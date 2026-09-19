#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "AudioEngine.h"
#include "AudioGraph.h"
#include "Vst3HostProcessor.h"

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

    explicit ProcessorControlsComponent (AudioEngine& engine);
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

    class PluginTab final : public juce::Component
    {
    public:
        explicit PluginTab (AudioEngine& engine);
        ~PluginTab() override;
        void resized() override;

    private:
        class EditorWindow;

        void rescanPlugins();
        void loadSelectedPlugin();
        void removePlugin();
        void rebuildParameterSliders();
        void openEditor();

        AudioEngine& engine;
        AudioGraph& graph;
        juce::TextButton rescanButton;
        juce::ComboBox pluginCombo;
        juce::TextButton loadButton;
        juce::TextButton removeButton;
        juce::ToggleButton bypassButton;
        juce::TextButton editorButton;
        juce::TextEditor statusText;
        juce::Component parameterPanel;
        std::vector<juce::Slider*> parameterSliders;
        std::vector<juce::Label*> parameterLabels;
        std::vector<juce::PluginDescription> scannedPlugins;
        std::unique_ptr<EditorWindow> editorWindow;
    };

    juce::TabbedComponent tabs;
    GainTab gainTab;
    GateTab gateTab;
    EqTab eqTab;
    CompressorTab compressorTab;
    DistortionTab distortionTab;
    CabinetTab cabinetTab;
    ReverbTab reverbTab;
    PluginTab pluginTab;
};
} // namespace daw
