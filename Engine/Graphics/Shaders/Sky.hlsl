#include "ShaderCommons.hlsl"

cbuffer SkyBuffer : register(b2)
{
	float4 zenithColor;
	float4 horizonColor;
	float4 sunDirection;
}

struct PS_Input
{
	float4 position : SV_POSITION;
	float4 clipPosition : TEXCOORD;
};

PS_Input VS_Main(uint vIdx : SV_VertexID)
{
	PS_Input output;

	// 1. Generate a Full Screen Triangle (covers -1 to 1 range)
	// ID 0 -> (-1, -1)
	// ID 1 -> (-1,  3)
	// ID 2 -> ( 3, -1)
	float2 texcoord = float2((vIdx << 1) & 2, vIdx & 2);
	output.position = float4(texcoord * float2(2, -2) + float2(-1, 1), 1.0, 1.0);
	// 2. Force Z to Far Plane (Clip Space Z=1)
	output.clipPosition = output.position;
    
	return output;
}

// A simple hashing function to generate pseudo-random numbers
// Input: A 3D point. Output: A random value between 0 and 1.
float hash(float3 p)
{
	p = frac(p * 0.3183099 + 0.1);
	p *= 17.0;
	return frac(p.x * p.y * p.z * (p.x + p.y + p.z));
}

// 3D Value Noise: Interpolates between random hash values on a grid
float noise(float3 x)
{
	float3 i = floor(x);
	float3 f = frac(x);
	f = f * f * (3.0 - 2.0 * f); // Smoothstep curve for smooth interpolation
    
	return lerp(lerp(lerp(hash(i + float3(0,0,0)), hash(i + float3(1,0,0)), f.x),
					 lerp(hash(i + float3(0,1,0)), hash(i + float3(1,1,0)), f.x), f.y),
				lerp(lerp(hash(i + float3(0,0,1)), hash(i + float3(1,0,1)), f.x),
					 lerp(hash(i + float3(0,1,1)), hash(i + float3(1,1,1)), f.x), f.y), f.z);
}

// Fractal Brownian Motion (FBM): Layers multiple noise octaves
// This creates "large continents" and "tiny craters" simultaneously.
float fbm(float3 x)
{
	float v = 0.0;
	float a = 0.5;
	// Layer 1: Base shape
	v += a * noise(x); x *= 2.0; a *= 0.5;
	// Layer 2: Medium details
	v += a * noise(x); x *= 2.0; a *= 0.5;
	// Layer 3: Small craters
	v += a * noise(x); x *= 2.0; a *= 0.5;
	return v;
}

// Generates a starfield by looking for high-value noise peaks
float GetStars(float3 viewDir)
{
	// 1. Scale the space significantly to make "tiny" features
	// Changing '300.0' changes the density/size of the star field
	float3 p = viewDir * 300.0;
    
	// 2. Sample the noise
	float s = noise(p);
    
	// 3. Sharpen the results to create dots
	// pow(s, 40) pushes everything below 0.9 down to near zero.
	// The result is mostly black (0), with rare spikes of 1.
	// pow(s, 40) turned out to be not enough :p
	s = pow(s, 110.0); 
    
	// 4. Boost brightness so the faint stars are visible
	return s * 10.0; 
}

float4 PS_Main(PS_Input input) : SV_TARGET
{
	static const float PI = 3.14159265358979323846f;
	float4 viewPos = mul(input.clipPosition, inverseProjection);
	viewPos /= viewPos.w;
	
	float3 viewDir = normalize(viewPos.xyz);
	
	float3 vDir = mul(viewDir, (float3x3)inverseView);
	vDir = normalize(vDir);
	
	// Base sky gradient
	float gradientFactor = saturate(vDir.y);
	
	// push the horizon color a bit lower
	gradientFactor = pow(gradientFactor, 0.5f);
	
	float4 skyColor = lerp(horizonColor, zenithColor, gradientFactor);
	
	// Draw the sun (sun direction is the light direction)
	float sunDot = saturate(dot(vDir, -sunDirection));
	float haloIntensity = pow(sunDot, 200.f);
	float sunDiscIntensity = pow(sunDot, 2000.f);
	
	float3 sunColor = float3(1.0f, 0.9f, 0.7f);
	skyColor.rgb += haloIntensity * 0.5 * sunColor;
	skyColor.rgb += sunDiscIntensity * 10.0f * sunColor;
	
	float3 moonDir = normalize(sunDirection); // Opposite to sun
	float moonDot = dot(vDir, moonDir);
    
	// 2. Define Moon Size (Cosine of the angle)
	// 0.999 is roughly a 2.5 degree disc.
	float moonRadius = 0.999; 
    
	float moonMask = smoothstep(moonRadius, moonRadius + 0.001, moonDot);
	
	float3 sunVec = normalize(-sunDirection);
    
	// If sun is below -0.2 (well below horizon) -> Night (Stars visible)
	// If sun is above 0.1 (just above horizon) -> Day (Stars invisible)
	// We blend smoothly between these two states (Twilight)
	float starVisibility = smoothstep(0.1, -0.2, sunVec.y);

	if (starVisibility > 0.0)
	{
		float3 stars = GetStars(vDir);
		// MASK A: Don't draw stars on top of the Moon
		stars *= (1.0 - moonMask); 
    
		// MASK B: Don't draw stars on top of the Sun
		stars *= (1.0 - sunDiscIntensity);
    
		// MASK C: Atmosphere / Horizon Fade
		// Stars shouldn't be visible near the horizon (thick atmosphere blocks them)
		// We define a "cutoff" starting at 20% up the sky
		float horizonFade = smoothstep(0.0, 0.2, vDir.y); 
		stars *= horizonFade;

		skyColor.rgb += stars;
	}
    
	if (moonMask > 0.0)
	{
		// We project the view direction into a noise function.
		// Multiply vDir by a frequency (e.g., 100) to control crater size.
		// Calculate a local coordinate system attached to the moon
		// As the moon moves, 'moonUV' moves with it.
		float3 moonUV = vDir - moonDir; 

		// Use this local coordinate for the noise
		// We multiply by 20.0 to tile the texture across the moon face
		float craters = fbm(moonUV * 100.0);
        
		// Tweak the noise to look like moon surface
		// Brighten it up and contrast it
		float moonTexture = smoothstep(0.2, 0.8, craters); 
        
		float3 baseMoonColor = float3(0.55f, 0.6f, 0.71f);
        
		// Combine texture with base color
		float3 finalMoonColor = baseMoonColor * (0.5 + 0.5 * moonTexture);
        
		// Add to sky
		// moonMask handles the shape. 
		// We multiply by moonMask to ensure we only draw inside the circle.
		skyColor.rgb += moonMask * finalMoonColor * 2.0; // * 2.0 for brightness
	}
	
	// This happens outside the hard moon disc too, so it's separate from the if or mask
	float moonGlow = pow(saturate(moonDot), 400.0f); 
	skyColor.rgb += moonGlow * 0.1 * float3(0.4, 0.5, 0.8); // Blueish glow
	
	skyColor.a = 1.0f;
	return skyColor;
}