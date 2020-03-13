#pragma once

#include "simple_fft/fft_settings.h"
#include "simple_fft/fft.h"

#include <algorithm>
#include <vector>

#include "math.h"

struct ComplexImage1D
{
    ComplexImage1D(size_t w)
    {
        m_width = w;
        pixels.resize(w, real_type(0.0f));
    }

    size_t m_width;
    std::vector<complex_type> pixels;

    complex_type& operator()(size_t x)
    {
        return pixels[x];
    }

    const complex_type& operator()(size_t x) const
    {
        return pixels[x];
    }
};

double GetMaxMagnitudeDFT(const std::vector<double>& imageSrc)
{
    double maxMag = 0.0f;
    for (double f : imageSrc)
        maxMag = std::max(f, maxMag);
    return maxMag;
}

void DFT1D(const std::vector<double>& imageSrc, std::vector<double>& magnitudes)
{
    // convert the source image to double and store it as complex so it can be DFTd
    size_t width = imageSrc.size();
    ComplexImage1D complexImageIn(width);
    for (size_t index = 0, count = width; index < count; ++index)
        complexImageIn.pixels[index] = imageSrc[index];

    // DFT the image to get frequency of the samples
    const char* error = nullptr;
    ComplexImage1D complexImageOut(width);
    simple_fft::FFT(complexImageIn, complexImageOut, width, error);

    // Zero out DC, we don't really care about it, and the value is huge.
    complexImageOut(0) = 0.0f;

    // get the magnitudes
    {
        magnitudes.resize(width, 0.0f);
        double* dest = magnitudes.data();
        for (size_t x = 0; x < width; ++x)
        {
            size_t srcX = (x + width / 2) % width;

            const complex_type& c = complexImageOut(srcX);
            double mag = double(sqrt(c.real()*c.real() + c.imag()*c.imag()));
            *dest = mag;
            ++dest;
        }
    }
}
