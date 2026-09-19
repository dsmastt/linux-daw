#pragma once

#include <cmath>

namespace daw
{
enum class DistortionMode : int
{
    soft = 0,
    hard = 1,
    cubic = 2,
    asymmetric = 3,
};

namespace shapers
{
inline float softClip (float x) noexcept
{
    return std::tanh (x);
}

inline float hardClip (float x) noexcept
{
    return x < -1.0f ? -1.0f : (x > 1.0f ? 1.0f : x);
}

inline float cubicClip (float x) noexcept
{
    return x - (x * x * x) / 3.0f;
}

inline float asymmetricClip (float x) noexcept
{
    return std::tanh (x) * (x >= 0.0f ? 1.0f : 0.6f);
}
} // namespace shapers
} // namespace daw
