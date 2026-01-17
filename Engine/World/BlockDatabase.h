#pragma once
#include <unordered_map>

#include "BlockData.h"
#include "BlockType.h"

class BlockDatabase
{
	friend class TextureManager;

public:
	BlockDatabase(const BlockDatabase&)			   = delete;
	BlockDatabase& operator=(const BlockDatabase&) = delete;

	BlockDatabase(BlockDatabase&&)			  = delete;
	BlockDatabase& operator=(BlockDatabase&&) = delete;

	[[nodiscard]] static BlockDatabase& GetDatabase();
	[[nodiscard]] const BlockData*		GetBlockData(BlockType) const;

private:
	BlockDatabase();

	// Mainly for TextureManager to fill the texture array coordinates
	[[nodiscard]] BlockData* GetMutableBlockData(BlockType);
	void					 Initialize();

	std::unordered_map<BlockType, BlockData> database_;
};
