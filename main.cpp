#include <random>
#include <vector>

#include "dft.h"
#include "ImageData.h"

typedef int64_t int64;

// --------------------- DFT Tests

#define DETERMINISTIC() 1

static const size_t c_DFTBucketCount = 2048;
static const size_t c_numTests = 100000;

static const size_t c_DFTImageWidth = 512;
static const size_t c_DFTImageHeight = 128;

#define IMAGE1D_WIDTH 600
#define IMAGE1D_HEIGHT 50
#define IMAGE_PAD   30
#define IMAGE1D_CENTERX ((IMAGE1D_WIDTH+IMAGE_PAD*2)/2)
#define IMAGE1D_CENTERY ((IMAGE1D_HEIGHT+IMAGE_PAD*2)/2)
#define AXIS_HEIGHT 40
#define DATA_HEIGHT 20

// --------------------- Coin Toss Tests

static const size_t c_numCoinTossTests = 10000;  // Do the test this many times
static const size_t c_numHeadsRequired = 10;    // flip a coin with this many heads, then see how often the next number is heads vs tails

// ---------------------

RGBA DataPointColor(size_t sampleIndex, size_t totalSamples)
{
    RGBA ret;
    float percent = (float(sampleIndex) / (float(totalSamples) - 1.0f));

    float r = 1.0f - percent;
    float g = 0;
    float b = percent;

    float mag = (float)sqrt(r * r + g * g + b * b);
    r /= mag;
    g /= mag;
    b /= mag;

    ret.R = (uint8)Clamp(r * 256.0f, 0.0f, 255.0f);
    ret.G = (uint8)Clamp(g * 256.0f, 0.0f, 255.0f);
    ret.B = (uint8)Clamp(b * 256.0f, 0.0f, 255.0f);
    ret.A = 255;

    return ret;
}

std::mt19937 GetRNG(size_t index)
{
#if DETERMINISTIC()
    std::seed_seq seq({ (uint32_t)index, (unsigned int)0x65cd8674, (unsigned int)0x7952426c, (unsigned int)0x2a816f2c, (unsigned int)0x689dbc5f, (unsigned int)0xe138d1e5, (unsigned int)0x91da7241, (unsigned int)0x57f2d0e0, (unsigned int)0xed41c211 });
    std::mt19937 rng(seq);
#else
    std::random_device rd;
    std::mt19937 rng(rd());
#endif
    return rng;
}

void SaveSamples1D(const std::vector<double>& points, const char* fileName)
{
    // size and clear the images
    SImageData image;
    image.Resize(IMAGE1D_WIDTH + IMAGE_PAD * 2, IMAGE1D_HEIGHT + IMAGE_PAD * 2);
    image.Fill(RGBA{ 255, 255, 255, 255 });

    // draw the points
    for (size_t index = 0; index < points.size(); ++index)
    {
        size_t pos = size_t(points[index] * float(IMAGE1D_WIDTH)) + IMAGE_PAD;
        RGBA color = DataPointColor(index, points.size());
        image.Box(pos, pos + 1, IMAGE1D_CENTERY - DATA_HEIGHT / 2, IMAGE1D_CENTERY + DATA_HEIGHT / 2, color);
    }

    // draw the axes lines. horizontal first then the two vertical
    image.Box(IMAGE_PAD, IMAGE1D_WIDTH + IMAGE_PAD, IMAGE1D_CENTERY, IMAGE1D_CENTERY + 1, RGBA{ 0, 0, 0, 255 });
    image.Box(IMAGE_PAD, IMAGE_PAD + 1, IMAGE1D_CENTERY - AXIS_HEIGHT / 2, IMAGE1D_CENTERY + AXIS_HEIGHT / 2, RGBA{ 0, 0, 0, 255 });
    image.Box(IMAGE1D_WIDTH + IMAGE_PAD, IMAGE1D_WIDTH + IMAGE_PAD + 1, IMAGE1D_CENTERY - AXIS_HEIGHT / 2, IMAGE1D_CENTERY + AXIS_HEIGHT / 2, RGBA{ 0, 0, 0, 255 });

    // save the image
    image.Save(fileName);
}

void SaveDFT1D(const std::vector<double>& dftData, const std::vector<double>& dftStdDevData, size_t imageWidth, size_t imageHeight, const char* fileName, bool showStdDev)
{
    // get the maximum magnitude so we can normalize the DFT values
    double maxMagnitude = GetMaxMagnitudeDFT(dftData);
    if (showStdDev)
        maxMagnitude += GetMaxMagnitudeDFT(dftStdDevData);

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
    int lastStdDevY;
    for (size_t index = 0; index < dftData.size(); ++index)
    {
        size_t pixelX = index * imageWidth / dftData.size();
        double f = dftData[index] / maxMagnitude;
        double pixelY = double(imageHeight) - f * double(imageHeight);

        if (showStdDev)
        {
            double stdDev = dftStdDevData[index] / maxMagnitude;
            int stdDevY = int(stdDev * double(imageHeight));

            if (index > 0)
            {
                image.DrawLine(lastX, lastY - lastStdDevY, int(pixelX), int(pixelY) - stdDevY, RGBA{ 128, 128, 128, 255 });
                image.DrawLine(lastX, lastY + lastStdDevY, int(pixelX), int(pixelY) + stdDevY, RGBA{ 128, 128, 128, 255 });
            }

            lastStdDevY = stdDevY;
        }

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
    std::vector<double> averageDFTSquared;
    std::vector<double> averageDFTStdDev;
    for (size_t testIndex = 0; testIndex < numTests; ++testIndex)
    {
        std::vector<int64> values;
        lambda(values, numValues, testIndex);

        int64 min = values[0];
        int64 max = values[0];
        for (int64 value : values)
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
        {
            averageDFT.resize(valuesDFT.size(), 0.0f);
            averageDFTSquared.resize(valuesDFT.size(), 0.0f);
            averageDFTStdDev.resize(valuesDFT.size(), 0.0f);
        }

        for (size_t index = 0; index < valuesDFT.size(); ++index)
        {
            averageDFT[index] = Lerp(averageDFT[index], valuesDFT[index], 1.0 / double(testIndex + 1));
            averageDFTSquared[index] = Lerp(averageDFTSquared[index], valuesDFT[index] * valuesDFT[index], 1.0 / double(testIndex + 1));
        }

        char filename[1024];
        if (testIndex == 0)
        {
            sprintf_s(filename, "out/%s.dft.png", name);
            SaveDFT1D(averageDFT, averageDFTStdDev, c_DFTImageWidth, c_DFTImageHeight, filename, false);

            sprintf_s(filename, "out/%s.png", name);
            SaveSamples1D(valuesdouble, filename);
        }
        else if (testIndex == c_numTests - 1)
        {
            for (size_t index = 0; index < valuesDFT.size(); ++index)
                averageDFTStdDev[index] = sqrt(abs(averageDFTSquared[index] - averageDFT[index] * averageDFT[index]));

            sprintf_s(filename, "out/%s.dftavg.png", name);
            SaveDFT1D(averageDFT, averageDFTStdDev, c_DFTImageWidth, c_DFTImageHeight, filename, true);
        }
    }
}

void RandomFibonacci(std::vector<int64>& values, size_t numValues, size_t rngIndex)
{
    values.resize(numValues);

    values[0] = 1;
    values[1] = 1;

    std::mt19937 rng = GetRNG(rngIndex);
    std::uniform_int_distribution<uint32_t> dist;

    uint32_t rngValueBitsLeft = 0;
    uint32_t rngValue = 0;

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

void Fibonacci(std::vector<int64>& values, size_t numValues)
{
    values.resize(numValues);

    values[0] = 1;
    values[1] = 1;

    for (size_t index = 2; index < numValues; ++index)
        values[index] = values[index - 2] + values[index - 1];
}

bool IsPrime(int64 value)
{
    if (value < 2)
        return false;

    if (value <= 3)
        return true;

    int64 maxCheck = ((int64)sqrt((double)value));
    for (int64 i = 2; i <= maxCheck; ++i)
    {
        if ((value % i) == 0)
            return false;
    }
    return true;
}

void Primes(std::vector<int64>& values, size_t numValues)
{
    int64 value = 1;
    while (values.size() < numValues)
    {
        if (IsPrime(value))
            values.push_back(value);
        value++;
    }
}

void UniformWhiteNoise(std::vector<int64>& values, size_t numValues, size_t rngIndex)
{
    std::mt19937 rng = GetRNG(rngIndex);
    std::uniform_int_distribution<int64> dist;

    values.resize(numValues);
    for (int64& value : values)
        value = dist(rng);
}

// flips a coin until it gets <count> heads in a row, and then returns what the next coin flip is
bool FlipHeads(std::mt19937& rng, size_t count)
{
    std::uniform_int_distribution<uint32_t> dist;

    int headsCount = 0;
    uint32_t rngValueBitsLeft = 0;
    uint32_t rngValue = 0;
    while (1)
    {
        if (rngValueBitsLeft == 0)
        {
            rngValue = dist(rng);
            rngValueBitsLeft = 32;
        }
        bool randomBit = rngValue & 1;
        rngValue >>= 1;
        rngValueBitsLeft--;

        if (headsCount == 10)
        {
            return randomBit;
        }

        if (randomBit)
        {
            headsCount++;
        }
        else
        {
            headsCount = 0;
        }
    }

}

void DoCoinTossTest()
{
    std::random_device rd;
    std::mt19937 rng(rd());

    int headsCount = 0;

    for (size_t index = 0; index < c_numCoinTossTests; ++index)
    {
        if (FlipHeads(rng, c_numHeadsRequired))
            headsCount++;
    }

    float percent = 100.0f * float(headsCount) / float(c_numCoinTossTests);
    printf("%zu times flipping %zu heads in a row. The next value was heads %0.2f percent of the time.\n\n", c_numCoinTossTests, c_numHeadsRequired, percent);
}

int main(int argc, char** argv)
{
    DoCoinTossTest();
    DoCoinTossTest();
    DoCoinTossTest();
    DoCoinTossTest();
    DoCoinTossTest();

    // test RandomFibonacci
    DoTest("RandomFibonacci", c_numTests, 90,
        [] (std::vector<int64>& values, size_t numValues, size_t testIndex)
        {
            RandomFibonacci(values, numValues, testIndex);
        }
    );

    // test UniformWhite
    DoTest("UniformWhite", c_numTests, 100,
        [](std::vector<int64>& values, size_t numValues, size_t testIndex)
        {
            UniformWhiteNoise(values, numValues, testIndex);
        }
    );

    // test Primes
    DoTest("Primes25", 1, 25,
        [](std::vector<int64>& values, size_t numValues, size_t testIndex)
        {
            Primes(values, numValues);
        }
    );
    DoTest("Primes100", 1, 100,
        [](std::vector<int64>& values, size_t numValues, size_t testIndex)
        {
            Primes(values, numValues);
        }
    );
    DoTest("Primes200", 1, 200,
        [](std::vector<int64>& values, size_t numValues, size_t testIndex)
        {
            Primes(values, numValues);
        }
    );
    DoTest("Primes1000", 1, 1000,
        [](std::vector<int64>& values, size_t numValues, size_t testIndex)
        {
            Primes(values, numValues);
        }
    );

    // test regular fibonacci
    DoTest("Fibonacci", 1, 90,
        [](std::vector<int64>& values, size_t numValues, size_t testIndex)
        {
            Fibonacci(values, numValues);
        }
    );

    return 0;
}
