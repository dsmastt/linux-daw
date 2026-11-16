#include "ProcessorControlsComponent.h"

#include "NamPreset.h"

#include <algorithm>

#include <unistd.h>

namespace daw
{
namespace
{
void setupBypassButton (juce::ToggleButton& button, AudioGraph& graph, ProcessorSlot slot)
{
    button.setButtonText ("Bypass");
    button.setToggleState (graph.isProcessorBypassed (slot), juce::dontSendNotification);
    button.onClick = [&button, &graph, slot]
    {
        graph.setProcessorBypassed (slot, button.getToggleState());
    };
}

void layoutRowsInBounds (const std::vector<std::unique_ptr<ProcessorControlsComponent::SliderRow>>& rows,
                         juce::Rectangle<int> bounds)
{
    for (const auto& row : rows)
    {
        row->setBounds (bounds.removeFromTop (88));
        bounds.removeFromTop (4);
    }
}
} // namespace

//==============================================================================
ProcessorControlsComponent::SliderRow::SliderRow (const juce::String& name,
                                                  double minValue, double maxValue, double step, double initialValue,
                                                  std::function<void (double)> onChange)
{
    label.setText (name, juce::dontSendNotification);
    label.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (label);

    slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 64, 20);
    slider.setRange (minValue, maxValue, step);
    slider.setValue (initialValue, juce::dontSendNotification);
    slider.onValueChange = [slider = &slider, onChange = std::move (onChange)]
    {
        onChange (slider->getValue());
    };

    addAndMakeVisible (slider);
}

ProcessorControlsComponent::SliderRow::~SliderRow() = default;

void ProcessorControlsComponent::SliderRow::resized()
{
    auto bounds = getLocalBounds();

    label.setBounds (bounds.removeFromTop (18));
    slider.setBounds (bounds);
}

//==============================================================================
ProcessorControlsComponent::GainTab::GainTab (AudioGraph& graphToUse)
    : graph (graphToUse)
{
    rows.push_back (std::make_unique<SliderRow> ("Gain (dB)", -60.0, 12.0, 0.5, graph.getGainDb(),
                                                 [this] (double value) { graph.setGainDb (static_cast<float> (value)); }));

    setupBypassButton (bypassButton, graph, ProcessorSlot::gain);
    addAndMakeVisible (bypassButton);

    for (auto& row : rows)
        addAndMakeVisible (*row);
}

void ProcessorControlsComponent::GainTab::resized()
{
    auto bounds = getLocalBounds().reduced (10);
    bypassButton.setBounds (bounds.removeFromTop (24));
    bounds.removeFromTop (6);
    layoutRowsInBounds (rows, bounds);
}

void ProcessorControlsComponent::GainTab::refreshFromGraph()
{
    rows[0]->slider.setValue (graph.getGainDb(), juce::dontSendNotification);
    bypassButton.setToggleState (graph.isProcessorBypassed (ProcessorSlot::gain), juce::dontSendNotification);
}

//==============================================================================
ProcessorControlsComponent::GateTab::GateTab (AudioGraph& graphToUse)
    : graph (graphToUse)
{
    rows.push_back (std::make_unique<SliderRow> ("Threshold (dB)", -90.0, 0.0, 0.5, graph.getGateThresholdDb(),
                                                 [this] (double value) { graph.setGateThresholdDb (static_cast<float> (value)); }));
    rows.push_back (std::make_unique<SliderRow> ("Attack (ms)", 0.1, 50.0, 0.1, graph.getGateAttackMs(),
                                                 [this] (double value) { graph.setGateAttackMs (static_cast<float> (value)); }));
    rows.push_back (std::make_unique<SliderRow> ("Release (ms)", 1.0, 1000.0, 1.0, graph.getGateReleaseMs(),
                                                 [this] (double value) { graph.setGateReleaseMs (static_cast<float> (value)); }));

    setupBypassButton (bypassButton, graph, ProcessorSlot::gate);
    addAndMakeVisible (bypassButton);

    for (auto& row : rows)
        addAndMakeVisible (*row);
}

void ProcessorControlsComponent::GateTab::resized()
{
    auto bounds = getLocalBounds().reduced (10);
    bypassButton.setBounds (bounds.removeFromTop (24));
    bounds.removeFromTop (6);
    layoutRowsInBounds (rows, bounds);
}

void ProcessorControlsComponent::GateTab::refreshFromGraph()
{
    rows[0]->slider.setValue (graph.getGateThresholdDb(), juce::dontSendNotification);
    rows[1]->slider.setValue (graph.getGateAttackMs(), juce::dontSendNotification);
    rows[2]->slider.setValue (graph.getGateReleaseMs(), juce::dontSendNotification);
    bypassButton.setToggleState (graph.isProcessorBypassed (ProcessorSlot::gate), juce::dontSendNotification);
}

//==============================================================================
ProcessorControlsComponent::EqTab::EqTab (AudioGraph& graphToUse)
    : graph (graphToUse)
{
    for (int band = 0; band < graph.getNumEqBands(); ++band)
    {
        rows.push_back (std::make_unique<SliderRow> ("Band " + juce::String (band + 1) + " Freq (Hz)",
                                                     20.0, 20000.0, 1.0, 1000.0,
                                                     [this, band] (double value) { graph.setEqBandParameters (band, value, graph.getEqBandGain (band), graph.getEqBandQ (band)); }));
        rows.back()->slider.setSkewFactorFromMidPoint (1000.0);

        rows.push_back (std::make_unique<SliderRow> ("Band " + juce::String (band + 1) + " Gain (dB)",
                                                     -12.0, 12.0, 0.1, 0.0,
                                                     [this, band] (double value) { graph.setEqBandParameters (band, graph.getEqBandFrequency (band), static_cast<float> (value), graph.getEqBandQ (band)); }));
        rows.push_back (std::make_unique<SliderRow> ("Band " + juce::String (band + 1) + " Q",
                                                     0.1, 10.0, 0.1, 0.7,
                                                     [this, band] (double value) { graph.setEqBandParameters (band, graph.getEqBandFrequency (band), graph.getEqBandGain (band), static_cast<float> (value)); }));
    }

    setupBypassButton (bypassButton, graph, ProcessorSlot::eq);
    addAndMakeVisible (bypassButton);

    for (auto& row : rows)
        addAndMakeVisible (*row);
}

void ProcessorControlsComponent::EqTab::refreshFromGraph()
{
    for (int band = 0; band < graph.getNumEqBands(); ++band)
    {
        rows[static_cast<size_t> (band * 3 + 0)]->slider.setValue (graph.getEqBandFrequency (band), juce::dontSendNotification);
        rows[static_cast<size_t> (band * 3 + 1)]->slider.setValue (graph.getEqBandGain (band), juce::dontSendNotification);
        rows[static_cast<size_t> (band * 3 + 2)]->slider.setValue (graph.getEqBandQ (band), juce::dontSendNotification);
    }

    bypassButton.setToggleState (graph.isProcessorBypassed (ProcessorSlot::eq), juce::dontSendNotification);
}

void ProcessorControlsComponent::EqTab::resized()
{
    auto bounds = getLocalBounds().reduced (10);
    bypassButton.setBounds (bounds.removeFromTop (24));
    bounds.removeFromTop (6);

    const int numBands = 4;
    const auto columnWidth = bounds.getWidth() / 2;

    auto leftColumn = bounds.removeFromLeft (columnWidth);
    auto rightColumn = bounds;

    for (int band = 0; band < numBands; ++band)
    {
        auto& column = band < 2 ? leftColumn : rightColumn;

        for (int rowIndex = 0; rowIndex < 3 && (band * 3 + rowIndex) < static_cast<int> (rows.size()); ++rowIndex)
        {
            rows[static_cast<size_t> (band * 3 + rowIndex)]->setBounds (column.removeFromTop (72));
            column.removeFromTop (4);
        }
    }
}

//==============================================================================
ProcessorControlsComponent::CompressorTab::CompressorTab (AudioGraph& graphToUse)
    : graph (graphToUse)
{
    rows.push_back (std::make_unique<SliderRow> ("Threshold (dB)", -60.0, 0.0, 0.5, -10.0,
                                                 [this] (double value) { graph.setCompressorThresholdDb (static_cast<float> (value)); }));
    rows.push_back (std::make_unique<SliderRow> ("Ratio", 1.0, 20.0, 0.1, 4.0,
                                                 [this] (double value) { graph.setCompressorRatio (static_cast<float> (value)); }));
    rows.push_back (std::make_unique<SliderRow> ("Attack (ms)", 0.1, 100.0, 0.1, 5.0,
                                                 [this] (double value) { graph.setCompressorAttackMs (static_cast<float> (value)); }));
    rows.push_back (std::make_unique<SliderRow> ("Release (ms)", 1.0, 1000.0, 1.0, 100.0,
                                                 [this] (double value) { graph.setCompressorReleaseMs (static_cast<float> (value)); }));
    rows.push_back (std::make_unique<SliderRow> ("Makeup (dB)", 0.0, 24.0, 0.5, 0.0,
                                                 [this] (double value) { graph.setCompressorMakeupGainDb (static_cast<float> (value)); }));

    setupBypassButton (bypassButton, graph, ProcessorSlot::compressor);
    addAndMakeVisible (bypassButton);

    for (auto& row : rows)
        addAndMakeVisible (*row);
}

void ProcessorControlsComponent::CompressorTab::resized()
{
    auto bounds = getLocalBounds().reduced (10);
    bypassButton.setBounds (bounds.removeFromTop (24));
    bounds.removeFromTop (6);
    layoutRowsInBounds (rows, bounds);
}

void ProcessorControlsComponent::CompressorTab::refreshFromGraph()
{
    rows[0]->slider.setValue (graph.getCompressorThresholdDb(), juce::dontSendNotification);
    rows[1]->slider.setValue (graph.getCompressorRatio(), juce::dontSendNotification);
    rows[2]->slider.setValue (graph.getCompressorAttackMs(), juce::dontSendNotification);
    rows[3]->slider.setValue (graph.getCompressorReleaseMs(), juce::dontSendNotification);
    rows[4]->slider.setValue (graph.getCompressorMakeupGainDb(), juce::dontSendNotification);
    bypassButton.setToggleState (graph.isProcessorBypassed (ProcessorSlot::compressor), juce::dontSendNotification);
}

//==============================================================================
ProcessorControlsComponent::DistortionTab::DistortionTab (AudioGraph& graphToUse)
    : graph (graphToUse)
{
    rows.push_back (std::make_unique<SliderRow> ("Drive (dB)", 0.0, 40.0, 0.5, 12.0,
                                                 [this] (double value) { graph.setDistortionDriveDb (static_cast<float> (value)); }));
    rows.push_back (std::make_unique<SliderRow> ("Tone", 0.0, 1.0, 0.01, 0.5,
                                                 [this] (double value) { graph.setDistortionTone (static_cast<float> (value)); }));

    modeLabel.setText ("Mode:", juce::dontSendNotification);
    addAndMakeVisible (modeLabel);

    modeCombo.addItem ("Soft clip", 1);
    modeCombo.addItem ("Hard clip", 2);
    modeCombo.addItem ("Cubic", 3);
    modeCombo.addItem ("Asymmetric", 4);
    modeCombo.setSelectedItemIndex (0);
    modeCombo.onChange = [this, combo = &modeCombo]
    {
        graph.setDistortionMode (static_cast<DistortionMode> (combo->getSelectedItemIndex()));
    };

    addAndMakeVisible (modeCombo);

    setupBypassButton (bypassButton, graph, ProcessorSlot::distortion);
    addAndMakeVisible (bypassButton);

    for (auto& row : rows)
        addAndMakeVisible (*row);
}

void ProcessorControlsComponent::DistortionTab::resized()
{
    auto bounds = getLocalBounds().reduced (10);
    bypassButton.setBounds (bounds.removeFromTop (24));
    bounds.removeFromTop (6);

    for (const auto& row : rows)
    {
        row->setBounds (bounds.removeFromTop (88));
        bounds.removeFromTop (4);
    }

    auto modeRow = bounds.removeFromTop (30);
    modeLabel.setBounds (modeRow.removeFromLeft (120));
    modeCombo.setBounds (modeRow);
}

void ProcessorControlsComponent::DistortionTab::refreshFromGraph()
{
    rows[0]->slider.setValue (graph.getDistortionDriveDb(), juce::dontSendNotification);
    rows[1]->slider.setValue (graph.getDistortionTone(), juce::dontSendNotification);
    modeCombo.setSelectedItemIndex (static_cast<int> (graph.getDistortionMode()), juce::dontSendNotification);
    bypassButton.setToggleState (graph.isProcessorBypassed (ProcessorSlot::distortion), juce::dontSendNotification);
}

//==============================================================================
ProcessorControlsComponent::CabinetTab::CabinetTab (AudioGraph& graphToUse, UndoManager& undoToUse)
    : graph (graphToUse),
      undo (undoToUse)
{
    loadButton.setButtonText ("Load IR...");
    loadButton.onClick = [this]
    {
        undo.captureUndoState (graph);

        auto chooser = std::make_shared<juce::FileChooser> ("Load Impulse Response",
                                                            juce::File(),
                                                            "*.wav");

        chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                              [this] (const juce::FileChooser& fc)
                              {
                                  const auto result = fc.getResult();

                                  juce::MessageManager::callAsync ([this, result]
                                  {
                                      if (result == juce::File())
                                          return;

                                      const auto error = graph.loadImpulseResponse (result);

                                      if (error.isNotEmpty())
                                      {
                                          statusText.setText ("Error: " + error, juce::dontSendNotification);
                                          return;
                                      }

                                      statusText.setText ("Loaded: " + result.getFileName()
                                                              + " (" + juce::String (graph.getImpulseResponseLength()) + " samples)",
                                                          juce::dontSendNotification);
                                  });
                              });
    };

    addAndMakeVisible (loadButton);

    clearButton.setButtonText ("Clear IR");
    clearButton.onClick = [this]
    {
        undo.captureUndoState (graph);
        graph.clearImpulseResponse();
        statusText.setText ("No impulse response loaded", juce::dontSendNotification);
    };

    addAndMakeVisible (clearButton);

    statusText.setMultiLine (true, false);
    statusText.setReadOnly (true);
    statusText.setCaretVisible (false);
    statusText.setText (graph.hasImpulseResponse()
                            ? "Impulse response loaded (" + juce::String (graph.getImpulseResponseLength()) + " samples)"
                            : "No impulse response loaded",
                        juce::dontSendNotification);

    addAndMakeVisible (statusText);

    setupBypassButton (bypassButton, graph, ProcessorSlot::convolution);
    addAndMakeVisible (bypassButton);
}

void ProcessorControlsComponent::CabinetTab::refreshFromGraph()
{
    bypassButton.setToggleState (graph.isProcessorBypassed (ProcessorSlot::convolution), juce::dontSendNotification);
    statusText.setText (graph.hasImpulseResponse()
                            ? "Impulse response loaded (" + juce::String (graph.getImpulseResponseLength()) + " samples)"
                            : "No impulse response loaded",
                        juce::dontSendNotification);
}

void ProcessorControlsComponent::CabinetTab::resized()
{
    auto bounds = getLocalBounds().reduced (10);
    bypassButton.setBounds (bounds.removeFromTop (24));
    bounds.removeFromTop (6);

    auto buttonRow = bounds.removeFromTop (34);
    loadButton.setBounds (buttonRow.removeFromLeft (110));
    buttonRow.removeFromLeft (10);
    clearButton.setBounds (buttonRow.removeFromLeft (110));

    bounds.removeFromTop (10);
    statusText.setBounds (bounds);
}

//==============================================================================
ProcessorControlsComponent::ReverbTab::ReverbTab (AudioGraph& graphToUse)
    : graph (graphToUse)
{
    rows.push_back (std::make_unique<SliderRow> ("Wet/Dry", 0.0, 1.0, 0.01, 0.2,
                                                 [this] (double value) { graph.setReverbWetDry (static_cast<float> (value)); }));
    rows.push_back (std::make_unique<SliderRow> ("Decay", 0.0, 1.0, 0.01, 0.5,
                                                 [this] (double value) { graph.setReverbDecay (static_cast<float> (value)); }));
    rows.push_back (std::make_unique<SliderRow> ("Size", 0.0, 1.0, 0.01, 0.5,
                                                 [this] (double value) { graph.setReverbSize (static_cast<float> (value)); }));

    setupBypassButton (bypassButton, graph, ProcessorSlot::reverb);
    addAndMakeVisible (bypassButton);

    for (auto& row : rows)
        addAndMakeVisible (*row);
}

void ProcessorControlsComponent::ReverbTab::resized()
{
    auto bounds = getLocalBounds().reduced (10);
    bypassButton.setBounds (bounds.removeFromTop (24));
    bounds.removeFromTop (6);
    layoutRowsInBounds (rows, bounds);
}

void ProcessorControlsComponent::ReverbTab::refreshFromGraph()
{
    rows[0]->slider.setValue (graph.getReverbWetDry(), juce::dontSendNotification);
    rows[1]->slider.setValue (graph.getReverbDecay(), juce::dontSendNotification);
    rows[2]->slider.setValue (graph.getReverbSize(), juce::dontSendNotification);
    bypassButton.setToggleState (graph.isProcessorBypassed (ProcessorSlot::reverb), juce::dontSendNotification);
}

//==============================================================================
ProcessorControlsComponent::ProcessorControlsComponent (AudioEngine& engineToUse, UndoManager& undoToUse)
    : tabs (juce::TabbedButtonBar::TabsAtTop),
      gainTab (engineToUse.getAudioGraph()),
      gateTab (engineToUse.getAudioGraph()),
      eqTab (engineToUse.getAudioGraph()),
      compressorTab (engineToUse.getAudioGraph()),
      distortionTab (engineToUse.getAudioGraph()),
      cabinetTab (engineToUse.getAudioGraph(), undoToUse),
      reverbTab (engineToUse.getAudioGraph()),
      pluginTab (engineToUse, undoToUse),
      engine (engineToUse),
      graph (engineToUse.getAudioGraph()),
      undo (undoToUse)
{
    addAndMakeVisible (tabs);

    tabs.addTab ("Gain", juce::Colours::darkgrey, &gainTab, false);
    tabs.addTab ("Gate", juce::Colours::darkgrey, &gateTab, false);
    tabs.addTab ("EQ", juce::Colours::darkgrey, &eqTab, false);
    tabs.addTab ("Compressor", juce::Colours::darkgrey, &compressorTab, false);
    tabs.addTab ("Distortion", juce::Colours::darkgrey, &distortionTab, false);
    tabs.addTab ("Cabinet", juce::Colours::darkgrey, &cabinetTab, false);
    tabs.addTab ("Reverb", juce::Colours::darkgrey, &reverbTab, false);
    tabs.addTab ("Plugin", juce::Colours::darkgrey, &pluginTab, false);

    tabs.getTabbedButtonBar().addChangeListener (this);

    wireUndoTab (gainTab);
    wireUndoTab (gateTab);
    wireUndoTab (eqTab);
    wireUndoTab (compressorTab);
    wireUndoTab (distortionTab);
    wireUndoTab (reverbTab);
}

void ProcessorControlsComponent::refreshFromGraph()
{
    gainTab.refreshFromGraph();
    gateTab.refreshFromGraph();
    eqTab.refreshFromGraph();
    compressorTab.refreshFromGraph();
    distortionTab.refreshFromGraph();
    cabinetTab.refreshFromGraph();
    reverbTab.refreshFromGraph();

    pluginTab.refreshFromGraph();
}

ProcessorControlsComponent::~ProcessorControlsComponent()
{
    tabs.getTabbedButtonBar().removeChangeListener (this);
}

void ProcessorControlsComponent::changeListenerCallback (juce::ChangeBroadcaster*)
{
    if (tabs.getCurrentTabIndex() == tabs.getNumTabs() - 1)
        pluginTab.scanPluginsIfNeeded();
}

void ProcessorControlsComponent::resized()
{
    tabs.setBounds (getLocalBounds());
}

//==============================================================================
class ProcessorControlsComponent::PluginTab::EditorWindow final : public juce::DocumentWindow
{
public:
    EditorWindow (const juce::String& name, juce::Component* content)
        : juce::DocumentWindow (name,
                                juce::Desktop::getInstance().getDefaultLookAndFeel()
                                    .findColour (juce::ResizableWindow::backgroundColourId),
                                juce::DocumentWindow::closeButton,
                                false)
    {
        setUsingNativeTitleBar (true);
        setContentOwned (content, true);
        setResizable (true, false);

        const int width = std::max (content->getWidth(), 480);
        const int height = std::max (content->getHeight(), 360);

        setSize (width, height + getTitleBarHeight());
        setVisible (true);
        toFront (true);
    }

    void closeButtonPressed() override
    {
        setVisible (false);
    }
};

//==============================================================================
ProcessorControlsComponent::PluginTab::PluginTab (AudioEngine& engineToUse, UndoManager& undoToUse)
    : engine (engineToUse),
      graph (engineToUse.getAudioGraph()),
      undo (undoToUse),
      scanPool (1)
{
    rescanButton.setButtonText ("Rescan");
    rescanButton.onClick = [this] { rescanPlugins(); };
    addAndMakeVisible (rescanButton);

    addAndMakeVisible (pluginCombo);

    loadButton.setButtonText ("Load");
    loadButton.onClick = [this]
    {
        undo.captureUndoState (graph);
        loadSelectedPlugin();
    };
    addAndMakeVisible (loadButton);

    removeButton.setButtonText ("Remove");
    removeButton.setEnabled (false);
    removeButton.onClick = [this]
    {
        undo.captureUndoState (graph);
        removePlugin();
    };
    addAndMakeVisible (removeButton);

    bypassButton.setButtonText ("Bypass");
    bypassButton.setEnabled (false);
    bypassButton.onClick = [this]
    {
        undo.captureUndoState (graph);
        graph.setVst3PluginBypassed (bypassButton.getToggleState());
    };
    addAndMakeVisible (bypassButton);

    editorButton.setButtonText ("Editor...");
    editorButton.setEnabled (false);
    editorButton.onClick = [this] { openEditor(); };
    addAndMakeVisible (editorButton);

    statusText.setMultiLine (true, false);
    statusText.setReadOnly (true);
    statusText.setCaretVisible (false);
    statusText.setText ("No plugin loaded", juce::dontSendNotification);
    addAndMakeVisible (statusText);

    nativePresetLabel.setText ("Preset", juce::dontSendNotification);
    addAndMakeVisible (nativePresetLabel);

    addAndMakeVisible (nativePresetCombo);

    nativePresetLoadButton.setButtonText ("Load");
    nativePresetLoadButton.onClick = [this] { loadNativePreset(); };
    addAndMakeVisible (nativePresetLoadButton);

    nativePresetRefreshButton.setButtonText ("Refresh");
    nativePresetRefreshButton.onClick = [this] { updatePluginSpecificControls (true); };
    addAndMakeVisible (nativePresetRefreshButton);

    namModelButton.setButtonText ("NAM model...");
    namModelButton.onClick = [this] { chooseNamModel(); };
    addAndMakeVisible (namModelButton);

    namIrButton.setButtonText ("NAM IR...");
    namIrButton.onClick = [this] { chooseNamIr(); };
    addAndMakeVisible (namIrButton);

    namPathLabel.setColour (juce::Label::textColourId, juce::Colours::lightgrey.withAlpha (0.75f));
    addAndMakeVisible (namPathLabel);

    nativePresetLabel.setVisible (false);
    nativePresetCombo.setVisible (false);
    nativePresetLoadButton.setVisible (false);
    nativePresetRefreshButton.setVisible (false);
    namModelButton.setVisible (false);
    namIrButton.setVisible (false);
    namPathLabel.setVisible (false);

    parameterViewport.setViewedComponent (&parameterPanel, false);
    addAndMakeVisible (parameterViewport);

    statusText.setText ("Open this tab to scan for plugins", juce::dontSendNotification);
}

ProcessorControlsComponent::PluginTab::~PluginTab()
{
    editorWindow.reset();
    graph.unloadVst3Plugin();
}

void ProcessorControlsComponent::PluginTab::resized()
{
    auto bounds = getLocalBounds().reduced (10);

    auto row = bounds.removeFromTop (30);
    rescanButton.setBounds (row.removeFromLeft (80));
    row.removeFromLeft (8);
    pluginCombo.setBounds (row);

    bounds.removeFromTop (8);

    row = bounds.removeFromTop (30);
    loadButton.setBounds (row.removeFromLeft (80));
    row.removeFromLeft (8);
    removeButton.setBounds (row.removeFromLeft (80));
    row.removeFromLeft (8);
    bypassButton.setBounds (row.removeFromLeft (80));
    row.removeFromLeft (8);
    editorButton.setBounds (row.removeFromLeft (80));

    bounds.removeFromTop (8);

    if (nativePresetCombo.isVisible())
    {
        row = bounds.removeFromTop (28);
        nativePresetLabel.setBounds (row.removeFromLeft (52));
        nativePresetLoadButton.setBounds (row.removeFromRight (60));
        row.removeFromRight (8);
        nativePresetRefreshButton.setBounds (row.removeFromRight (70));
        row.removeFromRight (8);
        nativePresetCombo.setBounds (row);
        bounds.removeFromTop (8);
    }

    if (namModelButton.isVisible())
    {
        row = bounds.removeFromTop (28);
        namModelButton.setBounds (row.removeFromLeft (110));
        row.removeFromLeft (8);
        namIrButton.setBounds (row.removeFromLeft (90));
        row.removeFromLeft (8);
        namPathLabel.setBounds (row);
        bounds.removeFromTop (8);
    }

    statusText.setBounds (bounds.removeFromTop (56));

    bounds.removeFromTop (8);

    parameterViewport.setBounds (bounds);
    parameterViewport.setScrollBarsShown (true, false, true, false);

    const int contentHeight = static_cast<int> (parameterSliders.size()) * 28;
    parameterPanel.setBounds (0, 0, std::max (parameterViewport.getWidth(), 100), std::max (contentHeight, 1));
    parameterViewport.setViewedComponent (&parameterPanel, false);

    auto parameterBounds = parameterPanel.getLocalBounds();

    for (size_t i = 0; i < parameterSliders.size(); ++i)
    {
        auto parameterRow = parameterBounds.removeFromTop (28);
        parameterLabels[i]->setBounds (parameterRow.removeFromLeft (180));
        parameterSliders[i]->setBounds (parameterRow);
    }
}

void ProcessorControlsComponent::PluginTab::scanPluginsIfNeeded()
{
    if (scanStarted)
        return;

    rescanPlugins();
}

void ProcessorControlsComponent::PluginTab::rescanPlugins()
{
    scanStarted = true;

    juce::StringArray paths;
    paths.add (juce::File::getSpecialLocation (juce::File::userHomeDirectory).getChildFile (".vst3").getFullPathName());
    paths.add ("/usr/lib/vst3");
    paths.add ("/usr/local/lib/vst3");
    paths.add ("build/linux-daw-test-plugin_artefacts/VST3");

    const auto deadMansPedalFile = juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
                                        .getChildFile ("linux-daw")
                                        .getChildFile ("dead_mans_pedal");

    const juce::Component::SafePointer<PluginTab> safeThis (this);

    scanPool.addJob ([safeThis, paths, deadMansPedalFile]
    {
        // Plugin scanning can load many DLLs (and, with yabridge, start
        // several Wine processes). Keep it out of the way of the audio
        // thread if the user starts audio while this is running.
        ::nice (10);

        auto found = scanVst3Plugins (paths, deadMansPedalFile);

        juce::MessageManager::callAsync ([safeThis, found = std::move (found)]
        {
            if (safeThis == nullptr)
                return;

            safeThis->scannedPlugins = found;

            safeThis->pluginCombo.clear();

            for (const auto& plugin : safeThis->scannedPlugins)
                safeThis->pluginCombo.addItem (plugin.name, safeThis->pluginCombo.getNumItems() + 1);

            safeThis->statusText.setText ("Found " + juce::String (safeThis->scannedPlugins.size()) + " VST3 plugins",
                                          juce::dontSendNotification);
        });

        return true;
    });
}

void ProcessorControlsComponent::closePluginEditor()
{
    pluginTab.closeEditor();
}

void ProcessorControlsComponent::PluginTab::closeEditor()
{
    editorWindow.reset();
}

void ProcessorControlsComponent::PluginTab::loadSelectedPlugin()
{
    const int index = pluginCombo.getSelectedItemIndex();

    if (index < 0 || index >= static_cast<int> (scannedPlugins.size()))
    {
        statusText.setText ("No plugin selected", juce::dontSendNotification);
        return;
    }

    // The editor owns a component that points at the old plugin instance.
    // It must be gone before the old instance is destroyed.
    editorWindow.reset();
    editorButton.setEnabled (false);
    parameterPanel.removeAllChildren();
    parameterSliders.clear();
    parameterLabels.clear();

    if (graph.isVst3PluginLoaded())
        graph.unloadVst3Plugin();

    const double rate = engine.isRunning() ? engine.getSampleRate() : 48000.0;
    const int buffer = engine.isRunning() ? engine.getBufferSize() : 128;

    const auto error = graph.loadVst3Plugin (scannedPlugins[static_cast<size_t> (index)], rate, buffer);

    if (error.isNotEmpty())
    {
        statusText.setText ("Error: " + error, juce::dontSendNotification);
        removeButton.setEnabled (false);
        bypassButton.setEnabled (false);
        editorButton.setEnabled (false);
        updatePluginSpecificControls (true);
        return;
    }

    statusText.setText ("Loaded: " + graph.getVst3PluginName(), juce::dontSendNotification);

    removeButton.setEnabled (true);
    bypassButton.setEnabled (true);
    bypassButton.setToggleState (false, juce::dontSendNotification);

    const auto* instance = graph.getVst3PluginInstance();
    editorButton.setEnabled (instance != nullptr && instance->hasEditor());

    rebuildParameterSliders();
    updatePluginSpecificControls (true);

    if (instance != nullptr && instance->hasEditor())
        openEditor();
}

void ProcessorControlsComponent::PluginTab::removePlugin()
{
    editorWindow.reset();
    graph.unloadVst3Plugin();

    statusText.setText ("No plugin loaded", juce::dontSendNotification);

    removeButton.setEnabled (false);
    bypassButton.setEnabled (false);
    bypassButton.setToggleState (false, juce::dontSendNotification);
    editorButton.setEnabled (false);

    parameterPanel.removeAllChildren();
    parameterSliders.clear();
    parameterLabels.clear();

    nativePresets.clear();
    nativePresetCombo.clear();
    loadedPluginName.clear();
    namModelWindowsPath.clear();
    namIrWindowsPath.clear();
    updatePluginSpecificControls (true);
}

void ProcessorControlsComponent::PluginTab::refreshFromGraph()
{
    editorWindow.reset();
    rebuildParameterSliders();
    removeButton.setEnabled (graph.isVst3PluginLoaded());
    bypassButton.setEnabled (graph.isVst3PluginLoaded());
    bypassButton.setToggleState (graph.isVst3PluginBypassed(), juce::dontSendNotification);

    if (auto* instance = graph.getVst3PluginInstance())
        editorButton.setEnabled (instance->hasEditor());
    else
        editorButton.setEnabled (false);

    statusText.setText (graph.isVst3PluginLoaded()
                            ? "Loaded: " + graph.getVst3PluginName()
                            : "No plugin loaded",
                        juce::dontSendNotification);

    updatePluginSpecificControls (false);
}

void ProcessorControlsComponent::PluginTab::updatePluginSpecificControls (bool forceRefresh)
{
    const auto pluginName = graph.isVst3PluginLoaded() ? graph.getVst3PluginName() : juce::String();

    if (forceRefresh || pluginName != loadedPluginName)
    {
        loadedPluginName = pluginName;
        nativePresets.clear();
        nativePresetCombo.clear();
        namModelWindowsPath.clear();
        namIrWindowsPath.clear();

        if (pluginName.isNotEmpty())
        {
            auto found = NeuralPreset::findPresets (pluginName);
            juce::StringArray seenNames;

            for (auto& entry : found)
            {
                if (seenNames.contains (entry.displayName))
                    continue;

                seenNames.add (entry.displayName);
                nativePresets.push_back (entry);
                nativePresetCombo.addItem (entry.displayName, static_cast<int> (nativePresets.size()));
            }

            if (! nativePresets.empty())
                nativePresetCombo.setSelectedItemIndex (0, juce::dontSendNotification);
        }
    }

    const bool hasNativePresets = ! nativePresets.empty();
    const auto lowerPluginName = pluginName.toLowerCase();
    const bool isNeuralDsp = lowerPluginName.contains ("archetype")
                             || lowerPluginName.contains ("fortin")
                             || lowerPluginName.contains ("darkglass")
                             || lowerPluginName.contains ("soldano")
                             || lowerPluginName.contains ("morgan")
                             || lowerPluginName.contains ("omega")
                             || lowerPluginName.contains ("parallax")
                             || lowerPluginName.contains ("granophyre")
                             || lowerPluginName.contains ("plini")
                             || lowerPluginName.contains ("nolly");
    const bool showNativeRow = hasNativePresets || isNeuralDsp;

    if (showNativeRow && ! hasNativePresets && nativePresetCombo.getNumItems() == 0)
    {
        nativePresetCombo.addItem ("No presets found - mount the Windows drive and press Refresh", 1);
        nativePresetCombo.setSelectedItemIndex (0, juce::dontSendNotification);
    }

    nativePresetLabel.setVisible (showNativeRow);
    nativePresetCombo.setVisible (showNativeRow);
    nativePresetLoadButton.setVisible (showNativeRow);
    nativePresetRefreshButton.setVisible (showNativeRow);
    nativePresetLoadButton.setEnabled (hasNativePresets);

    const bool isNam = pluginName.containsIgnoreCase ("NeuralAmpModeler")
                       || pluginName.containsIgnoreCase ("Neural Amp Modeler");

    namModelButton.setVisible (isNam);
    namIrButton.setVisible (isNam);
    namPathLabel.setVisible (isNam);

    if (isNam)
    {
        auto fileName = [] (const juce::String& path)
        {
            auto normalised = path.replaceCharacter ('\\', '/');
            return normalised.fromLastOccurrenceOf ("/", false, false);
        };

        juce::String text;

        if (namModelWindowsPath.isNotEmpty())
            text << "Model: " << fileName (namModelWindowsPath);

        if (namIrWindowsPath.isNotEmpty())
        {
            if (text.isNotEmpty())
                text << "   ";

            text << "IR: " << fileName (namIrWindowsPath);
        }

        if (text.isEmpty())
            text = "No NAM model or IR selected";

        namPathLabel.setText (text, juce::dontSendNotification);
    }

    resized();
}

void ProcessorControlsComponent::PluginTab::loadNativePreset()
{
    const int index = nativePresetCombo.getSelectedItemIndex();

    if (index < 0 || index >= static_cast<int> (nativePresets.size()))
        return;

    auto* instance = graph.getVst3PluginInstance();

    if (instance == nullptr)
        return;

    undo.captureUndoState (graph);

    int matched = 0;
    int total = 0;
    const auto error = NeuralPreset::load (nativePresets[static_cast<size_t> (index)].file,
                                           *instance, matched, total);

    if (error.isNotEmpty())
    {
        statusText.setText ("Preset: " + error, juce::dontSendNotification);
        return;
    }

    rebuildParameterSliders();

    if (editorWindow != nullptr)
    {
        editorWindow.reset();

        if (instance->hasEditor())
            openEditor();
    }

    statusText.setText ("Preset: " + nativePresets[static_cast<size_t> (index)].displayName
                            + " (" + juce::String (matched) + "/" + juce::String (total) + " parameters)",
                        juce::dontSendNotification);
}

void ProcessorControlsComponent::PluginTab::chooseNamModel()
{
    auto chooser = std::make_shared<juce::FileChooser> ("Load NAM model",
                                                        juce::File::getSpecialLocation (juce::File::userHomeDirectory),
                                                        "*.nam");
    juce::Component::SafePointer<PluginTab> safeThis (this);

    chooser->launchAsync (juce::FileBrowserComponent::openMode
                              | juce::FileBrowserComponent::canSelectFiles,
                          [safeThis, chooser] (const juce::FileChooser& fileChooser)
    {
        if (safeThis == nullptr)
            return;

        const auto file = fileChooser.getResult();

        if (file == juce::File())
            return;

        auto* instance = safeThis->graph.getVst3PluginInstance();

        if (instance == nullptr)
            return;

        const auto description = safeThis->graph.getVst3PluginDescription();

        juce::String newPath;
        const auto error = NamPreset::loadModel (*instance,
                                                 description.version,
                                                 file,
                                                 safeThis->namIrWindowsPath,
                                                 newPath);

        if (error.isNotEmpty())
        {
            safeThis->statusText.setText ("NAM model: " + error, juce::dontSendNotification);
            return;
        }

        safeThis->namModelWindowsPath = newPath;
        safeThis->statusText.setText ("NAM model: " + file.getFileName(), juce::dontSendNotification);
        safeThis->updatePluginSpecificControls (false);
    });
}

void ProcessorControlsComponent::PluginTab::chooseNamIr()
{
    auto chooser = std::make_shared<juce::FileChooser> ("Load NAM IR",
                                                        juce::File::getSpecialLocation (juce::File::userHomeDirectory),
                                                        "*.wav;*.nam");
    juce::Component::SafePointer<PluginTab> safeThis (this);

    chooser->launchAsync (juce::FileBrowserComponent::openMode
                              | juce::FileBrowserComponent::canSelectFiles,
                          [safeThis, chooser] (const juce::FileChooser& fileChooser)
    {
        if (safeThis == nullptr)
            return;

        const auto file = fileChooser.getResult();

        if (file == juce::File())
            return;

        auto* instance = safeThis->graph.getVst3PluginInstance();

        if (instance == nullptr)
            return;

        const auto description = safeThis->graph.getVst3PluginDescription();

        juce::String newPath;
        const auto error = NamPreset::loadIr (*instance,
                                              description.version,
                                              file,
                                              safeThis->namModelWindowsPath,
                                              newPath);

        if (error.isNotEmpty())
        {
            safeThis->statusText.setText ("NAM IR: " + error, juce::dontSendNotification);
            return;
        }

        safeThis->namIrWindowsPath = newPath;
        safeThis->statusText.setText ("NAM IR: " + file.getFileName(), juce::dontSendNotification);
        safeThis->updatePluginSpecificControls (false);
    });
}

void ProcessorControlsComponent::PluginTab::rebuildParameterSliders()

{
    parameterPanel.removeAllChildren();
    parameterSliders.clear();
    parameterLabels.clear();

    auto* instance = graph.getVst3PluginInstance();

    if (instance == nullptr)
        return;

    const auto& parameters = instance->getParameters();

    for (auto* parameter : parameters)
    {
        if (parameter == nullptr || !parameter->isAutomatable())
            continue;

        auto* label = new juce::Label (parameter->getName (64), parameter->getName (64));
        parameterLabels.push_back (label);
        parameterPanel.addAndMakeVisible (label);

        auto* slider = new juce::Slider (juce::Slider::LinearHorizontal, juce::Slider::TextBoxRight);
        slider->setRange (0.0, 1.0, 0.001);
        slider->setValue (parameter->getValue(), juce::dontSendNotification);
        slider->setTextBoxStyle (juce::Slider::TextBoxRight, false, 56, 22);
        slider->onValueChange = [parameter, slider]
        {
            parameter->setValue (static_cast<float> (slider->getValue()));
        };

        parameterSliders.push_back (slider);
        parameterPanel.addAndMakeVisible (slider);
    }

    resized();
}

void ProcessorControlsComponent::PluginTab::openEditor()
{
    auto* instance = graph.getVst3PluginInstance();

    if (instance == nullptr)
        return;

    if (editorWindow != nullptr)
    {
        editorWindow->setVisible (true);
        editorWindow->toFront (true);
        return;
    }

    auto* editor = instance->createEditorAndMakeActive();

    if (editor == nullptr)
    {
        statusText.setText ("Plugin has no editor", juce::dontSendNotification);
        return;
    }

    editorWindow = std::make_unique<EditorWindow> (graph.getVst3PluginName(), editor);
}
} // namespace daw
