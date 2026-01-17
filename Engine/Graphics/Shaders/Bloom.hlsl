// #include "DeferredCommons.hlsl"
#include "ShaderCommons.hlsl"

Texture2D bloomBuffer : register(t0);
SamplerState linearSampler : register(s1);

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

static const float weights[5] = {0.227027f, 0.1945946f, 0.1216216f, 0.054054f, 0.016216f};
static const float spreadFactor = 2.0f;
float4 PS_Main(PS_Input input) : SV_TARGET
{
	uint2 widthHeight;
	bloomBuffer.GetDimensions(widthHeight.x, widthHeight.y);
	float2 texelSize = 1.0f / float2(widthHeight);
	float3 result = bloomBuffer.Sample(linearSampler, input.texcoord) * weights[0];
	[unroll]
	for (int i = 1; i < 5; ++i)
	{
#ifdef VERTICAL
		result += bloomBuffer.Sample(linearSampler, input.texcoord + float2(0, texelSize.y * i * spreadFactor)) * weights[i];
		result += bloomBuffer.Sample(linearSampler, input.texcoord - float2(0, texelSize.y * i * spreadFactor)) * weights[i];
#else
		result += bloomBuffer.Sample(linearSampler, input.texcoord + float2(texelSize.x * i * spreadFactor, 0)) * weights[i];
		result += bloomBuffer.Sample(linearSampler, input.texcoord - float2(texelSize.x * i * spreadFactor, 0)) * weights[i];
#endif
	}
	return float4(result, 1.0f);
}