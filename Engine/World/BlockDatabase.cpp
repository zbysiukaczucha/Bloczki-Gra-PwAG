#include "BlockDatabase.h"

#include <assert.h>

BlockDatabase& BlockDatabase::GetDatabase()
{
	static BlockDatabase instance;
	return instance;
}

const BlockData* BlockDatabase::GetBlockData(BlockType type) const
{
	if (database_.contains(type))
	{
		return &database_.at(type);
	}
	else
	{
		return nullptr;
	}
}

BlockDatabase::BlockDatabase()
{
	database_.reserve(static_cast<std::size_t>(BlockType::MAX_BLOCKS_));
	Initialize();
}

BlockData* BlockDatabase::GetMutableBlockData(BlockType type)
{
	if (database_.contains(type))
	{
		return &database_[type];
	}
	else
	{
		return nullptr;
	}
}

void BlockDatabase::Initialize()
{
	database_[BlockType::Dirt]		  = {"dirt", false, true};
	database_[BlockType::Grass]		  = {"grass", false, true};
	database_[BlockType::Stone]		  = {"stone", false, true};
	database_[BlockType::Cobblestone] = {"cobblestone", false, true};
	database_[BlockType::Glass]		  = {"glass", true, true};
	database_[BlockType::Log]		  = {"log", false, true};
	database_[BlockType::Glowstone] =
		{"glowstone", false, true, 14, {249.0f / 255.0f, 221.f / 255.0f, 160.0f / 255.0f}, 5.0f};
	database_[BlockType::DiamondBlock] = {"diamondblock", false, true};
	database_[BlockType::IronBlock]	   = {"ironblock", false, true};

	// ensure no block types are omitted
	assert(database_.size() == static_cast<std::size_t>(BlockType::MAX_BLOCKS_) - 1);
}
