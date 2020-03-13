#pragma once

#include <math.h>
#include <array>

typedef std::array<float, 1> Vec1;
typedef std::array<float, 2> Vec2;
typedef std::array<float, 3> Vec3;
typedef std::array<float, 4> Vec4;

template <typename T>
T Clamp(T value, T minValue, T maxValue)
{
    if (value <= minValue)
        return minValue;
    else if (value >= maxValue)
        return maxValue;
    else
        return value;
}

inline float Lerp(float A, float B, float t)
{
    return A * (1.0f - t) + B * t;
}

inline double Lerp(double A, double B, double t)
{
    return A * (1.0 - t) + B * t;
}

inline float SmoothStep(float value, float min, float max)
{
    float x = (value - min) / (max - min);
    x = std::min(x, 1.0f);
    x = std::max(x, 0.0f);

    return 3.0f * x * x - 2.0f * x * x * x;
}

inline float LinearTosRGB(float x)
{
    return powf(x, 1.0f / 2.2f);
}

inline float Fract(float x)
{
    return x - floor(x);
}