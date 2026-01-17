#ifndef GEOMETRY_PASS_HLSL
#define GEOMETRY_PASS_HLSL

#include "ShaderCommons.hlsl"

struct VS_Input
{
	float3 position	  : POSITION;
	float3 normal	  : NORMAL;
	float3 tangent    : TANGENT;
	float3 bitangent  : BITANGENT;
	float2 texcoord   : TEXCOORD0;
	uint   materialID : TEXCOORD1;
	uint   lightLevel : TEXCOORD2;
	
};

struct PS_Input
{
	float4               position      : SV_POSITION;
	float3               normal		   : NORMAL;
	float3				 tangent	   : TANGENT;
	float3				 bitangent	   : BITANGENT;
	float2               texcoord	   : TEXCOORD0;
	
	nointerpolation uint materialID      : TEXCOORD1;
	float3				 tangentViewDir  : TEXCOORD2;
	float3				 tangentLightDir : TEXCOORD4;
	uint				 lightLevel		 : TEXCOORD5;
};

struct PS_Output
{
	float4 albedo   : SV_Target0;
	float3 normal   : SV_Target1;
	float4 ORMV     : SV_Target2;
	float4 emissive : SV_Target3;
};

struct MaterialData
{
	uint albedoIdx;
	uint normalIdx;
	uint ARMIdx; // Ambient, Roughness, Metallic
	uint heightmapIdx;
	uint emissiveIdx;
};

Texture2DArray albedoTextures    : register(t1);
Texture2DArray normalTextures    : register(t2);
Texture2DArray ARMTextures       : register(t3);
Texture2DArray heightMapTextures : register(t4);
Texture2DArray emissiveTextures  : register(t5);

StructuredBuffer<MaterialData> materialBuffer : register(t6);

SamplerState basicSampler : register(s0);


PS_Input VS_Main(VS_Input input)
{
	PS_Input output;

	// Transform position to world space
	float4 worldPosition = mul(float4(input.position, 1.0f), world);

	output.position = mul(worldPosition, viewProjection);

	// Transform normal to world space
	output.normal = normalize(mul(input.normal, (float3x3)world));
	
	// Transform tangent to world space
	output.tangent = normalize(mul(input.tangent, (float3x3)world));
	
	// Transform bitangent to world space
	output.bitangent = normalize(mul(input.bitangent, (float3x3)world));

	// Pass through texture coordinates and material ID
	output.texcoord   = input.texcoord;
	output.materialID = input.materialID;
	
	float3x3 TBN = float3x3(input.tangent, input.bitangent, input.normal);
	
	float3 worldViewDirection = cameraPosition.xyz  - worldPosition.xyz;
	output.tangentViewDir = mul(TBN, worldViewDirection);
	output.tangentLightDir = mul(TBN, -skyLightDirection.xyz);
	
	output.lightLevel = input.lightLevel;

	return output;
}

float2 GetParallaxCoord(
	float2 uv,                  // The original mesh UVs
	float3 tangentViewDir,      // Vector from Surface -> Camera (in Tangent Space)
	float textureArrayIndex,    // Which slice of the Texture2DArray to use
	float parallaxScale,        // Depth intensity (should be low, something like 0.05f)
	float quality,              // Loop count (more => better parallax, but more performance-heavy)
	out float surfaceHeight     // OUTPUT: The height where the ray hit
);

float GetSelfShadow(
	float2 initialUV,       // The UV coordinates RESULTING from the Parallax function
	float initialHeight,    // The 'surfaceHeight' returned by the Parallax function
	float3 tangentLightDir, // The light vector in Tangent Space
	float textureIndex,     // For the Texture2DArray
	float parallaxScale,    // Must match the scale used in GetParallaxCoord
	float quality,          // Shadow quality (steps)
	float shadowStrength    // How dark the shadows are (softness factor)
);

PS_Output PS_Main(PS_Input input) : SV_TARGET
{
	// Parallax mapping
    // Calculate View Direction in Tangent Space
	float3 tangentViewDir = normalize(input.tangentViewDir);
	float3 tangentLightDir = normalize(input.tangentLightDir);
    
    uint normalIndex = materialBuffer[input.materialID].normalIdx;
	uint heightmapIndex = materialBuffer[input.materialID].heightmapIdx;
    
	float depth = 0;
	const float parallaxScale = 0.065f;
	const float parallaxQuality = 64.f;
	float2 texCoords = GetParallaxCoord(input.texcoord, tangentViewDir, heightmapIndex, parallaxScale, parallaxQuality, depth);
	float POMShadowFactor = GetSelfShadow(texCoords, depth, tangentLightDir, heightmapIndex, parallaxScale, parallaxQuality, 0.5f);

	// Sample the remaining textures with UVs from parallax
    uint albedoIndex   = materialBuffer[input.materialID].albedoIdx;
    uint ARMIndex      = materialBuffer[input.materialID].ARMIdx;
	uint emissiveIndex = materialBuffer[input.materialID].emissiveIdx;

    float4 albedo = albedoTextures.Sample(basicSampler, float3(texCoords, albedoIndex));
    float4 sampledNormal = normalTextures.Sample(basicSampler, float3(texCoords, normalIndex));
    float4 arm    = ARMTextures.Sample(basicSampler, float3(texCoords, ARMIndex));
	float4 emissive = emissiveTextures.Sample(basicSampler, float3(texCoords, emissiveIndex));
	
	// convert normal from [0, 1] (texture range) to [-1, 1]
	sampledNormal = sampledNormal * 2.0f - 1.0f;
	

	float3 T = normalize(input.tangent);
	float3 B = normalize(input.bitangent);
	float3 N = normalize(input.normal);
	
	float3x3 TBN = float3x3(T, B, N);
	
	// transform to view space
	float3 worldNormal = mul(sampledNormal.xyz, TBN);
	float3 normal = normalize(mul(worldNormal, (float3x3)view));
	
	PS_Output output;
	output.albedo = float4(albedo.rgb, POMShadowFactor);
	output.normal = normal * 0.5f + 0.5f;
	output.ORMV = float4(arm.rgb, float(input.lightLevel)/ 255.0f);
	output.emissive = emissive;
	
	return output;
}


// Returns the new UV coordinates.
// 'surfaceHeight' is an output used later for the shadow calculation.
float2 GetParallaxCoord(
    float2 uv,                  // The original mesh UVs
    float3 tangentViewDir,      // Vector from Surface -> Camera (in Tangent Space)
    float textureArrayIndex,    // Which slice of the Texture2DArray to use
    float parallaxScale,        // Depth intensity (e.g. 0.05)
    float quality,              // Loop count (e.g. 16 to 64)
    out float surfaceHeight     // OUTPUT: The height where the ray hit
) 
{
    // 1. Setup step size
    float sampleStep = 1.0 / quality;
    float currentRayHeight = 1.0; // The ray starts at the "top" of the surface
    
    // 2. Calculate the vector to step along the texture
    // We divide by Z to account for perspective (looking at a slant covers more pixels).
    // Note: tangentViewDir.z must be > 0.0 (pointing out of the surface).
    float2 scaledDir = (tangentViewDir.xy / tangentViewDir.z) * parallaxScale;
    float2 stepDir = scaledDir * sampleStep;

    float2 currentUV = uv;
    
    // 3. Initial Sample
    float currentTextureHeight = heightMapTextures.Sample(basicSampler, float3(currentUV, textureArrayIndex), 0).r;

    // 4. The Steep Parallax Loop
    // We loop until the Ray Height dips BELOW the Texture Height
    [unroll]
    for(int i = 0; i < (int)quality; i++) 
    {
        // If the ray is currently below the surface, we hit!
        if (currentRayHeight <= currentTextureHeight) 
        {
            break; 
        }

        // Shift the UV coordinate to the "next slice"
        // We subtract because we are "pushing" the image deeper into the screen
        currentUV -= stepDir;
        
        // Move the ray down one slice
        currentRayHeight -= sampleStep;

		currentTextureHeight = heightMapTextures.Sample(basicSampler, float3(currentUV, textureArrayIndex), 0).r;
    }

    // Output the final height (used for self-shadowing later)
    surfaceHeight = currentRayHeight;

    return currentUV;
}

float GetSelfShadow(
    float2 initialUV,       // The UV coordinates RESULTING from your Parallax function
    float initialHeight,    // The 'surfaceHeight' returned by your Parallax function
    float3 tangentLightDir, // The light vector in Tangent Space
    float textureIndex,     // For your Texture2DArray
    float parallaxScale,    // Must match the scale used in GetParallaxCoord
    float quality,          // Shadow quality (steps)
    float shadowStrength    // How dark the shadows are (softness factor)
)
{
    // 0. Safety Check: If the surface is fully lit (height 1.0), skip
    // Or if the light is pointing "down" into the surface (backface), it's self-shaded anyway.
    if (tangentLightDir.z <= 0.0) return 0.0;

    // 1. Setup the Ray
    // We start AT the surface height we found previously
    float currentHeight = initialHeight;
    float sampleStep = 1.0 / quality;
    
    // We calculate the direction to move along the texture to "chase" the light
    // Note: This logic mirrors the Parallax function but points UP/TOWARDS light
    float2 lightStepDir = (tangentLightDir.xy / tangentLightDir.z) * parallaxScale * sampleStep;

    // We start at the modified UVs from the previous step
    float2 currentUV = initialUV;
    
    // This will hold our shadow factor (1.0 = Fully Lit, 0.0 = Fully In Shadow)
    float shadowFactor = 1.0;

    // 2. The Loop (Marching UP towards the light)
    // We stop if currentHeight > 1.0 (we escaped the texture volume)
	[unroll]
    for (int i = 0; i < (int)quality; i++)
    {
        // Move the ray UP
        currentHeight += sampleStep;
        
        // Move across the texture
        currentUV += lightStepDir;
        
        // If we have gone above the top of the texture, we are done
        if (currentHeight > 1.0) break;

        // Sample the height at this new location
        float heightAtNewPoint = heightMapTextures.SampleLevel(basicSampler, float3(currentUV, textureIndex), 0).a;

        // If the texture height here is TALLER than our ray, we are blocked!
        if (heightAtNewPoint > currentHeight)
        {
            // Soft Shadow Calculation (from the original GLSL of BSL Minecraft Shaders):
            // The deeper the intersection, the darker the shadow.
            float blockerDepth = heightAtNewPoint - currentHeight;
            float s = 1.0 - blockerDepth * shadowStrength;
            
            // Keep the darkest shadow we found along the path
            shadowFactor = min(shadowFactor, s);
        }
    }

    return saturate(shadowFactor);
}

#endif