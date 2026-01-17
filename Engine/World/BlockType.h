#pragma once
#include <cstddef>

enum class BlockType : std::size_t
{
	Air,
	Dirt,
	Grass,
	Stone,
	Cobblestone,
	Glass,
	Log,
	Glowstone,
	DiamondBlock,
	IronBlock,


	MAX_BLOCKS_,
	INVALID_
};
