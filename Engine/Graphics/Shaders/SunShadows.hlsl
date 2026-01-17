#include "ShaderCommons.hlsl"

struct VS_Input
{
	float3 position	  : POSITION;
};

struct VS_Output
{
	float4 position	  : SV_POSITION;
};


VS_Output  VS_Main(VS_Input input)
{
	
	float4 worldPos = mul(float4(input.position, 1.0f), world);
	
	//final, projected position
	VS_Output result;
	result.position = mul(worldPos, lightSpaceMatrix);
	
	return result;
}