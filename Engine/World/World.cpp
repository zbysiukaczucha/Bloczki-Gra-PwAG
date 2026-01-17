#include "World.h"

#include <algorithm>
#include <iostream>
#include <ranges>

#include "../Core/Timer.h"
#include "../Graphics/Mesher.h"
#include "../Utils/ChunkUtils.h"
#include "BlockDatabase.h"
#include "Chunk.h"
#include "ChunkGenerators/FlatGenerator.h"
World::World() :
	shuttingDown_(false),
	device_(nullptr),
	deviceContext_(nullptr),
	lightEngine_(this),
	timeOfDay_(0.5f)
{
}

World::~World()
{
	{
		std::unique_lock<std::mutex> lock(jobQueueMutex_);
		shuttingDown_ = true;
	}

	jobQueueCondition_.notify_all();

	for (auto& t : workers_)
	{
		t.join();
	}
}


bool World::Initialize(ID3D11Device* device)
{
	auto threadCount = (std::max)(std::thread::hardware_concurrency() - 1, 1u);
	for (unsigned i = 0; i < threadCount; i++)
	{
		workers_.emplace_back(&World::MesherLoop, this);
	}
	device_ = device;
	assert(device_ != nullptr);
	return true;
}

void World::GenerateTestChunks()
{
	DirectX::XMINT3 chunk1{0, 0, 0};
	DirectX::XMINT3 chunk2{0, 1, 0};
	DirectX::XMINT3 chunk3{0, -1, 0};
	DirectX::XMINT3 chunk4{1, 0, 0};
	DirectX::XMINT3 chunk5{-1, 0, 0};

	chunks_[chunk1] = std::make_unique<Chunk>(chunk1);
	chunks_[chunk2] = std::make_unique<Chunk>(chunk2);
	chunks_[chunk3] = std::make_unique<Chunk>(chunk3);
	chunks_[chunk4] = std::make_unique<Chunk>(chunk4);
	// chunks_[chunk5] = std::make_unique<Chunk>(chunk5);

	for (std::uint8_t z = 0; z < Chunk::CHUNK_SIZE; z++)
	{
		for (std::uint8_t y = 0; y < Chunk::CHUNK_SIZE; y++)
		{
			for (std::uint8_t x = 0; x < Chunk::CHUNK_SIZE; x++)
			{
				chunks_[chunk1]->SetBlockType(x, y, z, BlockType::Cobblestone);
				chunks_[chunk1]->SetBlockLightLevel(x, y, z, 0);
				chunks_[chunk1]->SetSkyLightLevel(x, y, z, 0);

				chunks_[chunk2]->SetBlockType(x, y, z, BlockType::Stone);
				chunks_[chunk2]->SetBlockLightLevel(x, y, z, 0);
				chunks_[chunk2]->SetSkyLightLevel(x, y, z, 0);

				chunks_[chunk3]->SetBlockType(x, y, z, BlockType::Dirt);
				chunks_[chunk3]->SetBlockLightLevel(x, y, z, 0);
				chunks_[chunk3]->SetSkyLightLevel(x, y, z, 0);


				chunks_[chunk4]->SetBlockType(x, y, z, BlockType::Log);
				chunks_[chunk4]->SetBlockLightLevel(x, y, z, 0);
				chunks_[chunk4]->SetSkyLightLevel(x, y, z, 0);
			}
		}
	}

	RequestChunkMeshUpdate(chunks_[chunk1].get());
	RequestChunkMeshUpdate(chunks_[chunk2].get());
	RequestChunkMeshUpdate(chunks_[chunk3].get());
	RequestChunkMeshUpdate(chunks_[chunk4].get());
	// RequestChunkMeshUpdate(chunks_[chunk5].get());
}

void World::GenerateTestWorld()
{
	FlatGenerator::ChunkTemplate chunkTemplate{
		{BlockType::Stone, 60},
		{BlockType::Dirt, 20},
		{BlockType::Grass, 1},
	};

	FlatGenerator generator(this, chunkTemplate);
	for (int z = -8; z < 8; ++z)
	{
		for (int x = -8; x < 8; ++x)
		{
			for (int y = 0; y < 16; ++y)
			{
				chunks_[{x, y, z}] = std::make_unique<Chunk>(DirectX::XMINT3{x, y, z});
				generator.FillChunk(chunks_[{x, y, z}].get());
			}
		}
	}

	for (const auto& chunk : chunks_ | std::views::values)
	{
		// RequestChunkMeshUpdate(chunk.get());
		MarkChunkDirty(chunk.get());
	}
}

DirectX::XMVECTOR World::GetLightDirection() const
{
	using namespace DirectX;
	const XMVECTOR noonDirection = XMVectorSet(0.0f, -1.0f, 0.0f, 0.0f);

	XMVECTOR axis			= XMVectorSet(0, 0, 1, 0);
	float	 earthTilt		= XMConvertToRadians(23.5f);
	XMVECTOR tiltQuaternion = XMQuaternionRotationAxis(XMVectorSet(1, 0, 0, 0), earthTilt);
	XMVECTOR finalOrbitAxis = XMVector3Rotate(axis, tiltQuaternion);

	float angle				 = XM_2PI * timeOfDay_;
	angle					+= XM_PI; // offset to noon
	XMVECTOR rotation		 = XMQuaternionRotationAxis(finalOrbitAxis, angle);
	float	 angleDegDebug	 = XMConvertToDegrees(angle);
	XMVECTOR lightDirection	 = XMVector3Normalize(XMVector3Rotate(noonDirection, rotation));
	return lightDirection;
}

DirectX::XMVECTOR World::GetLightDirection(float customTime)
{
	float currentTime = timeOfDay_;
	timeOfDay_		  = customTime;

	auto direction = GetLightDirection();

	timeOfDay_ = currentTime;
	return direction;
}

void World::PassWorldTime(float amount)
{
	timeOfDay_ += amount;
	if (timeOfDay_ >= 1.0f)
	{
		timeOfDay_ = 0.0f;
	}
	else if (timeOfDay_ < 0.0f)
	{
		timeOfDay_ = 1.0f;
	}
}

Chunk* World::GetChunkFromBlock(DirectX::XMFLOAT3 worldBlockCoordinates)
{
	using Utils::Coordinates::GetChunkCoordinate;

	std::int32_t x = GetChunkCoordinate<Chunk::CHUNK_SIZE>(worldBlockCoordinates.x);
	std::int32_t y = GetChunkCoordinate<Chunk::CHUNK_SIZE>(worldBlockCoordinates.y);
	std::int32_t z = GetChunkCoordinate<Chunk::CHUNK_SIZE>(worldBlockCoordinates.z);

	return chunks_.contains({x, y, z}) ? chunks_[{x, y, z}].get() : nullptr;
}

Chunk* World::GetChunkFromBlock(DirectX::XMINT3 worldBlockCoordinates)
{
	using Utils::Coordinates::GetChunkCoordinate;

	std::int32_t x = GetChunkCoordinate<Chunk::CHUNK_SIZE>(worldBlockCoordinates.x);
	std::int32_t y = GetChunkCoordinate<Chunk::CHUNK_SIZE>(worldBlockCoordinates.y);
	std::int32_t z = GetChunkCoordinate<Chunk::CHUNK_SIZE>(worldBlockCoordinates.z);

	return chunks_.contains({x, y, z}) ? chunks_[{x, y, z}].get() : nullptr;
}

Chunk* World::GetChunk(DirectX::XMFLOAT3 worldChunkCoordinates)
{
	std::int32_t x = std::lroundf(worldChunkCoordinates.x);
	std::int32_t y = std::lroundf(worldChunkCoordinates.y);
	std::int32_t z = std::lroundf(worldChunkCoordinates.z);
	return GetChunk(DirectX::XMINT3{x, y, z});
}

Chunk* World::GetChunk(DirectX::XMINT3 worldChunkCoordinates)
{
	if (chunks_.contains(worldChunkCoordinates))
	{
		return chunks_[worldChunkCoordinates].get();
	}

	return nullptr;
}

std::vector<Chunk*> World::GetChunks() const
{
	std::vector<Chunk*> chunks;
	chunks.reserve(chunks_.size());

	for (const auto& chunk : chunks_ | std::views::values)
	{
		chunks.push_back(chunk.get());
	}

	return chunks;
}

std::vector<Chunk*> World::GetChunksInFrustum(const DirectX::BoundingFrustum& frustum) const
{
	using namespace DirectX;

	std::vector<Chunk*> chunks;
	chunks.reserve(chunks_.size());

	for (const auto& chunk : chunks_ | std::views::values)
	{
		// THIS IF-CHECK IS A HUUUUUGE PERFORMANCE BOOST
		if (chunk.get()->indexCount_ == 0)
		{
			continue;
		}

		ContainmentType testResult = frustum.Contains(chunk->GetChunkBounds());
		if (testResult != DISJOINT)
		{
			chunks.push_back(chunk.get());
		}
	}

	return chunks;
}

std::vector<Chunk*> World::GetShadowChunksInFrustum(const DirectX::BoundingFrustum& frustum) const
{
	using namespace DirectX;

	std::vector<Chunk*> chunks;
	chunks.reserve(chunks_.size());

	for (const auto& chunk : chunks_ | std::views::values)
	{
		if (chunk.get()->shadowProxyIndexCount_ == 0)
		{
			continue;
		}

		ContainmentType testResult = frustum.Contains(chunk->GetChunkBounds());
		if (testResult != DISJOINT)
		{
			chunks.push_back(chunk.get());
		}
	}

	return chunks;
}

bool World::SetBlock(DirectX::XMFLOAT3 worldCoordinates, BlockType blockType, BlockFace frontFace)
{
	bool   success = false;
	Chunk* chunk   = GetChunkFromBlock(worldCoordinates);

	if (chunk == nullptr)
	{
		return success;
	}

	static constexpr std::int32_t bitMask = static_cast<std::int32_t>(Chunk::CHUNK_SIZE) - 1;

	std::int32_t x = static_cast<int32_t>(std::floorf(worldCoordinates.x)) & bitMask;
	std::int32_t y = static_cast<int32_t>(std::floorf(worldCoordinates.y)) & bitMask;
	std::int32_t z = static_cast<int32_t>(std::floorf(worldCoordinates.z)) & bitMask;

	Timer perfCounter;
	perfCounter.Reset();
	Block oldBlock = chunk->GetBlock(x, y, z);
	success		   = chunk->SetBlockType(x, y, z, blockType /*frontFace goes here*/);
	lightEngine_.UpdateSkyLight(static_cast<std::int32_t>(std::floorf(worldCoordinates.x)),
								static_cast<std::int32_t>(std::floorf(worldCoordinates.y)),
								static_cast<std::int32_t>(std::floorf(worldCoordinates.z)));

	lightEngine_.UpdateBlockLight(static_cast<std::int32_t>(std::floorf(worldCoordinates.x)),
								  static_cast<std::int32_t>(std::floorf(worldCoordinates.y)),
								  static_cast<std::int32_t>(std::floorf(worldCoordinates.z)),
								  oldBlock.type,
								  blockType);

	perfCounter.TickUncapped();
	double result = perfCounter.GetDeltaTime();
	std::cout << "light updates took: " << result << " seconds" << std::endl;

	MarkChunkDirty(chunk);

	DirectX::XMINT3 chunkCoordinates = chunk->GetChunkWorldPos();
	// find out if the block was modified on chunk borders
	std::vector<BlockFace> borderDirections = Chunk::IsBlockOnBorder(x, y, z);
	for (auto borderDirection : borderDirections)
	{
		DirectX::XMINT3 neighborChunkCoordinates = chunkCoordinates;
		switch (borderDirection)
		{
			case BlockFace::North:
			{
				neighborChunkCoordinates.z += 1;
				break;
			}
			case BlockFace::South:
			{
				neighborChunkCoordinates.z -= 1;
				break;
			}
			case BlockFace::West:
			{
				neighborChunkCoordinates.x -= 1;
				break;
			}
			case BlockFace::East:
			{
				neighborChunkCoordinates.x += 1;
				break;
			}
			case BlockFace::Top:
			{
				neighborChunkCoordinates.y += 1;
				break;
			}
			case BlockFace::Bottom:
			{
				neighborChunkCoordinates.y -= 1;
				break;
			}
		}

		Chunk* neighbor = GetChunk(neighborChunkCoordinates);
		if (neighbor != nullptr)
		{
			MarkChunkDirty(neighbor);
		}
	}

	return success;
}

bool World::SetBlock(DirectX::XMINT3 worldCoordinates, BlockType blockType, BlockFace frontFace)
{
	float x = static_cast<float>(worldCoordinates.x);
	float y = static_cast<float>(worldCoordinates.y);
	float z = static_cast<float>(worldCoordinates.z);

	return SetBlock(DirectX::XMFLOAT3{x, y, z}, blockType, frontFace);
}

// MAKE THIS FASTTTTT
Block World::GetBlock(DirectX::XMFLOAT3 worldCoordinates)
{

	std::int32_t	chunkX = Utils::Coordinates::GetChunkCoordinate<Chunk::CHUNK_SIZE>(worldCoordinates.x);
	std::int32_t	chunkY = Utils::Coordinates::GetChunkCoordinate<Chunk::CHUNK_SIZE>(worldCoordinates.y);
	std::int32_t	chunkZ = Utils::Coordinates::GetChunkCoordinate<Chunk::CHUNK_SIZE>(worldCoordinates.z);
	DirectX::XMINT3 chunkCoords(chunkX, chunkY, chunkZ);

	static Chunk* cachedChunk = nullptr;

	if (cachedChunk != nullptr && cachedChunk->GetChunkWorldPos() == chunkCoords)
	{
		static constexpr std::int32_t bitMask = static_cast<std::int32_t>(Chunk::CHUNK_SIZE) - 1;

		std::int32_t x = static_cast<int32_t>(std::floorf(worldCoordinates.x)) & bitMask;
		std::int32_t y = static_cast<int32_t>(std::floorf(worldCoordinates.y)) & bitMask;
		std::int32_t z = static_cast<int32_t>(std::floorf(worldCoordinates.z)) & bitMask;
		return cachedChunk->GetBlock(x, y, z);
	}
	// cachedChunk null or last chunk coords not same
	if (Chunk* chunk = GetChunkFromBlock(worldCoordinates); chunk != nullptr)
	{
		static constexpr std::int32_t bitMask = static_cast<std::int32_t>(Chunk::CHUNK_SIZE) - 1;

		std::int32_t x = static_cast<int32_t>(std::floorf(worldCoordinates.x)) & bitMask;
		std::int32_t y = static_cast<int32_t>(std::floorf(worldCoordinates.y)) & bitMask;
		std::int32_t z = static_cast<int32_t>(std::floorf(worldCoordinates.z)) & bitMask;

		cachedChunk = chunk;

		return chunk->GetBlock(x, y, z);
	}

	return {};
}

Block World::GetBlock(DirectX::XMINT3 worldCoordinates)
{
	float x = static_cast<float>(worldCoordinates.x);
	float y = static_cast<float>(worldCoordinates.y);
	float z = static_cast<float>(worldCoordinates.z);

	return GetBlock(DirectX::XMFLOAT3{x, y, z});
}
bool World::IsBlockSolid(DirectX::XMFLOAT3 worldCoordinates)
{

	Block block = GetBlock(worldCoordinates);

	// chunks shouldn't contain invalid blocks, but it might be there's just no chunk here
	if (block.type == BlockType::INVALID_)
	{
		return false;
	}
	BlockDatabase&	 database = BlockDatabase::GetDatabase();
	const BlockData* data	  = database.GetBlockData(block.type);
	if (data == nullptr)
	{
		return false;
	}

	return data->isSolid;
}
bool World::IsBlockSolid(DirectX::XMINT3 worldCoordinates)
{
	return IsBlockSolid(DirectX::XMFLOAT3(static_cast<float>(worldCoordinates.x),
										  static_cast<float>(worldCoordinates.y),
										  static_cast<float>(worldCoordinates.z)));
}

void World::Update()
{
	{
		std::lock_guard<std::mutex> lock(uploadQueueMutex_);

		for (const auto& result : uploadQueue_)
		{
			Chunk* chunk = GetChunk(result.chunkCoordinates);
			if (chunk != nullptr)
			{
				chunk->SetVertexBuffer(result.mesh.vertexBuffer);
				chunk->SetIndexBuffer(result.mesh.indexBuffer);
				chunk->SetIndexCount(result.mesh.indexCount);

				chunk->SetShadowProxyVertexBuffer(result.mesh.shadowProxyVertexBuffer);
				chunk->SetShadowProxyIndexBuffer(result.mesh.shadowProxyIndexBuffer);
				chunk->SetShadowProxyIndexCount(result.mesh.shadowProxyIndexCount);
				chunk->ClearDirtyState();
			}
		}

		uploadQueue_.clear();
	}

	for (const auto& chunk : dirtyChunks_)
	{
		RequestChunkMeshUpdate(chunk);
	}
	dirtyChunks_.clear();
}

void World::MarkChunkDirty(Chunk* chunk)
{
	dirtyChunks_.insert(chunk);
}

void World::RequestChunkMeshUpdate(Chunk* chunk)
{
	if (chunk == nullptr)
	{
		return;
	}

	{
		std::lock_guard<std::mutex> lock(jobQueueMutex_);

		meshQueue_.push(std::make_unique<ChunkContext>());
		auto chunkContext = meshQueue_.back().get();
		FillChunkContext(chunk, chunkContext);
	}

	jobQueueCondition_.notify_one();
}

void World::FillChunkContext(const Chunk* mainChunk, ChunkContext* outChunkContext)
{
	assert(mainChunk != nullptr);
	assert(outChunkContext != nullptr);

	outChunkContext->mainChunk			  = mainChunk->GetBlocks();
	DirectX::XMINT3 chunkCoordinates	  = mainChunk->GetChunkWorldPos();
	outChunkContext->mainChunkCoordinates = chunkCoordinates;

	std::fill(outChunkContext->hasNeighbors.begin(), outChunkContext->hasNeighbors.end(), false);
	// Get chunk neighbors
	Chunk* northNeighbor  = nullptr;
	Chunk* southNeighbor  = nullptr;
	Chunk* eastNeighbor	  = nullptr;
	Chunk* westNeighbor	  = nullptr;
	Chunk* topNeighbor	  = nullptr;
	Chunk* bottomNeighbor = nullptr;

	DirectX::XMINT3 northNeighborPos{chunkCoordinates.x, chunkCoordinates.y, chunkCoordinates.z + 1};
	DirectX::XMINT3 southNeighborPos{chunkCoordinates.x, chunkCoordinates.y, chunkCoordinates.z - 1};
	DirectX::XMINT3 westNeighborPos{chunkCoordinates.x - 1, chunkCoordinates.y, chunkCoordinates.z};
	DirectX::XMINT3 eastNeighborPos{chunkCoordinates.x + 1, chunkCoordinates.y, chunkCoordinates.z};
	DirectX::XMINT3 topNeighborPos{chunkCoordinates.x, chunkCoordinates.y + 1, chunkCoordinates.z};
	DirectX::XMINT3 bottomNeighborPos{chunkCoordinates.x, chunkCoordinates.y - 1, chunkCoordinates.z};

	northNeighbor  = GetChunk(northNeighborPos);
	southNeighbor  = GetChunk(southNeighborPos);
	westNeighbor   = GetChunk(westNeighborPos);
	eastNeighbor   = GetChunk(eastNeighborPos);
	topNeighbor	   = GetChunk(topNeighborPos);
	bottomNeighbor = GetChunk(bottomNeighborPos);

	if (northNeighbor != nullptr)
	{
		// Direction from the POV of the neighbor, so northern neighbor of a given chunk connects to the chunk by
		// its southern border
		northNeighbor->GetBorderSlice(outChunkContext->northNeighbor, BlockFace::South);
		outChunkContext->hasNeighbors[static_cast<std::uint8_t>(BlockFace::North)] = true;
	}
	if (southNeighbor != nullptr)
	{
		southNeighbor->GetBorderSlice(outChunkContext->southNeighbor, BlockFace::North);
		outChunkContext->hasNeighbors[static_cast<std::uint8_t>(BlockFace::South)] = true;
	}
	if (westNeighbor != nullptr)
	{
		westNeighbor->GetBorderSlice(outChunkContext->westNeighbor, BlockFace::East);
		outChunkContext->hasNeighbors[static_cast<std::uint8_t>(BlockFace::West)] = true;
	}
	if (eastNeighbor != nullptr)
	{
		eastNeighbor->GetBorderSlice(outChunkContext->eastNeighbor, BlockFace::West);
		outChunkContext->hasNeighbors[static_cast<std::uint8_t>(BlockFace::East)] = true;
	}
	if (topNeighbor != nullptr)
	{
		topNeighbor->GetBorderSlice(outChunkContext->topNeighbor, BlockFace::Bottom);
		outChunkContext->hasNeighbors[static_cast<std::uint8_t>(BlockFace::Top)] = true;
	}
	if (bottomNeighbor != nullptr)
	{
		bottomNeighbor->GetBorderSlice(outChunkContext->bottomNeighbor, BlockFace::Top);
		outChunkContext->hasNeighbors[static_cast<std::uint8_t>(BlockFace::Bottom)] = true;
	}
}

void World::MesherLoop()
{
	Mesher mesher;
	while (true)
	{
		std::unique_ptr<ChunkContext> job;
		{
			std::unique_lock<std::mutex> lock(jobQueueMutex_);
			jobQueueCondition_.wait(lock, [this] { return !meshQueue_.empty() || shuttingDown_; });

			if (shuttingDown_ && meshQueue_.empty())
			{
				return;
			}
			job = std::move(meshQueue_.front());
			meshQueue_.pop();
		}

		assert(job != nullptr);


		MeshGPUData mesh = mesher.CreateMesh(*job, device_);

		{
			std::unique_lock<std::mutex> lock(uploadQueueMutex_);
			uploadQueue_.emplace_back(job->mainChunkCoordinates, mesh);
		}
	}
}

// N S E W T B
static constexpr DirectX::XMINT3 offsets[]{{0, 0, 1}, {0, 0, -1}, {1, 0, 0}, {-1, 0, 0}, {0, 1, 0}, {0, -1, 0}};

void World::SetSkyLightLevel(DirectX::XMINT3 worldCoordinates, std::uint8_t lightLevel)
{
	if (Chunk* chunk = GetChunkFromBlock(worldCoordinates); chunk != nullptr)
	{
		// convert to chunk-space
		static constexpr std::int32_t bitMask = static_cast<std::int32_t>(Chunk::CHUNK_SIZE) - 1;

		std::int32_t x = worldCoordinates.x & bitMask;
		std::int32_t y = worldCoordinates.y & bitMask;
		std::int32_t z = worldCoordinates.z & bitMask;

		chunk->SetSkyLightLevel(x, y, z, lightLevel);
		MarkChunkDirty(chunk);

		for (const auto& face : chunk->IsBlockOnBorder(x, y, z))
		{
			DirectX::XMINT3 offset = offsets[static_cast<std::uint8_t>(face)];
			DirectX::XMINT3 nPos   = {worldCoordinates.x + offset.x,
									  worldCoordinates.y + offset.y,
									  worldCoordinates.z + offset.z};

			chunk = GetChunkFromBlock(nPos);
			if (chunk)
			{
				MarkChunkDirty(chunk);
			}
		}
	}
}

void World::SetBlockLightLevel(DirectX::XMINT3 worldCoordinates, std::uint8_t lightLevel)
{
	std::int32_t	chunkX = Utils::Coordinates::GetChunkCoordinate<Chunk::CHUNK_SIZE>(worldCoordinates.x);
	std::int32_t	chunkY = Utils::Coordinates::GetChunkCoordinate<Chunk::CHUNK_SIZE>(worldCoordinates.y);
	std::int32_t	chunkZ = Utils::Coordinates::GetChunkCoordinate<Chunk::CHUNK_SIZE>(worldCoordinates.z);
	DirectX::XMINT3 chunkCoords(chunkX, chunkY, chunkZ);

	static Chunk* cachedChunk = nullptr;

	// fast path
	if (cachedChunk != nullptr && chunkCoords == cachedChunk->GetChunkWorldPos())
	{
		static constexpr std::int32_t bitMask = static_cast<std::int32_t>(Chunk::CHUNK_SIZE) - 1;

		std::int32_t x = worldCoordinates.x & bitMask;
		std::int32_t y = worldCoordinates.y & bitMask;
		std::int32_t z = worldCoordinates.z & bitMask;

		cachedChunk->SetBlockLightLevel(x, y, z, lightLevel);
		MarkChunkDirty(cachedChunk);

		for (const auto& face : cachedChunk->IsBlockOnBorder(x, y, z))
		{
			DirectX::XMINT3 offset = offsets[static_cast<std::uint8_t>(face)];
			DirectX::XMINT3 nPos   = {worldCoordinates.x + offset.x,
									  worldCoordinates.y + offset.y,
									  worldCoordinates.z + offset.z};

			Chunk* chunk = GetChunkFromBlock(nPos);
			if (chunk)
			{
				MarkChunkDirty(chunk);
			}
		}
	}
	else if (Chunk* chunk = GetChunkFromBlock(worldCoordinates); chunk != nullptr)
	{
		// convert to chunk-space
		static constexpr std::int32_t bitMask = static_cast<std::int32_t>(Chunk::CHUNK_SIZE) - 1;

		std::int32_t x = worldCoordinates.x & bitMask;
		std::int32_t y = worldCoordinates.y & bitMask;
		std::int32_t z = worldCoordinates.z & bitMask;

		chunk->SetBlockLightLevel(x, y, z, lightLevel);

		cachedChunk = chunk;
		MarkChunkDirty(chunk);

		for (const auto& face : chunk->IsBlockOnBorder(x, y, z))
		{
			DirectX::XMINT3 offset = offsets[static_cast<std::uint8_t>(face)];
			DirectX::XMINT3 nPos   = {worldCoordinates.x + offset.x,
									  worldCoordinates.y + offset.y,
									  worldCoordinates.z + offset.z};

			chunk = GetChunkFromBlock(nPos);
			if (chunk)
			{
				MarkChunkDirty(chunk);
			}
		}
	}
}
