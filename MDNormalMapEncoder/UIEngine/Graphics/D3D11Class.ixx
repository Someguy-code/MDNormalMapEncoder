module;

/////////////
// LINKING //
/////////////
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")

#include <d3d11.h>
#include <directxmath.h>

export module D3D11Class;

import std;

using namespace DirectX;

export class D3D11Class
{
	static constexpr D3D_FEATURE_LEVEL m_scuFeatureLevel = D3D_FEATURE_LEVEL_10_0;
	static constexpr DXGI_FORMAT m_scuOutputFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
	static constexpr DXGI_FORMAT m_scuDepthStencilFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

	bool m_bVSyncEnabled;

	IDXGISwapChain* m_pSwapChain;
	ID3D11Device* m_pDevice;
	ID3D11DeviceContext* m_pDeviceContext;
	ID3D11RenderTargetView* m_pRenderTargetView;
	ID3D11Texture2D* m_pDepthStencilBuffer;
	/*cRasterState* m_pRasterState;
	cDepthStencilState* m_pDepthStencilState;
	cBlendState* m_pBlendState;*/
	ID3D11DepthStencilView* m_pDepthStencilView;

	D3D11_VIEWPORT m_oViewport;

	bool SetupSwapChain(HWND _hwnd, unsigned int _uScreenWidth, unsigned int _uScreenHeight, bool _bVsyncEnabled)
	{
		return {};
		/*DXGI_SWAP_CHAIN_DESC SwapChainDescription{};

		// Set to a single back buffer.
		SwapChainDescription.BufferCount = 2;

		// Set the handle for the window to render to.
		SwapChainDescription.OutputWindow = _hwnd;

		DXGI_MODE_DESC& BufferDescription = SwapChainDescription.BufferDesc;
		// Set the width and height of the back buffer.
		BufferDescription.Width = _uScreenWidth;
		BufferDescription.Height = _uScreenHeight;

		// Set the refresh rate of the back buffer.
		DXGI_RATIONAL& RefreshRateDescription = BufferDescription.RefreshRate;
		if (_bVsyncEnabled)
		{
			const DXGI_MODE_DESC* DisplayModeDescription = GetDisplayModeDescription(_uScreenWidth, _uScreenHeight);
			assert(DisplayModeDescription != nullptr);
			RefreshRateDescription = DisplayModeDescription->RefreshRate;
		}
		else
		{
			RefreshRateDescription.Numerator = 0;
			RefreshRateDescription.Denominator = 1;
		}

		// Set to full screen or windowed mod
		SwapChainDescription.Windowed = !_bFullScreen;

		// Set regular 32-bit surface for the back buffer.
		BufferDescription.Format = m_scuOutputFormat;

		// Set the usage of the back buffer.
		SwapChainDescription.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;

		// Turn multisampling off.
		DXGI_SAMPLE_DESC& SampleDescription = SwapChainDescription.SampleDesc;
		SampleDescription.Count = 1;
		SampleDescription.Quality = 0;

		// Set the scan line ordering and scaling to unspecified.
		BufferDescription.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
		BufferDescription.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;

		// Discard the back buffer contents after presenting.
		SwapChainDescription.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

		// Don't set the advanced flags.
		SwapChainDescription.Flags = 0;

		// Set the feature level to DirectX 11.
		D3D_FEATURE_LEVEL FeatureLevel = m_scuFeatureLevel;

		// Create the swap chain, Direct3D device, and Direct3D device context.
		HRESULT hResult = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, m_scuDirectXInitializationFlags, &FeatureLevel, 1, D3D11_SDK_VERSION,
			&SwapChainDescription, &m_pSwapChain, &m_pDevice, nullptr, &m_pDeviceContext);
		if (FAILED(hResult))
			DETAILED_LOCAL_ERROR("Failed to create swap chain");
		else
			bSuccess = true;

		return bSuccess;*/

	}
	bool SetupBackBufferRenderTargetView();
	bool SetupDepthBuffer(unsigned int _uScreenWidth, unsigned int _uScreenHeight);
	bool SetupDefaultDepthStencilState();
	bool SetupDepthStencilView();
	bool SetupDepthBufferAndStencil(unsigned int _uScreenWidth, unsigned int _uScreenHeight);
	bool SetupDefaultRasterState();
	void SetupViewport(unsigned int _uScreenWidth, unsigned int _uScreenHeight);
	void SetupDefaultMatrices(unsigned int _uScreenWidth, unsigned int _uScreenHeight);

	bool InitializeSystemResources();
public:
	D3D11Class(HWND _oWindowHandle, unsigned int _uScreenWidth, unsigned int _uScreenHeight, bool _bVsyncEnabled)
		: m_bVSyncEnabled(_bVsyncEnabled)
	{
		/*SetupSwapChain(_oWindowHandle, _uScreenWidth, _uScreenHeight, _bVsyncEnabled);
		InitializeSystemResources();
		SetupBackBufferRenderTargetView();
		SetupDepthBufferAndStencil(_uScreenWidth, _uScreenHeight);
		SetupDefaultRasterState();
		SetupViewport(_uScreenWidth, _uScreenHeight);
		SetupDefaultMatrices(_uScreenWidth, _uScreenHeight);*/
	}

	static void ReportLiveObjects();

	void BeginScene(float _fClearColorRed, float _fClearColorGreen, float _fClearColorBlue, float _fClearColorAlpha);
	void EndScene();

	ID3D11Device* GetDevice() const;
	ID3D11DeviceContext* GetDeviceContext() const;

	void GetVideoCardInfo(const char* sVideoCardName_, unsigned int& uVRAMAmmount_) const;

	void SetMainRenderTarget() const;

	/*void SetRasterState(cRasterState* _pRatserState);
	void SetDepthStencilState(cDepthStencilState* _pDepthStencilState);
	void SetBlendState(cBlendState* _pBlendState);*/

	void SetDepthTestEnabled(bool _bValue);

	bool OnWindowResized(unsigned int _uScreenWidth, unsigned int _uScreenHeight);
};


