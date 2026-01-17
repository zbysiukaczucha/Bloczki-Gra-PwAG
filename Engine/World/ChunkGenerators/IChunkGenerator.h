#pragma once

enum class GeneratorType
{
	RANDOM,
	FLAT,
};
class World;
class IChunkGenerator
{

public:
	IChunkGenerator()										 = delete;
	IChunkGenerator(const IChunkGenerator& other)			 = delete;
	IChunkGenerator(IChunkGenerator&& other)				 = delete;
	IChunkGenerator& operator=(const IChunkGenerator& other) = delete;
	IChunkGenerator& operator=(IChunkGenerator&& other)		 = delete;

	IChunkGenerator(World* world) :
		world_(world)
	{
	}
	virtual ~IChunkGenerator() = default;

	virtual void FillChunk(class Chunk* chunk) = 0;

protected:
	World* world_;
};
