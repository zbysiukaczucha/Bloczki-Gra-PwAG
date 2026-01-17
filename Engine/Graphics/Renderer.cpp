#include "Renderer.h"

#include <d3dcompiler.h>
#include <iostream>

#include "../Core/Player.h"
#include "../Utils/SkyColor.h"
#include "../Utils/TestCube.h"
#include "../Utils/ThirdParty/stb_truetype.h"
#include "../World/World.h"
#include "Camera.h"
#include "FrameConstants.h"
#include "SkyBuffer.h"

#define STB_TRUETYPE_IMPLEMENTATION

bool Renderer::Initialize(HWND handle, int windowWidth, int windowHeight, bool fullscreen, bool vSync)
{
	bool didInitSucceed = dx11Context_.Initialize(handle, windowWidth, windowHeight, fullscreen, vSync);
	if (didInitSucceed == false)
	{
		return false;
	}

	didInitSucceed = textureManager_.Initialize(dx11Context_.GetDevice(),
												dx11Context_.GetDeviceContext(),
												"./Resources/Textures/");
	if (didInitSucceed == false)
	{
		return false;
	}

	auto device = dx11Context_.GetDevice().Get();
	if (frameConstantsBufferNew_.Create(device) == false)
	{
		return false;
	}

	if (objectConstantsBufferNew_.Create(device) == false)
	{
		return false;
	}

	if (skyConstantsBufferNew_.Create(device) == false)
	{
		return false;
	}

	std::vector<D3D11_INPUT_ELEMENT_DESC> layoutDesc = {
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"BITANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"TEXCOORD", 1, DXGI_FORMAT_R32_UINT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"TEXCOORD", 2, DXGI_FORMAT_R8_UINT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
	};

	layoutDesc	   = {{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0}};
	didInitSucceed = CompileVertexShader(L"./Engine/Graphics/Shaders/SunShadows.hlsl",
										 "VS_Main",
										 layoutDesc,
										 shadowVertexShader_,
										 simpleVertexInputLayout_);
	if (didInitSucceed == false)
	{
		return false;
	}

	if (CreateOutlineBuffers() == false)
	{
		return false;
	}

	layoutDesc = {{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0}};

	didInitSucceed = CompileVertexShader(L"./Engine/Graphics/Shaders/BlockOutline.hlsl",
										 "VS_Main",
										 layoutDesc,
										 outlineVertexShader_,
										 simpleVertexInputLayout_);

	if (didInitSucceed == false)
	{
		return false;
	}

	didInitSucceed = CompilePixelShader(L"./Engine/Graphics/Shaders/BlockOutline.hlsl", "PS_Main", outlinePixelShader_);
	if (didInitSucceed == false)
	{
		return false;
	}

	Microsoft::WRL::ComPtr<ID3D11InputLayout> placeholder{};
	didInitSucceed = CompileVertexShader(L"./Engine/Graphics/Shaders/Sky.hlsl",
										 "VS_Main",
										 layoutDesc,
										 skyVertexShader_,
										 placeholder);
	if (didInitSucceed == false)
	{
		return false;
	}

	didInitSucceed = CompilePixelShader(L"./Engine/Graphics/Shaders/Sky.hlsl", "PS_Main", skyPixelShader_);
	if (didInitSucceed == false)
	{
		return false;
	}

	// Deferred stuff

	layoutDesc = {
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"BITANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"TEXCOORD", 1, DXGI_FORMAT_R32_UINT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"TEXCOORD", 2, DXGI_FORMAT_R8_UINT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
	};

	didInitSucceed = CompileVertexShader(L"./Engine/Graphics/Shaders/GeometryPass.hlsl",
										 "VS_Main",
										 layoutDesc,
										 geometryPassVertexShader_,
										 gBufferInputLayout_);
	if (didInitSucceed == false)
	{
		return false;
	}

	didInitSucceed = CompilePixelShader(L"./Engine/Graphics/Shaders/GeometryPass.hlsl",
										"PS_Main",
										geometryPassPixelShader_);
	if (didInitSucceed == false)
	{
		return false;
	}

	didInitSucceed = CompileVertexShader(L"./Engine/Graphics/Shaders/Sunlight.hlsl",
										 "VS_Main",
										 layoutDesc,
										 sunLightVertexShader_,
										 placeholder);
	if (didInitSucceed == false)
	{
		return false;
	}

	didInitSucceed = CompilePixelShader(L"./Engine/Graphics/Shaders/Sunlight.hlsl", "PS_Main", sunLightPixelShader_);
	if (didInitSucceed == false)
	{
		return false;
	}

	didInitSucceed = CompileVertexShader(L"./Engine/Graphics/Shaders/Ambient.hlsl",
										 "VS_Main",
										 layoutDesc,
										 ambientLightVertexShader_,
										 placeholder);
	if (didInitSucceed == false)
	{
		return false;
	}

	didInitSucceed = CompilePixelShader(L"./Engine/Graphics/Shaders/Ambient.hlsl", "PS_Main", ambientLightPixelShader_);
	if (didInitSucceed == false)
	{
		return false;
	}

	/*
	 * Position        - per vertex
	 * light center    - per instance
	 * light radius    - per instance
	 * light color     - per instance
	 * light intensity - per instance
	 */
	layoutDesc = {
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"TEXCOORD", 0, DXGI_FORMAT_R32G32B32_FLOAT, 1, 0, D3D11_INPUT_PER_INSTANCE_DATA, 1},
		{"TEXCOORD", 1, DXGI_FORMAT_R32_FLOAT, 1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1},
		{"COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1},
		{"TEXCOORD", 2, DXGI_FORMAT_R32_FLOAT, 1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1},
	};

	didInitSucceed = CompileVertexShader(L"./Engine/Graphics/Shaders/PointLights.hlsl",
										 "VS_Main",
										 layoutDesc,
										 pointLightVertexShader_,
										 pointLightInputLayout_);

	if (didInitSucceed == false)
	{
		return false;
	}

	didInitSucceed = CompilePixelShader(L"./Engine/Graphics/Shaders/PointLights.hlsl",
										"PS_Main",
										pointLightPixelShader_);
	if (didInitSucceed == false)
	{
		return false;
	}

	didInitSucceed = CompileVertexShader(L"./Engine/Graphics/Shaders/Bloom.hlsl",
										 "VS_Main",
										 layoutDesc,
										 bloomVertexShader_,
										 placeholder);

	if (didInitSucceed == false)
	{
		return false;
	}

	didInitSucceed = CompilePixelShader(L"./Engine/Graphics/Shaders/Bloom.hlsl",
										"PS_Main",
										bloomHorizontalBlurPixelShader_);
	if (didInitSucceed == false)
	{
		return false;
	}

	D3D_SHADER_MACRO defines[] = {"VERTICAL", "1", nullptr, nullptr};
	didInitSucceed			   = CompilePixelShader(L"./Engine/Graphics/Shaders/Bloom.hlsl",
										"PS_Main",
										bloomVerticalBlurPixelShader_,
										defines);
	if (didInitSucceed == false)
	{
		return false;
	}

	didInitSucceed = CompileVertexShader(L"./Engine/Graphics/Shaders/Composite.hlsl",
										 "VS_Main",
										 layoutDesc,
										 compositeVertexShader_,
										 placeholder);
	if (didInitSucceed == false)
	{
		return false;
	}

	didInitSucceed = CompilePixelShader(L"./Engine/Graphics/Shaders/Composite.hlsl", "PS_Main", compositePixelShader_);
	if (didInitSucceed == false)
	{
		return false;
	}


	didInitSucceed = CompileVertexShader(L"./Engine/Graphics/Shaders/Hotbar.hlsl",
										 "VS_Main",
										 layoutDesc,
										 hotbarVertexShader_,
										 placeholder);
	if (didInitSucceed == false)
	{
		return false;
	}

	didInitSucceed = CompilePixelShader(L"./Engine/Graphics/Shaders/Hotbar.hlsl", "PS_Main", hotbarPixelShader_);
	if (didInitSucceed == false)
	{
		return false;
	}
	didInitSucceed = CompileVertexShader(L"./Engine/Graphics/Shaders/Hotbar.hlsl",
										 "VS_Textured",
										 layoutDesc,
										 hotbarBlockVertexShader_,
										 placeholder);
	if (didInitSucceed == false)
	{
		return false;
	}

	didInitSucceed = CompilePixelShader(L"./Engine/Graphics/Shaders/Hotbar.hlsl",
										"PS_Textured",
										hotbarBlockPixelShader_);
	if (didInitSucceed == false)
	{
		return false;
	}

	didInitSucceed = CompileVertexShader(L"./Engine/Graphics/Shaders/Debug.hlsl",
										 "VS_Main",
										 layoutDesc,
										 debugModeVertexShader_,
										 placeholder);
	if (didInitSucceed == false)
	{
		return false;
	}

	didInitSucceed = CompilePixelShader(L"./Engine/Graphics/Shaders/Debug.hlsl", "PS_Main", debugModePixelShader_);
	if (didInitSucceed == false)
	{
		return false;
	}

	if (debugModeCBuffer_.Create(device) == false)
	{
		return false;
	}

	if (CreateSphereBuffers() == false)
	{
		return false;
	}

	if (CreatePointLightBuffer() == false)
	{
		return false;
	}

	if (UIConstantsBuffer_.Create(device) == false)
	{
		return false;
	}

	if (UIMatIdBuffer_.Create(device) == false)
	{
		return false;
	}

	if (hotbarColorBuffer_.Create(device) == false)
	{
		return false;
	}

	return true;
}

void Renderer::LoadFont(const std::wstring& fontPath)
{
}

void Renderer::BeginScene()
{
	// dx11Context_.ClearScreen(78 / 255.f, 159 / 255.f, 229 / 255.f, 1.0f);
	dx11Context_.ClearScreen(0, 0, 0, 1.0f);
}

void Renderer::EndScene()
{
	dx11Context_.SwapBuffers();
}

void Renderer::UpdateFrameConstants(const DirectX::XMMATRIX& view,
									const DirectX::XMMATRIX& projection,
									const DirectX::XMMATRIX& lightSpaceMatrix,
									const DirectX::XMVECTOR& position,
									const DirectX::XMVECTOR& skyLightDir)
{
	using namespace DirectX;
	auto context = dx11Context_.GetDeviceContext();

	static FrameConstants frameConstants{};

	XMStoreFloat4x4(&frameConstants.viewMatrix, XMMatrixTranspose(view));
	XMStoreFloat4x4(&frameConstants.projectionMatrix, XMMatrixTranspose(projection));
	XMStoreFloat4x4(&frameConstants.viewProjectionMatrix, XMMatrixTranspose(view * projection));
	XMStoreFloat4x4(&frameConstants.lightSpaceMatrix, XMMatrixTranspose(lightSpaceMatrix));

	XMStoreFloat4x4(&frameConstants.invViewMatrix, XMMatrixTranspose(XMMatrixInverse(nullptr, view)));
	XMStoreFloat4x4(&frameConstants.invProjectionMatrix, XMMatrixTranspose(XMMatrixInverse(nullptr, projection)));
	XMStoreFloat4x4(&frameConstants.invViewProjMatrix, XMMatrixTranspose(XMMatrixInverse(nullptr, view * projection)));
	XMStoreFloat4(&frameConstants.cameraPosition, position);

	XMStoreFloat4(&frameConstants.skyLightDirection, skyLightDir);
	frameConstants.ambientSkyColor	  = DirectX::XMFLOAT4(0.15f, 0.2f, 0.35f, 0);
	frameConstants.ambientGroundColor = DirectX::XMFLOAT4(0.1f, 0.08f, 0.05f, 0);

	frameConstantsBufferNew_.Update(context.Get(), frameConstants);
}

void Renderer::BindBlockSRVs()
{
	ID3D11ShaderResourceView* SRVs[] = {dx11Context_.GetShadowMapSRV(),
										textureManager_.GetTextureSRV(TextureType::Albedo),
										textureManager_.GetTextureSRV(TextureType::Normal),
										textureManager_.GetTextureSRV(TextureType::ARM),
										textureManager_.GetTextureSRV(TextureType::Heightmap),
										textureManager_.GetTextureSRV(TextureType::Emissive),
										textureManager_.GetMaterialBufferSRV()};

	auto context = dx11Context_.GetDeviceContext();
	context->PSSetShaderResources(0, std::size(SRVs), SRVs);
}
void Renderer::SetSamplers()
{
	auto context = dx11Context_.GetDeviceContext();
	context->PSSetSamplers(0, 1, dx11Context_.GetSamplerState().GetAddressOf());
	context->PSSetSamplers(1, 1, dx11Context_.GetShadowSamplerState().GetAddressOf());
}

void Renderer::UpdateSkyConstants(const DirectX::XMVECTOR skyLightDirection, float timeOfDay)
{
	auto context = dx11Context_.GetDeviceContext();

	static SkyBuffer skyConstants{};

	Utils::SkyColors skyColors = Utils::CalculateSkyColors(timeOfDay);
	DirectX::XMStoreFloat4(&skyConstants.sunDirection, skyLightDirection);
	skyConstants.zenithColor  = skyColors.zenithColor;
	skyConstants.horizonColor = skyColors.horizonColor;

	skyConstantsBufferNew_.Update(context.Get(), skyConstants);
	DirectX::XMStoreFloat3(&lightDir_, skyLightDirection);
}

void Renderer::SetDebugRenderMode(DebugRenderMode mode)
{
	auto context = dx11Context_.GetDeviceContext().Get();
	debugModeCBuffer_.Update(context, static_cast<std::underlying_type_t<DebugRenderMode>>(mode));
	currentDebugRenderMode_ = mode;
}

bool Renderer::CreateOutlineBuffers()
{
	SimpleVertex vertices[] = {// front

							   // top right => 0
							   {{1.0f, 1.0f, 0.0f}},
							   // bottom right => 1
							   {{1.0f, 0.0f, 0.0f}},
							   // bottom left => 2
							   {{0.0f, 0.0f, 0.0f}},
							   // top left => 3
							   {{0.0f, 1.0f, 0.0f}},

							   // back

							   // top right => 4
							   {{0.0f, 1.0f, 1.0f}},
							   // bottom right => 5
							   {{0.0f, 0.0f, 1.0f}},
							   // bottom left => 6
							   {{1.0f, 0.0f, 1.0f}},
							   // top left => 7
							   {{1.0f, 1.0f, 1.0f}}};


	D3D11_BUFFER_DESC vertexBufferDesc = {};
	vertexBufferDesc.Usage			   = D3D11_USAGE_IMMUTABLE;
	vertexBufferDesc.ByteWidth		   = sizeof(vertices);
	vertexBufferDesc.BindFlags		   = D3D11_BIND_VERTEX_BUFFER;

	D3D11_SUBRESOURCE_DATA vertexBufferData = {};
	vertexBufferData.pSysMem				= vertices;

	auto	device = dx11Context_.GetDevice();
	HRESULT result = device->CreateBuffer(&vertexBufferDesc, &vertexBufferData, &blockOutlineVertexBuffer_);
	if (FAILED(result))
	{
		return false;
	}


	// clang-format off
	std::uint32_t indices[] = {
		//front face
		0, 1, 2, 
		2, 3, 0,
		
		// top face
		7, 0, 3,
		3, 4, 7,
		
		// back face
		4, 5, 6,
		6, 7, 4,
		
		// bottom face
		1, 6, 5,
		5, 2, 1,
		
		// left face
		3, 2, 5,
		5, 4, 3,
		
		// right face
		0, 7, 6,
		6, 1, 0
		
	};
	// clang-format on

	D3D11_BUFFER_DESC indexBufferDesc = {};
	indexBufferDesc.Usage			  = D3D11_USAGE_IMMUTABLE;
	indexBufferDesc.ByteWidth		  = sizeof(indices);
	indexBufferDesc.BindFlags		  = D3D11_BIND_INDEX_BUFFER;

	D3D11_SUBRESOURCE_DATA indexBufferData = {};
	indexBufferData.pSysMem				   = indices;
	result = device->CreateBuffer(&indexBufferDesc, &indexBufferData, &blockOutlineIndexBuffer_);
	if (FAILED(result))
	{
		return false;
	}

	outlineIndexCount_ = std::size(indices);

	return true;
}

bool Renderer::CreateSphereBuffers()
{
	using namespace DirectX;
	// Code HEAVILY inspired by: https://github.com/Erkaman/cute-deferred-shading
	// (The "CreateSphere") method in main.cpp

	static constexpr std::uint32_t stacks		= 20;
	static constexpr std::uint32_t slices		= 20;
	static constexpr std::uint32_t stride		= slices + 1;
	static constexpr std::uint32_t NUM_VERTICES = (stacks + 1) * (slices + 1);
	static constexpr std::uint32_t NUM_QUADS	= slices * stacks;

	std::array<SimpleVertex, NUM_VERTICES>		vertices;
	std::array<std::uint32_t, NUM_QUADS * 6ull> indices;

	for (std::uint32_t stackIdx = 0; stackIdx <= stacks; stackIdx++)
	{
		const float V	= static_cast<float>(stackIdx) / static_cast<float>(stacks);
		const float phi = V * XM_PI;

		for (std::uint32_t sliceIdx = 0; sliceIdx <= slices; sliceIdx++)
		{
			const float U	  = static_cast<float>(sliceIdx) / static_cast<float>(slices);
			const float theta = U * XM_2PI;

			const float x = XMScalarCos(theta) * XMScalarSin(phi);
			const float y = XMScalarCos(phi);
			const float z = XMScalarSin(theta) * XMScalarSin(phi);

			vertices[stackIdx * stride + sliceIdx].position = XMFLOAT3(x, y, z);
		}
	}

	for (std::uint32_t k = 0, stackIdx = 0; stackIdx < stacks; ++stackIdx)
	{
		for (std::uint32_t sliceIdx = 0; sliceIdx < slices; ++sliceIdx)
		{
			// Quad indices
			const std::uint32_t topLeft	 = stackIdx * stride + sliceIdx;
			const std::uint32_t topRight = topLeft + 1;
			const std::uint32_t botLeft	 = (stackIdx + 1) * stride + sliceIdx;
			const std::uint32_t botRight = botLeft + 1;

			// Tri 1
			indices[k++] = topLeft;
			indices[k++] = topRight;
			indices[k++] = botRight;

			// Tri 2
			indices[k++] = botRight;
			indices[k++] = botLeft;
			indices[k++] = topLeft;
		}
	}

	D3D11_BUFFER_DESC vertexBufferDesc = {};
	vertexBufferDesc.Usage			   = D3D11_USAGE_IMMUTABLE;
	vertexBufferDesc.ByteWidth		   = vertices.size() * sizeof(vertices[0]);
	vertexBufferDesc.BindFlags		   = D3D11_BIND_VERTEX_BUFFER;

	auto device = dx11Context_.GetDevice();

	D3D11_SUBRESOURCE_DATA vertexBufferData = {};
	vertexBufferData.pSysMem				= vertices.data();
	HRESULT result = device->CreateBuffer(&vertexBufferDesc, &vertexBufferData, &sphereVertexBuffer_);
	if (FAILED(result))
	{
		return false;
	}

	D3D11_BUFFER_DESC indexBufferDesc = {};
	indexBufferDesc.Usage			  = D3D11_USAGE_IMMUTABLE;
	indexBufferDesc.ByteWidth		  = indices.size() * sizeof(indices[0]);
	indexBufferDesc.BindFlags		  = D3D11_BIND_INDEX_BUFFER;

	D3D11_SUBRESOURCE_DATA indexBufferData = {};
	indexBufferData.pSysMem				   = indices.data();
	result = device->CreateBuffer(&indexBufferDesc, &indexBufferData, &sphereIndexBuffer_);
	if (FAILED(result))
	{
		return false;
	}

	sphereIndexCount_ = indices.size();
	return true;
}

bool Renderer::CreatePointLightBuffer()
{
	D3D11_BUFFER_DESC pointLightBufferDesc = {};
	pointLightBufferDesc.Usage			   = D3D11_USAGE_DYNAMIC;
	pointLightBufferDesc.ByteWidth		   = sizeof(PointLightGPU) * MAX_POINT_LIGHTS;
	pointLightBufferDesc.BindFlags		   = D3D11_BIND_VERTEX_BUFFER;
	pointLightBufferDesc.CPUAccessFlags	   = D3D11_CPU_ACCESS_WRITE;

	auto	device = dx11Context_.GetDevice();
	HRESULT result = device->CreateBuffer(&pointLightBufferDesc, nullptr, &pointLightPerInstanceBuffer_);

	return (not FAILED(result));
}

void Renderer::UpdatePointLightBuffer(std::span<const PointLightGPU> pointLights)
{
	auto context = dx11Context_.GetDeviceContext();

	D3D11_MAPPED_SUBRESOURCE mappedResource;
	HRESULT result = context->Map(pointLightPerInstanceBuffer_.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
	if (FAILED(result))
	{
		return;
	}

	// IMPROTANT
	assert(pointLights.size() <= MAX_POINT_LIGHTS);
	currentVisiblePointLights_ = pointLights.size();

	PointLightGPU* buffer = static_cast<PointLightGPU*>(mappedResource.pData);

	context->Unmap(pointLightPerInstanceBuffer_.Get(), 0);
	std::ranges::copy(pointLights, buffer);
}

void Renderer::LightingPass()
{
	// disable wireframe for lighting
	bool wireframeEnabled = dx11Context_.IsWireframeEnabled();
	dx11Context_.ToggleWireframe(false);
	dx11Context_.PrepareLightPass();
	AmbientLightPass();
	DirectionalLightPass();
	PointLightPass();
	RenderBloom();
	RenderSky();
	if (currentDebugRenderMode_ == DebugRenderMode::NONE)
	{
		CompositePass();
	}
	dx11Context_.ToggleWireframe(wireframeEnabled);
}

void Renderer::ShadowPass()
{
	dx11Context_.PrepareShadowPass();
	BindShaders(shadowVertexShader_.Get(), nullptr, simpleVertexInputLayout_.Get());
}

void Renderer::AmbientLightPass()
{
	auto context = dx11Context_.GetDeviceContext();
	BindShaders(ambientLightVertexShader_.Get(), ambientLightPixelShader_.Get(), simpleVertexInputLayout_.Get());
	context->Draw(3, 0);
}

void Renderer::DirectionalLightPass()
{
	auto context = dx11Context_.GetDeviceContext();
	BindShaders(sunLightVertexShader_.Get(), sunLightPixelShader_.Get(), simpleVertexInputLayout_.Get());
	skyConstantsBufferNew_.Bind(context.Get(), 2, BindTarget::PixelShader);

	context->Draw(3, 0);
}

void Renderer::PointLightPass()
{
	dx11Context_.ToggleFrontCulling(true);
	ID3D11Buffer* const	  vertexBuffers[] = {sphereVertexBuffer_.Get(), pointLightPerInstanceBuffer_.Get()};
	static constexpr UINT strides[]		  = {sizeof(SimpleVertex), sizeof(PointLightGPU)};
	static constexpr UINT offsets[]		  = {0, 0};

	auto context = dx11Context_.GetDeviceContext();

	BindShaders(pointLightVertexShader_.Get(), pointLightPixelShader_.Get(), pointLightInputLayout_.Get());
	context->IASetVertexBuffers(0, 2, vertexBuffers, strides, offsets);
	context->IASetIndexBuffer(sphereIndexBuffer_.Get(), DXGI_FORMAT_R32_UINT, 0);

	context->DrawIndexedInstanced(sphereIndexCount_, /*formatting*/
								  currentVisiblePointLights_,
								  0,
								  0,
								  0);
	dx11Context_.ToggleFrontCulling(false);
}

void Renderer::RenderBloom()
{
	bool isHorizontalPass = false;
	dx11Context_.ToggleLightBlend(false);
	dx11Context_.ToggleZBuffer(false);

	// for first iteration we're going to get the light accumulation buffer as the source
	auto context	 = dx11Context_.GetDeviceContext();
	auto emissiveSRV = dx11Context_.GetGBufferSRVs()[static_cast<int>(GBufferTextures::Emissive)];

	static constexpr int MAX_BLOOM_ITERATIONS = 16;
	static_assert(MAX_BLOOM_ITERATIONS % 2 == 0);

	// "Manual" first iteration
	dx11Context_.SetPingPongBuffer();
	dx11Context_.ResetPingPongBuffers();
	context->PSSetShaderResources(0, 1, emissiveSRV.GetAddressOf());
	BindShaders(bloomVertexShader_.Get(), bloomHorizontalBlurPixelShader_.Get(), nullptr);
	context->Draw(3, 0);

	for (int i = 0; i < MAX_BLOOM_ITERATIONS - 1; ++i)
	{
		dx11Context_.SetPingPongBuffer();
		BindShaders(bloomVertexShader_.Get(),
					isHorizontalPass ? bloomHorizontalBlurPixelShader_.Get() : bloomVerticalBlurPixelShader_.Get(),
					nullptr);
		context->Draw(3, 0);
		isHorizontalPass = not isHorizontalPass;
	}
}

void Renderer::CompositePass()
{
	dx11Context_.SetBackBuffer(false);
	dx11Context_.ToggleZBuffer(false);
	dx11Context_.ToggleLightBlend(false);
	dx11Context_.ToggleAlphaBlend(true);
	ID3D11ShaderResourceView* SRVs[] = {dx11Context_.GetLightAccumulationSRV(), /*formatting*/
										dx11Context_.GetCurrentPingPongSRV()};

	auto  context		 = dx11Context_.GetDeviceContext();
	float blendFactor[4] = {0.0f, 0.0f, 0.0f, 0.0f};
	context->OMSetBlendState(nullptr, blendFactor, 0xffffffff);
	context->VSSetShaderResources(0, std::size(SRVs), SRVs);
	context->PSSetShaderResources(0, std::size(SRVs), SRVs);
	BindShaders(compositeVertexShader_.Get(), compositePixelShader_.Get(), nullptr);

	context->Draw(3, 0);
}

void Renderer::CalculateLightSpaceMatrix(Camera& camera)
{
	using namespace DirectX;
	auto position = camera.GetPosition();

	XMVECTOR skyLightDirection = XMLoadFloat3(&lightDir_);

	float	 distFromPlayer = 100.0f;
	XMVECTOR sunPosition	= position - (skyLightDirection * distFromPlayer);
	XMMATRIX sunViewMatrix	= XMMatrixLookAtLH(sunPosition, position, {0, 1, 0});

	float	 orthoSize	   = 100.f;
	float	 nearPlane	   = 1.0f;
	float	 farPlane	   = 200.f;
	XMMATRIX sunProjMatrix = XMMatrixOrthographicLH(orthoSize, orthoSize, nearPlane, farPlane);
	XMStoreFloat4x4(&lightSpaceMatrix_, sunViewMatrix * sunProjMatrix);

	XMMATRIX sunWorldMatrix	 = XMMatrixInverse(nullptr, sunViewMatrix);
	XMVECTOR sunRotationQuat = XMQuaternionRotationMatrix(sunWorldMatrix);
	BoundingFrustum::CreateFromMatrix(sunFrustum_, sunProjMatrix);
	XMStoreFloat3(&sunFrustum_.Origin, sunPosition);
	XMStoreFloat4(&sunFrustum_.Orientation, sunRotationQuat);
}

void Renderer::DebugRender()
{
	auto context = dx11Context_.GetDeviceContext().Get();
	dx11Context_.SetBackBuffer(false);
	frameConstantsBufferNew_.Bind(context, 0, BindTarget::PixelShader);
	debugModeCBuffer_.Bind(context, 2, BindTarget::PixelShader);
	SetSamplers();

	// Bind SRVs
	auto					  gBufferSRVs = dx11Context_.GetGBufferSRVs();
	ID3D11ShaderResourceView* bloomSRV	  = dx11Context_.GetCurrentPingPongSRV();
	ID3D11ShaderResourceView* SRVs[gBufferSRVs.size() + 1];
	for (int i = 0; const auto& SRV : gBufferSRVs)
	{
		SRVs[i] = gBufferSRVs[i].Get();
		++i;
	}
	SRVs[gBufferSRVs.size()] = bloomSRV;

	context->PSSetShaderResources(0, std::size(SRVs), SRVs);
	BindShaders(debugModeVertexShader_.Get(), debugModePixelShader_.Get(), nullptr);
	context->Draw(3, 0);
}

void Renderer::RenderSky()
{
	auto context = dx11Context_.GetDeviceContext();

	dx11Context_.SetLightAccumulationBuffer(true);
	dx11Context_.ToggleSkyDepthStencil(true);
	dx11Context_.ToggleLightBlend(false);

	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	frameConstantsBufferNew_.Bind(context.Get(), 0, BindTarget::PixelShader);
	skyConstantsBufferNew_.Bind(context.Get(), 2, BindTarget::PixelShader);

	BindShaders(skyVertexShader_.Get(), skyPixelShader_.Get(), nullptr);

	context->Draw(3, 0);

	dx11Context_.ToggleSkyDepthStencil(false);
}

void Renderer::RenderWorld(const World& world, Camera& camera)
{
	using namespace DirectX;
	auto		   context		= dx11Context_.GetDeviceContext();
	constexpr UINT stride		= sizeof(Vertex);
	constexpr UINT shadowStride = sizeof(SimpleVertex);
	constexpr UINT offset		= 0;

	CalculateLightSpaceMatrix(camera);
	UpdateFrameConstants(camera.GetViewMatrix(),
						 camera.GetProjectionMatrix(),
						 XMLoadFloat4x4(&lightSpaceMatrix_),
						 camera.GetPosition(),
						 // {-0.577, -0.577, -0.577, 0});
						 XMLoadFloat3(&lightDir_));

	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	frameConstantsBufferNew_.Bind(context.Get(), 0, BindTarget::VertexShader | BindTarget::PixelShader);
	objectConstantsBufferNew_.Bind(context.Get(), 1, BindTarget::VertexShader);
	SetSamplers();

	const auto shadowChunkBuffers = world.GetShadowChunksInFrustum(sunFrustum_);

	// SHADOW PASS
	ShadowPass();
	for (const auto& chunk : shadowChunkBuffers)
	{
		context->IASetVertexBuffers(0, 1, chunk->GetShadowProxyVertexBuffer().GetAddressOf(), &shadowStride, &offset);
		context->IASetIndexBuffer(chunk->GetShadowProxyIndexBuffer().Get(), DXGI_FORMAT_R32_UINT, 0);
		XMFLOAT4X4 shadowChunkWorldMatrix;
		XMStoreFloat4x4(&shadowChunkWorldMatrix, XMMatrixTranspose(chunk->GetWorldMatrix()));
		objectConstantsBufferNew_.Update(context.Get(), shadowChunkWorldMatrix);
		// UpdateObjectConstants(chunk->GetWorldMatrix());


		context->DrawIndexed(chunk->GetShadowProxyIndexCount(), 0, 0);
	}

	// REGULAR PASS
	const auto chunkBuffers = world.GetChunksInFrustum(camera.GetFrustum());
	dx11Context_.Deferred_ClearScreen(0, 0, 0, 1.0f);
	BindShaders(geometryPassVertexShader_.Get(), geometryPassPixelShader_.Get(), gBufferInputLayout_.Get());
	BindBlockSRVs();
	for (const auto& chunk : chunkBuffers)
	{
		context->IASetVertexBuffers(0, 1, chunk->GetVertexBuffer().GetAddressOf(), &stride, &offset);
		context->IASetIndexBuffer(chunk->GetIndexBuffer().Get(), DXGI_FORMAT_R32_UINT, 0);
		XMFLOAT4X4 chunkWorldMatrix;
		XMStoreFloat4x4(&chunkWorldMatrix, XMMatrixTranspose(chunk->GetWorldMatrix()));
		objectConstantsBufferNew_.Update(context.Get(), chunkWorldMatrix);
		// UpdateObjectConstants(chunk->GetWorldMatrix());


		context->DrawIndexed(chunk->GetIndexCount(), 0, 0);
	}

	UpdatePointLightBuffer(world.GetVoxelLightingEngine().GetLightsInFrustum(camera.GetFrustum()));
	LightingPass();
	if (currentDebugRenderMode_ != DebugRenderMode::NONE)
	{
		DebugRender();
	}
}

void Renderer::RenderUI(Player& player)
{
	using namespace DirectX;

	const auto& blockBar = player.GetBlockBar();
	auto		ortho	 = dx11Context_.GetOrthoMatrix();

	UIConstants uiConstants;
	XMStoreFloat4x4(&uiConstants.orthoProjectionMatrix, XMMatrixTranspose(ortho));

	auto context = dx11Context_.GetDeviceContext().Get();
	UIConstantsBuffer_.Update(context, uiConstants);
	hotbarColorBuffer_.Update(context, XMFLOAT4(0.15f, 0.15f, 0.15f, 1.0f));

	// Slot size was made with 1920x1080 in mind, multiply accordingly
	const auto width  = static_cast<float>(dx11Context_.GetWindowWidth());
	const auto height = static_cast<float>(dx11Context_.GetWindowHeight());

	const float resolutionMultiplier = (std::min)(width / 1920.0f, height / 1080.0f);
	const float SLOT_SIZE			 = 80.0f * resolutionMultiplier;
	float		startX				 = -(blockBar.size() * SLOT_SIZE) / 2.0f;
	float		yRef				 = (-dx11Context_.GetWindowHeight() / 2.0f)
			   + SLOT_SIZE
			   + (10.0f * resolutionMultiplier); // Keep 10 pixel offset from bottom

	auto& blockDatabase = BlockDatabase::GetDatabase();

	// Bind cBuffers and shaders
	dx11Context_.SetBackBuffer(false);
	UIConstantsBuffer_.Bind(context, 0, BindTarget::VertexShader | BindTarget::PixelShader);
	UIMatIdBuffer_.Bind(context, 1, BindTarget::PixelShader);
	objectConstantsBufferNew_.Bind(context, 2, BindTarget::VertexShader);
	// BindObjectConstants(2);
	hotbarColorBuffer_.Bind(context, 3, BindTarget::PixelShader);
	BindBlockSRVs();
	SetSamplers();
	dx11Context_.ToggleFrontCulling(true);
	dx11Context_.ToggleAlphaBlend(true);

	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	const XMMATRIX hotbarSlotScale		= XMMatrixScaling(SLOT_SIZE, SLOT_SIZE, 1.0f);
	const XMMATRIX hotbarBlockScale		= XMMatrixScaling(SLOT_SIZE * 0.8f, SLOT_SIZE * 0.8f, 1.0f);
	const XMMATRIX hotbarSelectionScale = XMMatrixScaling(SLOT_SIZE * 1.1f, SLOT_SIZE * 1.2f, 1.0f);
	const XMMATRIX crosshairDotScale	= XMMatrixScaling(SLOT_SIZE * 0.2f, SLOT_SIZE * 0.2f, 1.0f);
	XMFLOAT4X4	   matrix;
	for (int i = 0; const auto& block : blockBar)
	{
		// Calculate the proper offset for the world matrix
		float xPos = startX + (SLOT_SIZE * i) + (SLOT_SIZE / 2.0f);
		float yPos = yRef + (SLOT_SIZE / 2.0f); // + half slot size to center the box
		// Render the slot

		XMMATRIX translation = XMMatrixTranslation(xPos, yPos, 0.2f);

		XMStoreFloat4x4(&matrix, XMMatrixTranspose(hotbarSlotScale * translation));
		objectConstantsBufferNew_.Update(context, matrix);
		// UpdateObjectConstants(hotbarSlotScale * translation);
		BindShaders(hotbarVertexShader_.Get(), hotbarPixelShader_.Get(), nullptr);
		context->Draw(4, 0);

		// Render the block
		const BlockData* blockData = blockDatabase.GetBlockData(block);
		if (blockData != nullptr)
		{
			BindShaders(hotbarBlockVertexShader_.Get(), hotbarBlockPixelShader_.Get(), nullptr);
			XMStoreFloat4x4(&matrix, XMMatrixTranspose(hotbarBlockScale * translation));
			objectConstantsBufferNew_.Update(context, matrix);
			// UpdateObjectConstants(hotbarBlockScale * translation);

			// idx 4 is "top" face of the block, usually the most distinct
			UIMatIdBuffer_.Update(context, blockData->textureIndices[static_cast<std::size_t>(BlockFace::Top)]);

			context->Draw(4, 0);
		}

		++i;
	}

	// Draw the "Selected" overlay
	hotbarColorBuffer_.Update(context, XMFLOAT4(0.25f, 0.25f, 0.45f, 1.0f));
	auto	 currentSlot = static_cast<float>(player.GetBlockBarIndex());
	float	 xPos		 = startX + (SLOT_SIZE * currentSlot) + (SLOT_SIZE / 2.0f);
	float	 yPos		 = yRef + (SLOT_SIZE / 2.0f);
	XMMATRIX translation = XMMatrixTranslation(xPos, yPos, 0.2f);

	XMStoreFloat4x4(&matrix, XMMatrixTranspose(hotbarSelectionScale * translation));
	objectConstantsBufferNew_.Update(context, matrix);
	// UpdateObjectConstants(hotbarSelectionScale * translation);
	BindShaders(hotbarVertexShader_.Get(), hotbarPixelShader_.Get(), nullptr);
	context->Draw(4, 0);

	// Draw the crosshair
	xPos		= 0;
	yPos		= 0;
	translation = XMMatrixTranslation(xPos, yPos, 0.2f);
	XMStoreFloat4x4(&matrix, XMMatrixTranspose(crosshairDotScale * translation));
	objectConstantsBufferNew_.Update(context, matrix);
	// UpdateObjectConstants(crosshairDotScale * translation);
	hotbarColorBuffer_.Update(context, XMFLOAT4(1.0f, 1.0f, 1.0f, 0.7f));
	context->Draw(4, 0);
}

bool Renderer::CompileVertexShader(const std::wstring&							shaderPath,
								   const std::string&							entryPoint,
								   const std::vector<D3D11_INPUT_ELEMENT_DESC>& inputLayoutDesc,
								   Microsoft::WRL::ComPtr<ID3D11VertexShader>&	outVertexShader,
								   Microsoft::WRL::ComPtr<ID3D11InputLayout>&	outInputLayout)
{
	Microsoft::WRL::ComPtr<ID3DBlob> vertexShaderBlob;
	Microsoft::WRL::ComPtr<ID3DBlob> error;

	HRESULT result = D3DCompileFromFile(shaderPath.c_str(),
										nullptr,
										D3D_COMPILE_STANDARD_FILE_INCLUDE,
										entryPoint.c_str(),
										"vs_5_0",
										D3D10_SHADER_ENABLE_STRICTNESS,
										0,
										&vertexShaderBlob,
										&error);

	if (FAILED(result))
	{
		if (error != nullptr)
		{
			auto errorMsg = static_cast<char*>(error->GetBufferPointer());
			std::wcerr << "Vertex shader at " << shaderPath << " failed to compile. Reason: " << errorMsg << std::endl;
		}
		else
		{
			std::wcerr << "Vertex shader: " << shaderPath << " not found" << std::endl;
		}

		return false;
	}

	auto device = dx11Context_.GetDevice();
	result		= device->CreateVertexShader(vertexShaderBlob->GetBufferPointer(),
										 vertexShaderBlob->GetBufferSize(),
										 nullptr,
										 &outVertexShader);
	if (FAILED(result))
	{
		return false;
	}

	result = device->CreateInputLayout(inputLayoutDesc.data(),
									   inputLayoutDesc.size(),
									   vertexShaderBlob->GetBufferPointer(),
									   vertexShaderBlob->GetBufferSize(),
									   &outInputLayout);

	if (FAILED(result))
	{
		return false;
	}

	return true;
}

bool Renderer::CompilePixelShader(const std::wstring&						 shaderPath,
								  const std::string&						 entryPoint,
								  Microsoft::WRL::ComPtr<ID3D11PixelShader>& outPixelShader,
								  const D3D_SHADER_MACRO*					 defines)
{
	Microsoft::WRL::ComPtr<ID3DBlob> pixelShaderBlob;
	Microsoft::WRL::ComPtr<ID3DBlob> error;

	HRESULT result = D3DCompileFromFile(shaderPath.c_str(),
										defines,
										D3D_COMPILE_STANDARD_FILE_INCLUDE,
										entryPoint.c_str(),
										"ps_5_0",
										D3D10_SHADER_ENABLE_STRICTNESS,
										0,
										&pixelShaderBlob,
										&error);

	if (FAILED(result))
	{
		if (error != nullptr)
		{
			auto errorMsg = static_cast<char*>(error->GetBufferPointer());
			std::wcerr << "Pixel shader at " << shaderPath << " failed to compile. Reason: " << errorMsg << std::endl;
		}
		else
		{
			std::wcerr << "Pixel shader: " << shaderPath << " not found" << std::endl;
		}

		return false;
	}

	auto device = dx11Context_.GetDevice();
	result		= device->CreatePixelShader(pixelShaderBlob->GetBufferPointer(),
										pixelShaderBlob->GetBufferSize(),
										nullptr,
										&outPixelShader);
	if (FAILED(result))
	{
		return false;
	}

	return true;
}

void Renderer::BindShaders(ID3D11VertexShader* vertexShader, //
						   ID3D11PixelShader*  pixelShader,
						   ID3D11InputLayout*  inputLayout)
{
	auto context = dx11Context_.GetDeviceContext();

	context->VSSetShader(vertexShader, nullptr, 0);
	context->PSSetShader(pixelShader, nullptr, 0);
	context->IASetInputLayout(inputLayout);
}

void Renderer::DrawBlockOutline(const BlockRaycastResult& result)
{
	using namespace DirectX;
	if (result.success == false)
	{
		// Don't draw a block outline for a raycast that hit nothing
		return;
	}

	dx11Context_.SetBackBuffer();

	float scaleFactor = 1.002f;						 // 0.2% larger
	float lineOffset  = (1.0f - scaleFactor) * 0.5f; // Center it

	XMMATRIX scale		 = XMMatrixScaling(scaleFactor, scaleFactor, scaleFactor);
	XMMATRIX adjust		 = XMMatrixTranslation(lineOffset, lineOffset, lineOffset);
	XMMATRIX translation = XMMatrixTranslationFromVector(XMLoadSInt3(&result.blockPosition));

	// Final World Matrix = Scale * Adjustment * BlockPosition
	XMMATRIX world = scale * adjust * translation;

	XMFLOAT4X4 matrix;
	XMStoreFloat4x4(&matrix, XMMatrixTranspose(world));
	// UpdateObjectConstants(world);
	auto context = dx11Context_.GetDeviceContext();
	objectConstantsBufferNew_.Update(context.Get(), matrix);
	frameConstantsBufferNew_.Bind(context.Get(), 0, BindTarget::VertexShader);
	objectConstantsBufferNew_.Bind(context.Get(), 1, BindTarget::VertexShader);
	// BindObjectConstants();

	constexpr UINT stride = sizeof(SimpleVertex);
	constexpr UINT offset = 0;

	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	context->IASetVertexBuffers(0, 1, blockOutlineVertexBuffer_.GetAddressOf(), &stride, &offset);
	context->IASetIndexBuffer(blockOutlineIndexBuffer_.Get(), DXGI_FORMAT_R32_UINT, 0);
	BindShaders(outlineVertexShader_.Get(), outlinePixelShader_.Get(), simpleVertexInputLayout_.Get());

	dx11Context_.ToggleAlphaBlend(true);
	context->DrawIndexed(outlineIndexCount_, 0, 0);
	dx11Context_.ToggleAlphaBlend(false);
}
