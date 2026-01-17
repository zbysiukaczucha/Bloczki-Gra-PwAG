#pragma once
#include <DirectXMath.h>
#include <queue>
#include <unordered_map>

#include "../Graphics/Light.h"
#include "../Math/DirectXMathOperators.h"
#include "Block.h"

struct LightNode
{
	DirectX::XMINT3 position;
	std::uint8_t	lightLevel;

	LightNode(const std::int32_t x, const std::int32_t y, const std::int32_t z, const std::uint8_t lightLevel) :
		position(x, y, z),
		lightLevel(lightLevel)
	{
	}

	LightNode(const DirectX::XMINT3 lightPosition, const std::uint8_t lightLevel) :
		position(lightPosition),
		lightLevel(lightLevel)
	{
	}
};
class World;
class VoxelLightingEngine
{
	World*				  world_;
	std::queue<LightNode> propagationQueue;
	std::queue<LightNode> darknessQueue;

public:
	VoxelLightingEngine() = delete;
	VoxelLightingEngine(World* world);

	void AddLightSource(std::int32_t x, std::int32_t y, std::int32_t z, std::uint8_t lightLevel);
	void AddLightSource(DirectX::XMINT3 position, const std::uint8_t lightLevel);

	void UpdateSkyLight(DirectX::XMINT3 position);
	void UpdateSkyLight(std::int32_t x, std::int32_t y, std::int32_t z);

	void UpdateBlockLight(DirectX::XMINT3 position, BlockType oldBlock, BlockType newBlock);
	void UpdateBlockLight(std::int32_t x, std::int32_t y, std::int32_t z, BlockType oldBlock, BlockType newBlock);

	[[nodiscard]] std::vector<PointLightGPU> GetLightsInFrustum(const DirectX::BoundingFrustum& frustum) const;

private:
	void PropagateBlockLight();
	void PropagateBlockDarkness();
	void PropagateSkyLight();
	void PropagateSkyDarkness();

	void AddBlockLight(const DirectX::XMINT3 position, const struct BlockData& blockData);
	void RemoveBlockLight(const DirectX::XMINT3 position);


	std::vector<LightNode> GetNodeNeighbors(const LightNode& node, bool getBlockLight);
	std::vector<LightNode> GetHorizontalNodeNeighbors(const LightNode& node, bool getBlockLight);

	std::unordered_map<DirectX::XMINT3, PointLightCPU, Math::XMINT3Hash> lights_;
};
