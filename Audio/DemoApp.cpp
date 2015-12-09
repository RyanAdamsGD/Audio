#include "DemoApp.h"

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
	IMMDevice* device = GetDefaultDevice();
	if (device)
	{

		capturer = new (std::nothrow) WinAudioCapture(device, true, eConsole);
		if (capturer == NULL)
		{
			printf("Unable to allocate capturer\n");
			return;
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

	SafeRelease(&m_pDirect2dFactory);
	SafeRelease(&m_pDWriteFactory);
	SafeRelease(&m_pTextFormat);
	SafeRelease(&m_pRenderTarget);
	SafeRelease(&m_pLightSlateGrayBrush);
	SafeRelease(&m_pCornflowerBlueBrush);

}

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
		wcex.lpszClassName = L"D2DDemoApp";

		RegisterClassEx(&wcex);


		// Because the CreateWindow function takes its size in pixels,
		// obtain the system DPI and use it to scale the window size.
		FLOAT dpiX, dpiY;

		// The factory returns the current system DPI. This is also the value it will use
		// to create its own windows.
		m_pDirect2dFactory->GetDesktopDpi(&dpiX, &dpiY);


		// Create the window.
		m_hwnd = CreateWindow(
			L"D2DDemoApp",
			L"Direct2D Demo App",
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
			pSink->BeginFigure(
				D2D1::Point2F(0, 0),
				D2D1_FIGURE_BEGIN_HOLLOW
				);
			for (int j = 0; j < channelCount; j++)
			{
				pSink->AddLines(points[j], size / channelCount);
			}
			pSink->EndFigure(D2D1_FIGURE_END_OPEN);
			hr = pSink->Close();

		}
		//render the wave in the middle of the screen
		m_pRenderTarget->SetTransform(D2D1::Matrix3x2F::Translation(0,windowSize.height * 0.5f));

		m_pRenderTarget->DrawGeometry(geometry, m_pCornflowerBlueBrush, 5);

		D2D1_RECT_F textLocation = D2D1::RectF(windowSize.width - 50, -25, windowSize.width, windowSize.height);
		std::wstring text = std::to_wstring(size);
		//static const WCHAR renderText[] = text.c_str();
		m_pRenderTarget->DrawTextW(text.c_str(), text.length() - 1, m_pTextFormat, textLocation, m_pCornflowerBlueBrush);
		// Draw the outline of a rectangle.
		hr = m_pRenderTarget->EndDraw();
	}

	if (hr == D2DERR_RECREATE_TARGET)
	{
		hr = S_OK;
		DiscardDeviceResources();
	}
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

void DemoApp::Update()
{
	if (m_hwnd == NULL)
		return;

	static int count = 0;
	//OnRender(count++);
	if (capturer->initialized)
		RenderWaveData(reinterpret_cast<float*>(captureBuffer), capturer->BytesCaptured(), capturer->ChannelCount());
}

void UpdateLoop(DemoApp* app)
{
	int timer = 0;
	while (!app->finished)
	{
		app->Update();
		Sleep(16);
		timer += 16;
		if (timer >= 10000 - 16)
			app->StopCapture();
	}
}

void DemoApp::Start()
{
	//magic numbers yay!
	//really need to test these
	StartCapture(10, 20);
}

void DemoApp::RenderWaveData(const float* data, int size, int channelCount)
{
	size = size / sizeof(float);
	D2D1_POINT_2F** channelPoints = new D2D1_POINT_2F*[channelCount];
	for (int i = 0; i < channelCount; i++)
		channelPoints[i] = new D2D1_POINT_2F[size / channelCount];

	//only render the newest data
	size_t start = (size / channelCount) - (windowSize.width * 0.01);
	start = start > 0 ? start : 0;
	//waves take up at most 80% of the window
	float heightMultiplyer = windowSize.height * 0.4f;
	for (size_t i = start, x = 0; i < size; i += channelCount, x++)
	{
		for (int j = 0; j < channelCount; j++)
		{
			channelPoints[j][x] = D2D1::Point2F(x * 10, data[i + j] * heightMultiplyer);
		}
	}

	DrawPoints(channelPoints, size, channelCount);
	for (int i = 0; i < channelCount; i++)
		delete channelPoints[i];
	delete channelPoints;
}

IMMDevice* DemoApp::GetDefaultDevice()
{
	HRESULT hr;
	IMMDeviceEnumerator *deviceEnumerator = NULL;

	//create a device enumerator
	hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&deviceEnumerator));
	if (FAILED(hr))
	{
		printf("Unable to instantiate device enumerator: %x\n", hr);
		return NULL;
	}

	IMMDevice *device = NULL;

	hr = deviceEnumerator->GetDefaultAudioEndpoint(eCapture, eConsole, &device);
	if (FAILED(hr))
		printf("Unable to get default device for role %d: default device\n", hr);
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
			size_t captureBufferSize = capturer->SamplesPerSecond() * TargetDurationInSec * capturer->FrameSize();
			captureBuffer = new (std::nothrow) BYTE[captureBufferSize];

			if (captureBuffer == NULL)
			{
				printf("Unable to allocate capture buffer\n");
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