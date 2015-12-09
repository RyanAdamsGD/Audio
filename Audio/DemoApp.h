#pragma once
#include "stdAfx.h"
#include "WinAudioCapture.h"
#include <thread>
#include <dwrite.h>
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

private:
	// Initialize device-independent resources.
	HRESULT CreateDeviceIndependentResources();

	// Initialize device-dependent resources.
	HRESULT CreateDeviceResources();

	// Release device-dependent resource.
	void DiscardDeviceResources();
	
	IMMDevice* GetDefaultDevice();
	void RenderWaveData(const float* data, int size, int channelCount);
	void DrawPoints(D2D1_POINT_2F** const data, int size, int channelCount);
	void Start();

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

	HWND m_hwnd;
	ID2D1Factory* m_pDirect2dFactory;
	ID2D1HwndRenderTarget* m_pRenderTarget;
	ID2D1SolidColorBrush* m_pLightSlateGrayBrush;
	ID2D1SolidColorBrush* m_pCornflowerBlueBrush;
	IDWriteFactory* m_pDWriteFactory;
	IDWriteTextFormat* m_pTextFormat;
	WinAudioCapture* capturer;
	BYTE *captureBuffer;
	D2D1_SIZE_U windowSize;
};

