#include "ToneGenerator.h"
#include <math.h>


ToneGenerator::ToneGenerator()
{
}


ToneGenerator::~ToneGenerator()
{
}

void ToneGenerator::GenerateSineWave(float* data, float Hz, float Volume, float durationInSeconds, int samplesPerSecond, float start, int channelCount)
{
	float startx = asin(start) / Hz;

	int numberOfSamples = durationInSeconds * samplesPerSecond;
	float incrementPerSample = durationInSeconds / samplesPerSecond;
	float value;
	for (size_t i = 0; i < numberOfSamples; i++)
	{
		value = sin(Hz * (startx + (incrementPerSample * i))) * Volume;
		for (size_t j = 0; j < channelCount; j++)
		{
			data[(i * channelCount) + j] = value;
		}
	}
}

void ToneGenerator::GenerateSineWave(float* data, float Hz, int size, int samplesPerSecond, float start)const
{

}