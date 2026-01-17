#pragma once
#include <DirectXMath.h>

struct SkyBuffer
{
	DirectX::XMFLOAT4 zenithColor;
	DirectX::XMFLOAT4 horizonColor;
	DirectX::XMFLOAT4 sunDirection;
};
