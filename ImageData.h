#pragma once

#include <algorithm>
#include <vector>
#include <stdint.h>

using uint8 = uint8_t;

struct RGBA
{
    uint8 R, G, B, A;
};

struct SImageData
{
    size_t m_width = 0;
    size_t m_height = 0;
    std::vector<RGBA> m_pixels;

    void Resize(size_t width, size_t height, RGBA fill = RGBA{ 255, 255, 255, 255 })
    {
        m_width = width;
        m_height = height;
        m_pixels.resize(width * height, fill);
    }

    void Fill(const RGBA& color)
    {
        std::fill(m_pixels.begin(), m_pixels.end(), color);
    }

    void Box(size_t x1, size_t x2, size_t y1, size_t y2, const RGBA& color)
    {
        for (size_t y = y1; y < y2; ++y)
        {
            RGBA* start = &m_pixels[y * m_width + x1];
            std::fill(start, start + x2 - x1, color);
        }
    }

    void DrawLine(int x1, int y1, int x2, int y2, const RGBA& color);

    void Save(const char* fileName);

    void AppendHorizontal(const SImageData& image, bool allowResize = false);
    void AppendVertical(const SImageData& image, bool allowResize = false);
};
