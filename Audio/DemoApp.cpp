#include "DemoApp.h"
#include "stdafx.h"

#define BACKGROUND_NOISE_VOLUME 0.001f
//need to know what this actually is
#define TARGET_LATENCY 30
//buffer durations in seconds
//basically allocate a buffer of this amount of time
#define BUFFER_DURATION_IN_SECONDS 10

void UpdateLoop(DemoApp*);

DemoApp::DemoApp() :
m_hwnd(NULL),
m_pDirect2dFactory(NULL),
m_pRenderTarget(NULL),
m_pLightSlateGrayBrush(NULL),
m_pCornflowerBlueBrush(NULL),
updateLoop(UpdateLoop, this),
finished(false),
captureBuffer(NULL)
{
	_CrtSetReportMode;
	IMMDevice* device = GetDefaultCaptureDevice();
	if (device)
	{
		capturer = new (std::nothrow) WinAudioCapture(device, true, eConsole);
		if (capturer == NULL)
		{
			LOGERROR("Unable to allocate capturer\n");
			return;
		}
	}
	else


	device = GetDefaultRenderDevice();
	if (device)
	{
		renderer = new (std::nothrow) WinAudioRenderer(device, true, eConsole);
		if (renderer == NULL)
		{
			LOGERROR("Unable to allocate renderer/n");
		}
	}

	Start();
}


DemoApp::~DemoApp()
{
	if (capturer)
	{
		capturer->Stop();
		capturer->Shutdown();
		SafeRelease(&capturer);
	}
	if (captureBuffer)
		delete[]captureBuffer;

	if (renderer)
	{
		renderer->Stop();
		renderer->Shutdown();
		SafeRelease(&renderer);
	}

	//renderQueue is a circularly linked list
	// just delete the next one until we get to the first one
	if (renderQueue)
	{
		RenderBuffer *current, *next = renderQueue->_Next;
		while (next != renderQueue)
		{
			current = next;
			next = current->_Next;
			delete current;
		}
		delete renderQueue;
	}

	SafeRelease(&m_pDirect2dFactory);
	SafeRelease(&m_pDWriteFactory);
	SafeRelease(&m_pTextFormat);
	SafeRelease(&m_pRenderTarget);
	SafeRelease(&m_pLightSlateGrayBrush);
	SafeRelease(&m_pCornflowerBlueBrush);

}

#pragma region Windows Calls
void DemoApp::RunMessageLoop()
{
	MSG msg;

	while (GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

}

HRESULT DemoApp::Initialize()
{
	HRESULT hr;

	// Initialize device-indpendent resources, such
	// as the Direct2D factory.
	hr = CreateDeviceIndependentResources();

	if (SUCCEEDED(hr))
	{
		// Register the window class.
		WNDCLASSEX wcex = { sizeof(WNDCLASSEX) };
		wcex.style = CS_HREDRAW | CS_VREDRAW;
		wcex.lpfnWndProc = DemoApp::WndProc;
		wcex.cbClsExtra = 0;
		wcex.cbWndExtra = sizeof(LONG_PTR);
		wcex.hInstance = HINST_THISCOMPONENT;
		wcex.hbrBackground = NULL;
		wcex.lpszMenuName = NULL;
		wcex.hCursor = LoadCursor(NULL, IDI_APPLICATION);
		wcex.lpszClassName = "D2DDemoApp";

		RegisterClassEx(&wcex);


		// Because the CreateWindow function takes its size in pixels,
		// obtain the system DPI and use it to scale the window size.
		FLOAT dpiX, dpiY;

		// The factory returns the current system DPI. This is also the value it will use
		// to create its own windows.
		m_pDirect2dFactory->GetDesktopDpi(&dpiX, &dpiY);


		// Create the window.
		m_hwnd = CreateWindow(
			"D2DDemoApp",
			"Wave Renderer",
			WS_OVERLAPPEDWINDOW,
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			static_cast<UINT>(ceil(640.f * dpiX / 96.f)),
			static_cast<UINT>(ceil(480.f * dpiY / 96.f)),
			NULL,
			NULL,
			HINST_THISCOMPONENT,
			this
			);
		hr = m_hwnd ? S_OK : E_FAIL;
		if (SUCCEEDED(hr))
		{
			ShowWindow(m_hwnd, SW_SHOWNORMAL);
			UpdateWindow(m_hwnd);
		}
	}

	return hr;
}

int WINAPI WinMain(
	HINSTANCE /* hInstance */,
	HINSTANCE /* hPrevInstance */,
	LPSTR /* lpCmdLine */,
	int /* nCmdShow */
	)
{
	// Use HeapSetInformation to specify that the process should
	// terminate if the heap manager detects an error in any heap used
	// by the process.
	// The return value is ignored, because we want to continue running in the
	// unlikely event that HeapSetInformation fails.
	HeapSetInformation(NULL, HeapEnableTerminationOnCorruption, NULL, 0);

	if (SUCCEEDED(CoInitialize(NULL)))
	{
		{
			DemoApp app;

			if (SUCCEEDED(app.Initialize()))
			{
				app.RunMessageLoop();
			}
		}
		CoUninitialize();
	}

	return 0;
}

HRESULT DemoApp::CreateDeviceIndependentResources()
{
	HRESULT hr = S_OK;

	// Create a Direct2D factory.
	hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &m_pDirect2dFactory);

	return hr;
}

HRESULT DemoApp::CreateDeviceResources()
{
	HRESULT hr = S_OK;

	static const WCHAR msc_fontName[] = L"Arial";
	static const FLOAT msc_fontSize = 12;

	hr = DWriteCreateFactory(
		DWRITE_FACTORY_TYPE_SHARED,
		__uuidof(m_pDWriteFactory),
		reinterpret_cast<IUnknown **>(&m_pDWriteFactory)
		);

	if (SUCCEEDED(hr))
	{
		// Create a DirectWrite text format object.
		hr = m_pDWriteFactory->CreateTextFormat(
			msc_fontName,
			NULL,
			DWRITE_FONT_WEIGHT_NORMAL,
			DWRITE_FONT_STYLE_NORMAL,
			DWRITE_FONT_STRETCH_NORMAL,
			msc_fontSize,
			L"", //locale
			&m_pTextFormat
			);
	}
	if (SUCCEEDED(hr))
	{
		// Center the text horizontally and vertically.
		m_pTextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);

		m_pTextFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);


	}

	if (!m_pRenderTarget)
	{
		RECT rc;
		GetClientRect(m_hwnd, &rc);

		windowSize = D2D1::SizeU(
			rc.right - rc.left,
			rc.bottom - rc.top
			);
		// Create a Direct2D render target.
		hr = m_pDirect2dFactory->CreateHwndRenderTarget(
			D2D1::RenderTargetProperties(),
			D2D1::HwndRenderTargetProperties(m_hwnd, windowSize),
			&m_pRenderTarget
			);


		if (SUCCEEDED(hr))
		{
			// Create a gray brush.
			hr = m_pRenderTarget->CreateSolidColorBrush(
				D2D1::ColorF(D2D1::ColorF::LightSlateGray),
				&m_pLightSlateGrayBrush
				);
		}
		if (SUCCEEDED(hr))
		{
			// Create a blue brush.
			hr = m_pRenderTarget->CreateSolidColorBrush(
				D2D1::ColorF(D2D1::ColorF::CornflowerBlue),
				&m_pCornflowerBlueBrush
				);
		}
	}

	return hr;
}

void DemoApp::DiscardDeviceResources()
{
	SafeRelease(&m_pDWriteFactory);
	SafeRelease(&m_pTextFormat);
	SafeRelease(&m_pRenderTarget);
	SafeRelease(&m_pLightSlateGrayBrush);
	SafeRelease(&m_pCornflowerBlueBrush);
}

LRESULT CALLBACK DemoApp::WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	LRESULT result = 0;

	if (message == WM_CREATE)
	{
		LPCREATESTRUCT pcs = (LPCREATESTRUCT)lParam;
		DemoApp *pDemoApp = (DemoApp *)pcs->lpCreateParams;

		::SetWindowLongPtrW(
			hwnd,
			GWLP_USERDATA,
			PtrToUlong(pDemoApp)
			);

		result = 1;
	}
	else
	{
		DemoApp *pDemoApp = reinterpret_cast<DemoApp *>(static_cast<LONG_PTR>(
			::GetWindowLongPtrW(
			hwnd,
			GWLP_USERDATA
			)));

		bool wasHandled = false;

		if (pDemoApp)
		{
			switch (message)
			{
			case WM_SIZE:
			{
				UINT width = LOWORD(lParam);
				UINT height = HIWORD(lParam);
				pDemoApp->OnResize(width, height);
			}
			result = 0;
			wasHandled = true;
			break;

			case WM_DISPLAYCHANGE:
			{
				InvalidateRect(hwnd, NULL, FALSE);
			}
			result = 0;
			wasHandled = true;
			break;

			case WM_PAINT:
			{
				ValidateRect(hwnd, NULL);
			}
			result = 0;
			wasHandled = true;
			break;

			case WM_DESTROY:
			{
				pDemoApp->finished = true;
				pDemoApp->updateLoop.join();
				PostQuitMessage(0);
			}
			result = 1;
			wasHandled = true;
			break;
			}
		}

		if (!wasHandled)
		{
			result = DefWindowProc(hwnd, message, wParam, lParam);
		}
	}

	return result;
}

void DemoApp::OnResize(UINT width, UINT height)
{
	if (m_pRenderTarget)
	{
		// Note: This method can fail, but it's okay to ignore the
		// error here, because the error will be returned again
		// the next time EndDraw is called.
		windowSize.width = width;
		windowSize.height = height;
		m_pRenderTarget->Resize(D2D1::SizeU(width, height));
	}
}
#pragma endregion

void DemoApp::DrawPoints(D2D1_POINT_2F** const points, int size, int channelCount)
{
	HRESULT hr = S_OK;

	hr = CreateDeviceResources();
	if (SUCCEEDED(hr))
	{

		m_pRenderTarget->BeginDraw();


		m_pRenderTarget->Clear(D2D1::ColorF(D2D1::ColorF::White));

		D2D1_SIZE_F rtSize = m_pRenderTarget->GetSize();


		ID2D1PathGeometry* geometry;
		m_pDirect2dFactory->CreatePathGeometry(&geometry);

		ID2D1GeometrySink *pSink = NULL;

		hr = geometry->Open(&pSink);
		if (SUCCEEDED(hr))
		{
			for (int j = 0; j < channelCount; j++)
			{
				pSink->BeginFigure(points[j][0], D2D1_FIGURE_BEGIN_HOLLOW);
				pSink->AddLines(points[j], size / channelCount);
				pSink->EndFigure(D2D1_FIGURE_END_OPEN);
			}
			hr = pSink->Close();

		}
		//render the wave in the middle of the screen
		m_pRenderTarget->SetTransform(D2D1::Matrix3x2F::Translation(0, (float)windowSize.height / (float)(channelCount + 1)));

		m_pRenderTarget->DrawGeometry(geometry, m_pCornflowerBlueBrush, 5);

		size_t stringCount = stringsToRender.size();
		for (size_t i = 0; i < stringCount; i++)
		{
			DrawnString textToDraw = stringsToRender[i];
			D2D1_RECT_F textLocation = D2D1::RectF(textToDraw.x, textToDraw.y, windowSize.width, windowSize.height);
			m_pRenderTarget->DrawText(textToDraw.text.c_str(), textToDraw.text.length(), m_pTextFormat, textLocation, m_pCornflowerBlueBrush);

		}
		// Draw the outline of a rectangle.
		hr = m_pRenderTarget->EndDraw();
		stringsToRender.clear();
	}

	if (hr == D2DERR_RECREATE_TARGET)
	{
		hr = S_OK;
		DiscardDeviceResources();
	}
}

//only call this after calling draw points
//because I'm too laze to extract it all
void DemoApp::DrawString(int x, int y, const std::wstring& text)
{
	stringsToRender.push_back(DrawnString(x, y, text));
}

void DemoApp::Update()
{
	if (m_hwnd == NULL)
		return;
	if (capturer->initialized)
	{
		RenderWaveData(reinterpret_cast<float*>(captureBuffer), capturer->BytesCaptured(), capturer->ChannelCount());

		if (renderer->Initialized())
			WriteCaptureBufferToRenderBuffer();
		else
		{
			//because magic numbers that I really need to look up
			renderer->Initialize(TARGET_LATENCY);
			CreateRenderQueue(BUFFER_DURATION_IN_SECONDS);
			renderer->Start(renderQueue);
		}
	}

	//swap our buffers when we get close to filling up the current buffer
	if (captureBufferSize * 0.9f < capturer->BytesCaptured())
		ResetCaptureBuffer();
}

void UpdateLoop(DemoApp* app)
{
	int timer = 0;
	while (!app->finished)
	{
		//let a frame pass after start before calling update
		Sleep(16);
		app->Update();
	}
}

void DemoApp::Start()
{
	StartCapture(BUFFER_DURATION_IN_SECONDS, TARGET_LATENCY);
}

void DemoApp::ResetCaptureBuffer()
{
	if (capturer->initialized && captureBuffer != NULL)
	{
		//make sure to write any extra data that may have been capture before moving
		//back to the beginning of the array
		WriteCaptureBufferToRenderBuffer();
		capturer->SwitchBuffer(captureBuffer, captureBufferSize);
	}
}

void DemoApp::RenderWaveData(const float* data, int size, int channelCount)
{
	size = size / sizeof(float);
	float unitsPerSoundPoint = 1;
	//only render the newest data
	//and figure out how much to render
	size_t amountThatCanBeDrawn = (windowSize.width / unitsPerSoundPoint) * channelCount;
	size_t amountThatWillBeDrawn = size > amountThatCanBeDrawn ? amountThatCanBeDrawn : size;

	//create the buffers to store the points that will be rendered
	D2D1_POINT_2F** channelPoints = new D2D1_POINT_2F*[channelCount];
	for (int i = 0; i < channelCount; i++)
		channelPoints[i] = new D2D1_POINT_2F[amountThatWillBeDrawn / channelCount];


	//waves take up at most 80% of the window
	float heightMultiplyer = windowSize.height * 0.8f;
	float channelSpread = windowSize.height * 0.8f / channelCount;
	for (size_t i = size - amountThatWillBeDrawn, x = 0; i < size; i += channelCount, x++)
	{
		for (int j = 0; j < channelCount; j++)
		{
			channelPoints[j][x] = D2D1::Point2F(x * unitsPerSoundPoint, data[i + j] * heightMultiplyer + j * channelSpread);
		}
	}


	DrawString(windowSize.width - 50, -25, std::to_wstring(amountThatWillBeDrawn));

	float sampleDurationInSeconds = (float)amountThatWillBeDrawn / capturer->SamplesPerSecond();
	float frequency = FindFrequencyInHerz(reinterpret_cast<const float*>(data + (size - amountThatWillBeDrawn)), amountThatWillBeDrawn, sampleDurationInSeconds);
	DrawString(0, 0, std::to_wstring(frequency));
	DrawPoints(channelPoints, amountThatWillBeDrawn, channelCount);
	for (int i = 0; i < channelCount; i++)
		delete channelPoints[i];
	delete channelPoints;
}

float DemoApp::FindFrequencyInHerz(const float* const data, int size, float sampleDurationInSeconds)
{
	if (size < 2)
		return 0;

	bool wasPositive = false;
	bool previousState = false;
	int numberOfCrestsAndTroughs = 0;
	int previousCrestOrTroughIndex = 0;
	std::vector<float> samplesPerPeriod;
	samplesPerPeriod.reserve(1000);
	int samplesPerPeriodIndex = 0;
	int previousValueWasIgnoredCount = 0;

	for (size_t i = 1; i < size; i++)
	{
		if (data[i] < BACKGROUND_NOISE_VOLUME && data[i] > -BACKGROUND_NOISE_VOLUME)
		{
			//should do some logic here to not include background noise
			//and do extra calculations because of it
			previousValueWasIgnoredCount++;
			continue;
		}

		wasPositive = 0.0f > data[i];
		if (previousState != wasPositive)
		{
			samplesPerPeriod.push_back(0);
			numberOfCrestsAndTroughs++;
			previousState = wasPositive;
			samplesPerPeriod[samplesPerPeriodIndex] = i;
		}
	}

	int averageSamplesPerPeriod = 0;
	size_t samplesPerPeriodSize = samplesPerPeriod.size();
	for (size_t i = 0; i < samplesPerPeriodSize; i++)
		averageSamplesPerPeriod += samplesPerPeriod[i];
	if (samplesPerPeriodSize != 0)
		averageSamplesPerPeriod /= samplesPerPeriodSize;
	DrawString(0, 20, std::to_wstring(averageSamplesPerPeriod));

	int numberOfPeriods = numberOfCrestsAndTroughs * 0.5f;
	return numberOfPeriods / sampleDurationInSeconds;
}

IMMDevice* DemoApp::GetDefaultCaptureDevice()
{
	HRESULT hr;
	IMMDeviceEnumerator *deviceEnumerator = NULL;

	//create a device enumerator
	hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&deviceEnumerator));
	if (FAILED(hr))
	{
		LOGERROR("Unable to instantiate device enumerator: %x\n", hr);
		return NULL;
	}

	IMMDevice *device = NULL;

	hr = deviceEnumerator->GetDefaultAudioEndpoint(eCapture, eConsole, &device);
	if (FAILED(hr))
		LOGERROR("Unable to get default device for role %d: default device\n", hr);
	SafeRelease(&deviceEnumerator);
	return device;
}

bool DemoApp::StartCapture(int TargetDurationInSec, int TargetLatency)
{
	bool successfullyStarted = capturer != nullptr;
	if (successfullyStarted)
	{
		successfullyStarted = capturer->Initialize(TargetLatency);
		if (successfullyStarted)
		{
			//
			//  We've initialized the capturer.  Once we've done that, we know some information about the
			//  mix format and we can allocate the buffer that we're going to capture.
			//
			//
			//  The buffer is going to contain "TargetDuration" seconds worth of PCM data.  That means 
			//  we're going to have TargetDuration*samples/second frames multiplied by the frame size.
			//
			captureBufferSize = capturer->SamplesPerSecond() * TargetDurationInSec * capturer->FrameSize();
			captureBuffer = new (std::nothrow) BYTE[captureBufferSize];

			if (captureBuffer == NULL)
			{
				LOGERROR("Unable to allocate capture buffer\n");
				return false;
			}

			successfullyStarted = capturer->Start(captureBuffer, captureBufferSize);
		}
	}
	return successfullyStarted;
}

void DemoApp::StopCapture()
{
	capturer->Stop();
}

IMMDevice* DemoApp::GetDefaultRenderDevice()
{
	IMMDevice *device = NULL;
	HRESULT hr;

	IMMDeviceEnumerator *deviceEnumerator = NULL;
	ERole deviceRole = eConsole;    // Assume we're using the console role.
	hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&deviceEnumerator));
	if (FAILED(hr))
	{
		LOGERROR("Unable to create output device enumerator");
		return NULL;
	}

	hr = deviceEnumerator->GetDefaultAudioEndpoint(eRender, deviceRole, &device);

	if (FAILED(hr))
		LOGERROR("Unable to get default device for role %d: %x\n", deviceRole, hr);

	SafeRelease(&deviceEnumerator);
	return device;
}

void DemoApp::WriteCaptureBufferToRenderBuffer()
{
	//size - currentPosition + start
	size_t amountThatCanBewrittenInCurrentBuffer = currentRenderBufferBeingWrittenTo->_BufferLength - (int)currentRenderBufferPosition - (int)currentRenderBufferBeingWrittenTo->_Buffer;
	size_t amountThatNeedsToBeWritten = capturer->BytesCaptured() - previousCaptureBufferSize;

	//a negative number likely indicates the buffer was swapped or reset
	//in this case start reading from the start of the new buffer
	if (amountThatNeedsToBeWritten < 0)
	{
		amountThatNeedsToBeWritten = capturer->BytesCaptured();
		previousCaptureBufferSize = 0;
	}

	//write what we can to the current buffer
	while (amountThatNeedsToBeWritten > amountThatCanBewrittenInCurrentBuffer)
	{
		//write as much as possible into this buffer
		memcpy(currentRenderBufferPosition, captureBuffer + previousCaptureBufferSize, amountThatCanBewrittenInCurrentBuffer);

		//get the necessary information for writing to the next buffer
		amountThatNeedsToBeWritten -= amountThatCanBewrittenInCurrentBuffer;
		currentRenderBufferBeingWrittenTo = currentRenderBufferBeingWrittenTo->_Next;
		previousCaptureBufferSize += amountThatCanBewrittenInCurrentBuffer;
		amountThatCanBewrittenInCurrentBuffer = currentRenderBufferBeingWrittenTo->_BufferLength;
		currentRenderBufferPosition = currentRenderBufferBeingWrittenTo->_Buffer;
	}

	//set up any information that may be needed in the next write
	memcpy(currentRenderBufferPosition, captureBuffer + previousCaptureBufferSize, amountThatNeedsToBeWritten);
	currentRenderBufferPosition = currentRenderBufferBeingWrittenTo->_Buffer + amountThatNeedsToBeWritten;
	previousCaptureBufferSize += amountThatNeedsToBeWritten;
}

void DemoApp::CreateRenderQueue(int BufferDurationInSeconds)
{

	if (renderer->Initialized())
	{
		//
		//  We've initialized the renderer.  Once we've done that, we know some information about the
		//  mix format and we can allocate the buffer that we're going to render.
		//
		//
		//  The buffer is going to contain "TargetDuration" seconds worth of PCM data.  That means 
		//  we're going to have TargetDuration*samples/second frames multiplied by the frame size.
		//
		UINT32 renderBufferSizeInBytes = (renderer->BufferSizePerPeriod()  * renderer->FrameSize());
		size_t renderDataLength = (renderer->SamplesPerSecond() * BufferDurationInSeconds * renderer->FrameSize()) + (renderBufferSizeInBytes - 1);
		size_t renderBufferCount = renderDataLength / (renderBufferSizeInBytes);
		//
		//  Render buffer queue. Because we need to insert each buffer at the end of the linked list instead of at the head, 
		//  we keep a pointer to a pointer to the variable which holds the tail of the current list in currentBufferTail.
		//
		renderQueue = NULL;
		RenderBuffer **currentBufferTail = &renderQueue;

		double theta = 0;

		for (size_t i = 0; i < renderBufferCount; i += 1)
		{
			RenderBuffer *renderBuffer = new (std::nothrow) RenderBuffer();
			if (renderBuffer == NULL)
			{
				LOGERROR("Unable to allocate render buffer\n");
			}
			renderBuffer->_BufferLength = renderBufferSizeInBytes;
			renderBuffer->_Buffer = new (std::nothrow) BYTE[renderBufferSizeInBytes];
			if (renderBuffer->_Buffer == NULL)
			{
				LOGERROR("Unable to allocate render buffer\n");
			}


			//
			//  Link the newly allocated and filled buffer into the queue.  
			//
			*currentBufferTail = renderBuffer;
			currentBufferTail = &renderBuffer->_Next;
		}

		//make the end of the render queue point to the start of the render queue
		//making it a circular linked list
		*currentBufferTail = renderQueue;
	}
	else
		LOGERROR("Renderer has not been initialized. Please initialize it before starting it");
}

void DemoApp::StopAudioRender()
{
	renderer->Stop();
}