#ifndef POINT_LIGHTS_HLSL
#define POINT_LIGHTS_HLSL
#include "ShaderCommons.hlsl"
#include "DeferredCommons.hlsl"

struct VS_Input
{
	float3 position	  : POSITION;
	
	float3 lightCenter    : TEXCOORD0;
	float  lightRadius    : TEXCOORD1;
	float3 lightColor     : COLOR;
	float  lightIntensity : TEXCOORD2;
};

struct PS_Input
{
	float4 position	: SV_POSITION;
	
	nointerpolation float3 lightCenter    : TEXCOORD0;
	nointerpolation float  lightRadius    : TEXCOORD1;
	nointerpolation float3 lightColor     : COLOR;
	nointerpolation float  lightIntensity : TEXCOORD2;
};


PS_Input VS_Main(VS_Input input)
{
	
	float3 scaledPos = input.position * input.lightRadius;
	float3 worldPos = scaledPos + input.lightCenter;
	
	//final, projected position
	PS_Input result;
	result.position = mul(float4(worldPos, 1.0f), viewProjection);
	result.lightCenter = input.lightCenter;
	result.lightRadius = input.lightRadius;
	result.lightColor = input.lightColor;
	result.lightIntensity = input.lightIntensity;
	
	return result;
}

float GetAttenuation(float distance, float radius, float intensity);

// source: https://lisyarus.github.io/blog/posts/point-light-attenuation.html
float AttenuateNoCusp(float distance, float radius, float intensity);

float4 PS_Main(PS_Input input) : SV_TARGET
{
	uint2 dimensions;
	albedo.GetDimensions(dimensions.x ,dimensions.y);
	
	float2 uv = input.position.xy / dimensions.xy;
	const float4 basecolor = albedo.Sample(pointSampler, uv);
	const float4 ormv = ORMV.Sample(pointSampler, uv);
	
	float Z = depth.Sample(pointSampler, uv);
	if (Z >= 1.0f) // prevent Sky NaNs
	{
		return float4(0.0f, 0.0f, 0.0f, 0.0f);
	}
	const float3 fragWorldPos = CalculateWorldPos(uv, Z, inverseViewProjection);
	
	const float3 sampledNormal = normal.Sample(pointSampler, uv);
	
	float3 N = normalize(sampledNormal * 2.0f - 1.0f);
	const float3 L = normalize(mul(input.lightCenter - fragWorldPos, (float3x3)view));
	const float3 V = normalize(mul(cameraPosition.xyz - fragWorldPos, (float3x3)view));
	const float3 R = reflect(-V, N);
	
	const float3 H = normalize(L + V);
	
	
	// The 'wrap' value determines how far light wraps around the object.
	// Useful in situations where a block is embedded in a flat wall, to make the wall itself lit as well
	// 0.0 = Hard cutoff (Standard Lambert)
	// 0.5 = Soft wrap (Half-Lambert)
	// 1.0 = Full wrap (Light reaches the back)

	float wrap = 0.8f; 
	float NdotL_raw = dot(N, L);
	float NdotL = saturate((NdotL_raw + wrap) / (1.0f + wrap));	
	const float NdotV = saturate(dot(N, V));
	const float NdotH = saturate(dot(N, H));
	const float HdotV = saturate(dot(H, V));
	
	
	// base reflectivity (F0)
	// non-metals are usually ~0.04, metals use their albedo color
	const float metallic = ormv.b;
	float3 F0;
	F0.xyz = 0.04f;
	
	F0 = lerp(F0, basecolor.rgb, metallic);
	
	// D F G
	
	const float roughness = saturate(ormv.g + 0.04);
	
	// D: Normal distribution function (GGX / Trowbridge-Reitz)
	// Controls the size and sharpness of the specular highlight.
	const float roughnessSqr = (roughness * roughness); 
	const float alphaSqr = (roughnessSqr * roughnessSqr); // Disney uses roughness ^ 4
	const float denom = (NdotH * NdotH * (alphaSqr - 1.0f) + 1.0f);
	const float D = alphaSqr / (PI * denom * denom);
	
	// G: Geometry function (Smith-Schlick)
	// Simulates micro-facets self-shadowing each other
	const float k = roughnessSqr;
	const float smithL = NdotL / (NdotL * (1.0f - k) + k);
	const float smithV = NdotV / (NdotV * (1.0f - k) + k);
	const float G = smithL * smithV;
	
	// F: Fresnel equation (Schlick Approximation)
	// Light reflects more strongly at grazing angles
	const float3 F = F0 + (1.0f - F0) * pow(1.0f - HdotV, 5.0f);
	
	
	// Combine Specular (Cook-Torrance BRDF)
	// Numerator = D * F * G
	// Denominator = 4 * (N.L) * (N.V) + epsilon, to prevent division by 0
	const float3 numerator = D * F * G;
	const float denominator = 4.0f * NdotL * NdotV + 0.00001f;
	const float3 specular = numerator / denominator;
	
	// Combine Diffuse (Lambert)
	// metals absorb all refracted light, so they have no diffuse reflection.
	// The fresnel term (F) represents the light that reflected (specular)
	// so (1.0 - F) is the light that entered the surface (diffuse)
	const float3 kS = F;
	const float3 kD = (float3(1.0f, 1.0f, 1.0f) - kS) * (1.0f - metallic);
	
	// Standard Lambertian diffuse
	const float3 diffuse = kD * (basecolor.rgb / PI);
	
	float distance = length(input.lightCenter - fragWorldPos);
	float attenuation = AttenuateNoCusp(distance, input.lightRadius, input.lightIntensity);
	
	// Final composition
	// We multiply by NdotL because light spreats out over the surface area
	 float3 directLight = (diffuse + specular) * NdotL * attenuation * input.lightColor;
	
	const uint voxelLighting = ormv.a * 255.0f;
	const float skyLightLevel = ((voxelLighting & 0xF0) >> 4) / 15.0f;
	const float blockLightLevel = (voxelLighting & 0x0F) / 15.0f;
	
	directLight *= blockLightLevel;
	
	return float4(directLight, 1.0f);
}

float GetAttenuation(float distance, float radius, float intensity)
{
	// 1. Standard Inverse Square (intensity / dist^2)
	// The +1.0 prevents dividing by zero when distance is tiny.
	float attenuation = intensity / (distance * distance + 1.0f);
    
	// 2. The "Windowing" function
	// This creates a smooth curve that hits 0.0 exactly at 'radius'
	float falloff = saturate(1.0f - pow(distance / radius, 4.0f));
    
	// Square it for a softer, more natural fade
	falloff = falloff * falloff;
    
	return attenuation * falloff;
}

float sqr(float x)
{
	return x * x;
}

float AttenuateNoCusp(float distance, float radius, float intensity)
{
	float s = distance / radius;
	
	if (s >= 1.0f)
	{
		return 0.0f;
	}
	
	float s2 = sqr(s);
	
	return intensity * sqr(1 - s2) / (1 + s2);
}
#endif