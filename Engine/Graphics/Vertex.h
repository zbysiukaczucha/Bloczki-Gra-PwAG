#pragma once
#include <DirectXMath.h>
#include <cstdint>

struct Vertex
{
	DirectX::XMFLOAT3 position;
	DirectX::XMFLOAT3 normal;
	DirectX::XMFLOAT3 tangent;
	DirectX::XMFLOAT3 bitangent;
	DirectX::XMFLOAT2 uv;
	std::uint32_t	  materialID;
	std::uint8_t	  lightLevel;
};

struct SimpleVertex
{
	DirectX::XMFLOAT3 position;
};
