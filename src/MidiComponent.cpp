#include "MidiComponent.h"

namespace daw
{
MidiComponent::MidiComponent (AudioEngine& engineToUse)
    : engine (engineToUse)
{
    refreshButton.setButtonText ("Refresh");
    refreshButton.onClick = [this] { refreshDevices(); };
    addAndMakeVisible (refreshButton);

    addAndMakeVisible (deviceCombo);

    openButton.setButtonText ("Open");
    openButton.onClick = [this] { openSelectedDevice(); };
    addAndMakeVisible (openButton);

    closeButton.setButtonText ("Close");
    closeButton.setEnabled (false);
    closeButton.onClick = [this] { closeDevice(); };
    addAndMakeVisible (closeButton);

    statusText.setMultiLine (true, false);
    statusText.setReadOnly (true);
    statusText.setCaretVisible (false);
    statusText.setText ("No MIDI device open", juce::dontSendNotification);
    addAndMakeVisible (statusText);

    refreshDevices();

    startTimerHz (4);
}

MidiComponent::~MidiComponent()
{
    stopTimer();
}

void MidiComponent::resized()
{
    auto bounds = getLocalBounds().reduced (10);

    auto row = bounds.removeFromTop (30);
    refreshButton.setBounds (row.removeFromLeft (80));
    row.removeFromLeft (8);
    deviceCombo.setBounds (row.removeFromLeft (row.getWidth() - 8 - 2 * 86));
    row.removeFromLeft (8);
    openButton.setBounds (row.removeFromLeft (80));
    row.removeFromLeft (6);
    closeButton.setBounds (row.removeFromLeft (80));

    bounds.removeFromTop (10);

    statusText.setBounds (bounds);
}

void MidiComponent::timerCallback()
{
    const auto& handler = engine.getMidiHandler();

    if (!handler.isOpen())
    {
        statusText.setText ("No MIDI device open", juce::dontSendNotification);
        return;
    }

    juce::String status;

    status << "Device: " << handler.getDeviceName() << "\n"
           << "Note ons: " << handler.getNoteOnCount()
           << ", note offs: " << handler.getNoteOffCount()
           << ", controllers: " << handler.getCcCount() << "\n";

    if (handler.getLastNoteNumber() >= 0)
        status << "Last note: " << handler.getLastNoteNumber()
               << " (velocity " << handler.getLastVelocity() << ")\n";

    if (handler.getLastControllerNumber() >= 0)
        status << "Last controller: " << handler.getLastControllerNumber()
               << " = " << handler.getLastControllerValue() << "\n";

    if (handler.getDroppedEvents() > 0)
        status << "Dropped events: " << handler.getDroppedEvents() << "\n";

    statusText.setText (status, juce::dontSendNotification);
}

void MidiComponent::refreshDevices()
{
    deviceCombo.clear();

    const auto devices = daw::MidiInputHandler::getAvailableDevices();

    for (const auto& device : devices)
        deviceCombo.addItem (device.name, deviceCombo.getNumItems() + 1);

    if (deviceCombo.getNumItems() == 0)
        deviceCombo.addItem ("No MIDI devices found", 1);
}

void MidiComponent::openSelectedDevice()
{
    const int index = deviceCombo.getSelectedItemIndex();

    if (index < 0)
        return;

    const auto devices = daw::MidiInputHandler::getAvailableDevices();

    if (index >= devices.size())
    {
        statusText.setText ("No MIDI device selected", juce::dontSendNotification);
        return;
    }

    const auto error = engine.openMidiDevice (devices.getReference (index).identifier);

    if (error.isNotEmpty())
    {
        statusText.setText ("Error: " + error, juce::dontSendNotification);
        return;
    }

    closeButton.setEnabled (true);
}

void MidiComponent::closeDevice()
{
    engine.closeMidiDevice();
    closeButton.setEnabled (false);
}
} // namespace daw
