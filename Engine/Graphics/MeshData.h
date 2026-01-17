#pragma once
#include <d3d11.h>
#include <vector>
#include <wrl/client.h>

#include "Vertex.h"

struct MeshCPUData
{
	std::vector<Vertex>	  vertices;
	std::vector<uint32_t> indices;
};

struct MeshGPUData
{
	Microsoft::WRL::ComPtr<ID3D11Buffer> vertexBuffer = nullptr;
	Microsoft::WRL::ComPtr<ID3D11Buffer> indexBuffer  = nullptr;
	uint32_t							 indexCount	  = 0;

	Microsoft::WRL::ComPtr<ID3D11Buffer> shadowProxyVertexBuffer = nullptr;
	Microsoft::WRL::ComPtr<ID3D11Buffer> shadowProxyIndexBuffer	 = nullptr;
	uint32_t							 shadowProxyIndexCount	 = 0;
};
