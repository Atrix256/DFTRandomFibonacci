#include "ImageData.h"
#include "math.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

RGBA AlphaBlend(const RGBA& A, const RGBA& B, float alpha)
{
    // alpha blends using premultiplied alpha for correct results
    Vec4 v4a = {
        float(A.R) / 255.0f,
        float(A.G) / 255.0f,
        float(A.B) / 255.0f,
        float(A.A) / 255.0f
    };
    v4a[0] *= v4a[3];
    v4a[1] *= v4a[3];
    v4a[2] *= v4a[3];

    Vec4 v4b = {
        float(B.R) / 255.0f,
        float(B.G) / 255.0f,
        float(B.B) / 255.0f,
        float(B.A) / 255.0f
    };
    v4b[0] *= v4b[3];
    v4b[1] *= v4b[3];
    v4b[2] *= v4b[3];

    Vec4 blended;
    for (int index = 0; index < 4; ++index)
        blended[index] = Lerp(v4a[index], v4b[index], alpha);

    if (blended[3] > 0.0f)
    {
        blended[0] /= blended[3];
        blended[1] /= blended[3];
        blended[2] /= blended[3];
    }
    else
    {
        blended = Vec4{ 0.0f, 0.0f, 0.0f, 0.0f };
    }

    RGBA ret;
    ret.R = (uint8)Clamp(blended[0] * 256.0f, 0.0f, 255.0f);
    ret.G = (uint8)Clamp(blended[1] * 256.0f, 0.0f, 255.0f);
    ret.B = (uint8)Clamp(blended[2] * 256.0f, 0.0f, 255.0f);
    ret.A = (uint8)Clamp(blended[3] * 256.0f, 0.0f, 255.0f);
    return ret;
}

void SImageData::DrawLine(int x1, int y1, int x2, int y2, const RGBA& color)
{
    // pad the AABB of pixels we scan, to account for anti aliasing
    int startX = std::max(std::min(x1, x2) - 4, 0);
    int startY = std::max(std::min(y1, y2) - 4, 0);
    int endX = std::min(std::max(x1, x2) + 4, (int)m_width - 1);
    int endY = std::min(std::max(y1, y2) + 4, (int)m_height - 1);

    // if (x1,y1) is A and (x2,y2) is B, get a normalized vector from A to B called AB
    float ABX = float(x2 - x1);
    float ABY = float(y2 - y1);
    float ABLen = (float)sqrt(ABX * ABX + ABY * ABY);
    ABX /= ABLen;
    ABY /= ABLen;

    // scan the AABB of our line segment, drawing pixels for the line, as is appropriate
    for (int iy = startY; iy <= endY; ++iy)
    {
        for (int ix = startX; ix <= endX; ++ix)
        {
            RGBA& pixel = m_pixels[iy * m_width + ix];

            // project this current pixel onto the line segment to get the closest point on the line segment to the point
            float ACX = float(ix - x1);
            float ACY = float(iy - y1);
            float lineSegmentT = ACX * ABX + ACY * ABY;
            lineSegmentT = std::min(lineSegmentT, ABLen);
            lineSegmentT = std::max(lineSegmentT, 0.0f);
            float closestX = float(x1) + lineSegmentT * ABX;
            float closestY = float(y1) + lineSegmentT * ABY;

            // calculate the distance from this pixel to the closest point on the line segment
            float distanceX = float(ix) - closestX;
            float distanceY = float(iy) - closestY;
            float distance = (float)sqrt(distanceX * distanceX + distanceY * distanceY);

            // use the distance to figure out how transparent the pixel should be, and apply the color to the pixel
            float alpha = SmoothStep(distance, 2.0f, 0.0f);

            if (alpha > 0.0f)
            {
                pixel = AlphaBlend(pixel, color, alpha);
            }
        }
    }
}

void SImageData::Save(const char* fileName)
{
    stbi_write_png(fileName, int(m_width), int(m_height), 4, m_pixels.data(), int(m_width * 4));
}

void SImageData::AppendHorizontal(const SImageData& image_, bool allowResize)
{
    // if this image is empty, just copy the other image
    if (m_width == 0 && m_height == 0)
    {
        *this = image_;
        return;
    }

    // must be same height
    SImageData image = image_;
    if (image.m_height != m_height)
    {
        if (!allowResize)
        {
            printf("AppendHorizontal() image height mismatch! %zu vs %zu.\n", image.m_height, m_height);
            return;
        }
        else if (m_height < image.m_height)
        {
            SImageData pad;
            pad.Resize(m_width, image.m_height - m_height);
            AppendVertical(pad);
        }
        else
        {
            SImageData pad;
            pad.Resize(image.m_width, m_height - image.m_height);
            image.AppendVertical(pad);
        }

        // TODO: remove! should be an assert
        if (image.m_height != m_height)
        {
            printf("FAIL AppendHorizontal() image height mismatch! %zu vs %zu.\n", image.m_height, m_height);
            return;
        }
    }

    SImageData result;
    result.Resize(m_width + image.m_width, m_height);

    const RGBA* sourceLeft = m_pixels.data();
    const RGBA* sourceRight = image.m_pixels.data();

    size_t strideLeft = m_width;
    size_t strideRight = image.m_width;

    RGBA* dest = result.m_pixels.data();

    for (size_t y = 0; y < m_height; ++y)
    {
        memcpy(dest, sourceLeft, strideLeft * 4);
        dest += strideLeft;
        sourceLeft += strideLeft;

        memcpy(dest, sourceRight, strideRight * 4);
        dest += strideRight;
        sourceRight += strideRight;
    }
    
    *this = result;
}

void SImageData::AppendVertical(const SImageData& image_, bool allowResize)
{
    // if this image is empty, just copy the other image
    if (m_width == 0 && m_height == 0)
    {
        *this = image_;
        return;
    }

    // must be same width
    SImageData image = image_;
    if (image.m_width != m_width)
    {
        if (!allowResize)
        {
            printf("AppendVertical() image width mismatch! %zu vs %zu.\n", image.m_width, m_width);
            return;
        }
        else if (m_width < image.m_width)
        {
            SImageData pad;
            pad.Resize(image.m_width - m_width, m_height);
            AppendHorizontal(pad);
        }
        else
        {
            SImageData pad;
            pad.Resize(m_width - image.m_width, image.m_height);
            image.AppendHorizontal(pad);
        }

        // TODO: remove! should be an assert
        if (image.m_width != m_width)
        {
            printf("FAIL AppendHorizontal() image height mismatch! %zu vs %zu.\n", image.m_height, m_height);
            return;
        }
    }

    SImageData result;
    result.Resize(m_width, m_height + image.m_height);
    memcpy(result.m_pixels.data(), m_pixels.data(), m_width * m_height * 4);
    memcpy(&result.m_pixels[m_width * m_height], image.m_pixels.data(), image.m_width * image.m_height * 4);
    *this = result;
}