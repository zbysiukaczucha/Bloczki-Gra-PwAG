#include "DeferredCommons.hlsl"
#include "ShaderCommons.hlsl"

// Keep these the same in cpp
// ENUM for debug modes
#define ALBEDO_MODE 1
#define NORMALS_MODE 2
#define AO_MODE 3
#define ROUGHNESS_MODE 4
#define METALLIC_MODE 5
#define SKYLIGHT_MODE 6
#define BLOCKLIGHT_MODE 7
#define EMISSIVE_MODE 8
#define DEPTH_MODE 9
#define BLOOM_MODE 10

Texture2D bloomBuffer : register(t5);

cbuffer DebugMode : register(b2)
{
	uint1 mode;
}

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
	float4 outputColor = float4(0.0f, 0.0f, 0.0f, 1.0f);
	switch (mode)
	{
		case ALBEDO_MODE:
			outputColor.rgb = albedo.Sample(pointSampler, uv);
			break;
			
		case NORMALS_MODE:
		{
			float3 rawNormal= normal.Sample(pointSampler, uv);
			// convert to world-space
			rawNormal = rawNormal * 2.0f - 1.0f;
			float3 worldN = mul(rawNormal, (float3x3)  inverseView);
			outputColor.rgb = worldN.rgb;
			
			// Display black over sky
			float Z = depth.Sample(pointSampler, uv);
			if (Z >= 1.0f)
			{
				outputColor.rgb = float3(0.0f, 0.0f, 0.0f);
			}
			break;
		}
			
		case AO_MODE:
			outputColor.rgb = ORMV.Sample(pointSampler, uv).r;
			break;
			
		case ROUGHNESS_MODE:
			outputColor.rgb = ORMV.Sample(pointSampler, uv).g;
			break;
			
		case METALLIC_MODE:
			outputColor.rgb = ORMV.Sample(pointSampler, uv).b;
			break;
			
		case SKYLIGHT_MODE:
		{
			const uint voxelLight = ORMV.Sample(pointSampler, uv).a * 255;
			const float skyLight = ((voxelLight & 0xF0) >> 4) * 15.0f;
			outputColor.rgb = skyLight;
			break;
		}
			
		case BLOCKLIGHT_MODE:
		{
			const uint voxelLight = ORMV.Sample(pointSampler, uv).a * 255;
			const float blockLight = (voxelLight & 0x0F) * 15.0f;
			outputColor.rgb = blockLight;
			break;
		}
		case EMISSIVE_MODE:
			outputColor.rgb = emissive.Sample(pointSampler, uv);
			break;
			
		case BLOOM_MODE:
			outputColor.rgb = bloomBuffer.Sample(pointSampler, uv);
			break;
			
		case DEPTH_MODE:
		{
			const float nearPlane = 0.1f;
			const float farPlane = 1000.0f;
			float rawDepth  = depth.Sample(pointSampler, uv);
			
			float linearDepth = (nearPlane * farPlane) / (farPlane - rawDepth * (farPlane - nearPlane)) / farPlane;
			outputColor.rgb = linearDepth;
			break;
		}
		default:
			outputColor = float4(0.0f, 0.0f, 0.0f, 1.0f);
			break;
	}
	
	return outputColor;
}