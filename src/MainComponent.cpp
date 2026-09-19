#include "MainComponent.h"

namespace daw
{
namespace
{
juce::AudioIODeviceType* findAlsaType (const juce::OwnedArray<juce::AudioIODeviceType>& types)
{
    for (auto* type : types)
        if (type->getTypeName() == "ALSA")
            return type;

    return nullptr;
}

void selectItemContaining (juce::ComboBox& combo, const juce::String& text)
{
    for (int i = 0; i < combo.getNumItems(); ++i)
    {
        if (combo.getItemText (i).contains (text))
        {
            combo.setSelectedItemIndex (i);
            return;
        }
    }
}
} // namespace

MainComponent::MainComponent()
    : viewTabs (juce::TabbedButtonBar::TabsAtTop),
      inputMeter ("Input"),
      outputMeter ("Output")
{
    titleLabel.setText ("Linux DAW", juce::dontSendNotification);
    titleLabel.setFont (juce::FontOptions{}.withHeight (22.0f).withStyle ("Bold"));
    titleLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (titleLabel);

    inputLabel.setText ("Input:", juce::dontSendNotification);
    outputLabel.setText ("Output:", juce::dontSendNotification);
    sampleRateLabel.setText ("Sample rate:", juce::dontSendNotification);
    bufferLabel.setText ("Buffer:", juce::dontSendNotification);

    addAndMakeVisible (inputLabel);
    addAndMakeVisible (inputCombo);
    addAndMakeVisible (outputLabel);
    addAndMakeVisible (outputCombo);
    addAndMakeVisible (sampleRateLabel);
    addAndMakeVisible (sampleRateCombo);
    addAndMakeVisible (bufferLabel);
    addAndMakeVisible (bufferCombo);

    sampleRateCombo.addItemList ({ "44100", "48000", "88200", "96000", "176400", "192000" }, 1);
    sampleRateCombo.setSelectedItemIndex (1);

    bufferCombo.addItemList ({ "64", "128", "256", "512", "1024" }, 1);
    bufferCombo.setSelectedItemIndex (1);

    startButton.setButtonText ("Start Audio");
    stopButton.setButtonText ("Stop Audio");
    stopButton.setEnabled (false);

    startButton.onClick = [this] { startAudio(); };
    stopButton.onClick = [this] { stopAudio(); };

    addAndMakeVisible (startButton);
    addAndMakeVisible (stopButton);

    transportPlayButton.setButtonText ("Play");
    transportPlayButton.onClick = [this] { engine.getAudioGraph().playTransport(); };
    addAndMakeVisible (transportPlayButton);

    transportPauseButton.setButtonText ("Pause");
    transportPauseButton.onClick = [this] { engine.getAudioGraph().pauseTransport(); };
    addAndMakeVisible (transportPauseButton);

    transportStopButton.setButtonText ("Stop");
    transportStopButton.onClick = [this] { engine.getAudioGraph().stopTransport(); };
    addAndMakeVisible (transportStopButton);

    tempoLabel.setText ("Tempo:", juce::dontSendNotification);
    addAndMakeVisible (tempoLabel);

    tempoSlider.setRange (40.0, 240.0, 1.0);
    tempoSlider.setValue (120.0, juce::dontSendNotification);
    tempoSlider.setTextBoxStyle (juce::Slider::TextBoxRight, false, 48, 22);
    tempoSlider.onValueChange = [this]
    {
        engine.getAudioGraph().getTransport().setTempoBpm (tempoSlider.getValue());
    };
    addAndMakeVisible (tempoSlider);

    transportStatus.setMultiLine (false, false);
    transportStatus.setReadOnly (true);
    transportStatus.setCaretVisible (false);
    addAndMakeVisible (transportStatus);

    processorControls = std::make_unique<ProcessorControlsComponent> (engine);
    mixer = std::make_unique<MixerComponent> (engine.getAudioGraph());
    timeline = std::make_unique<TimelineComponent> (engine);
    midi = std::make_unique<MidiComponent> (engine);
    automation = std::make_unique<AutomationComponent> (engine);

    viewTabs.addTab ("DSP", juce::Colours::darkgrey, processorControls.get(), false);
    viewTabs.addTab ("Mixer", juce::Colours::darkgrey, mixer.get(), false);
    viewTabs.addTab ("Timeline", juce::Colours::darkgrey, timeline.get(), false);
    viewTabs.addTab ("MIDI", juce::Colours::darkgrey, midi.get(), false);
    viewTabs.addTab ("Automation", juce::Colours::darkgrey, automation.get(), false);
    addAndMakeVisible (viewTabs);

    addAndMakeVisible (inputMeter);
    addAndMakeVisible (outputMeter);

    statusLabel.setMultiLine (true, false);
    statusLabel.setReadOnly (true);
    statusLabel.setCaretVisible (false);
    statusLabel.setColour (juce::TextEditor::backgroundColourId, juce::Colours::black);
    statusLabel.setColour (juce::TextEditor::textColourId, juce::Colours::lightgreen);
    addAndMakeVisible (statusLabel);

    refreshDevices();

    startTimerHz (4);

    setSize (800, 880);
}

MainComponent::~MainComponent()
{
    stopTimer();
    stopAudio();
}

void MainComponent::resized()
{
    auto bounds = getLocalBounds().reduced (12);

    titleLabel.setBounds (bounds.removeFromTop (40));

    bounds.removeFromTop (8);

    auto makeRow = [&bounds] (juce::Label& label, juce::ComboBox& combo)
    {
        auto row = bounds.removeFromTop (30);
        label.setBounds (row.removeFromLeft (100));
        bounds.removeFromTop (6);
        combo.setBounds (row);
    };

    makeRow (inputLabel, inputCombo);
    makeRow (outputLabel, outputCombo);
    makeRow (sampleRateLabel, sampleRateCombo);
    makeRow (bufferLabel, bufferCombo);

    bounds.removeFromTop (10);

    auto buttonRow = bounds.removeFromTop (34);
    startButton.setBounds (buttonRow.removeFromLeft (140));
    buttonRow.removeFromLeft (10);
    stopButton.setBounds (buttonRow.removeFromLeft (140));

    bounds.removeFromTop (10);

    auto transportRow = bounds.removeFromTop (34);
    transportPlayButton.setBounds (transportRow.removeFromLeft (70));
    transportRow.removeFromLeft (8);
    transportPauseButton.setBounds (transportRow.removeFromLeft (70));
    transportRow.removeFromLeft (8);
    transportStopButton.setBounds (transportRow.removeFromLeft (70));
    transportRow.removeFromLeft (12);
    tempoLabel.setBounds (transportRow.removeFromLeft (60));
    tempoSlider.setBounds (transportRow.removeFromLeft (110));
    transportRow.removeFromLeft (12);
    transportStatus.setBounds (transportRow);

    bounds.removeFromTop (10);

    auto statusBounds = bounds.removeFromBottom (120);
    bounds.removeFromBottom (10);

    auto metersBounds = statusBounds.removeFromLeft (110);
    inputMeter.setBounds (metersBounds.removeFromLeft (50));
    metersBounds.removeFromLeft (10);
    outputMeter.setBounds (metersBounds.removeFromLeft (50));

    statusLabel.setBounds (statusBounds);
    viewTabs.setBounds (bounds);
}

void MainComponent::timerCallback()
{
    updateStatus();

    const auto& transport = engine.getAudioGraph().getTransport();
    const auto positionSamples = transport.getPositionSamples();
    const auto positionSeconds = engine.isRunning() && engine.getSampleRate() > 0.0
                                   ? static_cast<double> (positionSamples) / engine.getSampleRate()
                                   : 0.0;

    juce::String transportText;

    switch (transport.getState())
    {
        case daw::TransportState::playing: transportText = "Playing"; break;
        case daw::TransportState::paused:  transportText = "Paused"; break;
        case daw::TransportState::stopped: transportText = "Stopped"; break;
    }

    transportText << "  " << positionSeconds << " s  (" << positionSamples << " samples)  "
                  << static_cast<int> (transport.getTempoBpm()) << " BPM";

    transportStatus.setText (transportText, juce::dontSendNotification);

    if (engine.isRunning())
    {
        displayedInputLevel = std::max (engine.consumeInputPeak(), displayedInputLevel * 0.75f);
        displayedOutputLevel = std::max (engine.consumeOutputPeak(), displayedOutputLevel * 0.75f);

        const auto clipCount = engine.getClipCount();

        if (clipCount != lastClipCount)
        {
            lastClipCount = clipCount;
            lastClipTime = juce::Time::getCurrentTime();
        }

        const bool clippingActive = juce::Time::getCurrentTime() - lastClipTime
                                      < juce::RelativeTime::milliseconds (1200);
        outputMeter.setClipping (clippingActive);
    }
    else
    {
        engine.consumeInputPeak();
        engine.consumeOutputPeak();

        displayedInputLevel = 0.0f;
        displayedOutputLevel = 0.0f;
        outputMeter.setClipping (false);
    }

    inputMeter.setLevel (displayedInputLevel);
    outputMeter.setLevel (displayedOutputLevel);
}

void MainComponent::refreshDevices()
{
    deviceTypes.clear();
    deviceManager.createAudioDeviceTypes (deviceTypes);

    auto* alsaType = findAlsaType (deviceTypes);

    inputCombo.clear();
    outputCombo.clear();

    if (alsaType == nullptr)
    {
        inputCombo.addItem ("No ALSA devices", 1);
        outputCombo.addItem ("No ALSA devices", 1);
        return;
    }

    alsaType->scanForDevices();

    int id = 1;

    for (const auto& name : alsaType->getDeviceNames (true))
        inputCombo.addItem (name, id++);

    id = 1;

    for (const auto& name : alsaType->getDeviceNames (false))
        outputCombo.addItem (name, id++);

    selectItemContaining (inputCombo, "AI-04");
    selectItemContaining (outputCombo, "AI-04");
}

void MainComponent::startAudio()
{
    const auto inputDeviceName = inputCombo.getText();
    const auto outputDeviceName = outputCombo.getText();

    if (inputDeviceName.isEmpty() || outputDeviceName.isEmpty())
    {
        statusLabel.setText ("No audio device selected.", juce::sendNotification);
        return;
    }

    const auto sampleRate = sampleRateCombo.getText().getDoubleValue();
    const auto bufferSize = bufferCombo.getText().getIntValue();

    const auto error = engine.start (inputDeviceName, outputDeviceName, sampleRate, bufferSize);

    if (error.isNotEmpty())
    {
        statusLabel.setText ("Error: " + error, juce::sendNotification);
        return;
    }

    startButton.setEnabled (false);
    stopButton.setEnabled (true);

    updateStatus();
}

void MainComponent::stopAudio()
{
    engine.stop();

    startButton.setEnabled (true);
    stopButton.setEnabled (false);

    updateStatus();
}

void MainComponent::updateStatus()
{
    if (!engine.isRunning())
    {
        statusLabel.setText ("Stopped", juce::dontSendNotification);
        return;
    }

    const auto totalLatencySamples = engine.getInputLatencySamples() + engine.getOutputLatencySamples();
    const auto latencyMs = (1000.0 * totalLatencySamples) / engine.getSampleRate();

    juce::String status;

    status << "Running\n"
           << "Sample rate: " << static_cast<int> (engine.getSampleRate()) << " Hz\n"
           << "Buffer size: " << engine.getBufferSize() << " samples\n"
           << "Channels: " << engine.getNumInputChannels() << " in / "
           << engine.getNumOutputChannels() << " out\n"
           << "Latency: " << latencyMs << " ms\n"
           << "Xruns: " << engine.getXRunCount() << "\n"
           << "Callbacks: " << engine.getNumCallbacks();

    statusLabel.setText (status, juce::dontSendNotification);
}
} // namespace daw
