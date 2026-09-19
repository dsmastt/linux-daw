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

    statusLabel.setMultiLine (true, false);
    statusLabel.setReadOnly (true);
    statusLabel.setCaretVisible (false);
    statusLabel.setColour (juce::TextEditor::backgroundColourId, juce::Colours::black);
    statusLabel.setColour (juce::TextEditor::textColourId, juce::Colours::lightgreen);
    addAndMakeVisible (statusLabel);

    refreshDevices();

    startTimerHz (4);

    setSize (560, 420);
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

    statusLabel.setBounds (bounds);
}

void MainComponent::timerCallback()
{
    updateStatus();
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
