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
ProcessorControlsComponent::GainTab::GainTab (AudioGraph& graph)
{
    rows.push_back (std::make_unique<SliderRow> ("Gain (dB)", -60.0, 12.0, 0.5, graph.getGainDb(),
                                                 [&graph] (double value) { graph.setGainDb (static_cast<float> (value)); }));

    for (auto& row : rows)
        addAndMakeVisible (*row);
}

void ProcessorControlsComponent::GainTab::resized()
{
    layoutRows (*this, rows);
}

//==============================================================================
ProcessorControlsComponent::GateTab::GateTab (AudioGraph& graph)
{
    rows.push_back (std::make_unique<SliderRow> ("Threshold (dB)", -90.0, 0.0, 0.5, graph.getGateThresholdDb(),
                                                 [&graph] (double value) { graph.setGateThresholdDb (static_cast<float> (value)); }));
    rows.push_back (std::make_unique<SliderRow> ("Attack (ms)", 0.1, 50.0, 0.1, graph.getGateAttackMs(),
                                                 [&graph] (double value) { graph.setGateAttackMs (static_cast<float> (value)); }));
    rows.push_back (std::make_unique<SliderRow> ("Release (ms)", 1.0, 1000.0, 1.0, graph.getGateReleaseMs(),
                                                 [&graph] (double value) { graph.setGateReleaseMs (static_cast<float> (value)); }));

    for (auto& row : rows)
        addAndMakeVisible (*row);
}

void ProcessorControlsComponent::GateTab::resized()
{
    layoutRows (*this, rows);
}

//==============================================================================
ProcessorControlsComponent::EqTab::EqTab (AudioGraph& graph)
{
    for (int band = 0; band < graph.getNumEqBands(); ++band)
    {
        rows.push_back (std::make_unique<SliderRow> ("Band " + juce::String (band + 1) + " Freq (Hz)",
                                                     20.0, 20000.0, 1.0, 1000.0,
                                                     [&graph, band] (double value) { graph.setEqBandParameters (band, value, graph.getEqBandGain (band), graph.getEqBandQ (band)); }));
        rows.back()->slider.setSkewFactorFromMidPoint (1000.0);

        rows.push_back (std::make_unique<SliderRow> ("Band " + juce::String (band + 1) + " Gain (dB)",
                                                     -12.0, 12.0, 0.1, 0.0,
                                                     [&graph, band] (double value) { graph.setEqBandParameters (band, graph.getEqBandFrequency (band), static_cast<float> (value), graph.getEqBandQ (band)); }));
        rows.push_back (std::make_unique<SliderRow> ("Band " + juce::String (band + 1) + " Q",
                                                     0.1, 10.0, 0.1, 0.7,
                                                     [&graph, band] (double value) { graph.setEqBandParameters (band, graph.getEqBandFrequency (band), graph.getEqBandGain (band), static_cast<float> (value)); }));
    }

    for (auto& row : rows)
        addAndMakeVisible (*row);
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
ProcessorControlsComponent::CompressorTab::CompressorTab (AudioGraph& graph)
{
    rows.push_back (std::make_unique<SliderRow> ("Threshold (dB)", -60.0, 0.0, 0.5, -10.0,
                                                 [&graph] (double value) { graph.setCompressorThresholdDb (static_cast<float> (value)); }));
    rows.push_back (std::make_unique<SliderRow> ("Ratio", 1.0, 20.0, 0.1, 4.0,
                                                 [&graph] (double value) { graph.setCompressorRatio (static_cast<float> (value)); }));
    rows.push_back (std::make_unique<SliderRow> ("Attack (ms)", 0.1, 100.0, 0.1, 5.0,
                                                 [&graph] (double value) { graph.setCompressorAttackMs (static_cast<float> (value)); }));
    rows.push_back (std::make_unique<SliderRow> ("Release (ms)", 1.0, 1000.0, 1.0, 100.0,
                                                 [&graph] (double value) { graph.setCompressorReleaseMs (static_cast<float> (value)); }));
    rows.push_back (std::make_unique<SliderRow> ("Makeup (dB)", 0.0, 24.0, 0.5, 0.0,
                                                 [&graph] (double value) { graph.setCompressorMakeupGainDb (static_cast<float> (value)); }));

    for (auto& row : rows)
        addAndMakeVisible (*row);
}

void ProcessorControlsComponent::CompressorTab::resized()
{
    layoutRows (*this, rows);
}

//==============================================================================
ProcessorControlsComponent::DistortionTab::DistortionTab (AudioGraph& graph)
{
    rows.push_back (std::make_unique<SliderRow> ("Drive (dB)", 0.0, 40.0, 0.5, 12.0,
                                                 [&graph] (double value) { graph.setDistortionDriveDb (static_cast<float> (value)); }));
    rows.push_back (std::make_unique<SliderRow> ("Tone", 0.0, 1.0, 0.01, 0.5,
                                                 [&graph] (double value) { graph.setDistortionTone (static_cast<float> (value)); }));

    modeLabel.setText ("Mode:", juce::dontSendNotification);
    addAndMakeVisible (modeLabel);

    modeCombo.addItem ("Soft clip", 1);
    modeCombo.addItem ("Hard clip", 2);
    modeCombo.addItem ("Cubic", 3);
    modeCombo.addItem ("Asymmetric", 4);
    modeCombo.setSelectedItemIndex (0);
    modeCombo.onChange = [&graph, combo = &modeCombo]
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

//==============================================================================
ProcessorControlsComponent::CabinetTab::CabinetTab (AudioGraph& graphToUse)
    : graph (graphToUse)
{
    loadButton.setButtonText ("Load IR...");
    loadButton.onClick = [this]
    {
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
ProcessorControlsComponent::ReverbTab::ReverbTab (AudioGraph& graph)
{
    rows.push_back (std::make_unique<SliderRow> ("Wet/Dry", 0.0, 1.0, 0.01, 0.2,
                                                 [&graph] (double value) { graph.setReverbWetDry (static_cast<float> (value)); }));
    rows.push_back (std::make_unique<SliderRow> ("Decay", 0.0, 1.0, 0.01, 0.5,
                                                 [&graph] (double value) { graph.setReverbDecay (static_cast<float> (value)); }));
    rows.push_back (std::make_unique<SliderRow> ("Size", 0.0, 1.0, 0.01, 0.5,
                                                 [&graph] (double value) { graph.setReverbSize (static_cast<float> (value)); }));

    for (auto& row : rows)
        addAndMakeVisible (*row);
}

void ProcessorControlsComponent::ReverbTab::resized()
{
    layoutRows (*this, rows);
}

//==============================================================================
ProcessorControlsComponent::ProcessorControlsComponent (AudioGraph& graph)
    : tabs (juce::TabbedButtonBar::TabsAtTop),
      gainTab (graph),
      gateTab (graph),
      eqTab (graph),
      compressorTab (graph),
      distortionTab (graph),
      cabinetTab (graph),
      reverbTab (graph)
{
    addAndMakeVisible (tabs);

    tabs.addTab ("Gain", juce::Colours::darkgrey, &gainTab, false);
    tabs.addTab ("Gate", juce::Colours::darkgrey, &gateTab, false);
    tabs.addTab ("EQ", juce::Colours::darkgrey, &eqTab, false);
    tabs.addTab ("Compressor", juce::Colours::darkgrey, &compressorTab, false);
    tabs.addTab ("Distortion", juce::Colours::darkgrey, &distortionTab, false);
    tabs.addTab ("Cabinet", juce::Colours::darkgrey, &cabinetTab, false);
    tabs.addTab ("Reverb", juce::Colours::darkgrey, &reverbTab, false);
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
} // namespace daw
