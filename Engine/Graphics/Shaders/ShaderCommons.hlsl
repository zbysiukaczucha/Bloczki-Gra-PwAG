#ifndef SHADER_COMMONS_HLSL
#define SHADER_COMMONS_HLSL

cbuffer FrameConstants : register(b0)
{
	float4x4 view;
	float4x4 projection;
	float4x4 viewProjection;
	float4x4 lightSpaceMatrix;
	
	float4x4 inverseView;
	float4x4 inverseProjection;
	float4x4 inverseViewProjection;

	float4 skyLightDirection;

	float4 cameraPosition;

	float4 ambientSkyColor;
	float4 ambientGroundColor;
};

cbuffer ObjectConstants : register(b1)
{
	float4x4 world;
};

#endif
