#pragma once
class ToneGenerator
{
public:
	ToneGenerator();
	static void GenerateSineWave(float* data, float Hz, float Volume, float durationInSeconds, int samplesPerSecond, float start, int channelCount);
	void GenerateSineWave(float* data, float Hz, int size, int samplesPerSecond, float start)const;
private:
	~ToneGenerator();
};

