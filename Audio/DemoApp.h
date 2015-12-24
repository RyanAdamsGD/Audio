#pragma once
#include "stdAfx.h"
#include "WinAudioCapture.h"
#include "WinAudioRenderer.h"
#include <thread>
#include <dwrite.h>
#include <vector>
#pragma comment(lib,"d2d1")
#pragma comment(lib, "Dwrite")

class DemoApp
{
public:
	DemoApp();
	~DemoApp();

	// Register the window class and call methods for instantiating drawing resources
	HRESULT Initialize();

	// Process and dispatch messages
	void RunMessageLoop();

	void Update();
	bool finished;
	std::thread updateLoop;
	bool StartCapture(int TargetDurationInSec, int TargetLatency);
	void StopCapture();
	void StopAudioRender();
	void CreateRenderQueue(int BufferDurationInSeconds);

private:
	// Initialize device-independent resources.
	HRESULT CreateDeviceIndependentResources();

	// Initialize device-dependent resources.
	HRESULT CreateDeviceResources();

	// Release device-dependent resource.
	void DiscardDeviceResources();
	
	IMMDevice* GetDefaultCaptureDevice();
	IMMDevice* GetDefaultRenderDevice();
	void RenderWaveData(float* data, int size, int channelCount);
	void DrawPoints(D2D1_POINT_2F** const data, int size, int channelCount);
	void Start();
	//only call this after calling draw points
	//because I'm too laze to extract it all
	void DrawString(int x, int y,const std::wstring& text);
	void FindFrequencyInHerz(float* data, int size, float sampleDurationInSeconds);

	// Resize the render target.
	void OnResize(
		UINT width,
		UINT height
		);

	// The windows procedure.
	static LRESULT CALLBACK WndProc(
		HWND hWnd,
		UINT message,
		WPARAM wParam,
		LPARAM lParam
		);

	//d2d
	HWND m_hwnd;
	ID2D1Factory* m_pDirect2dFactory;
	ID2D1HwndRenderTarget* m_pRenderTarget;
	ID2D1SolidColorBrush* m_pLightSlateGrayBrush;
	ID2D1SolidColorBrush* m_pCornflowerBlueBrush;
	IDWriteFactory* m_pDWriteFactory;
	IDWriteTextFormat* m_pTextFormat;
	struct DrawnString
	{
		float x, y;
		const std::wstring text;
		DrawnString()
		:text(std::to_wstring(0)){}
		DrawnString(float x, float y, const std::wstring& text)
			:x(x), y(y), text(text){}
	};
	std::vector<DrawnString> stringsToRender;

	//rendering
	WinAudioRenderer* renderer;
	RenderBuffer* renderQueue;
	RenderBuffer* currentRenderBufferBeingWrittenTo;
	BYTE* currentRenderBufferPosition;
	size_t previousCaptureBufferSize;
	//copies the data from the capturer into the render buffer
	void WriteCaptureBufferToRenderBuffer();

	//capturing
	WinAudioCapture* capturer;

	BYTE *captureBuffer;
	size_t captureBufferSize;
	D2D1_SIZE_U windowSize;
	//has the capturer start writing over the old data
	void ResetCaptureBuffer();
};

