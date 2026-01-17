#pragma once
#include <DirectXMath.h>
#include <functional>

namespace Math
{
	struct XMINT3Hash
	{
		std::size_t operator()(const DirectX::XMINT3& k) const
		{
			// XOR Shift method
			std::size_t h1 = std::hash<std::int32_t>()(k.x);
			std::size_t h2 = std::hash<std::int32_t>()(k.y);
			std::size_t h3 = std::hash<std::int32_t>()(k.z);

			std::size_t seed  = 0;
			seed			 ^= h1 + 0x9e3779b9 + (seed << 6) + (seed >> 2);
			seed			 ^= h2 + 0x9e3779b9 + (seed << 6) + (seed >> 2);
			seed			 ^= h3 + 0x9e3779b9 + (seed << 6) + (seed >> 2);
			return seed;
		}
	};
} // namespace Math
