cbuffer UIConstants : register (b0)
{
	float4x4 orthoProjection;
}

cbuffer MaterialID : register(b1)
{
	uint materialID;
}

cbuffer ObjectConstants : register(b2)
{
	float4x4 world;
};

cbuffer Color : register (b3)
{
	float4 color;
}

struct MaterialData
{
	uint albedoIdx;
	uint normalIdx;
	uint ARMIdx; // Ambient, Roughness, Metallic
	uint heightmapIdx;
	uint emissiveIdx;
};

Texture2DArray albedoTextures    : register(t1);
StructuredBuffer<MaterialData> materialBuffer : register(t6);

SamplerState basicSampler : register(s0);

struct PS_Input
{
	float4 position : SV_POSITION;
	float4 localPos : TEXCOORD;
};

struct PS_TexturedInput
{
	float4 position : SV_POSITION;
	float2 texcoord : TEXCOORD0;
};

PS_Input VS_Main(uint vIdx : SV_VertexID)
{
	PS_Input output;
	
	// make a position out of indices
	float x = (vIdx & 1) ? 0.5f : -0.5f;
	float y = (vIdx >> 1) ? 0.5f : -0.5f;
	output.localPos = float4(x, y, 0, 1);
	
	output.position = mul(output.localPos, world);
	output.position = mul(output.position, orthoProjection);
	output.localPos = float4(x, y, 0, 1);
	
	return output;
}

PS_TexturedInput VS_Textured(uint vIdx : SV_VertexID)
{
	PS_TexturedInput output;
	// make a position out of indices
	float x = (vIdx & 1) ? 0.5f : -0.5f;
	float y = (vIdx >> 1) ? 0.5f : -0.5f;
	float2 UV = float2(x, y);
	float4 localPos = float4(x, y, 0, 1);
	
	UV += 0.5f;
	UV.y = 1.0f - UV.y; //flip y
	
	output.position = mul(localPos, world);
	output.position = mul(output.position, orthoProjection);
	output.texcoord = UV;
	
	return output;
}

float4 PS_Main(PS_Input input) : SV_TARGET
{
	float thickness = 0.1f;
	
	float2 distFromCenter = abs(input.localPos);
	float2 isBorder = step(0.5f - thickness, distFromCenter);
	if (max(isBorder.x, isBorder.y) > 0.0f)
	{
		return color;
	}
	
	return float4(0,0,0,0);
}

float4 PS_Textured(PS_TexturedInput input) : SV_TARGET
{
	uint albedoID = materialBuffer[materialID].albedoIdx;
	
	float4 albedo = albedoTextures.Sample(basicSampler, float3(input.texcoord, albedoID));
	
	return albedo;
}