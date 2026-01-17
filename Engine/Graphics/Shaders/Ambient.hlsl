#include "ShaderCommons.hlsl"
#include "DeferredCommons.hlsl"

Texture2D shadowMap : register(t4);
struct PS_Input
{
	float4 position	  : SV_POSITION;
	float2 texcoord	  : TEXCOORD0;
};


PS_Input VS_Main(uint vIdx : SV_VertexID)
{
	PS_Input output;
	float2 texcoord = float2((vIdx << 1) & 2, vIdx & 2);
	output.position = float4(texcoord * float2(2, -2) + float2(-1, 1), 1.0, 1.0);
	output.texcoord = texcoord;
	
	
	return output;
}

float4 PS_Main(PS_Input input) : SV_TARGET
{
	float2 uv = input.texcoord;
	const float4 basecolor = albedo.Sample(pointSampler, uv);
	const float4 ormv = ORMV.Sample(pointSampler, uv);
	
	const float3 sampledNormal = normal.Sample(pointSampler, uv);
	
	float3 N = normalize(sampledNormal * 2.0f - 1.0f);
	
	// Simple ambient
	const float ao = ormv.r;
	const float up = N.y * 0.5f + 0.5f;
	const float3 ambientLight = lerp(ambientGroundColor.rgb, ambientSkyColor.rgb, up);
	 float3 ambient = ambientLight * basecolor.rgb * ao * 0.1f;
	
	// Inspired by BSL shaders
	const float3 NdotU = clamp(dot(N, float3(0, 1, 0)), -1.0f, 1.0f);
	const float3 NdotE = clamp(dot(N, float3(1, 0, 0)), -1.0f, 1.0f);
	const float vanillaFactor = (0.25 * NdotU + 0.75) + (0.667 - abs(NdotE)) * (1.0 - abs(NdotU)) * 0.15;
	// ultimately unused
	
	const uint voxelLighting = ormv.a * 255.0f;
	const float skyLightLevel = ((voxelLighting & 0xF0) >> 4) / 15.0f;
	
	const float voxelSkyLightFactor = max(skyLightLevel, 0.25f); // to prevent absolute pitch-black darkness, but make ambient darker without sky access
	
	ambient *= voxelSkyLightFactor;
	
	// add "emissive" to ambient
	float4 emissiveLight = emissive.Sample(pointSampler, uv);
	
	ambient += emissiveLight;
	
	return float4(ambient, 1.0f);
}