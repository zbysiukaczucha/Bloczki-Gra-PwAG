#include "Chunk.h"

#include <filesystem>

Chunk::Chunk(DirectX::XMINT3 chunkWorldPos)
{
	using namespace DirectX;
	chunkWorldPos_ = chunkWorldPos;
	blocks_.fill({BlockType::Air, 0b11110000});
	dirty_ = true;

	// calculate the world matrix
	float x = static_cast<float>(chunkWorldPos.x) * static_cast<float>(CHUNK_SIZE);
	float y = static_cast<float>(chunkWorldPos.y) * static_cast<float>(CHUNK_SIZE);
	float z = static_cast<float>(chunkWorldPos.z) * static_cast<float>(CHUNK_SIZE);

	XMMATRIX mat = DirectX::XMMatrixTranslation(x, y, z);
	XMStoreFloat4x4(&chunkWorldMatrix_, mat);

	// Calculate chunk bounding box
	XMVECTOR			  pt1	 = XMVectorSet(x, y, z, 1.0f);
	const static XMVECTOR offset = XMVectorSet(CHUNK_SIZE, CHUNK_SIZE, CHUNK_SIZE, 1.0f);
	XMVECTOR			  pt2	 = pt1 + offset;
	BoundingBox::CreateFromPoints(chunkBounds_, pt1, pt2);
}

Block Chunk::GetBlock(DirectX::XMUINT3 block) const
{
	return GetBlock(block.x, block.y, block.z);
}

Block Chunk::GetBlock(std::size_t x, std::size_t y, std::size_t z) const
{
	if (x >= CHUNK_SIZE || y >= CHUNK_SIZE || z >= CHUNK_SIZE)
	{
		return {};
	}

	std::size_t index = x + (y * CHUNK_SIZE) + (z * CHUNK_SIZE * CHUNK_SIZE);
	return blocks_[index];
}

bool Chunk::SetBlockType(DirectX::XMUINT3 block, BlockType blockType)
{
	return SetBlockType(block.x, block.y, block.z, blockType);
}

bool Chunk::SetSkyLightLevel(DirectX::XMUINT3 block, std::uint8_t lightLevel)
{
	return SetSkyLightLevel(block.x, block.y, block.z, lightLevel);
}
bool Chunk::SetBlockLightLevel(DirectX::XMUINT3 block, std::uint8_t lightLevel)
{
	return SetBlockLightLevel(block.x, block.y, block.z, lightLevel);
}

bool Chunk::SetSkyLightLevel(std::size_t x, std::size_t y, std::size_t z, std::uint8_t lightLevel)
{
	if (x >= CHUNK_SIZE || y >= CHUNK_SIZE || z >= CHUNK_SIZE)
	{
		return false;
	}

	std::size_t index = x + (y * CHUNK_SIZE) + (z * CHUNK_SIZE * CHUNK_SIZE);
	blocks_[index].SetSkyLightLevel(lightLevel);
	dirty_ = true;
	return true;
}

bool Chunk::SetBlockLightLevel(std::size_t x, std::size_t y, std::size_t z, std::uint8_t lightLevel)
{
	if (x >= CHUNK_SIZE || y >= CHUNK_SIZE || z >= CHUNK_SIZE)
	{
		return false;
	}

	std::size_t index = x + (y * CHUNK_SIZE) + (z * CHUNK_SIZE * CHUNK_SIZE);
	blocks_[index].SetBlockLightLevel(lightLevel);
	dirty_ = true;
	return true;
}

void Chunk::GetBorderSlice(std::array<Block, CHUNK_SIZE * CHUNK_SIZE>& outBlocks, BlockFace direction) const
{
	constexpr std::size_t strideX = 1;
	constexpr std::size_t strideY = CHUNK_SIZE;
	constexpr std::size_t strideZ = CHUNK_SIZE * CHUNK_SIZE;

	std::size_t startIndex = 0;
	std::size_t strideU	   = 0;
	std::size_t strideV	   = 0;

	switch (direction)
	{
		case BlockFace::North:
		{
			std::size_t targetZ = CHUNK_SIZE - 1;
			startIndex			= targetZ * strideZ;
			strideU				= strideX;
			strideV				= strideY;
			break;
		}
		case BlockFace::South:
		{
			std::size_t targetZ = 0;
			startIndex			= targetZ * strideZ;
			strideU				= strideX;
			strideV				= strideY;
			break;
		}
		case BlockFace::West:
		{
			std::size_t targetX = 0;
			startIndex			= targetX * strideX;
			strideU				= strideZ;
			strideV				= strideY;
			break;
		}
		case BlockFace::East:
		{
			std::size_t targetX = CHUNK_SIZE - 1;
			startIndex			= targetX * strideX;
			strideU				= strideZ;
			strideV				= strideY;
			break;
		}
		case BlockFace::Top:
		{
			std::size_t targetY = CHUNK_SIZE - 1;
			startIndex			= targetY * strideY;
			strideU				= strideX;
			strideV				= strideZ;
			break;
		}
		case BlockFace::Bottom:
		{
			std::size_t targetY = 0;
			startIndex			= targetY * strideY;
			strideU				= strideX;
			strideV				= strideZ;
			break;
		}
	}

	std::size_t readPtr	 = startIndex;
	std::size_t writePtr = 0;
	for (std::size_t v = 0; v < CHUNK_SIZE; v++)
	{
		std::size_t rowStart = readPtr;
		for (std::size_t u = 0; u < CHUNK_SIZE; u++)
		{
			outBlocks[writePtr]	 = blocks_[readPtr];
			readPtr				+= strideU;
			++writePtr;
		}
		readPtr = rowStart + strideV;
	}
}
std::vector<BlockFace> Chunk::IsBlockOnBorder(DirectX::XMINT3 block)
{
	return IsBlockOnBorder(block.x, block.y, block.z);
}

std::vector<BlockFace> Chunk::IsBlockOnBorder(std::size_t x, std::size_t y, std::size_t z)
{
	if (x >= CHUNK_SIZE || y >= CHUNK_SIZE || z >= CHUNK_SIZE)
	{
		// Block lies outside the chunk
		return {};
	}

	std::vector<BlockFace> borderDirections;
	borderDirections.reserve(3);

	if (x == CHUNK_SIZE - 1)
	{
		borderDirections.push_back(BlockFace::East);
	}
	else if (x == 0)
	{
		borderDirections.push_back(BlockFace::West);
	}

	if (y == CHUNK_SIZE - 1)
	{
		borderDirections.push_back(BlockFace::Top);
	}
	else if (y == 0)
	{
		borderDirections.push_back(BlockFace::Bottom);
	}

	if (z == CHUNK_SIZE - 1)
	{
		borderDirections.push_back(BlockFace::North);
	}
	else if (z == 0)
	{
		borderDirections.push_back(BlockFace::South);
	}

	return borderDirections;
}

bool Chunk::SetBlockType(std::size_t x, std::size_t y, std::size_t z, BlockType blockType)
{
	if (blockType == BlockType::MAX_BLOCKS_ || blockType == BlockType::INVALID_)
	{
		return false;
	}

	if (x >= CHUNK_SIZE || y >= CHUNK_SIZE || z >= CHUNK_SIZE)
	{
		return false;
	}

	std::size_t index = x + (y * CHUNK_SIZE) + (z * CHUNK_SIZE * CHUNK_SIZE);

	blocks_[index].type = blockType;
	dirty_				= true;

	return true;
}
