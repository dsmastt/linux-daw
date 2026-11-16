#include "MainComponent.h"

#include <pthread.h>
#include <sched.h>

#include "DarkLookAndFeel.h"

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

bool selectItemContaining (juce::ComboBox& combo, const juce::String& text)
{
    for (int i = 0; i < combo.getNumItems(); ++i)
    {
        if (combo.getItemText (i).contains (text))
        {
            combo.setSelectedItemIndex (i);
            return true;
        }
    }

    return false;
}
} // namespace

MainComponent::MainComponent()
    : viewTabs (juce::TabbedButtonBar::TabsAtTop),
      inputMeter ("Input"),
      outputMeter ("Output")
{
    juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
        .getChildFile ("linux-daw")
        .createDirectory();
    titleLabel.setText ("Linux DAW", juce::dontSendNotification);
    titleLabel.setFont (juce::FontOptions{}.withHeight (22.0f).withStyle ("Bold"));
    titleLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (titleLabel);

    inputLabel.setText ("Input:", juce::dontSendNotification);
    outputLabel.setText ("Output:", juce::dontSendNotification);
    inputChannelsLabel.setText ("Input channels:", juce::dontSendNotification);
    sampleRateLabel.setText ("Sample rate:", juce::dontSendNotification);
    bufferLabel.setText ("Buffer:", juce::dontSendNotification);

    addAndMakeVisible (inputLabel);
    addAndMakeVisible (inputCombo);
    addAndMakeVisible (outputLabel);
    addAndMakeVisible (outputCombo);
    addAndMakeVisible (inputChannelsLabel);
    addAndMakeVisible (inputChannelsCombo);
    addAndMakeVisible (sampleRateLabel);
    addAndMakeVisible (sampleRateCombo);
    addAndMakeVisible (bufferLabel);
    addAndMakeVisible (bufferCombo);

    inputChannelsCombo.addItem ("Stereo (2)", 1);
    inputChannelsCombo.addItem ("Mono (Left)", 2);
    inputChannelsCombo.setSelectedItemIndex (1);

    sampleRateCombo.addItemList ({ "44100", "48000", "88200", "96000", "176400", "192000" }, 1);
    sampleRateCombo.setSelectedItemIndex (1);

    bufferCombo.addItemList ({ "64", "128", "256", "512", "1024" }, 1);
    bufferCombo.setSelectedItemIndex (2);

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

    metronomeButton.setButtonText ("Click");
    metronomeButton.onClick = [this]
    {
        engine.getAudioGraph().getMetronome().setEnabled (metronomeButton.getToggleState());
    };
    addAndMakeVisible (metronomeButton);

    metronomeVolume.setSliderStyle (juce::Slider::LinearHorizontal);
    metronomeVolume.setRange (0.0, 1.0, 0.01);
    metronomeVolume.setValue (0.5, juce::dontSendNotification);
    metronomeVolume.setTextBoxStyle (juce::Slider::TextBoxRight, false, 34, 20);
    metronomeVolume.setTooltip ("Metronome volume");
    metronomeVolume.onValueChange = [this]
    {
        engine.getAudioGraph().getMetronome().setVolume (static_cast<float> (metronomeVolume.getValue()));
    };
    addAndMakeVisible (metronomeVolume);

    transportStatus.setMultiLine (false, false);
    transportStatus.setReadOnly (true);
    transportStatus.setCaretVisible (false);
    addAndMakeVisible (transportStatus);

    processorControls = std::make_unique<ProcessorControlsComponent> (engine, undoManager);
    mixer = std::make_unique<MixerComponent> (engine.getAudioGraph(), undoManager);
    timeline = std::make_unique<TimelineComponent> (engine, undoManager);
    midi = std::make_unique<MidiComponent> (engine);
    automation = std::make_unique<AutomationComponent> (engine, undoManager);
    tunerComponent = std::make_unique<TunerComponent> (engine);
    presetComponent = std::make_unique<PresetComponent> (engine, undoManager,
                                                          [this] { refreshAfterUndoRedo(); },
                                                          [this] { closePluginEditor(); });

    viewTabs.addTab ("DSP", juce::Colours::darkgrey, processorControls.get(), false);
    viewTabs.addTab ("Tuner", juce::Colours::darkgrey, tunerComponent.get(), false);
    viewTabs.addTab ("Mixer", juce::Colours::darkgrey, mixer.get(), false);
    viewTabs.addTab ("Timeline", juce::Colours::darkgrey, timeline.get(), false);
    viewTabs.addTab ("MIDI", juce::Colours::darkgrey, midi.get(), false);
    viewTabs.addTab ("Automation", juce::Colours::darkgrey, automation.get(), false);
    viewTabs.addTab ("Presets", juce::Colours::darkgrey, presetComponent.get(), false);
    addAndMakeVisible (viewTabs);

    addAndMakeVisible (inputMeter);
    addAndMakeVisible (outputMeter);

    statusLabel.setMultiLine (true, false);
    statusLabel.setReadOnly (true);
    statusLabel.setCaretVisible (false);
    statusLabel.setColour (juce::TextEditor::backgroundColourId, DarkLookAndFeel::basePanel());
    statusLabel.setColour (juce::TextEditor::textColourId, DarkLookAndFeel::accent());
    addAndMakeVisible (statusLabel);

    refreshDevices();

    startTimerHz (4);

    setSize (800, 880);
}

MainComponent::~MainComponent()
{
    stopTimer();
    stopAudio();

    // These components own timers and plugin editors that reference the
    // engine, so destroy them while the engine is still alive.
    tunerComponent.reset();
    presetComponent.reset();
    automation.reset();
    midi.reset();
    timeline.reset();
    mixer.reset();
    processorControls.reset();
}

void MainComponent::closePluginEditor()
{
    if (processorControls != nullptr)
        processorControls->closePluginEditor();
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
    makeRow (inputChannelsLabel, inputChannelsCombo);
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
    transportRow.removeFromLeft (10);
    metronomeButton.setBounds (transportRow.removeFromLeft (62));
    transportRow.removeFromLeft (6);
    metronomeVolume.setBounds (transportRow.removeFromLeft (70));
    transportRow.removeFromLeft (10);
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

void MainComponent::refreshAfterUndoRedo()
{
    processorControls->refreshFromGraph();
    mixer->refreshFromGraph();
    timeline->repaint();
    automation->repaint();
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

    if (! selectItemContaining (inputCombo, "AI-04") && inputCombo.getNumItems() > 0)
        inputCombo.setSelectedItemIndex (0);

    if (! selectItemContaining (outputCombo, "Default ALSA Output") && outputCombo.getNumItems() > 0)
        outputCombo.setSelectedItemIndex (0);

    inputChannelsCombo.setSelectedItemIndex (1);
    bufferCombo.setSelectedItemIndex (2);
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
    const auto forceMono = inputChannelsCombo.getSelectedItemIndex() == 1;

    engine.setForceMonoInput (forceMono);

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
        startButton.setEnabled (true);
        stopButton.setEnabled (false);
        statusLabel.setText ("Stopped", juce::dontSendNotification);
        return;
    }

    const auto totalLatencySamples = engine.getInputLatencySamples() + engine.getOutputLatencySamples();
    const auto latencyMs = (1000.0 * totalLatencySamples) / engine.getSampleRate();

    juce::String status;

    const auto& track = engine.getAudioGraph().getTrack (0);
    const auto periodMs = (1000.0 * engine.getBufferSize()) / engine.getSampleRate();
    juce::String worstProcessor = "-";
    double worstShare = 0.0;

    for (int i = 0; i < track.getMeasuredProcessorCount(); ++i)
    {
        const auto callbacks = engine.getNumCallbacks();
        const auto avgMs = callbacks > 0 ? track.getProcessorTotalTimeMs (i) / callbacks : 0.0;

        if (avgMs > worstShare)
        {
            worstShare = avgMs;
            worstProcessor = track.getProcessorName (i);
        }
    }

    const auto maxCallbackMs = engine.getMaxCallbackTimeMs();

    status << "Running\n"
           << "Sample rate: " << static_cast<int> (engine.getSampleRate()) << " Hz\n"
           << "Buffer size: " << engine.getBufferSize() << " samples\n"
           << "Channels: " << engine.getNumInputChannels() << " in / "
           << engine.getNumOutputChannels() << " out\n"
           << "Latency: " << latencyMs << " ms\n"
           << "Xruns: " << engine.getXRunCount() << "\n"
           << "Callbacks: " << engine.getNumCallbacks() << "\n"
           << "Max callback: " << maxCallbackMs << " ms";

    if (periodMs > 0.0)
        status << " (" << static_cast<int> (100.0 * maxCallbackMs / periodMs) << "% of period)";

    status << "\nHeaviest DSP: " << worstProcessor;

    if (worstShare > 0.0 && periodMs > 0.0)
        status << " (" << static_cast<int> (100.0 * worstShare / periodMs) << "% of period)";

    const auto memoryMb = juce::File ("/proc/self/status").loadFileAsString()
                              .fromFirstOccurrenceOf ("VmRSS:", false, false)
                              .upToFirstOccurrenceOf ("kB", false, false)
                              .trim()
                              .getIntValue() / 1024;

    status << "\nMemory: " << memoryMb << " MB";

    int policy = -1;
    sched_param scheduling {};
    pthread_getschedparam (pthread_self(), &policy, &scheduling);

    status << "\nThread scheduling: policy " << policy << ", priority " << scheduling.sched_priority;

    if (policy == SCHED_OTHER)
        status << " (normal priority; realtime scheduling unavailable)";

    statusLabel.setText (status, juce::dontSendNotification);
}
} // namespace daw
