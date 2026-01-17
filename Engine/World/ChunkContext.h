#pragma once
#include "Block.h"
#include "Chunk.h"

class Chunk;
struct ChunkContext
{
	using ChunkSlice = std::array<Block, Chunk::CHUNK_SIZE * Chunk::CHUNK_SIZE>;

	std::array<Block, Chunk::CHUNK_SIZE * Chunk::CHUNK_SIZE * Chunk::CHUNK_SIZE> mainChunk;

	ChunkSlice northNeighbor;
	ChunkSlice southNeighbor;
	ChunkSlice westNeighbor;
	ChunkSlice eastNeighbor;
	ChunkSlice topNeighbor;
	ChunkSlice bottomNeighbor;

	std::array<bool, 6> hasNeighbors;

	DirectX::XMINT3 mainChunkCoordinates;
};
