#ifndef DEFERRED_COMMONS_HLSL
#define DEFERRED_COMMONS_HLSL

static const float PI = 3.1415926535897932384626433832795;
static const float PI_2 = PI * 2.0f;
static const float INV_PI = 1.0f / PI;

Texture2D albedo   : register(t0);
Texture2D normal   : register(t1);
Texture2D ORMV     : register(t2);
Texture2D emissive : register(t3);
Texture2D depth    : register(t4);

SamplerState pointSampler  : register(s0);
SamplerState shadowSampler : register(s1);

float3 CalculateWorldPos(float2 uv, float depth, matrix invViewProj)
{
	// remap x and y
	float x = uv.x * 2.0f - 1.0f;
	float y = 1.0f - uv.y * 2.0f; 
	
	float z = depth;
	
	float4 clipPos = float4(x, y, z, 1.0f);
	
	// convert to world
	float4 worldPos = mul(clipPos, invViewProj);
	
	return worldPos.xyz / worldPos.w;
}

#endif