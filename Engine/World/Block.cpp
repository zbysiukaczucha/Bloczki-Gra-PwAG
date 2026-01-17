#include "Block.h"

#include <algorithm>

static constexpr std::uint8_t SKY_LIGHT_MASK   = 0b11110000;
static constexpr std::uint8_t BLOCK_LIGHT_MASK = 0b00001111;


void Block::SetSkyLightLevel(std::uint8_t newLightLevel)
{

	newLightLevel	= std::clamp<std::uint8_t>(newLightLevel, 0, 15);
	newLightLevel <<= 4;

	// Clear the current sky light level
	lightLevel &= ~SKY_LIGHT_MASK;
	lightLevel |= newLightLevel;
}

void Block::SetBlockLightLevel(std::uint8_t newLightLevel)
{
	newLightLevel = std::clamp<std::uint8_t>(newLightLevel, 0, 15);

	// Clear the current block light level
	lightLevel &= ~BLOCK_LIGHT_MASK;
	lightLevel |= newLightLevel;
}

std::uint8_t Block::GetSkyLightLevel() const
{

	return (lightLevel & SKY_LIGHT_MASK) >> 4;
}
std::uint8_t Block::GetBlockLightLevel() const
{

	return lightLevel & BLOCK_LIGHT_MASK;
}
