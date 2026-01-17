#pragma once
#include <array>
#include <cstdint>

enum class BlockFace : std::uint8_t
{
	North  = 0, // +Z
	South  = 1, // -Z
	East   = 2, // +X
	West   = 3, // -X
	Top	   = 4, // +Y
	Bottom = 5, // -Y
};

static constexpr std::array<BlockFace, 6> ALL_BLOCKFACES = {
	BlockFace::North, BlockFace::South, BlockFace::East, BlockFace::West, BlockFace::Top, BlockFace::Bottom};
