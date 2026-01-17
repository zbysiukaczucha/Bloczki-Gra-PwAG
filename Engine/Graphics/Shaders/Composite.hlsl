Texture2D lightAccumulationBuffer : register(t0);
Texture2D bloomBuffer			  : register(t1);
SamplerState pointSampler		  : register(s0);

struct PS_Input
{
	float4 position	  : SV_POSITION;
	float2 texcoord	  : TEXCOORD0;
};

float GetLuminance(float3 color)
{
	return dot(color, float3(0.2126f, 0.7152f, 0.0722f));
}

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
	const float3 hdrColor = lightAccumulationBuffer.Sample(pointSampler, input.texcoord);
	const float3 bloom = bloomBuffer.Sample(pointSampler, input.texcoord) * 3.0f;
	
	float3 compositeColor = hdrColor.rgb + bloom.rgb;
	
	// Reinhard (with luminance) Tone Mapping
	float L = GetLuminance(compositeColor);
	
	float3 finalColor = compositeColor.rgb / (1.0f + L);
	
	return float4(finalColor, 1.0f);
}