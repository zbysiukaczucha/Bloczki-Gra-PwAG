#include "ShaderCommons.hlsl"
#include "DeferredCommons.hlsl"

Texture2D shadowMap : register(t5);
cbuffer SkyBuffer : register(b2)
{
	float4 zenithColor;
	float4 horizonColor;
	float4 sunDirection;
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

float3 SimpleSkyColor(float3 rayDirection);
float CalculateShadow(float4 lightSpacePos, float3 normal, float3 lightDir);
float4 PS_Main(PS_Input input) : SV_TARGET
{
	float2 uv = input.texcoord;
	const float4 basecolor = albedo.Sample(pointSampler, uv);
	const float4 ormv = ORMV.Sample(pointSampler, uv);
	
	float Z = depth.Sample(pointSampler, uv);
	if (Z >= 1.0f) //prevent sky NaN
	{
		return float4(0.0f, 0.0f, 0.0f, 0.0f);
	}
	const float3 fragWorldPos = CalculateWorldPos(uv, Z, inverseViewProjection);
	
	const float3 sampledNormal = normal.Sample(pointSampler, uv);
	
	float3 N = normalize(sampledNormal * 2.0f - 1.0f);
	const float3 V = normalize(mul(cameraPosition.rgb - fragWorldPos, (float3x3)view));
	const float3 L = normalize(mul(-skyLightDirection.rgb, (float3x3)view));
	const float3 H = normalize(L + V);
	const float3 R = reflect(-V, N); 
	
	const float NdotL = saturate(dot(N, L));
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
	const float roughnessSqr = roughness * roughness;
	const float alphaSqr = roughnessSqr * roughnessSqr; // Disney uses roughness ^ 4
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
	const float3 numerator = D *F * G;
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
	
	float shadowFactor = 1.0f - CalculateShadow(mul(float4(fragWorldPos, 1.0f), lightSpaceMatrix), N, skyLightDirection);
	// shadowFactor = 1.0f;
	
	// Final composition
	// We multiply by NdotL because light spreats out over the surface area
	const float POMShadowFactor = basecolor.a;
	 float3 directLight = (diffuse + specular) * NdotL * POMShadowFactor * shadowFactor;
	
	// Simple sky reflection, mainly to make metallic surfaces have something to reflect
	float3 skyColor = SimpleSkyColor(mul(R, (float3x3) inverseView));
	float3 fresnel = F0 + (max(float3(1.0f - roughness, 1.0f - roughness, 1.0f - roughness), F0) - F0) * pow(1.0f - NdotV, 5.0f);
	skyColor *= saturate(1.0f - roughness) * fresnel * NdotL;
	directLight += skyColor;
	
	const uint voxelLighting = ormv.a * 255.0f;
	const float skyLightLevel = ((voxelLighting & 0xF0) >> 4) / 15.0f;
	directLight *= skyLightLevel;
	
	return float4(directLight, 1.0f);
}

// Helper function to calculate shadow factor
// Returns 1.0 (In Shadow) or 0.0 (Lit)
float CalculateShadow(float4 lightSpacePos, float3 normal, float3 lightDir)
{
	// 1. Perspective Divide
	// Converts from Clip Space (Homogeneous) to NDC [-1, 1]
	// For Ortho projection w is 1.0, but good to keep for correctness.
	float3 projCoords = lightSpacePos.xyz / lightSpacePos.w;
	projCoords = lightSpacePos;

	// 2. Texture Coordinate Mapping
	// NDC [-1, 1] -> UV [0, 1]
	// In DirectX:
	// X: -1 -> 0, 1 -> 1  =>  x * 0.5 + 0.5
	// Y: -1 -> 1, 1 -> 0  =>  -y * 0.5 + 0.5 (Flip Y!)
	float2 shadowUV;
	shadowUV.x = projCoords.x * 0.5 + 0.5;
	shadowUV.y = -projCoords.y * 0.5 + 0.5;

	// 3. Check if outside map bounds
	// If the pixel is outside the shadow map area, assume it's lit
	if (shadowUV.x < 0.0 || shadowUV.x > 1.0 || shadowUV.y < 0.0 || shadowUV.y > 1.0 || projCoords.z > 1.0f)
		return 0.0;

	// 4. The Depth Comparison
	// closestDepth = The nearest object to the sun (from the texture)
	// currentDepth = How far THIS pixel is from the sun
	float closestDepth = shadowMap.Sample(shadowSampler, shadowUV).r;
	float currentDepth = projCoords.z;

	// 5. Test
	// If currentDepth is larger than closestDepth, we are behind something.
	float shadow = currentDepth > closestDepth ? 1.0 : 0.0;

	return shadow;
}

// Simplified "Sky" shader, used for reflections
// Ray direction assumed in world space 
float3 SimpleSkyColor(float3 rayDirection)
{
	
	if (rayDirection.y < 0.0f)
	{
		return float3(0.0f, 0.0f, 0.0f);
	}
	rayDirection = normalize(rayDirection);
	float gradientFactor = saturate(rayDirection.y);
	gradientFactor = pow(gradientFactor, 0.5f);
	float3 skyColor= lerp(horizonColor, zenithColor, gradientFactor);
	return skyColor;
	
	float sunDot = saturate(dot(rayDirection, -sunDirection));
	float haloIntensity = pow(sunDot, 200.f);
	float sunDiscIntensity = pow(sunDot, 2000.f);
	
	float3 sunColor = float3(1.0f, 0.9f, 0.7f);
	skyColor.rgb += haloIntensity * 0.5 * sunColor;
	skyColor.rgb += sunDiscIntensity * 10.0f * sunColor;
	
	return skyColor;
}
