#pragma once
#include <DirectXCollision.h>
#include <DirectXMath.h>

static constexpr std::size_t MAX_POINT_LIGHTS = 1024;

struct PointLightCPU
{
	DirectX::BoundingSphere bounds;
	DirectX::XMFLOAT3		color;
	float					intensity;
	bool					operator==(const PointLightCPU& other) const;
};

struct PointLightGPU
{
	DirectX::XMFLOAT3 position;
	float			  radius;
	DirectX::XMFLOAT3 color;
	float			  intensity;
};
