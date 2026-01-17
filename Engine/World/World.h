#pragma once
#include <DirectXMath.h>
#include <memory>
#include <mutex>
#include <queue>
#include <unordered_map>
#include <unordered_set>

#include "../Graphics/MeshData.h"
#include "../Math/DirectXMathOperators.h"
#include "BlockFace.h"
#include "BlockType.h"
#include "ChunkContext.h"
#include "VoxelLightingEngine.h"

class Chunk;
class World
{
public:
	friend class VoxelLightingEngine;
	World();
	~World();

	World(const World&)			   = delete;
	World(World&&)				   = delete;
	World& operator=(const World&) = delete;
	World& operator=(World&&)	   = delete;

	bool Initialize(ID3D11Device* device);
	void GenerateTestChunks();
	void GenerateTestWorld();

	[[nodiscard]] DirectX::XMVECTOR GetLightDirection() const;
	[[nodiscard]] DirectX::XMVECTOR GetLightDirection(float customTime);
	void							PassWorldTime(float amount);

	[[nodiscard]] Chunk* GetChunkFromBlock(DirectX::XMFLOAT3 worldBlockCoordinates);
	[[nodiscard]] Chunk* GetChunkFromBlock(DirectX::XMINT3 worldBlockCoordinates);

	[[nodiscard]] Chunk* GetChunk(DirectX::XMFLOAT3 worldChunkCoordinates);
	[[nodiscard]] Chunk* GetChunk(DirectX::XMINT3 worldChunkCoordinates);

	[[nodiscard]] std::vector<Chunk*> GetChunks() const;
	[[nodiscard]] std::vector<Chunk*> GetChunksInFrustum(const DirectX::BoundingFrustum& frustum) const;
	[[nodiscard]] std::vector<Chunk*> GetShadowChunksInFrustum(const DirectX::BoundingFrustum& frustum) const;

	// Front means facing the player here
	bool				SetBlock(DirectX::XMFLOAT3 worldCoordinates, BlockType blockType, BlockFace frontFace);
	bool				SetBlock(DirectX::XMINT3 worldCoordinates, BlockType blockType, BlockFace frontFace);
	[[nodiscard]] Block GetBlock(DirectX::XMFLOAT3 worldCoordinates);
	[[nodiscard]] Block GetBlock(DirectX::XMINT3 worldCoordinates);

	[[nodiscard]] bool						 IsBlockSolid(DirectX::XMFLOAT3 worldCoordinates);
	[[nodiscard]] bool						 IsBlockSolid(DirectX::XMINT3 worldCoordinates);
	[[nodiscard]] float						 GetWorldTime() const { return timeOfDay_; };
	[[nodiscard]] const VoxelLightingEngine& GetVoxelLightingEngine() const { return lightEngine_; };

	void Update();

private:
	void MarkChunkDirty(Chunk* chunk);
	void RequestChunkMeshUpdate(Chunk* chunk);
	void FillChunkContext(const Chunk* mainChunk, ChunkContext* outChunkContext);
	void MesherLoop();
	void SetSkyLightLevel(DirectX::XMINT3 worldCoordinates, std::uint8_t lightLevel);
	void SetBlockLightLevel(DirectX::XMINT3 worldCoordinates, std::uint8_t lightLevel);
	// Meshing stuff

	std::unordered_set<Chunk*> dirtyChunks_;

	// Queue of chunks waiting for meshing
	std::queue<std::unique_ptr<ChunkContext>> meshQueue_;
	std::mutex								  jobQueueMutex_;
	std::condition_variable					  jobQueueCondition_;

	// Worker threads
	std::vector<std::thread> workers_;
	bool					 shuttingDown_;

	// "Completed" queue
	struct MeshResult
	{
		DirectX::XMINT3 chunkCoordinates;
		MeshGPUData		mesh;
	};
	std::vector<MeshResult> uploadQueue_;
	std::mutex				uploadQueueMutex_;

	ID3D11Device*		 device_;
	ID3D11DeviceContext* deviceContext_;

	// Light engine
	VoxelLightingEngine lightEngine_;

	// Sun
	float timeOfDay_; // 0.0 - midnight, 0.5 - noon

private:
	// TODO: Try https://github.com/martinus/unordered_dense if we get a CPU bottleneck here
	std::unordered_map<DirectX::XMINT3, std::unique_ptr<Chunk>, Math::XMINT3Hash> chunks_;
};
