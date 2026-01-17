#include "ShaderCommons.hlsl"

struct VS_Input
{
	float3 position : POSITION;
};

struct PS_Input
{
	float4 position : SV_POSITION;
	float4 localPos : TEXCOORD;
};

PS_Input VS_Main(VS_Input input)
{
	
	float4 worldPos = mul(float4(input.position, 1.0f), world);
	float4 viewPos = mul(worldPos, view);
	
	//final, projected position
	PS_Input result;
	result.position = mul(viewPos, projection);
	result.localPos = float4(input.position, 1.0f);
	return result;
}

float4 PS_Main(PS_Input input) : SV_TARGET
{
	float4 color = float4(0.2f, 0.2f, 0.2f, 0.5f);
	float thickness = 0.01f;
	
	float3 distToEdge = min(input.localPos, 1.0f - input.localPos);
	float3 isCenter = step(thickness, distToEdge);
	if (isCenter.x + isCenter.y + isCenter.z > 1.0f)
	{
		discard;
	}
	
	return color;
}