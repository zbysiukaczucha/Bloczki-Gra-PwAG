#include "DX11Context.h"

#include <iostream>
#include <ostream>
#include <vector>

bool DX11Context::Initialize(HWND handle, int windowWidth, int windowHeight, bool fullscreen, bool vSync)
{
	HRESULT result;

	windowWidth_  = windowWidth;
	windowHeight_ = windowHeight;
	screenDepth_  = 1000.f;
	screenNear_	  = 0.1f;
	fullscreen_	  = fullscreen;
	vSync_		  = vSync;
	VRAM_		  = -1; // for now, but it can indicate something's wrong in case it's not set later
	fov_		  = DirectX::XM_PIDIV4;
	windowHandle_ = handle;

	result = CreateDXGIFactory(__uuidof(IDXGIFactory), &factory_);
	if (FAILED(result))
	{
		std::cerr << "Failed to create DXGIFactory!" << std::endl;
		return false;
	}

	result = factory_->EnumAdapters(0, &adapter_);
	if (FAILED(result))
	{
		std::cerr << "Failed to enumerate adapter!" << std::endl;
		return false;
	}

	Microsoft::WRL::ComPtr<IDXGIOutput> adapterOutput;
	result = adapter_->EnumOutputs(0, &adapterOutput);
	if (FAILED(result))
	{
		std::cerr << "Failed to enumerate adapter outputs!" << std::endl;
		return false;
	}

	unsigned int numModes = 0;
	result				  = adapterOutput->GetDisplayModeList(DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
												  DXGI_ENUM_MODES_INTERLACED,
												  &numModes,
												  nullptr);
	if (FAILED(result))
	{
		std::cerr << "Failed to get display mode amount!" << std::endl;
		return false;
	}

	std::unique_ptr<DXGI_MODE_DESC[]> displayModeList(new DXGI_MODE_DESC[numModes]);
	result = adapterOutput->GetDisplayModeList(DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
											   DXGI_ENUM_MODES_INTERLACED,
											   &numModes,
											   displayModeList.get());
	if (FAILED(result))
	{
		std::cerr << "Failed to fill display mode list!" << std::endl;
		return false;
	}

	UINT numerator	 = 0;
	UINT denominator = 1;
	for (UINT i = 0; i < numModes; ++i)
	{
		UINT width	= displayModeList[i].Width;
		UINT height = displayModeList[i].Height;
		if (std::cmp_equal(width, windowWidth_) && std::cmp_equal(height, windowHeight_))
		{
			numerator	= displayModeList[i].RefreshRate.Numerator;
			denominator = displayModeList[i].RefreshRate.Denominator;
		}
	}

	DXGI_ADAPTER_DESC adapterDesc = {};
	result						  = adapter_->GetDesc(&adapterDesc);
	if (FAILED(result))
	{
		std::cerr << "Failed to get adapter description!" << std::endl;
		return false;
	}

	VRAM_	 = static_cast<int>(adapterDesc.DedicatedVideoMemory / (1024ull * 1024ull));
	GPUDesc_ = adapterDesc.Description;

	DXGI_SWAP_CHAIN_DESC swapChainDesc				 = {};
	swapChainDesc.BufferCount						 = 1; // TODO: experiment with 2 or 3 later
	swapChainDesc.BufferDesc.Width					 = windowWidth_;
	swapChainDesc.BufferDesc.Height					 = windowHeight_;
	swapChainDesc.BufferDesc.Format					 = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	swapChainDesc.BufferDesc.RefreshRate.Numerator	 = 0;
	swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
	swapChainDesc.BufferUsage						 = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.OutputWindow						 = handle;

	// not using MSAA for now
	swapChainDesc.SampleDesc.Count			  = 1;
	swapChainDesc.SampleDesc.Quality		  = 0;
	swapChainDesc.Windowed					  = TRUE;
	swapChainDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	swapChainDesc.BufferDesc.Scaling		  = DXGI_MODE_SCALING_UNSPECIFIED;
	swapChainDesc.SwapEffect				  = DXGI_SWAP_EFFECT_DISCARD;
	swapChainDesc.Flags						  = 0; // no advanced flags for now TODO: research what the flags can do
	D3D_FEATURE_LEVEL featureLevel			  = D3D_FEATURE_LEVEL_11_0; // only supporting one for now

	result = D3D11CreateDeviceAndSwapChain(nullptr,
										   D3D_DRIVER_TYPE_HARDWARE,
										   nullptr,
										   0,
										   &featureLevel,
										   1,
										   D3D11_SDK_VERSION,
										   &swapChainDesc,
										   &swapChain_,
										   &device_,
										   nullptr,
										   &context_);

	if (FAILED(result))
	{
		std::cerr << "Failed to create device and/or device context and/or swap chain!" << std::endl;
		return false;
	}

	Microsoft::WRL::ComPtr<ID3D11Texture2D> backBuffer;
	result = swapChain_->GetBuffer(0, __uuidof(ID3D11Texture2D), &backBuffer);
	if (FAILED(result))
	{
		std::cerr << "Failed to get back buffer pointer!" << std::endl;
		return false;
	}

	result = device_->CreateRenderTargetView(backBuffer.Get(), nullptr, &renderTargetView_);
	if (FAILED(result))
	{
		std::cerr << "Failed to create back buffer render target view!" << std::endl;
		return false;
	}

	D3D11_TEXTURE2D_DESC depthBufferDesc = {};
	depthBufferDesc.Width				 = windowWidth_;
	depthBufferDesc.Height				 = windowHeight_;
	depthBufferDesc.MipLevels			 = 1;
	depthBufferDesc.ArraySize			 = 1;
	depthBufferDesc.Format				 = DXGI_FORMAT_R24G8_TYPELESS;
	depthBufferDesc.SampleDesc.Count	 = 1;
	depthBufferDesc.SampleDesc.Quality	 = 0;
	depthBufferDesc.Usage				 = D3D11_USAGE_DEFAULT;
	depthBufferDesc.BindFlags			 = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
	depthBufferDesc.CPUAccessFlags		 = 0;
	depthBufferDesc.MiscFlags			 = 0;

	result = device_->CreateTexture2D(&depthBufferDesc, nullptr, &depthStencilBuffer_);
	if (FAILED(result))
	{
		std::cerr << "Failed to create depth stencil buffer!" << std::endl;
		return false;
	}

	D3D11_DEPTH_STENCIL_DESC depthStencilDesc = {};
	depthStencilDesc.DepthEnable			  = true;
	depthStencilDesc.DepthWriteMask			  = D3D11_DEPTH_WRITE_MASK_ALL;
	depthStencilDesc.DepthFunc				  = D3D11_COMPARISON_LESS;
	depthStencilDesc.StencilEnable			  = true;
	depthStencilDesc.StencilReadMask		  = 0xFF;
	depthStencilDesc.StencilWriteMask		  = 0xFF;

	// Stencil operations if pixel is front-facing
	depthStencilDesc.FrontFace.StencilFailOp	  = D3D11_STENCIL_OP_KEEP;
	depthStencilDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_INCR;
	depthStencilDesc.FrontFace.StencilPassOp	  = D3D11_STENCIL_OP_KEEP;
	depthStencilDesc.FrontFace.StencilFunc		  = D3D11_COMPARISON_ALWAYS;

	// Stencil operations if pixel is back-facing
	depthStencilDesc.BackFace.StencilFailOp		 = D3D11_STENCIL_OP_KEEP;
	depthStencilDesc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_DECR;
	depthStencilDesc.BackFace.StencilPassOp		 = D3D11_STENCIL_OP_KEEP;
	depthStencilDesc.BackFace.StencilFunc		 = D3D11_COMPARISON_ALWAYS;

	result = device_->CreateDepthStencilState(&depthStencilDesc, &depthStencilState_);
	if (FAILED(result))
	{
		std::cerr << "Failed to create depth stencil state!" << std::endl;
		return false;
	}

	context_->OMSetDepthStencilState(depthStencilState_.Get(), 1);

	D3D11_DEPTH_STENCIL_DESC depthDisabledStencilDesc = depthStencilDesc;
	depthDisabledStencilDesc.DepthEnable			  = false;

	result = device_->CreateDepthStencilState(&depthDisabledStencilDesc, &depthDisabledStencilState_);
	if (FAILED(result))
	{
		std::cerr << "Failed to create depth disabled stencil state!" << std::endl;
		return false;
	}

	D3D11_DEPTH_STENCIL_DESC skyDepthStencilDesc = depthStencilDesc;
	skyDepthStencilDesc.DepthWriteMask			 = D3D11_DEPTH_WRITE_MASK_ZERO;
	skyDepthStencilDesc.DepthFunc				 = D3D11_COMPARISON_LESS_EQUAL;

	result = device_->CreateDepthStencilState(&skyDepthStencilDesc, &skyDepthStencilState_);
	if (FAILED(result))
	{
		std::cerr << "Failed to create sky depth stencil state!" << std::endl;
	}

	D3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc = {};
	depthStencilViewDesc.Format						   = DXGI_FORMAT_D24_UNORM_S8_UINT;
	depthStencilViewDesc.ViewDimension				   = D3D11_DSV_DIMENSION_TEXTURE2D;
	depthStencilViewDesc.Texture2D.MipSlice			   = 0;

	result = device_->CreateDepthStencilView(depthStencilBuffer_.Get(), &depthStencilViewDesc, &depthStencilView_);
	if (FAILED(result))
	{
		std::cerr << "Failed to create depth stencil view!" << std::endl;
		return false;
	}

	context_->OMSetRenderTargets(1, renderTargetView_.GetAddressOf(), depthStencilView_.Get());

	D3D11_RASTERIZER_DESC rasterDesc = {};
	rasterDesc.AntialiasedLineEnable = false;
	rasterDesc.CullMode				 = D3D11_CULL_BACK;
	rasterDesc.FillMode				 = D3D11_FILL_SOLID;
	rasterDesc.DepthBias			 = 0.0f;
	rasterDesc.DepthBiasClamp		 = 0.0f;
	rasterDesc.SlopeScaledDepthBias	 = 0.0f;
	rasterDesc.DepthClipEnable		 = true;
	rasterDesc.ScissorEnable		 = false;
	rasterDesc.MultisampleEnable	 = false;
	rasterDesc.FrontCounterClockwise = false;

	result = device_->CreateRasterizerState(&rasterDesc, &rasterState_);
	if (FAILED(result))
	{
		std::cerr << "Failed to create rasterizer state!" << std::endl;
		return false;
	}

	rasterDesc.CullMode = D3D11_CULL_FRONT;
	result				= device_->CreateRasterizerState(&rasterDesc, &cullFrontRasterState_);
	if (FAILED(result))
	{
		std::cerr << "Failed to create cull front raster state!" << std::endl;
		return false;
	}

	rasterDesc.FillMode = D3D11_FILL_WIREFRAME;
	result				= device_->CreateRasterizerState(&rasterDesc, &cullFrontWireframeRasterState_);
	if (FAILED(result))
	{
		std::cerr << "Failed to create cull front wireframe raster state!" << std::endl;
	}

	rasterDesc.CullMode = D3D11_CULL_BACK;
	result				= device_->CreateRasterizerState(&rasterDesc, &wireframeRasterState_);
	if (FAILED(result))
	{
		std::cerr << "Failed to create wireframe raster state!" << std::endl;
	}

	currentDefaultRasterState_	 = rasterState_.Get();
	currentCullFrontRasterState_ = cullFrontRasterState_.Get();

	D3D11_SAMPLER_DESC samplerDesc = {};
	samplerDesc.Filter			   = D3D11_FILTER_MIN_MAG_MIP_POINT;
	samplerDesc.AddressU		   = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressV		   = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressW		   = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.ComparisonFunc	   = D3D11_COMPARISON_NEVER; // TODO: research what this does
	samplerDesc.MipLODBias		   = 0.0f;
	samplerDesc.MaxAnisotropy	   = 1;
	samplerDesc.BorderColor[0]	   = 0.0f;
	samplerDesc.BorderColor[1]	   = 0.0f;
	samplerDesc.BorderColor[2]	   = 0.0f;
	samplerDesc.BorderColor[3]	   = 0.0f;
	samplerDesc.MinLOD			   = 0;
	samplerDesc.MaxLOD			   = D3D11_FLOAT32_MAX;

	result = device_->CreateSamplerState(&samplerDesc, &samplerState_);
	if (FAILED(result))
	{
		std::cerr << "Failed to create sampler state!" << std::endl;
		return false;
	}

	D3D11_SAMPLER_DESC shadowSamplerDesc = {};
	shadowSamplerDesc.Filter			 = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	shadowSamplerDesc.AddressU			 = D3D11_TEXTURE_ADDRESS_BORDER;
	shadowSamplerDesc.AddressV			 = D3D11_TEXTURE_ADDRESS_BORDER;
	shadowSamplerDesc.AddressW			 = D3D11_TEXTURE_ADDRESS_BORDER;
	shadowSamplerDesc.ComparisonFunc	 = D3D11_COMPARISON_NEVER; // TODO: research what this does
	shadowSamplerDesc.MipLODBias		 = 0.0f;
	shadowSamplerDesc.MaxAnisotropy		 = 1;
	shadowSamplerDesc.BorderColor[0]	 = 0.0f;
	shadowSamplerDesc.BorderColor[1]	 = 0.0f;
	shadowSamplerDesc.BorderColor[2]	 = 0.0f;
	shadowSamplerDesc.BorderColor[3]	 = 1.0f;
	shadowSamplerDesc.MinLOD			 = 0;
	shadowSamplerDesc.MaxLOD			 = D3D11_FLOAT32_MAX;

	result = device_->CreateSamplerState(&shadowSamplerDesc, &shadowSamplerState_);
	if (FAILED(result))
	{
		std::cerr << "Failed to create sampler state!" << std::endl;
		return false;
	}

	context_->RSSetState(currentDefaultRasterState_);

	D3D11_VIEWPORT viewport = {};
	viewport.Width			= windowWidth_;
	viewport.Height			= windowHeight_;
	viewport.MinDepth		= 0.0f;
	viewport.MaxDepth		= 1.0f;
	viewport.TopLeftX		= 0.0f;
	viewport.TopLeftY		= 0.0f;

	viewport_ = viewport;

	context_->RSSetViewports(1, &viewport);

	DirectX::XMMATRIX temp = DirectX::XMMatrixOrthographicLH(windowWidth, windowHeight, screenNear_, screenDepth_);
	DirectX::XMStoreFloat4x4(&orthoMatrix_, temp);

	if (InitBlendState() == false)
	{
		return false;
	}

	if (InitShadowMaps() == false)
	{
		return false;
	}

	if (CreateGBuffer() == false)
	{
		return false;
	}

	if (InitPingPongBuffers() == false)
	{
		return false;
	}
	wireframeEnabled = false;
	return true;
}

void DX11Context::ClearScreen(float r, float g, float b, float a)
{
	ID3D11ShaderResourceView* nullSRVs[16] = {nullptr};

	context_->PSSetShaderResources(0, 16, nullSRVs);
	context_->VSSetShaderResources(0, 16, nullSRVs);

	ToggleLightBlend(false);
	context_->OMSetDepthStencilState(depthStencilState_.Get(), 1);
	context_->RSSetState(currentDefaultRasterState_);
	context_->RSSetViewports(1, &viewport_);

	const float color[4] = {r, g, b, a};

	context_->OMSetRenderTargets(1, renderTargetView_.GetAddressOf(), depthStencilView_.Get());
	context_->ClearRenderTargetView(renderTargetView_.Get(), color);
	context_->ClearDepthStencilView(depthStencilView_.Get(), D3D11_CLEAR_DEPTH, 1.0f, 0);
}

void DX11Context::Deferred_ClearScreen(float r, float g, float b, float a)
{
	ToggleLightBlend(false);

	context_->OMSetDepthStencilState(depthStencilState_.Get(), 0);
	context_->RSSetState(currentDefaultRasterState_);
	context_->RSSetViewports(1, &viewport_);

	const float color[4] = {r, g, b, a};

	// Unbind gBuffer SRVs
	ID3D11ShaderResourceView* SRVs[NUM_G_TEXTURES + 1] = {nullptr};
	context_->VSSetShaderResources(0, NUM_G_TEXTURES + 1, SRVs);
	context_->PSSetShaderResources(0, NUM_G_TEXTURES + 1, SRVs);

	ID3D11RenderTargetView* rawRTVs[NUM_G_TEXTURES];
	for (std::size_t i = 0; const auto& rtv : gBufferRTVs_)
	{
		rawRTVs[i] = rtv.Get();
		++i;
	}

	context_->OMSetRenderTargets(NUM_G_TEXTURES, rawRTVs, depthStencilView_.Get());

	for (auto i = 0; i < NUM_G_TEXTURES; ++i)
	{
		context_->ClearRenderTargetView(rawRTVs[i], color);
	}

	context_->ClearDepthStencilView(depthStencilView_.Get(), D3D11_CLEAR_DEPTH, 1.0f, 0);
}

void DX11Context::PrepareShadowPass()
{
	ID3D11ShaderResourceView* SRVs[16]{nullptr};
	ToggleAlphaBlend(false);

	context_->VSSetShaderResources(0, 16, SRVs);
	context_->PSSetShaderResources(0, 16, SRVs); // making sure depth texture is not bound as an srv

	context_->OMSetDepthStencilState(sunDepthStencilState_.Get(), 1);
	context_->OMSetRenderTargets(0, nullptr, sunDepthStencilView_.Get());
	context_->RSSetState(sunShadowMapRasterState_.Get());
	context_->RSSetViewports(1, &sunViewport_);
	context_->VSSetShader(nullptr, nullptr, 0);
	context_->PSSetShader(nullptr, nullptr, 0);

	context_->ClearDepthStencilView(sunDepthStencilView_.Get(), D3D11_CLEAR_DEPTH, 1.0f, 0);
}

void DX11Context::PrepareLightPass()
{
	ID3D11ShaderResourceView* rawSRVs[NUM_G_TEXTURES + 2];
	for (std::size_t i = 0; const auto& srv : gBufferSRVs_)
	{
		rawSRVs[i] = srv.Get();
		++i;
	}
	rawSRVs[NUM_G_TEXTURES + 1] = sunShadowMapSRV_.Get();

	context_->RSSetState(currentDefaultRasterState_);
	ToggleZBuffer(false);
	context_->RSSetViewports(1, &viewport_);

	context_->OMSetRenderTargets(1, lightAccumulationRTV_.GetAddressOf(), nullptr);

	constexpr float color[4]{0.0f, 0.0f, 0.0f, 1.0f};
	context_->ClearRenderTargetView(lightAccumulationRTV_.Get(), color);

	ToggleLightBlend(true);
	context_->VSSetShaderResources(0, std::size(rawSRVs), rawSRVs);
	context_->PSSetShaderResources(0, std::size(rawSRVs), rawSRVs);
	context_->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

void DX11Context::SetBackBuffer(float clearR, float clearG, float clearB, float clearA)
{
	const float color[4] = {clearR, clearG, clearB, clearA};
	// unbind srvs
	ID3D11ShaderResourceView* SRVs[NUM_G_TEXTURES + 1] = {nullptr};
	context_->VSSetShaderResources(0, NUM_G_TEXTURES + 1, SRVs);
	context_->PSSetShaderResources(0, NUM_G_TEXTURES + 1, SRVs);

	context_->OMSetDepthStencilState(depthStencilState_.Get(), 0);
	context_->RSSetState(currentDefaultRasterState_);
	context_->RSSetViewports(1, &viewport_);
	context_->OMSetRenderTargets(1, renderTargetView_.GetAddressOf(), depthStencilView_.Get());
	context_->ClearRenderTargetView(renderTargetView_.Get(), color);
	context_->ClearDepthStencilView(depthStencilView_.Get(), D3D11_CLEAR_DEPTH, 1.0f, 0);
}

void DX11Context::SetBackBuffer(bool bindDepthStencilView)
{
	// unbind srvs
	ID3D11ShaderResourceView* SRVs[NUM_G_TEXTURES + 1] = {nullptr};
	context_->VSSetShaderResources(0, NUM_G_TEXTURES + 1, SRVs);
	context_->PSSetShaderResources(0, NUM_G_TEXTURES + 1, SRVs);

	context_->OMSetDepthStencilState(depthStencilState_.Get(), 0);
	context_->RSSetState(currentDefaultRasterState_);
	context_->RSSetViewports(1, &viewport_);
	context_->OMSetRenderTargets(1,
								 renderTargetView_.GetAddressOf(),
								 bindDepthStencilView ? depthStencilView_.Get() : nullptr);
}

// similar to PrepareLightPass but doesn't clear the LAB contents
void DX11Context::SetLightAccumulationBuffer(bool bindDepthStencilBuffer)
{
	ID3D11ShaderResourceView* rawSRVs[NUM_G_TEXTURES + 2];
	for (std::size_t i = 0; const auto& srv : gBufferSRVs_)
	{
		rawSRVs[i] = srv.Get();
		++i;
	}
	rawSRVs[NUM_G_TEXTURES + 1] = sunShadowMapSRV_.Get();

	if (bindDepthStencilBuffer)
	{
		rawSRVs[NUM_G_TEXTURES] = nullptr;
		// don't bind depth as SRV if it's to be used as a depth stencil view
	}

	context_->RSSetState(currentDefaultRasterState_);
	ToggleZBuffer(false);
	context_->RSSetViewports(1, &viewport_);

	context_->OMSetRenderTargets(1,
								 lightAccumulationRTV_.GetAddressOf(),
								 bindDepthStencilBuffer ? depthStencilView_.Get() : nullptr);

	context_->VSSetShaderResources(0, std::size(rawSRVs), rawSRVs);
	context_->PSSetShaderResources(0, std::size(rawSRVs), rawSRVs);
	context_->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	ToggleLightBlend(true);
}

void DX11Context::SetPingPongBuffer()
{
	// unbind srvs as usual
	currentPingPongBuffer = currentPingPongBuffer == 1 ? 0 : 1;

	ID3D11ShaderResourceView* SRVs[1] = {nullptr};
	context_->VSSetShaderResources(0, 1, SRVs);
	context_->PSSetShaderResources(0, 1, SRVs);


	context_->RSSetState(currentDefaultRasterState_);
	context_->RSSetViewports(1, &viewport_);
	context_->OMSetRenderTargets(1, pingPongRTVs_[currentPingPongBuffer].GetAddressOf(), nullptr);

	std::uint8_t currentPingPongSRV = currentPingPongBuffer == 1 ? 0 : 1;
	context_->VSSetShaderResources(0, 1, pingPongSRVs_[currentPingPongSRV].GetAddressOf());
	context_->PSSetShaderResources(0, 1, pingPongSRVs_[currentPingPongSRV].GetAddressOf());
}

void DX11Context::ResetPingPongBuffers()
{
	// unbind srvs
	ID3D11ShaderResourceView* SRVs[16 /* just a "reasonable" amount to unbind*/] = {nullptr};
	context_->VSSetShaderResources(0, 16, SRVs);
	context_->PSSetShaderResources(0, 16, SRVs);

	static const float color[] = {0.0f, 0.0f, 0.0f, 1.0f};
	context_->ClearRenderTargetView(pingPongRTVs_[0].Get(), color);
	context_->ClearRenderTargetView(pingPongRTVs_[1].Get(), color);
}

bool DX11Context::InitPingPongBuffers()
{
	D3D11_TEXTURE2D_DESC desc = {};
	desc.Width				  = windowWidth_;
	desc.Height				  = windowHeight_;
	desc.MipLevels			  = 1;
	desc.ArraySize			  = 1;
	desc.Format				  = DXGI_FORMAT_R11G11B10_FLOAT;
	desc.SampleDesc.Count	  = 1;
	desc.SampleDesc.Quality	  = 0;
	desc.Usage				  = D3D11_USAGE_DEFAULT;
	desc.BindFlags			  = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
	desc.CPUAccessFlags		  = 0;
	desc.MiscFlags			  = 0;

	D3D11_RENDER_TARGET_VIEW_DESC rtvDesc = {};
	rtvDesc.ViewDimension				  = D3D11_RTV_DIMENSION_TEXTURE2D;
	rtvDesc.Texture2D.MipSlice			  = 0;
	rtvDesc.Format						  = desc.Format;

	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.ViewDimension					= D3D11_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels				= 1;
	srvDesc.Texture2D.MostDetailedMip		= 0;
	srvDesc.Format							= desc.Format;

	for (int i = 0; i < 2; i++)
	{
		HRESULT result = device_->CreateTexture2D(&desc, nullptr, &pingPongBuffer_[i]);
		if (FAILED(result))
		{
			return false;
		}

		result = device_->CreateRenderTargetView(pingPongBuffer_[i].Get(), &rtvDesc, &pingPongRTVs_[i]);
		if (FAILED(result))
		{
			return false;
		}

		result = device_->CreateShaderResourceView(pingPongBuffer_[i].Get(), &srvDesc, &pingPongSRVs_[i]);
		if (FAILED(result))
		{
			return false;
		}
	}

	return true;
}

void DX11Context::SwapBuffers()
{
	if (vSync_)
	{
		swapChain_->Present(1, 0);
	}
	else
	{
		swapChain_->Present(0, 0);
	}
}
void DX11Context::ToggleAlphaBlend(bool enable)
{
	static constexpr float blendFactor[] = {0.0f, 0.0f, 0.0f, 0.0f};
	static constexpr UINT  sampleMask	 = 0xffffffff;

	if (context_ == nullptr)
	{
		return;
	}

	context_->OMSetBlendState(enable ? alphaBlendState_.Get() : nullptr, blendFactor, sampleMask);
}

void DX11Context::ToggleLightBlend(bool enable)
{
	static constexpr float blendFactor[] = {0.0f, 0.0f, 0.0f, 0.0f};
	static constexpr UINT  sampleMask	 = 0xffffffff;

	if (context_ == nullptr)
	{
		return;
	}

	context_->OMSetBlendState(enable ? lightBlendState_.Get() : nullptr, blendFactor, sampleMask);
}

void DX11Context::ToggleZBuffer(bool enable)
{
	context_->OMSetDepthStencilState(enable ? depthStencilState_.Get() : depthDisabledStencilState_.Get(), 0);
}

void DX11Context::ToggleSkyDepthStencil(bool enable)
{
	context_->OMSetDepthStencilState(enable ? skyDepthStencilState_.Get() : depthStencilState_.Get(), 0);
}

void DX11Context::ToggleFrontCulling(bool enable)
{
	context_->RSSetState(enable ? currentCullFrontRasterState_ : currentDefaultRasterState_);
}

void DX11Context::ToggleWireframe(bool enable)
{
	currentDefaultRasterState_	 = enable ? wireframeRasterState_.Get() : rasterState_.Get();
	currentCullFrontRasterState_ = enable ? cullFrontWireframeRasterState_.Get() : cullFrontRasterState_.Get();
	wireframeEnabled			 = enable;
}

void DX11Context::OnWindowResize(std::uint32_t width, std::uint32_t height)
{
	if (width == windowWidth_ && height == windowHeight_)
	{
		// If for some reason this is called without any actual resizing happening, do nothing
		return;
	}

	windowWidth_  = width;
	windowHeight_ = height;

	// Recreate swap chain, depth buffer, viewport
	ResizeSwapChain();
	CreateDepthBuffer();

	viewport_.Width	 = static_cast<FLOAT>(windowWidth_);
	viewport_.Height = static_cast<FLOAT>(windowHeight_);
	context_->RSSetViewports(1, &viewport_);

	CreateGBuffer();
	InitPingPongBuffers();
	DirectX::XMMATRIX temp = DirectX::XMMatrixOrthographicLH(static_cast<float>(windowWidth_),
															 static_cast<float>(windowHeight_),
															 screenNear_,
															 screenDepth_);
	DirectX::XMStoreFloat4x4(&orthoMatrix_, temp);
}

bool DX11Context::ResizeSwapChain()
{
	// Release any existing references
	context_->ClearState();
	renderTargetView_.Reset();
	depthStencilView_.Reset();
	depthStencilState_.Reset();
	depthDisabledStencilState_.Reset();


	HRESULT result = swapChain_->ResizeBuffers(2, windowWidth_, windowHeight_, DXGI_FORMAT_UNKNOWN, 0);
	if (FAILED(result))
	{
		std::cerr << "Failed to resize swap chain!" << std::endl;
		return false;
	}

	Microsoft::WRL::ComPtr<ID3D11Texture2D> backBuffer;
	result = swapChain_->GetBuffer(0, __uuidof(ID3D11Texture2D), &backBuffer);
	if (FAILED(result))
	{
		std::cerr << "Failed to get back buffer pointer!" << std::endl;
		return false;
	}

	result = device_->CreateRenderTargetView(backBuffer.Get(), nullptr, &renderTargetView_);
	if (FAILED(result))
	{
		std::cerr << "Failed to create back buffer render target view!" << std::endl;
		return false;
	}

	return true;
}

bool DX11Context::CreateDepthBuffer()
{
	D3D11_TEXTURE2D_DESC depthBufferDesc = {};
	depthBufferDesc.Width				 = windowWidth_;
	depthBufferDesc.Height				 = windowHeight_;
	depthBufferDesc.MipLevels			 = 1;
	depthBufferDesc.ArraySize			 = 1;
	depthBufferDesc.Format				 = DXGI_FORMAT_R24G8_TYPELESS;
	depthBufferDesc.SampleDesc.Count	 = 1;
	depthBufferDesc.SampleDesc.Quality	 = 0;
	depthBufferDesc.Usage				 = D3D11_USAGE_DEFAULT;
	depthBufferDesc.BindFlags			 = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
	depthBufferDesc.CPUAccessFlags		 = 0;
	depthBufferDesc.MiscFlags			 = 0;

	HRESULT result = device_->CreateTexture2D(&depthBufferDesc, nullptr, &depthStencilBuffer_);
	if (FAILED(result))
	{
		std::cerr << "Failed to create depth stencil buffer!" << std::endl;
		return false;
	}

	D3D11_DEPTH_STENCIL_DESC depthStencilDesc = {};
	depthStencilDesc.DepthEnable			  = true;
	depthStencilDesc.DepthWriteMask			  = D3D11_DEPTH_WRITE_MASK_ALL;
	depthStencilDesc.DepthFunc				  = D3D11_COMPARISON_LESS;
	depthStencilDesc.StencilEnable			  = true;
	depthStencilDesc.StencilReadMask		  = 0xFF;
	depthStencilDesc.StencilWriteMask		  = 0xFF;

	// Stencil operations if pixel is front-facing
	depthStencilDesc.FrontFace.StencilFailOp	  = D3D11_STENCIL_OP_KEEP;
	depthStencilDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_INCR;
	depthStencilDesc.FrontFace.StencilPassOp	  = D3D11_STENCIL_OP_KEEP;
	depthStencilDesc.FrontFace.StencilFunc		  = D3D11_COMPARISON_ALWAYS;

	// Stencil operations if pixel is back-facing
	depthStencilDesc.BackFace.StencilFailOp		 = D3D11_STENCIL_OP_KEEP;
	depthStencilDesc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_DECR;
	depthStencilDesc.BackFace.StencilPassOp		 = D3D11_STENCIL_OP_KEEP;
	depthStencilDesc.BackFace.StencilFunc		 = D3D11_COMPARISON_ALWAYS;

	result = device_->CreateDepthStencilState(&depthStencilDesc, &depthStencilState_);
	if (FAILED(result))
	{
		std::cerr << "Failed to create depth stencil state!" << std::endl;
		return false;
	}

	context_->OMSetDepthStencilState(depthStencilState_.Get(), 1);

	D3D11_DEPTH_STENCIL_DESC depthDisabledStencilDesc = depthStencilDesc;
	depthDisabledStencilDesc.DepthEnable			  = false;

	result = device_->CreateDepthStencilState(&depthDisabledStencilDesc, &depthDisabledStencilState_);
	if (FAILED(result))
	{
		std::cerr << "Failed to create depth disabled stencil state!" << std::endl;
		return false;
	}

	D3D11_DEPTH_STENCIL_DESC skyDepthStencilDesc = depthStencilDesc;
	skyDepthStencilDesc.DepthWriteMask			 = D3D11_DEPTH_WRITE_MASK_ZERO;
	skyDepthStencilDesc.DepthFunc				 = D3D11_COMPARISON_LESS_EQUAL;

	result = device_->CreateDepthStencilState(&skyDepthStencilDesc, &skyDepthStencilState_);
	if (FAILED(result))
	{
		std::cerr << "Failed to create sky depth stencil state!" << std::endl;
	}

	D3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc = {};
	depthStencilViewDesc.Format						   = DXGI_FORMAT_D24_UNORM_S8_UINT;
	depthStencilViewDesc.ViewDimension				   = D3D11_DSV_DIMENSION_TEXTURE2D;
	depthStencilViewDesc.Texture2D.MipSlice			   = 0;

	result = device_->CreateDepthStencilView(depthStencilBuffer_.Get(), &depthStencilViewDesc, &depthStencilView_);
	if (FAILED(result))
	{
		std::cerr << "Failed to create depth stencil view!" << std::endl;
		return false;
	}

	context_->OMSetRenderTargets(1, renderTargetView_.GetAddressOf(), depthStencilView_.Get());

	return true;
}

bool DX11Context::CreateRasterizerState(D3D11_RASTERIZER_DESC rasterDesc)
{
	return true;
}

bool DX11Context::CreateDepthStencilState()
{
	return true;
}

bool DX11Context::InitBlendState()
{
	D3D11_BLEND_DESC blendDesc = {};

	blendDesc.RenderTarget[0].BlendEnable = true;
	blendDesc.RenderTarget[0].SrcBlend	  = D3D11_BLEND_SRC_ALPHA;
	blendDesc.RenderTarget[0].DestBlend	  = D3D11_BLEND_INV_SRC_ALPHA;
	blendDesc.RenderTarget[0].BlendOp	  = D3D11_BLEND_OP_ADD;

	blendDesc.RenderTarget[0].SrcBlendAlpha	 = D3D11_BLEND_ONE;
	blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
	blendDesc.RenderTarget[0].BlendOpAlpha	 = D3D11_BLEND_OP_ADD;

	blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

	if (device_ == nullptr)
	{
		return false;
	}

	HRESULT result = device_->CreateBlendState(&blendDesc, &alphaBlendState_);
	if (FAILED(result))
	{
		std::cerr << "Failed to create alpha blendState!" << std::endl;
		return false;
	}

	blendDesc.RenderTarget[0].SrcBlend	= D3D11_BLEND_ONE;
	blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_ONE;
	blendDesc.RenderTarget[0].BlendOp	= D3D11_BLEND_OP_ADD;

	result = device_->CreateBlendState(&blendDesc, &lightBlendState_);
	if (FAILED(result))
	{
		std::cerr << "Failed to create light blendState!" << std::endl;
		return false;
	}
	return true;
}

bool DX11Context::CreateGBuffer()
{
	static constexpr DXGI_FORMAT FORMATS[NUM_G_TEXTURES]{
		DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, // albedo + pom self shadow
		DXGI_FORMAT_R10G10B10A2_UNORM,	 // normal
		DXGI_FORMAT_R8G8B8A8_UNORM,		 // orm + voxel lighting
		DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, // emissive
	};

	D3D11_TEXTURE2D_DESC gBufferTextureDesc = {};
	gBufferTextureDesc.Width				= windowWidth_;
	gBufferTextureDesc.Height				= windowHeight_;
	gBufferTextureDesc.MipLevels			= 1;
	gBufferTextureDesc.ArraySize			= 1;
	gBufferTextureDesc.SampleDesc.Count		= 1;
	gBufferTextureDesc.Usage				= D3D11_USAGE_DEFAULT;
	gBufferTextureDesc.BindFlags			= D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
	gBufferTextureDesc.CPUAccessFlags		= 0;
	gBufferTextureDesc.MiscFlags			= 0;

	D3D11_RENDER_TARGET_VIEW_DESC renderTargetViewDesc = {};
	renderTargetViewDesc.ViewDimension				   = D3D11_RTV_DIMENSION_TEXTURE2D;
	renderTargetViewDesc.Texture2D.MipSlice			   = 0;

	D3D11_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc = {};
	shaderResourceViewDesc.ViewDimension				   = D3D11_SRV_DIMENSION_TEXTURE2D;
	shaderResourceViewDesc.Texture2D.MostDetailedMip	   = 0;
	shaderResourceViewDesc.Texture2D.MipLevels			   = 1;

	HRESULT result;
	for (std::size_t i = 0; i < NUM_G_TEXTURES; ++i)
	{
		gBufferTextureDesc.Format	  = FORMATS[i];
		renderTargetViewDesc.Format	  = FORMATS[i];
		shaderResourceViewDesc.Format = FORMATS[i];

		result = device_->CreateTexture2D(&gBufferTextureDesc, nullptr, &gBufferTextures_[i]);

		if (FAILED(result))
		{
			std::cerr << "Failed to create gBuffer Texture: " + std::to_string(i) + "!" << std::endl;
			return false;
		}

		result = device_->CreateRenderTargetView(gBufferTextures_[i].Get(), &renderTargetViewDesc, &gBufferRTVs_[i]);
		if (FAILED(result))
		{
			std::cerr << "Failed to create gBufferRTV: " + std::to_string(i) + "!" << std::endl;
			return false;
		}

		result = device_->CreateShaderResourceView(gBufferTextures_[i].Get(),
												   &shaderResourceViewDesc,
												   &gBufferSRVs_[i]);
		if (FAILED(result))
		{
			std::cerr << "Failed to create gBufferSRV: " + std::to_string(i) + "!" << std::endl;
			return false;
		}
	}

	shaderResourceViewDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;

	result = device_->CreateShaderResourceView(depthStencilBuffer_.Get(),
											   &shaderResourceViewDesc,
											   &gBufferSRVs_[NUM_G_TEXTURES]);

	if (FAILED(result))
	{
		std::cerr << "Failed to create Depth SRV" << std::endl;
		return false;
	}

	// Create the light accumulation buffer while we're here as well
	gBufferTextureDesc.Format	  = DXGI_FORMAT_R11G11B10_FLOAT;
	shaderResourceViewDesc.Format = gBufferTextureDesc.Format;
	renderTargetViewDesc.Format	  = gBufferTextureDesc.Format;

	result = device_->CreateTexture2D(&gBufferTextureDesc, nullptr, &lightAccumulationBuffer_);
	if (FAILED(result))
	{
		std::cerr << "Failed to create Light Accumulation Buffer";
		return false;
	}

	result = device_->CreateRenderTargetView(lightAccumulationBuffer_.Get(),
											 &renderTargetViewDesc,
											 &lightAccumulationRTV_);
	if (FAILED(result))
	{
		std::cerr << "Failed to create Light Accumulation RTV";
		return false;
	}

	result = device_->CreateShaderResourceView(lightAccumulationBuffer_.Get(),
											   &shaderResourceViewDesc,
											   &lightAccumulationSRV_);
	if (FAILED(result))
	{
		std::cerr << "Failed to create Light Accumulation SRV";
		return false;
	}

	return true;
}

bool DX11Context::InitShadowMaps()
{
	constexpr static std::size_t SHADOWMAP_RESOLUTION = 8192;
	D3D11_TEXTURE2D_DESC		 shadowMapDesc		  = {};
	shadowMapDesc.Width								  = SHADOWMAP_RESOLUTION;
	shadowMapDesc.Height							  = SHADOWMAP_RESOLUTION;
	shadowMapDesc.Format							  = DXGI_FORMAT_R32_TYPELESS;
	shadowMapDesc.MipLevels							  = 1;
	shadowMapDesc.ArraySize							  = 1;
	shadowMapDesc.SampleDesc.Count					  = 1;
	shadowMapDesc.Usage								  = D3D11_USAGE_DEFAULT;
	shadowMapDesc.BindFlags							  = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
	shadowMapDesc.CPUAccessFlags					  = 0;
	shadowMapDesc.MiscFlags							  = 0;

	if (device_ == nullptr)
	{
		return false;
	}

	HRESULT result = device_->CreateTexture2D(&shadowMapDesc, nullptr, &sunShadowMap_);
	if (FAILED(result))
	{
		return false;
	}

	D3D11_DEPTH_STENCIL_DESC depthStencilDesc = {};
	depthStencilDesc.DepthEnable			  = true;
	depthStencilDesc.DepthWriteMask			  = D3D11_DEPTH_WRITE_MASK_ALL;
	depthStencilDesc.DepthFunc				  = D3D11_COMPARISON_LESS;
	depthStencilDesc.StencilEnable			  = false;
	depthStencilDesc.StencilReadMask		  = 0xFF;
	depthStencilDesc.StencilWriteMask		  = 0xFF;

	// Stencil operations if pixel is front-facing
	depthStencilDesc.FrontFace.StencilFailOp	  = D3D11_STENCIL_OP_KEEP;
	depthStencilDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_INCR;
	depthStencilDesc.FrontFace.StencilPassOp	  = D3D11_STENCIL_OP_KEEP;
	depthStencilDesc.FrontFace.StencilFunc		  = D3D11_COMPARISON_ALWAYS;

	// Stencil operations if pixel is back-facing
	depthStencilDesc.BackFace.StencilFailOp		 = D3D11_STENCIL_OP_KEEP;
	depthStencilDesc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_DECR;
	depthStencilDesc.BackFace.StencilPassOp		 = D3D11_STENCIL_OP_KEEP;
	depthStencilDesc.BackFace.StencilFunc		 = D3D11_COMPARISON_ALWAYS;

	result = device_->CreateDepthStencilState(&depthStencilDesc, &sunDepthStencilState_);
	if (FAILED(result))
	{
		return false;
	}

	D3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc = {};
	depthStencilViewDesc.Format						   = DXGI_FORMAT_D32_FLOAT;
	depthStencilViewDesc.ViewDimension				   = D3D11_DSV_DIMENSION_TEXTURE2D;
	depthStencilViewDesc.Texture2D.MipSlice			   = 0;

	result = device_->CreateDepthStencilView(sunShadowMap_.Get(), &depthStencilViewDesc, &sunDepthStencilView_);
	if (FAILED(result))
	{
		return false;
	}

	D3D11_RASTERIZER_DESC rasterDesc = {};
	rasterDesc.AntialiasedLineEnable = false;
	rasterDesc.CullMode				 = D3D11_CULL_FRONT;
	rasterDesc.FillMode				 = D3D11_FILL_SOLID;
	rasterDesc.DepthBias			 = 50;
	rasterDesc.DepthBiasClamp		 = 0.0f;
	rasterDesc.SlopeScaledDepthBias	 = 2.0f;
	rasterDesc.DepthClipEnable		 = true;
	rasterDesc.ScissorEnable		 = false;
	rasterDesc.MultisampleEnable	 = false;
	rasterDesc.FrontCounterClockwise = false;

	result = device_->CreateRasterizerState(&rasterDesc, &sunShadowMapRasterState_);
	if (FAILED(result))
	{
		return false;
	}

	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.ViewDimension					= D3D11_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Format							= DXGI_FORMAT_R32_FLOAT;
	srvDesc.Texture2D.MostDetailedMip		= 0;
	srvDesc.Texture2D.MipLevels				= 1;

	result = device_->CreateShaderResourceView(sunShadowMap_.Get(), &srvDesc, &sunShadowMapSRV_);
	if (FAILED(result))
	{
		return false;
	}


	D3D11_VIEWPORT viewport = {};
	viewport.Width			= SHADOWMAP_RESOLUTION;
	viewport.Height			= SHADOWMAP_RESOLUTION;
	viewport.MinDepth		= 0.0f;
	viewport.MaxDepth		= 1.0f;
	viewport.TopLeftX		= 0.0f;
	viewport.TopLeftY		= 0.0f;

	sunViewport_ = viewport;

	return true;
}

const std::array<Microsoft::WRL::ComPtr<ID3D11Texture2D>, DX11Context::NUM_G_TEXTURES>& DX11Context::
	GetGBufferTextures() const
{
	return gBufferTextures_;
}
const std::array<Microsoft::WRL::ComPtr<ID3D11RenderTargetView>, DX11Context::NUM_G_TEXTURES>& DX11Context::
	GetGBufferRTVs() const
{
	return gBufferRTVs_;
}
const std::array<Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>, DX11Context::NUM_G_TEXTURES + 1>& DX11Context::
	GetGBufferSRVs() const
{
	return gBufferSRVs_;
}
