#pragma once
class ToneGenerator
{
public:
	ToneGenerator();
	void GenerateSineWave(float* data, float Hz, float durationInSeconds, int samplesPerSecond, float start)const;
	void GenerateSineWave(float* data, float Hz, int size, int samplesPerSecond, float start)const;
private:
	~ToneGenerator();
};

