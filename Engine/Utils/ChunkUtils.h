#pragma once
#include <DirectXMath.h>
#include <cmath>

namespace Utils::Coordinates
{
	// only positive numbers
	constexpr bool IsPowerOfTwo(std::size_t x)
	{
		return x > 0 && x % 2 == 0;
	}

	constexpr std::size_t GetLog2(std::size_t x)
	{
		std::size_t result = 0;
		while ((x >>= 1))
		{
			result++;
		}
		return result;
	}

	/**
	 * THIS DOES NOT RETURN CHUNK-SPACE COORDINATES OF A BLOCK
	 * @tparam chunkSize size of chunks, preferably a power of 2
	 * @param blockCoordinate WORLD-SPACE BLOCK COORDINATES
	 * @return world coordinates of the chunk the block lies in
	 */
	template <std::size_t chunkSize, typename coordType>
	inline std::int32_t GetChunkCoordinate(coordType blockCoordinate)
	{
		if constexpr (std::is_floating_point_v<coordType>)
		{
			const auto coord = static_cast<std::int32_t>(std::floor(blockCoordinate));
			if constexpr (IsPowerOfTwo(chunkSize))
			{
				return coord >> GetLog2(chunkSize);
			}
			else
			{
				return static_cast<std::int32_t>(std::floor(blockCoordinate / chunkSize));
			}
		}
		else
		{
			if constexpr (IsPowerOfTwo(chunkSize))
			{
				return static_cast<std::int32_t>(blockCoordinate >> GetLog2(chunkSize));
			}
			else
			{
				return static_cast<std::int32_t>(std::floor(static_cast<float>(blockCoordinate) / chunkSize));
			}
		}
	}
} // namespace Utils::Coordinates
