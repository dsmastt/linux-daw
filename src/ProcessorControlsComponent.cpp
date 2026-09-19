#include "ProcessorControlsComponent.h"

#include <algorithm>

namespace daw
{
//==============================================================================
ProcessorControlsComponent::SliderRow::SliderRow (const juce::String& name,
                                                  double minValue, double maxValue, double step, double initialValue,
                                                  std::function<void (double)> onChange)
{
    label.setText (name, juce::dontSendNotification);
    addAndMakeVisible (label);

    slider.setRange (minValue, maxValue, step);
    slider.setValue (initialValue, juce::dontSendNotification);
    slider.setTextBoxStyle (juce::Slider::TextBoxRight, false, 64, 22);
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
    label.setBounds (bounds.removeFromLeft (120));
    slider.setBounds (bounds);
}

//==============================================================================
ProcessorControlsComponent::GainTab::GainTab (AudioGraph& graphToUse)
    : graph (graphToUse)
{
    rows.push_back (std::make_unique<SliderRow> ("Gain (dB)", -60.0, 12.0, 0.5, graph.getGainDb(),
                                                 [this] (double value) { graph.setGainDb (static_cast<float> (value)); }));

    for (auto& row : rows)
        addAndMakeVisible (*row);
}

void ProcessorControlsComponent::GainTab::resized()
{
    layoutRows (*this, rows);
}

void ProcessorControlsComponent::GainTab::refreshFromGraph()
{
    rows[0]->slider.setValue (graph.getGainDb(), juce::dontSendNotification);
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

    for (auto& row : rows)
        addAndMakeVisible (*row);
}

void ProcessorControlsComponent::GateTab::resized()
{
    layoutRows (*this, rows);
}

void ProcessorControlsComponent::GateTab::refreshFromGraph()
{
    rows[0]->slider.setValue (graph.getGateThresholdDb(), juce::dontSendNotification);
    rows[1]->slider.setValue (graph.getGateAttackMs(), juce::dontSendNotification);
    rows[2]->slider.setValue (graph.getGateReleaseMs(), juce::dontSendNotification);
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
}

void ProcessorControlsComponent::EqTab::resized()
{
    auto bounds = getLocalBounds().reduced (10);

    const int numBands = 4;
    const auto columnWidth = bounds.getWidth() / 2;

    auto leftColumn = bounds.removeFromLeft (columnWidth);
    auto rightColumn = bounds;

    for (int band = 0; band < numBands; ++band)
    {
        auto& column = band < 2 ? leftColumn : rightColumn;

        for (int rowIndex = 0; rowIndex < 3 && (band * 3 + rowIndex) < static_cast<int> (rows.size()); ++rowIndex)
        {
            rows[static_cast<size_t> (band * 3 + rowIndex)]->setBounds (column.removeFromTop (38));
            column.removeFromTop (2);
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

    for (auto& row : rows)
        addAndMakeVisible (*row);
}

void ProcessorControlsComponent::CompressorTab::resized()
{
    layoutRows (*this, rows);
}

void ProcessorControlsComponent::CompressorTab::refreshFromGraph()
{
    rows[0]->slider.setValue (graph.getCompressorThresholdDb(), juce::dontSendNotification);
    rows[1]->slider.setValue (graph.getCompressorRatio(), juce::dontSendNotification);
    rows[2]->slider.setValue (graph.getCompressorAttackMs(), juce::dontSendNotification);
    rows[3]->slider.setValue (graph.getCompressorReleaseMs(), juce::dontSendNotification);
    rows[4]->slider.setValue (graph.getCompressorMakeupGainDb(), juce::dontSendNotification);
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

    for (auto& row : rows)
        addAndMakeVisible (*row);
}

void ProcessorControlsComponent::DistortionTab::resized()
{
    auto bounds = getLocalBounds().reduced (10);

    for (const auto& row : rows)
    {
        row->setBounds (bounds.removeFromTop (54));
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
}

void ProcessorControlsComponent::CabinetTab::resized()
{
    auto bounds = getLocalBounds().reduced (10);

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

    for (auto& row : rows)
        addAndMakeVisible (*row);
}

void ProcessorControlsComponent::ReverbTab::resized()
{
    layoutRows (*this, rows);
}

void ProcessorControlsComponent::ReverbTab::refreshFromGraph()
{
    rows[0]->slider.setValue (graph.getReverbWetDry(), juce::dontSendNotification);
    rows[1]->slider.setValue (graph.getReverbDecay(), juce::dontSendNotification);
    rows[2]->slider.setValue (graph.getReverbSize(), juce::dontSendNotification);
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
    reverbTab.refreshFromGraph();

    pluginTab.refreshFromGraph();
}

ProcessorControlsComponent::~ProcessorControlsComponent() = default;

void ProcessorControlsComponent::resized()
{
    tabs.setBounds (getLocalBounds());
}

void ProcessorControlsComponent::layoutRows (juce::Component& parent, const std::vector<std::unique_ptr<SliderRow>>& rows)
{
    auto bounds = parent.getLocalBounds().reduced (10);

    for (const auto& row : rows)
    {
        row->setBounds (bounds.removeFromTop (54));
        bounds.removeFromTop (4);
    }
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
        setSize (content->getWidth(), content->getHeight() + getTitleBarHeight());
        setVisible (true);
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

    addAndMakeVisible (parameterPanel);

    rescanPlugins();
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

    statusText.setBounds (bounds.removeFromTop (56));

    bounds.removeFromTop (8);

    parameterPanel.setBounds (bounds);

    auto parameterBounds = parameterPanel.getLocalBounds();

    for (size_t i = 0; i < parameterSliders.size(); ++i)
    {
        auto parameterRow = parameterBounds.removeFromTop (28);
        parameterLabels[i]->setBounds (parameterRow.removeFromLeft (180));
        parameterSliders[i]->setBounds (parameterRow);
    }
}

void ProcessorControlsComponent::PluginTab::rescanPlugins()
{
    juce::StringArray paths;
    paths.add (juce::File::getSpecialLocation (juce::File::userHomeDirectory).getChildFile (".vst3").getFullPathName());
    paths.add ("/usr/lib/vst3");
    paths.add ("/usr/local/lib/vst3");
    paths.add ("build/linux-daw-test-plugin_artefacts/VST3");

    const juce::Component::SafePointer<PluginTab> safeThis (this);

    scanPool.addJob ([safeThis, paths]
    {
        auto found = scanVst3Plugins (paths);

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

void ProcessorControlsComponent::PluginTab::loadSelectedPlugin()
{
    const int index = pluginCombo.getSelectedItemIndex();

    if (index < 0 || index >= static_cast<int> (scannedPlugins.size()))
    {
        statusText.setText ("No plugin selected", juce::dontSendNotification);
        return;
    }

    const double rate = engine.isRunning() ? engine.getSampleRate() : 48000.0;
    const int buffer = engine.isRunning() ? engine.getBufferSize() : 128;

    const auto error = graph.loadVst3Plugin (scannedPlugins[static_cast<size_t> (index)], rate, buffer);

    if (error.isNotEmpty())
    {
        statusText.setText ("Error: " + error, juce::dontSendNotification);
        return;
    }

    statusText.setText ("Loaded: " + graph.getVst3PluginName(), juce::dontSendNotification);

    removeButton.setEnabled (true);
    bypassButton.setEnabled (true);
    bypassButton.setToggleState (false, juce::dontSendNotification);

    const auto* instance = graph.getVst3PluginInstance();
    editorButton.setEnabled (instance != nullptr && instance->hasEditor());

    rebuildParameterSliders();
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
}

void ProcessorControlsComponent::PluginTab::refreshFromGraph()
{
    rebuildParameterSliders();
    removeButton.setEnabled (graph.isVst3PluginLoaded());
    bypassButton.setEnabled (graph.isVst3PluginLoaded());
    bypassButton.setToggleState (graph.isVst3PluginBypassed(), juce::dontSendNotification);

    if (auto* instance = graph.getVst3PluginInstance())
        editorButton.setEnabled (instance->hasEditor());
    else
        editorButton.setEnabled (false);

    editorWindow.reset();
    statusText.setText (graph.isVst3PluginLoaded()
                            ? "Loaded: " + graph.getVst3PluginName()
                            : "No plugin loaded",
                        juce::dontSendNotification);
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
