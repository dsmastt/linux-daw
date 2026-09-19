#pragma once

#include <atomic>
#include <cstdint>

namespace daw
{
constexpr int maxMidiEventsPerTrack = 2048;

struct MidiEventRecord
{
    std::atomic<std::int64_t> timeSamples { 0 };
    std::uint8_t bytes[4] = {};
    int numBytes = 0;
};
} // namespace daw
