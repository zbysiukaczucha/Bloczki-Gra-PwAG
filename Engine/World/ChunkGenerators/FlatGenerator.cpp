#include "FlatGenerator.h"
#include "../Chunk.h"

#include <DirectXMath.h>

#include "../BlockDatabase.h"

FlatGenerator::FlatGenerator(World* world, const ChunkTemplate& chunkTemplate) :
	IChunkGenerator(world),
	chunkTemplate_(chunkTemplate)
{
}
void FlatGenerator::FillChunk(Chunk* chunk)
{
	using namespace DirectX;

	XMINT3		   chunkWorldPos = chunk->GetChunkWorldPos();
	BlockDatabase& database		 = BlockDatabase::GetDatabase();

	for (std::size_t y = 0; y < Chunk::CHUNK_SIZE; ++y)
	{
		BlockType		 blockType = GetBlockType(chunkWorldPos.y * Chunk::CHUNK_SIZE + y);
		const BlockData* data	   = database.GetBlockData(blockType);
		for (std::size_t x = 0; x < Chunk::CHUNK_SIZE; ++x)
		{
			for (std::size_t z = 0; z < Chunk::CHUNK_SIZE; ++z)
			{
				chunk->SetBlockType(x, y, z, blockType);
				if (data)
				{
					chunk->SetSkyLightLevel(x, y, z, 0);
					chunk->SetBlockLightLevel(x, y, z, 0);
				}
				if (data && data->lightEmissionLevel > 0)
				{
					chunk->SetBlockLightLevel(x, y, z, data->lightEmissionLevel);
				}
			}
		}
	}
}

BlockType FlatGenerator::GetBlockType(std::int64_t worldY)
{
	for (std::int64_t i = 0; const auto& layer : chunkTemplate_)
	{
		if (worldY < layer.height + i)
		{
			return layer.blockType;
		}

		i += layer.height;
	}

	return BlockType::Air;
}
