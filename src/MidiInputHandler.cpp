#include "MidiInputHandler.h"

namespace daw
{
MidiInputHandler::~MidiInputHandler()
{
    closeDevice();
}

juce::Array<juce::MidiDeviceInfo> MidiInputHandler::getAvailableDevices()
{
    return juce::MidiInput::getAvailableDevices();
}

juce::String MidiInputHandler::openDevice (const juce::String& identifier)
{
    closeDevice();

    input = juce::MidiInput::openDevice (identifier, this);

    if (input == nullptr)
        return "Could not open MIDI device: " + identifier;

    input->start();

    deviceName = input->getName();
    open.store (true, std::memory_order_release);

    return {};
}

juce::String MidiInputHandler::openVirtualDevice (const juce::String& name)
{
    closeDevice();

    input = juce::MidiInput::createNewDevice (name, this);

    if (input == nullptr)
        return "Could not create virtual MIDI device: " + name;

    input->start();

    deviceName = input->getName();
    open.store (true, std::memory_order_release);

    return {};
}

void MidiInputHandler::closeDevice() noexcept
{
    open.store (false, std::memory_order_release);

    if (input != nullptr)
        input->stop();

    input.reset();
    deviceName.clear();

    writeIndex.store (0, std::memory_order_relaxed);
    readIndex.store (0, std::memory_order_relaxed);
}

void MidiInputHandler::handleIncomingMidiMessage (juce::MidiInput*, const juce::MidiMessage& message)
{
    const auto* rawData = message.getRawData();
    const int rawSize = message.getRawDataSize();

    if (message.isNoteOn())
    {
        noteOnCount.fetch_add (1, std::memory_order_relaxed);
        lastNoteNumber.store (message.getNoteNumber(), std::memory_order_relaxed);
        lastVelocity.store (message.getVelocity(), std::memory_order_relaxed);
    }
    else if (message.isNoteOff())
    {
        noteOffCount.fetch_add (1, std::memory_order_relaxed);
    }
    else if (message.isController())
    {
        ccCount.fetch_add (1, std::memory_order_relaxed);
        lastControllerNumber.store (message.getControllerNumber(), std::memory_order_relaxed);
        lastControllerValue.store (message.getControllerValue(), std::memory_order_relaxed);
    }

    const int writePos = writeIndex.load (std::memory_order_relaxed);
    const int nextPos = (writePos + 1) % queueCapacity;

    if (nextPos == readIndex.load (std::memory_order_acquire))
    {
        droppedEvents.fetch_add (1, std::memory_order_relaxed);
        return;
    }

    auto& event = queue[static_cast<size_t> (writePos)];
    event.numBytes = rawSize;

    for (int i = 0; i < rawSize && i < 4; ++i)
        event.bytes[static_cast<size_t> (i)] = rawData[i];

    writeIndex.store (nextPos, std::memory_order_release);
}

void MidiInputHandler::drainInto (juce::MidiBuffer& buffer) noexcept
{
    while (true)
    {
        const int readPos = readIndex.load (std::memory_order_relaxed);

        if (readPos == writeIndex.load (std::memory_order_acquire))
            return;

        const auto& event = queue[static_cast<size_t> (readPos)];

        buffer.addEvent (event.bytes, event.numBytes, 0);

        readIndex.store ((readPos + 1) % queueCapacity, std::memory_order_release);
    }
}
} // namespace daw
