#pragma once

#include <atomic>
#include <cstdint>

namespace daw
{
constexpr int maxClipsPerTrack = 16;

struct Clip
{
    std::atomic<std::int64_t> startSamples { 0 };
    std::atomic<std::int64_t> lengthSamples { 0 };
    std::atomic<std::int64_t> sourceOffsetSamples { 0 };
};

struct ClipSource
{
    const float* data = nullptr;
    std::int64_t frames = 0;
    int channels = 0;
};
} // namespace daw
