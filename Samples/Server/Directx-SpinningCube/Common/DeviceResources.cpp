#include "pch.h"
#include "DeviceResources.h"
#include "macros.h"

using namespace DX;

// Constructor for DeviceResources.
DeviceResources::DeviceResources(bool isStereo) :
	m_isStereo(isStereo)
{
	CreateDeviceResources();
}

// Destructor for DeviceResources.
DeviceResources::~DeviceResources()
{
	CleanupResources();
}

void DeviceResources::CleanupResources()
{
	delete []m_screenViewport;
	SAFE_RELEASE(m_d3dContext);
	SAFE_RELEASE(m_d3dRenderTargetView);
	SAFE_RELEASE(m_swapChain);
	SAFE_RELEASE(m_d3dDevice);
}

SIZE DeviceResources::GetOutputSize() const
{
	return m_outputSize;
}

bool DeviceResources::IsStereo() const
{
	return m_isStereo;
}

ID3D11Device1* DeviceResources::GetD3DDevice() const
{
	return m_d3dDevice;
}

ID3D11DeviceContext1* DeviceResources::GetD3DDeviceContext() const
{
	return m_d3dContext;
}

IDXGISwapChain1* DeviceResources::GetSwapChain() const
{
	return m_swapChain;
}

ID3D11RenderTargetView* DeviceResources::GetBackBufferRenderTargetView() const
{
	return m_d3dRenderTargetView;
}

D3D11_VIEWPORT* DeviceResources::GetScreenViewport() const
{
	return m_screenViewport;
}

// Configures the Direct3D device, and stores handles to it and the device context.
HRESULT DeviceResources::CreateDeviceResources()
{
	ID3D11Device* device = nullptr;
	ID3D11DeviceContext* context = nullptr;
	HRESULT hr = S_OK;
	UINT createDeviceFlags = 0;

	// Creates D3D11 device.
	hr = D3D11CreateDevice(
		nullptr, 
		D3D_DRIVER_TYPE_HARDWARE, 
		nullptr, 
		createDeviceFlags,
		nullptr,
		0,
		D3D11_SDK_VERSION,
		&device,
		nullptr, 
		&context);

	if (FAILED(hr))
	{
		return hr;
	}

	hr = device->QueryInterface(__uuidof(ID3D11Device1), reinterpret_cast<void**>(&m_d3dDevice));
	if (SUCCEEDED(hr))
	{
		(void)context->QueryInterface(
			__uuidof(ID3D11DeviceContext1), 
			reinterpret_cast<void**>(&m_d3dContext));
	}

	// Cleanup.
	SAFE_RELEASE(context);
	SAFE_RELEASE(device);

	return hr;
}

// These resources need to be recreated every time the window size is changed.
HRESULT DeviceResources::CreateWindowSizeDependentResources(HWND hWnd)
{
	// Obtains DXGI factory from device.
	HRESULT hr = S_OK;
	IDXGIDevice* dxgiDevice = nullptr;
	IDXGIFactory1* dxgiFactory = nullptr;
	hr = m_d3dDevice->QueryInterface(__uuidof(IDXGIDevice), reinterpret_cast<void**>(&dxgiDevice));
	if (SUCCEEDED(hr))
	{
		IDXGIAdapter* adapter = nullptr;
		hr = dxgiDevice->GetAdapter(&adapter);
		if (SUCCEEDED(hr))
		{
			hr = adapter->GetParent(__uuidof(IDXGIFactory1), reinterpret_cast<void**>(&dxgiFactory));
			adapter->Release();
		}

		dxgiDevice->Release();
	}

	if (FAILED(hr))
	{
		return hr;
	}

	// Creates swap chain.
	IDXGIFactory2* dxgiFactory2 = nullptr;
	hr = dxgiFactory->QueryInterface(__uuidof(IDXGIFactory2), reinterpret_cast<void**>(&dxgiFactory2));
	if (dxgiFactory2)
	{
		DXGI_SWAP_CHAIN_DESC1 swapChainDesc = { 0 };
		swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		swapChainDesc.BufferCount = 2; // Front and back buffer to swap
		swapChainDesc.SampleDesc.Count = 1; // Disable anti-aliasing
		swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
		swapChainDesc.Width = m_isStereo ? FRAME_BUFFER_WIDTH << 1 : FRAME_BUFFER_WIDTH;
		swapChainDesc.Height = FRAME_BUFFER_HEIGHT;

		hr = dxgiFactory2->CreateSwapChainForHwnd(
			m_d3dDevice, 
			hWnd, 
			&swapChainDesc,
			nullptr, 
			nullptr, 
			&m_swapChain);

		if (FAILED(hr))
		{
			return hr;
		}

		dxgiFactory2->Release();
	}

	// Cleanup.
	SAFE_RELEASE(dxgiFactory);

	// Creates the render target view.
	ID3D11Texture2D* frameBuffer = nullptr;
	hr = m_swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&frameBuffer));
	if (FAILED(hr))
	{
		return hr;
	}

	hr = m_d3dDevice->CreateRenderTargetView(frameBuffer, nullptr, &m_d3dRenderTargetView);
	SAFE_RELEASE(frameBuffer);
	if (FAILED(hr))
	{
		return hr;
	}

	// Initializes the viewport.
	if (m_isStereo)
	{
		m_screenViewport = new D3D11_VIEWPORT[2];

		// Left eye.
		m_screenViewport[0] = CD3D11_VIEWPORT(
			0.0f,
			0.0f,
			(FLOAT)FRAME_BUFFER_WIDTH,
			(FLOAT)FRAME_BUFFER_HEIGHT);

		// Right eye.
		m_screenViewport[1] = CD3D11_VIEWPORT(
			(FLOAT)FRAME_BUFFER_WIDTH,
			0.0f,
			(FLOAT)FRAME_BUFFER_WIDTH,
			(FLOAT)FRAME_BUFFER_HEIGHT);
	}
	else
	{
		m_screenViewport = new D3D11_VIEWPORT[1];
		m_screenViewport[0] = CD3D11_VIEWPORT(
			0.0f,
			0.0f,
			(FLOAT)FRAME_BUFFER_WIDTH,
			(FLOAT)FRAME_BUFFER_HEIGHT);

		m_d3dContext->RSSetViewports(1, m_screenViewport);
	}

	m_outputSize.cx = FRAME_BUFFER_WIDTH;
	m_outputSize.cy = FRAME_BUFFER_HEIGHT;

	return hr;
}

void DeviceResources::SetWindow(HWND hWnd)
{
	CreateWindowSizeDependentResources(hWnd);
}

// Presents the contents of the swap chain to the screen.
void DeviceResources::Present()
{
	m_swapChain->Present(1, 0);
}
