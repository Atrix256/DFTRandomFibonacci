#include <random>
#include <vector>

#include "dft.h"
#include "ImageData.h"

typedef int32_t int32;
typedef uint32_t uint32;

#define DETERMINISTIC() 1

static const size_t c_DFTBucketCount = 1024;
static const size_t c_numTests = 1000;

static const size_t c_DFTImageWidth = 1024;
static const size_t c_DFTImageHeight = 256;


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

void SaveDFT1D(const std::vector<double>& dftData, size_t imageWidth, size_t imageHeight, const char* fileName)
{
    // get the maximum magnitude so we can normalize the DFT values
    double maxMagnitude = GetMaxMagnitudeDFT(dftData);
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
        int y = int(double(i + 1) * double(lineSpacing));
        image.Box(0, imageWidth - 1, y, y + 1, RGBA{ 192, 192, 192, 255 });
    }
    for (size_t i = 0; i < numCols - 1; ++i)
    {
        int x = int(double(i + 1) * double(lineSpacing));
        image.Box(x, x + 1, 0, imageHeight - 1, RGBA{ 192, 192, 192, 255 });
    }

    // draw the graph
    int lastX, lastY;
    //int lastStdDevY;
    for (size_t index = 0; index < dftData.size(); ++index)
    {
        size_t pixelX = index * imageWidth / dftData.size();
        double f = dftData[index] / maxMagnitude;
        double pixelY = double(imageHeight) - f * double(imageHeight);

        /*
        if (stdDevDFTs)
        {
            double stdDev = stdDevDFTs->imageDFT[index] / maxMagnitude;
            int stdDevY = int(stdDev * double(imageHeight));

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

void CalculateDFT1D(const std::vector<double>& values, size_t bucketCount, std::vector<double>& valuesDFTMag)
{
    // make an image of the samples
    std::vector<double> sampleImage(bucketCount, 0.0f);
    for (const double value : values)
    {
        size_t x = (size_t)Clamp(value * double(bucketCount), 0.0, double(bucketCount - 1));
        sampleImage[x] = 1.0f;
    }

    // DFT the image
    DFT1D(sampleImage, valuesDFTMag);
}

template <typename LAMBDA>
void DoTest(const char* name, size_t numTests, size_t numValues, const LAMBDA& lambda)
{
    printf("%s...\n", name);
    std::vector<double> averageDFT;
    for (size_t testIndex = 0; testIndex < numTests; ++testIndex)
    {
        std::vector<int32> values;
        lambda(values, numValues, testIndex);

        int32 min = values[0];
        int32 max = values[0];
        for (int32 value : values)
        {
            min = std::min(min, value);
            max = std::max(max, value);
        }

        std::vector<double> valuesdouble(values.size());
        for (size_t index = 0; index < values.size(); ++index)
            valuesdouble[index] = double(double(values[index] - min) / double(max - min));

        std::vector<double> valuesDFT;
        CalculateDFT1D(valuesdouble, c_DFTBucketCount, valuesDFT);

        if (averageDFT.size() == 0)
            averageDFT.resize(valuesDFT.size(), 0.0f);

        for (size_t index = 0; index < valuesDFT.size(); ++index)
            averageDFT[index] = Lerp(averageDFT[index], valuesDFT[index], 1.0 / double(testIndex + 1));

        double damin = averageDFT[0];
        double damax = averageDFT[0];
        for (double value : averageDFT)
        {
            damin = std::min(damin, value);
            damax = std::max(damax, value);
        }


        char filename[1024];
        if (testIndex == 0)
        {
            sprintf_s(filename, "out/%s.png", name);
            SaveDFT1D(averageDFT, c_DFTImageWidth, c_DFTImageHeight, filename);
        }
        else if (testIndex == c_numTests - 1)
        {
            sprintf_s(filename, "out/%s.avg.png", name);
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

bool IsPrime(int32 value)
{
    if (value < 2)
        return false;

    if (value <= 3)
        return true;

    int32 maxCheck = ((int32)sqrt((double)value));
    for (int32 i = 2; i <= maxCheck; ++i)
    {
        if ((value % i) == 0)
            return false;
    }
    return true;
}

void Primes(std::vector<int32>& values, size_t numValues)
{
    int32 value = 1;
    while (values.size() < numValues)
    {
        if (IsPrime(value))
            values.push_back(value);
        value++;
    }
}

void UniformWhiteNoise(std::vector<int32>& values, size_t numValues, size_t rngIndex)
{
    std::mt19937 rng = GetRNG((uint32)rngIndex);
    std::uniform_int_distribution<int32> dist;

    values.resize(numValues);
    for (int32& value : values)
        value = dist(rng);
}

int main(int argc, char** argv)
{
    // test RandomFibonacci
    DoTest("RandomFibonacci", c_numTests, 100,
        [] (std::vector<int32>& values, size_t numValues, size_t testIndex)
        {
            RandomFibonacci(values, numValues, testIndex);
        }
    );

    // test UniformWhite
    DoTest("UniformWhite", c_numTests, 100,
        [](std::vector<int32>& values, size_t numValues, size_t testIndex)
        {
            UniformWhiteNoise(values, numValues, testIndex);
        }
    );

    // test Primes
    DoTest("Primes", 1, 200,
        [](std::vector<int32>& values, size_t numValues, size_t testIndex)
        {
            Primes(values, numValues);
        }
    );

    return 0;
}

// TODO: clean up. like the stddev stuff if you don't need it
// TODO: variance too?
// Doing more than 100 values makes random fib do a weird thing. maybe switch to doubles?
// TODO: show points on a numberline
