#pragma once
#include <DirectXCollision.h>
#include <DirectXMath.h>


#include "../Core/BlockRaycastResult.h"
#include "CBuffer.h"
#include "DX11Context.h"
#include "DebugRenderMode.h"
#include "FrameConstants.h"
#include "SkyBuffer.h"
#include "TextureManager.h"
#include "UIConstants.h"

class Renderer
{
public:
	bool Initialize(HWND handle, int windowWidth, int windowHeight, bool fullscreen, bool vSync);
	void LoadFont(const std::wstring& fontPath);

	void BeginScene();
	void EndScene();
#ifdef _DEBUG
	void RenderTestCube(DirectX::XMMATRIX view, DirectX::XMMATRIX projection);
#endif
	void RenderSky();
	void RenderWorld(const class World& world, class Camera& boundingFrustum);
	void RenderUI(class Player& player);

	// TODO: make the input layout creation a separate function, it's currently leading to some dirty stuff
	bool CompileVertexShader(const std::wstring&						  shaderPath,
							 const std::string&							  entryPoint,
							 const std::vector<D3D11_INPUT_ELEMENT_DESC>& inputLayoutDesc,
							 Microsoft::WRL::ComPtr<ID3D11VertexShader>&  outVertexShader,
							 Microsoft::WRL::ComPtr<ID3D11InputLayout>&	  outInputLayout);

	bool CompilePixelShader(const std::wstring&						   shaderPath,
							const std::string&						   entryPoint,
							Microsoft::WRL::ComPtr<ID3D11PixelShader>& outPixelShader,
							const D3D_SHADER_MACRO*					   defines = nullptr);

	void BindShaders(ID3D11VertexShader* vertexShader, ID3D11PixelShader* pixelShader, ID3D11InputLayout* inputLayout);
	void DrawBlockOutline(const BlockRaycastResult& result);

	void UpdateFrameConstants(const DirectX::XMMATRIX& view,
							  const DirectX::XMMATRIX& projection,
							  const DirectX::XMMATRIX& lightSpaceMatrix,
							  const DirectX::XMVECTOR& position,
							  const DirectX::XMVECTOR& skyLightDir);

	void BindBlockSRVs();
	void SetSamplers();
	void UpdateSkyConstants(const DirectX::XMVECTOR skyLightDirection, float timeOfDay);
	void UpdatePointLightBuffer(std::span<const struct PointLightGPU> pointLights);
	void ToggleWireframe(bool enable) { dx11Context_.ToggleWireframe(enable); };
	void OnWindowResize(std::uint32_t width, std::uint32_t height) { dx11Context_.OnWindowResize(width, height); };
	void SetDebugRenderMode(DebugRenderMode mode);

private:
	bool CreateOutlineBuffers();
	bool CreateSphereBuffers();
	bool CreatePointLightBuffer();
	void LightingPass();
	void ShadowPass();
	void AmbientLightPass();
	void DirectionalLightPass();
	void PointLightPass();
	void RenderBloom();
	void CompositePass();
	void CalculateLightSpaceMatrix(Camera& camera);
	void DebugRender();

	DX11Context	   dx11Context_;
	TextureManager textureManager_;

	DirectX::XMFLOAT3		 lightDir_;
	DirectX::XMFLOAT4X4		 lightSpaceMatrix_;
	DirectX::BoundingFrustum sunFrustum_; // for culling chunks

#ifdef _DEBUG
	// Test stuff only, no need to put it into the release build
	void InitTestCube();

	// Buffers
	Microsoft::WRL::ComPtr<ID3D11Buffer> testVertexBuffer_;
	Microsoft::WRL::ComPtr<ID3D11Buffer> testIndexBuffer_;
#endif

	CBuffer<FrameConstants>		 frameConstantsBufferNew_;
	CBuffer<DirectX::XMFLOAT4X4> objectConstantsBufferNew_;
	CBuffer<SkyBuffer>			 skyConstantsBufferNew_;

	CBuffer<UIConstants>	   UIConstantsBuffer_;
	CBuffer<std::uint32_t>	   UIMatIdBuffer_;
	CBuffer<DirectX::XMFLOAT4> hotbarColorBuffer_;
	CBuffer<std::uint32_t>	   debugModeCBuffer_;


	Microsoft::WRL::ComPtr<ID3D11Buffer> blockOutlineVertexBuffer_;
	Microsoft::WRL::ComPtr<ID3D11Buffer> blockOutlineIndexBuffer_;

	// Layout
	Microsoft::WRL::ComPtr<ID3D11InputLayout> chunkInputLayout_;
	Microsoft::WRL::ComPtr<ID3D11InputLayout> simpleVertexInputLayout_;
	Microsoft::WRL::ComPtr<ID3D11InputLayout> gBufferInputLayout_;
	Microsoft::WRL::ComPtr<ID3D11InputLayout> pointLightInputLayout_;

	// Shaders
	Microsoft::WRL::ComPtr<ID3D11VertexShader> outlineVertexShader_;
	Microsoft::WRL::ComPtr<ID3D11PixelShader>  outlinePixelShader_;
	UINT									   outlineIndexCount_ = 0;

	Microsoft::WRL::ComPtr<ID3D11VertexShader> skyVertexShader_;
	Microsoft::WRL::ComPtr<ID3D11PixelShader>  skyPixelShader_;
	UINT									   skyIndexCount_ = 0;

	Microsoft::WRL::ComPtr<ID3D11VertexShader> shadowVertexShader_;

	Microsoft::WRL::ComPtr<ID3D11VertexShader> hotbarVertexShader_;
	Microsoft::WRL::ComPtr<ID3D11PixelShader>  hotbarPixelShader_;

	Microsoft::WRL::ComPtr<ID3D11VertexShader> hotbarBlockVertexShader_;
	Microsoft::WRL::ComPtr<ID3D11PixelShader>  hotbarBlockPixelShader_;

	// Deferred stuff
	Microsoft::WRL::ComPtr<ID3D11VertexShader> geometryPassVertexShader_;
	Microsoft::WRL::ComPtr<ID3D11PixelShader>  geometryPassPixelShader_;

	Microsoft::WRL::ComPtr<ID3D11VertexShader> sunLightVertexShader_;
	Microsoft::WRL::ComPtr<ID3D11PixelShader>  sunLightPixelShader_;

	Microsoft::WRL::ComPtr<ID3D11VertexShader> pointLightVertexShader_;
	Microsoft::WRL::ComPtr<ID3D11PixelShader>  pointLightPixelShader_;

	Microsoft::WRL::ComPtr<ID3D11VertexShader> ambientLightVertexShader_;
	Microsoft::WRL::ComPtr<ID3D11PixelShader>  ambientLightPixelShader_;

	Microsoft::WRL::ComPtr<ID3D11VertexShader> bloomVertexShader_;
	Microsoft::WRL::ComPtr<ID3D11PixelShader>  bloomHorizontalBlurPixelShader_;
	Microsoft::WRL::ComPtr<ID3D11PixelShader>  bloomVerticalBlurPixelShader_;

	Microsoft::WRL::ComPtr<ID3D11VertexShader> compositeVertexShader_;
	Microsoft::WRL::ComPtr<ID3D11PixelShader>  compositePixelShader_;

	Microsoft::WRL::ComPtr<ID3D11VertexShader> debugModeVertexShader_;
	Microsoft::WRL::ComPtr<ID3D11PixelShader>  debugModePixelShader_;

	// Used for the point light volume
	Microsoft::WRL::ComPtr<ID3D11Buffer> sphereVertexBuffer_;
	Microsoft::WRL::ComPtr<ID3D11Buffer> sphereIndexBuffer_;
	UINT								 sphereIndexCount_ = 0;

	Microsoft::WRL::ComPtr<ID3D11Buffer> pointLightPerInstanceBuffer_;
	std::size_t							 currentVisiblePointLights_ = 0;

	DebugRenderMode currentDebugRenderMode_ = DebugRenderMode::NONE;

public:
	// Getters
	[[nodiscard]] float GetNearZ() const { return dx11Context_.GetNearZ(); }
	[[nodiscard]] float GetFarZ() const { return dx11Context_.GetFarZ(); }
	[[nodiscard]] auto	GetDevice() const { return dx11Context_.GetDevice(); };
	[[nodiscard]] auto	GetDeviceContext() const { return dx11Context_.GetDeviceContext(); }
};
