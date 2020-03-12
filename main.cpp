#include <random>
#include <vector>

#include "dft.h"
#include "ImageData.h"

typedef int32_t int32;
typedef uint32_t uint32;

#define DETERMINISTIC() 1

static const size_t c_DFTBucketCount = 256;
static const size_t c_numValues = 100;
static const size_t c_numTests = 10000;

static const size_t c_DFTImageWidth = 256;
static const size_t c_DFTImageHeight = 64;


std::mt19937 GetRNG(uint32 index)
{
#if DETERMINISTIC()
    std::seed_seq seq({ index, (unsigned int)0x65cd8674, (unsigned int)0x7952426c, (unsigned int)0x2a816f2c, (unsigned int)0x689dbc5f, (unsigned int)0xe138d1e5, (unsigned int)0x91da7241, (unsigned int)0x57f2d0e0, (unsigned int)0xed41c211 });
    std::mt19937 rng(seq);
#else
    std::random_device rd;
    std::mt19937 rng(rd());
#endif
    return rng;
}

void SaveDFT1D(const std::vector<float>& dftData, size_t imageWidth, size_t imageHeight, const char* fileName)
{
    // get the maximum magnitude so we can normalize the DFT values
    float maxMagnitude = GetMaxMagnitudeDFT(dftData);
    //if (stdDevDFTs)
        //maxMagnitude += GetMaxMagnitudeDFT(stdDevDFTs->imageDFT);

    // size and clear the image
    SImageData image;
    image.Resize(imageWidth, imageHeight);
    image.Fill(RGBA{ 255, 255, 255, 255 });

    // draw a dim background grid
    size_t lineSpacing = imageHeight / 8;
    size_t numRows = imageHeight / lineSpacing;
    size_t numCols = imageWidth / lineSpacing;
    for (size_t i = 0; i < numRows - 1; ++i)
    {
        int y = int(float(i + 1) * float(lineSpacing));
        image.Box(0, imageWidth - 1, y, y + 1, RGBA{ 192, 192, 192, 255 });
    }
    for (size_t i = 0; i < numCols - 1; ++i)
    {
        int x = int(float(i + 1) * float(lineSpacing));
        image.Box(x, x + 1, 0, imageHeight - 1, RGBA{ 192, 192, 192, 255 });
    }

    // draw the graph
    int lastX, lastY;
    //int lastStdDevY;
    for (size_t index = 0; index < dftData.size(); ++index)
    {
        size_t pixelX = index * imageWidth / dftData.size();
        float f = dftData[index] / maxMagnitude;
        float pixelY = float(imageHeight) - f * float(imageHeight);

        /*
        if (stdDevDFTs)
        {
            float stdDev = stdDevDFTs->imageDFT[index] / maxMagnitude;
            int stdDevY = int(stdDev * float(imageHeight));

            if (index > 0)
            {
                image.DrawLine(lastX, lastY - lastStdDevY, int(pixelX), int(pixelY) - stdDevY, RGBA{ 128, 128, 128, 255 });
                image.DrawLine(lastX, lastY + lastStdDevY, int(pixelX), int(pixelY) + stdDevY, RGBA{ 128, 128, 128, 255 });
            }

            lastStdDevY = stdDevY;
        }
        */

        if (index > 0)
            image.DrawLine(lastX, lastY, int(pixelX), int(pixelY), RGBA{ 64, 64, 64, 255 });
        lastX = int(pixelX);
        lastY = int(pixelY);
    }

    // save the image
    image.Save(fileName);
}

void CalculateDFT1D(const std::vector<float>& values, size_t bucketCount, std::vector<float>& valuesDFTMag)
{
    // make an image of the samples
    std::vector<float> sampleImage(bucketCount, 0.0f);
    for (const float value : values)
    {
        size_t x = (size_t)Clamp(value * float(bucketCount), 0.0f, float(bucketCount - 1));
        sampleImage[x] = 1.0f;
    }

    // DFT the image
    DFT1D(sampleImage, valuesDFTMag);
}

template <typename LAMBDA>
void DoTest(const char* name, const LAMBDA& lambda)
{
    std::vector<float> averageDFT;

    for (size_t testIndex = 0; testIndex < c_numTests; ++testIndex)
    {
        std::vector<int32> values;
        lambda(values, c_numValues, testIndex);

        int32 min = values[0];
        int32 max = values[0];
        for (int32 value : values)
        {
            min = std::min(min, value);
            max = std::max(max, value);
        }

        std::vector<float> valuesFloat(values.size());
        for (size_t index = 0; index < values.size(); ++index)
            valuesFloat[index] = float(double(values[index] - min) / double(max - min));

        std::vector<float> valuesDFT;
        CalculateDFT1D(valuesFloat, c_DFTBucketCount, valuesDFT);

        if (averageDFT.size() == 0)
            averageDFT.resize(valuesDFT.size(), 0.0f);

        for (size_t index = 0; index < valuesDFT.size(); ++index)
            averageDFT[index] = Lerp(averageDFT[index], valuesDFT[index], 1.0f / float(testIndex + 1));

        char filename[1024];
        if (testIndex == 0)
        {
            sprintf_s(filename, "%s.png", name);
            SaveDFT1D(averageDFT, c_DFTImageWidth, c_DFTImageHeight, filename);
        }
        else if (testIndex == c_numTests - 1)
        {
            sprintf_s(filename, "%s.avg.png", name);
            SaveDFT1D(averageDFT, c_DFTImageWidth, c_DFTImageHeight, filename);
        }
    }
}

void RandomFibonacci(std::vector<int32>& values, size_t numValues, size_t rngIndex)
{
    values.resize(numValues);

    values[0] = 1;
    values[1] = 1;

    std::mt19937 rng = GetRNG((uint32)rngIndex);
    std::uniform_int_distribution<uint32> dist;

    uint32 rngValueBitsLeft = 0;
    uint32 rngValue = 0;

    for (size_t index = 2; index < numValues; ++index)
    {
        if (rngValueBitsLeft == 0)
        {
            rngValue = dist(rng);
            rngValueBitsLeft = 32;
        }
        bool randomBit = rngValue & 1;
        rngValue >>= 1;
        rngValueBitsLeft--;

        if (randomBit)
            values[index] = values[index - 2] + values[index - 1];
        else
            values[index] = values[index - 2] - values[index - 1];
    }

}

void Primes(std::vector<int32>& values, size_t numValues)
{

}

void UniformWhiteNoise(std::vector<int32>& values, size_t numValues, size_t rngIndex)
{
    std::mt19937 rng = GetRNG(0);
    std::uniform_int_distribution<int32> dist;

    values.resize(numValues);
    for (int32& value : values)
        value = dist(rng);
}

int main(int argc, char** argv)
{
    // test RandomFibonacci
    DoTest("RandomFibonacci",
        [] (std::vector<int32>& values, size_t numValues, size_t testIndex)
        {
            RandomFibonacci(values, numValues, testIndex);
        }
    );

    int ijkl = 0;

    // test UniformWhite
    DoTest("UniformWhite",
        [](std::vector<int32>& values, size_t numValues, size_t testIndex)
        {
            UniformWhiteNoise(values, numValues, testIndex);
        }
    );

    return 0;
}

// TODO: primes. Any other sequence? maybe white noise and blue noise just to show some stuff? maybe also regular fibonacci (vs golden ratio or??)?
// TODO: show an individual DFT as well as an average (average the un-normalized DFTs!) of several
// TODO: clean up. like the stddev stuff if you don't need it
