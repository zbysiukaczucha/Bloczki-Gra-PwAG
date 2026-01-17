#pragma once

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")

#include <DirectXMath.h>
#include <array>
#include <d3d11.h>
#include <span>
#include <string>
#include <wrl/client.h>

enum class GBufferTextures : std::uint8_t
{
	Albedo = 0,
	Normal,
	ORM,
	Emissive,

	NumTextures,
};

class DX11Context
{
public:
	DX11Context()  = default;
	~DX11Context() = default;

	bool Initialize(HWND handle, int windowWidth, int windowHeight, bool fullscreen, bool vSync);

	void ClearScreen(float r, float g, float b, float a);
	void Deferred_ClearScreen(float r, float g, float b, float a);
	void PrepareShadowPass();
	void PrepareLightPass();
	void SetBackBuffer(float clearR, float clearG, float clearB, float clearA);
	void SetBackBuffer(bool bindDepthStencilView = true);				// no clear
	void SetLightAccumulationBuffer(bool bindDepthStencilView = false); // no clear
	void SetPingPongBuffer();
	void ResetPingPongBuffers();
	void SwapBuffers();

	void ToggleAlphaBlend(bool enable);
	void ToggleLightBlend(bool enable);
	void ToggleZBuffer(bool enable);
	void ToggleSkyDepthStencil(bool enable);
	void ToggleFrontCulling(bool enable);
	void ToggleWireframe(bool enable);

	void OnWindowResize(std::uint32_t width, std::uint32_t height);

private:
	bool ResizeSwapChain();
	bool CreateDepthBuffer();
	bool CreateRasterizerState(D3D11_RASTERIZER_DESC rasterDesc);
	bool CreateDepthStencilState();
	bool InitBlendState();
	bool CreateGBuffer();
	bool InitShadowMaps();
	bool InitPingPongBuffers();

	int			 windowWidth_;
	int			 windowHeight_;
	float		 screenDepth_;
	float		 screenNear_;
	int			 VRAM_;
	std::wstring GPUDesc_;
	bool		 fullscreen_;
	bool		 vSync_;
	float		 fov_;

	bool shouldRebuildState_;

private:
	HWND										windowHandle_;
	Microsoft::WRL::ComPtr<ID3D11Device>		device_;
	Microsoft::WRL::ComPtr<ID3D11DeviceContext> context_;
	Microsoft::WRL::ComPtr<IDXGISwapChain>		swapChain_;

	Microsoft::WRL::ComPtr<ID3D11RenderTargetView>	renderTargetView_;
	Microsoft::WRL::ComPtr<ID3D11DepthStencilView>	depthStencilView_;
	Microsoft::WRL::ComPtr<ID3D11DepthStencilState> depthStencilState_;
	Microsoft::WRL::ComPtr<ID3D11DepthStencilState> depthDisabledStencilState_;
	Microsoft::WRL::ComPtr<ID3D11DepthStencilState> skyDepthStencilState_;
	Microsoft::WRL::ComPtr<ID3D11Texture2D>			depthStencilBuffer_;
	ID3D11RasterizerState*							currentDefaultRasterState_;
	ID3D11RasterizerState*							currentCullFrontRasterState_;
	Microsoft::WRL::ComPtr<ID3D11RasterizerState>	rasterState_;
	Microsoft::WRL::ComPtr<ID3D11RasterizerState>	cullFrontRasterState_;
	Microsoft::WRL::ComPtr<ID3D11RasterizerState>	wireframeRasterState_;
	Microsoft::WRL::ComPtr<ID3D11RasterizerState>	cullFrontWireframeRasterState_;
	Microsoft::WRL::ComPtr<ID3D11SamplerState>		samplerState_;
	Microsoft::WRL::ComPtr<ID3D11SamplerState>		shadowSamplerState_;
	Microsoft::WRL::ComPtr<ID3D11BlendState>		alphaBlendState_;
	Microsoft::WRL::ComPtr<ID3D11BlendState>		lightBlendState_;

	// deferred stuff
	static constexpr std::size_t NUM_G_TEXTURES = static_cast<std::size_t>(GBufferTextures::NumTextures);

	std::array<Microsoft::WRL::ComPtr<ID3D11Texture2D>, NUM_G_TEXTURES>		   gBufferTextures_;
	std::array<Microsoft::WRL::ComPtr<ID3D11RenderTargetView>, NUM_G_TEXTURES> gBufferRTVs_;

	// +1 because depth is also read as an srv
	std::array<Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>, NUM_G_TEXTURES + 1> gBufferSRVs_;

	Microsoft::WRL::ComPtr<ID3D11Texture2D>			 lightAccumulationBuffer_;
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView>	 lightAccumulationRTV_;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> lightAccumulationSRV_;

	Microsoft::WRL::ComPtr<ID3D11Texture2D>			 sunShadowMap_;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> sunShadowMapSRV_;
	Microsoft::WRL::ComPtr<ID3D11RasterizerState>	 sunShadowMapRasterState_;
	Microsoft::WRL::ComPtr<ID3D11DepthStencilView>	 sunDepthStencilView_;
	Microsoft::WRL::ComPtr<ID3D11DepthStencilState>	 sunDepthStencilState_;
	D3D11_VIEWPORT									 sunViewport_;

	Microsoft::WRL::ComPtr<IDXGIAdapter> adapter_;
	Microsoft::WRL::ComPtr<IDXGIFactory> factory_;

	DirectX::XMFLOAT4X4 orthoMatrix_;

	D3D11_VIEWPORT viewport_;

	// mainly for Bloom
	Microsoft::WRL::ComPtr<ID3D11Texture2D>			 pingPongBuffer_[2];
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> pingPongSRVs_[2];
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView>	 pingPongRTVs_[2];
	std::uint8_t									 currentPingPongBuffer = 0;

	bool wireframeEnabled;

public:
	// Getters
	[[nodiscard]] Microsoft::WRL::ComPtr<ID3D11Device>		  GetDevice() const { return device_; }
	[[nodiscard]] Microsoft::WRL::ComPtr<ID3D11DeviceContext> GetDeviceContext() const { return context_; }
	[[nodiscard]] Microsoft::WRL::ComPtr<ID3D11SamplerState>  GetSamplerState() const { return samplerState_; }
	[[nodiscard]] Microsoft::WRL::ComPtr<ID3D11SamplerState>  GetShadowSamplerState() const
	{
		return shadowSamplerState_;
	}

	[[nodiscard]] float GetFarZ() const { return screenDepth_; }
	[[nodiscard]] float GetNearZ() const { return screenNear_; }
	[[nodiscard]] int	GetWindowWidth() const { return windowWidth_; }
	[[nodiscard]] int	GetWindowHeight() const { return windowHeight_; }
	[[nodiscard]] float GetAspectRatio() const
	{
		return static_cast<float>(windowWidth_) / static_cast<float>(windowHeight_);
	}
	[[nodiscard]] DirectX::XMMATRIX GetOrthoMatrix() const { return DirectX::XMLoadFloat4x4(&orthoMatrix_); }

	[[nodiscard]] const std::array<Microsoft::WRL::ComPtr<ID3D11Texture2D>, NUM_G_TEXTURES>& GetGBufferTextures() const;
	[[nodiscard]] const std::array<Microsoft::WRL::ComPtr<ID3D11RenderTargetView>, NUM_G_TEXTURES>& GetGBufferRTVs()
		const;
	[[nodiscard]] const std::array<Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>, NUM_G_TEXTURES + 1>&
	GetGBufferSRVs() const;

	[[nodiscard]] ID3D11ShaderResourceView* GetShadowMapSRV() const { return sunShadowMapSRV_.Get(); }
	[[nodiscard]] ID3D11ShaderResourceView* GetCurrentPingPongSRV() const
	{
		return pingPongSRVs_[currentPingPongBuffer].Get();
	}
	[[nodiscard]] ID3D11ShaderResourceView* GetLightAccumulationSRV() const { return lightAccumulationSRV_.Get(); }
	[[nodiscard]] bool						IsWireframeEnabled() const { return wireframeEnabled; }
};
