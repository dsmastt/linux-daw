#pragma once

#include <array>
#include <atomic>
#include <cstdint>
#include <memory>

#include <juce_audio_devices/juce_audio_devices.h>

namespace daw
{
struct PackedMidiEvent
{
    std::uint8_t bytes[4] = {};
    int numBytes = 0;
};

class MidiInputHandler final : public juce::MidiInputCallback
{
public:
    MidiInputHandler() = default;
    ~MidiInputHandler() override;

    static juce::Array<juce::MidiDeviceInfo> getAvailableDevices();

    juce::String openDevice (const juce::String& identifier);
    juce::String openVirtualDevice (const juce::String& name);
    void closeDevice() noexcept;

    bool isOpen() const noexcept { return open.load(); }
    juce::String getDeviceName() const noexcept { return deviceName; }

    void handleIncomingMidiMessage (juce::MidiInput* source, const juce::MidiMessage& message) override;

    void drainInto (juce::MidiBuffer& buffer) noexcept;

    int getNoteOnCount() const noexcept { return noteOnCount.load(); }
    int getNoteOffCount() const noexcept { return noteOffCount.load(); }
    int getCcCount() const noexcept { return ccCount.load(); }
    int getLastNoteNumber() const noexcept { return lastNoteNumber.load(); }
    int getLastVelocity() const noexcept { return lastVelocity.load(); }
    int getLastControllerNumber() const noexcept { return lastControllerNumber.load(); }
    int getLastControllerValue() const noexcept { return lastControllerValue.load(); }
    int getDroppedEvents() const noexcept { return droppedEvents.load(); }

private:
    static constexpr int queueCapacity = 512;

    std::unique_ptr<juce::MidiInput> input;
    std::atomic<bool> open { false };
    juce::String deviceName;

    std::array<PackedMidiEvent, queueCapacity> queue;
    std::atomic<int> writeIndex { 0 };
    std::atomic<int> readIndex { 0 };

    std::atomic<int> noteOnCount { 0 };
    std::atomic<int> noteOffCount { 0 };
    std::atomic<int> ccCount { 0 };
    std::atomic<int> lastNoteNumber { -1 };
    std::atomic<int> lastVelocity { 0 };
    std::atomic<int> lastControllerNumber { -1 };
    std::atomic<int> lastControllerValue { 0 };
    std::atomic<int> droppedEvents { 0 };
};
} // namespace daw
