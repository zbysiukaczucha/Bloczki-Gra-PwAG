#pragma once
#include <cstdint>

enum class DebugRenderMode : std::uint32_t
{
	NONE = 0,
	Albedo,
	Normals,
	AmbientOcclusion,
	Roughness,
	Metallic,
	Skylight,
	BlockLight,
	Emissive,
	Depth,
	Bloom,
	Wireframe
};
