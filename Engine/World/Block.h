#pragma once
#include <cstdint>
#include "BlockType.h"

struct Block
{
	BlockType	 type		= BlockType::INVALID_;
	std::uint8_t lightLevel = 0b11110000; // 4 LSbits - block light like torches, 4MSbits - skylight

	// helpers
	void		 SetSkyLightLevel(std::uint8_t newLightLevel);
	void		 SetBlockLightLevel(std::uint8_t newLightLevel);
	std::uint8_t GetSkyLightLevel() const;
	std::uint8_t GetBlockLightLevel() const;
};
