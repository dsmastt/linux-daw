#pragma once

#include <atomic>
#include <cstdint>

namespace daw
{
constexpr int maxAutomationPointsPerCurve = 256;
constexpr int maxAutomationCurvesPerTrack = 8;

enum class AutomationParameter
{
    trackVolumeDb,
    trackPan,
    gainDb,
    pluginParameter,
};

struct AutomationPoint
{
    std::atomic<std::int64_t> timeSamples { 0 };
    std::atomic<float> value { 0.0f };
};

struct AutomationCurve
{
    AutomationParameter parameter = AutomationParameter::trackVolumeDb;
    int pluginParameterIndex = -1;
    std::atomic<int> pointCount { 0 };
    AutomationPoint points[maxAutomationPointsPerCurve];
};
} // namespace daw
