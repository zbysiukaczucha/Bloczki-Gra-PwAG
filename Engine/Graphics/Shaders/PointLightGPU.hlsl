#ifndef POINT_LIGHT_DATA_HLSL
#define POINT_LIGHT_DATA_HLSL

struct PointLightGPU
{
	float3 position;
	float radius;
	float3 color;
	float intensity;
};


#endif