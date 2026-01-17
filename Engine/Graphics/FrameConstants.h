#pragma once
#include <DirectXMath.h>

struct FrameConstants
{
	DirectX::XMFLOAT4X4 viewMatrix;
	DirectX::XMFLOAT4X4 projectionMatrix;
	DirectX::XMFLOAT4X4 viewProjectionMatrix;
	DirectX::XMFLOAT4X4 lightSpaceMatrix;

	DirectX::XMFLOAT4X4 invViewMatrix;
	DirectX::XMFLOAT4X4 invProjectionMatrix;
	DirectX::XMFLOAT4X4 invViewProjMatrix;

	DirectX::XMFLOAT4 skyLightDirection;

	DirectX::XMFLOAT4 cameraPosition;

	DirectX::XMFLOAT4 ambientSkyColor;
	DirectX::XMFLOAT4 ambientGroundColor;
};
