#pragma once
#include <vector>


#include "../BlockType.h"
#include "IChunkGenerator.h"


struct ChunkLayer
{
	BlockType	blockType;
	std::size_t height; // world-space y limit
};

class FlatGenerator : public IChunkGenerator
{
public:
	using ChunkTemplate = std::vector<ChunkLayer>;

	FlatGenerator(World* world, const ChunkTemplate& chunkTemplate);
	void FillChunk(Chunk* chunk) override;

private:
	BlockType	  GetBlockType(std::int64_t worldY);
	ChunkTemplate chunkTemplate_;
};
