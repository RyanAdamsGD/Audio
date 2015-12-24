#include "ToneGenerator.h"
#include <math.h>


ToneGenerator::ToneGenerator()
{
}


ToneGenerator::~ToneGenerator()
{
}

void ToneGenerator::GenerateSineWave(float* data, float Hz, float Volume, float durationInSeconds, int samplesPerSecond, float start)
{
	float startx = asin(start) / Hz;

	int numberOfSamples = durationInSeconds * samplesPerSecond;
	float incrementPerSample = durationInSeconds / samplesPerSecond;
	for (size_t i = 0; i < numberOfSamples; i++)
	{
		data[i] = sin(Hz * (startx + (incrementPerSample * i))) * Volume;
	}
}

void ToneGenerator::GenerateSineWave(float* data, float Hz, int size, int samplesPerSecond, float start)const
{

}