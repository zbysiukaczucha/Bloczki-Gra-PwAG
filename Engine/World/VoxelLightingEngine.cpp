#include "VoxelLightingEngine.h"

#include <ranges>

#include "BlockDatabase.h"
#include "World.h"

static constexpr DirectX::XMINT3 offsets[] = {{0, 1, 0}, /**/
											  {0, -1, 0},
											  {1, 0, 0},
											  {-1, 0, 0},
											  {0, 0, 1},
											  {0, 0, -1}};

VoxelLightingEngine::VoxelLightingEngine(World* world)
{
	assert(world != nullptr);
	world_ = world;
}

void VoxelLightingEngine::AddLightSource(std::int32_t x, std::int32_t y, std::int32_t z, std::uint8_t lightLevel)
{
	AddLightSource({x, y, z}, lightLevel);
}

void VoxelLightingEngine::AddLightSource(DirectX::XMINT3 position, const std::uint8_t lightLevel)
{
	world_->SetBlockLightLevel(position, lightLevel);
	propagationQueue.push({position, lightLevel});

	PropagateBlockLight();
}

void VoxelLightingEngine::UpdateSkyLight(DirectX::XMINT3 position)
{
	using namespace DirectX;


	Block			 block		   = world_->GetBlock(position);
	BlockDatabase&	 blockDatabase = BlockDatabase::GetDatabase();
	const BlockData* ogBlockData   = blockDatabase.GetBlockData(block.type);

	// This happens when the block in question was broken
	if (ogBlockData == nullptr)
	{
		XMINT3 blockAbovePos  = position;
		blockAbovePos.y		 += 1;
		Block blockAbove	  = world_->GetBlock(blockAbovePos);

		if (blockAbove.type
			== BlockType::INVALID_
			|| (blockAbove.type == BlockType::Air && blockAbove.GetSkyLightLevel() == 15))
		{
			XMINT3 blockPos = position;
			while (true)
			{
				block = world_->GetBlock(blockPos);
				if (block.type == BlockType::INVALID_)
				{
					break;
				}

				const BlockData* blockData = blockDatabase.GetBlockData(block.type);
				if (blockData == nullptr || blockData->isTransparent == true)
				{
					world_->SetSkyLightLevel(blockPos, 15);
					propagationQueue.emplace(blockPos, 15);
				}
				else
				{
					break;
				}

				blockPos.y -= 1;
			}
		}


		// Local Neighborhood Check
		// Even if the sun isn't directly above, we might be next to a light source.
		// We "wake up" all neighbors that have light and force them to propagate.

		for (const auto& offset : offsets)
		{
			XMINT3 nPos	  = {position.x + offset.x, position.y + offset.y, position.z + offset.z};
			Block  nBlock = world_->GetBlock(nPos);

			// If neighbor has light, add it to the queue.
			// The Propagate function will process it, and spread to us (the new air block).
			if (nBlock.type != BlockType::INVALID_ && nBlock.GetSkyLightLevel() > 0)
			{
				propagationQueue.push({nPos.x, nPos.y, nPos.z, nBlock.GetSkyLightLevel()});
			}
		}
	}
	else if (ogBlockData->isTransparent == false)
	{
		darknessQueue.emplace(position, block.GetSkyLightLevel());
		world_->SetSkyLightLevel(position, 0);
		PropagateSkyDarkness();
	}

	PropagateSkyLight();
}

void VoxelLightingEngine::UpdateSkyLight(std::int32_t x, std::int32_t y, std::int32_t z)
{
	UpdateSkyLight({x, y, z});
}

void VoxelLightingEngine::UpdateBlockLight(DirectX::XMINT3 position, BlockType oldBlock, BlockType newBlock)
{
	using namespace DirectX;


	BlockDatabase&	 blockDatabase = BlockDatabase::GetDatabase();
	const BlockData* ogBlockData   = blockDatabase.GetBlockData(oldBlock);
	const BlockData* newBlockData  = blockDatabase.GetBlockData(newBlock);

	// Scenario A - a light has been placed
	if (newBlockData != nullptr && newBlockData->lightEmissionLevel > 0)
	{
		world_->SetBlockLightLevel(position, newBlockData->lightEmissionLevel);
		propagationQueue.emplace(position, newBlockData->lightEmissionLevel);
		AddBlockLight(position, *newBlockData);
	}
	else if (ogBlockData != nullptr && ogBlockData->lightEmissionLevel > 0) // B - removing a light source
	{
		darknessQueue.emplace(position, ogBlockData->lightEmissionLevel);
		world_->SetBlockLightLevel(position, 0);
		PropagateBlockDarkness();
		RemoveBlockLight(position);
	}
	else if (newBlockData
			 != nullptr
			 && newBlockData->lightEmissionLevel
			 == 0
			 && newBlockData->isTransparent
			 == false) // C - placing a non-emissive, opaque block
	{
		darknessQueue.emplace(position, world_->GetBlock(position).GetBlockLightLevel());
		world_->SetBlockLightLevel(position, 0);
		PropagateBlockDarkness();
	}
	else // D - the block in question was non-emissive and broken OR a transparent block was placed. Either way, light
		 // needs to be spread
	{
		// Wake up the neighbors
		for (const auto& offset : offsets)
		{
			XMINT3		 pos		= {position.x + offset.x, position.y + offset.y, position.z + offset.z};
			Block		 block		= world_->GetBlock(pos);
			std::uint8_t lightLevel = block.GetBlockLightLevel();
			if (block.type != BlockType::INVALID_ && lightLevel > 0)
			{
				propagationQueue.emplace(pos, lightLevel);
			}
		}
	}

	PropagateBlockLight();
}

void VoxelLightingEngine::UpdateBlockLight(int32_t x, int32_t y, int32_t z, BlockType oldBlock, BlockType newBlock)
{
	UpdateBlockLight({x, y, z}, oldBlock, newBlock);
}

std::vector<PointLightGPU> VoxelLightingEngine::GetLightsInFrustum(const DirectX::BoundingFrustum& frustum) const
{
	using namespace DirectX;
	std::vector<PointLightGPU> lightsGPU;
	lightsGPU.reserve((std::min)(lights_.size(), MAX_POINT_LIGHTS));
	auto view = lights_
			  | std::views::values
			  | std::views::filter(
					[&](auto& light)
					{
						ContainmentType containment = frustum.Contains(light.bounds);
						return containment != DISJOINT;
					})
			  | std::views::take(MAX_POINT_LIGHTS);

	for (const auto& [bounds, color, intensity] : view)
	{
		lightsGPU.emplace_back(bounds.Center, bounds.Radius, color, intensity);
	}

	return lightsGPU;
}

void VoxelLightingEngine::PropagateBlockLight()
{
	using namespace DirectX;

	BlockDatabase& blockDatabase = BlockDatabase::GetDatabase();

	// to stop the queue from magically containing 2.5 MILLION entries, most of them dupes
	std::unordered_set<XMINT3, Math::XMINT3Hash> guardianSet;
	while (propagationQueue.empty() == false)
	{
		LightNode node = propagationQueue.front();
		propagationQueue.pop();
		if (guardianSet.contains(node.position))
		{
			// the node goes bye bye, this is a one-time only ride
			continue;
		}

		guardianSet.insert(node.position);

		for (const auto& offset : offsets)
		{
			XMINT3 neighborPos = {node.position.x + offset.x, node.position.y + offset.y, node.position.z + offset.z};
			const Block neighborBlock = world_->GetBlock(neighborPos);

			if (neighborBlock.type == BlockType::INVALID_)
			{
				continue;
			}

			int nextLightLevel = node.lightLevel - 1;
			if (nextLightLevel <= 0)
			{
				continue;
			}

			const BlockData* data = blockDatabase.GetBlockData(neighborBlock.type);

			// Opaque, non-emissive check
			if (data != nullptr && data->isTransparent == false && data->lightEmissionLevel == 0)
			{
				continue;
			}

			int neighborLightLevel = neighborBlock.GetBlockLightLevel();
			if (neighborLightLevel < nextLightLevel)
			{
				world_->SetBlockLightLevel(neighborPos, nextLightLevel);
				propagationQueue.emplace(neighborPos, static_cast<uint8_t>(nextLightLevel));
			}
		}
	}
}

void VoxelLightingEngine::PropagateBlockDarkness()
{
	using namespace DirectX;

	BlockDatabase&								 blockDatabase = BlockDatabase::GetDatabase();
	std::unordered_set<XMINT3, Math::XMINT3Hash> guardianSet;

	while (darknessQueue.empty() == false)
	{
		LightNode node = darknessQueue.front();
		darknessQueue.pop();
		if (guardianSet.contains(node.position))
		{
			continue;
		}
		guardianSet.insert(node.position);

		for (const auto& offset : offsets)
		{
			XMINT3 neighborPos{node.position.x + offset.x, node.position.y + offset.y, node.position.z + offset.z};
			Block  neighborBlock = world_->GetBlock(neighborPos);
			if (neighborBlock.type == BlockType::INVALID_)
			{
				continue;
			}

			const BlockData* data = blockDatabase.GetBlockData(neighborBlock.type);
			if (data != nullptr && data->isTransparent == false && data->lightEmissionLevel == 0)
			{
				continue;
			}

			// block lit by current node, take away its light
			std::uint8_t neighborLightLevel = neighborBlock.GetBlockLightLevel();
			if (neighborLightLevel < node.lightLevel)
			{
				darknessQueue.emplace(neighborPos, neighborLightLevel);
				world_->SetBlockLightLevel(neighborPos, 0);
			}
			else // block receives some lighting from elsewhere, add it for re-lighting
			{
				propagationQueue.emplace(neighborPos, neighborLightLevel);
			}
		}
	}
}

void VoxelLightingEngine::PropagateSkyLight()
{
	BlockDatabase& blockDatabase = BlockDatabase::GetDatabase();

	std::unordered_set<DirectX::XMINT3, Math::XMINT3Hash> guardianSet;

	while (!propagationQueue.empty())
	{
		LightNode node = propagationQueue.front();
		propagationQueue.pop();
		if (guardianSet.contains(node.position))
		{
			continue;
		}
		guardianSet.insert(node.position);

		for (const auto& offset : offsets)
		{
			DirectX::XMINT3 neighborPos = {node.position.x + offset.x,
										   node.position.y + offset.y,
										   node.position.z + offset.z};

			Block neighborBlock = world_->GetBlock(neighborPos);
			if (neighborBlock.type == BlockType::INVALID_)
			{
				continue;
			}


			// Sky Light Rule: Down = No Decay, Others = Decay 1
			int nextLightLevel = (offset.y == -1) ? node.lightLevel : node.lightLevel - 1;

			if (nextLightLevel <= 0)
			{
				continue;
			}

			const BlockData* data = blockDatabase.GetBlockData(neighborBlock.type);

			// Opaque check (Air is transparent)
			if (data != nullptr && !data->isTransparent)
			{
				continue;
			}

			int currentNeighborLevel = neighborBlock.GetSkyLightLevel();

			if (currentNeighborLevel < nextLightLevel)
			{
				world_->SetSkyLightLevel(neighborPos, nextLightLevel);

				// BUG FIX: Push the NEW level, not the OLD one
				propagationQueue.emplace(neighborPos, static_cast<uint8_t>(nextLightLevel));
			}
		}
	}
}
void VoxelLightingEngine::PropagateSkyDarkness()
{
	using namespace DirectX;

	BlockDatabase& blockDatabase = BlockDatabase::GetDatabase();

	std::unordered_set<XMINT3, Math::XMINT3Hash> guardianSet;

	while (darknessQueue.empty() == false)
	{
		LightNode node = darknessQueue.front();
		darknessQueue.pop();
		if (guardianSet.contains(node.position))
		{
			continue;
		}

		guardianSet.insert(node.position);

		for (const auto& offset : offsets)
		{
			XMINT3 neighborPos{node.position.x + offset.x, node.position.y + offset.y, node.position.z + offset.z};
			Block  neighborBlock			= world_->GetBlock(neighborPos);
			std::uint8_t neighborLightLevel = neighborBlock.GetSkyLightLevel();
			if (neighborBlock.type == BlockType::INVALID_)
			{
				continue;
			}

			const BlockData* data = blockDatabase.GetBlockData(neighborBlock.type);
			if (data != nullptr && data->isTransparent == false)
			{
				continue;
			}

			bool litByThisNode;
			if (offset.y == -1)
			{
				litByThisNode = neighborLightLevel == node.lightLevel;
			}
			else
			{
				litByThisNode = neighborLightLevel < node.lightLevel;
			}
			// block lit by current node, take away its light
			if (litByThisNode)
			{
				darknessQueue.emplace(neighborPos, neighborLightLevel);
				world_->SetSkyLightLevel(neighborPos, 0);
			}
			else // block receives some lighting from elsewhere, add it for re-lighting
			{
				propagationQueue.emplace(neighborPos, neighborLightLevel);
			}
		}
	}
}

void VoxelLightingEngine::AddBlockLight(const DirectX::XMINT3 position, const struct BlockData& blockData)
{
	using namespace DirectX;
	const float x = static_cast<float>(position.x) + 0.5f;
	const float y = static_cast<float>(position.y) + 0.5f;
	const float z = static_cast<float>(position.z) + 0.5f;

	const float			 radius = blockData.lightEmissionLevel;
	const BoundingSphere lightBounds({x, y, z}, radius);

	lights_[position] = {lightBounds, blockData.lightColor, blockData.lightIntensity};
}

void VoxelLightingEngine::RemoveBlockLight(const DirectX::XMINT3 position)
{
	lights_.erase(position);
}


std::vector<LightNode> VoxelLightingEngine::GetNodeNeighbors(const LightNode& node, bool useBlockLight)
{
	using namespace DirectX;
	std::vector<LightNode> result;
	result.reserve(6);

	XMVECTOR current = XMLoadSInt3(&node.position);

	for (const auto& offset : offsets)
	{
		XMVECTOR next = XMLoadSInt3(&offset);
		XMINT3	 neighbor;
		XMStoreSInt3(&neighbor, next + current);

		Block block = world_->GetBlock(neighbor);
		if (block.type != BlockType::INVALID_)
		{
			result.emplace_back(neighbor, useBlockLight ? block.GetBlockLightLevel() : block.GetSkyLightLevel());
		}
	}

	return result;
}
std::vector<LightNode> VoxelLightingEngine::GetHorizontalNodeNeighbors(const LightNode& node, bool useBlockLight)
{
	using namespace DirectX;
	static constexpr XMINT3 offsets[]{{1, 0, 0}, {-1, 0, 0}, {0, 0, 1}, {0, 0, -1}};
	std::vector<LightNode>	result;
	result.reserve(4);

	XMVECTOR current = XMLoadSInt3(&node.position);

	for (const auto& offset : offsets)
	{
		XMVECTOR next = XMLoadSInt3(&offset);
		XMINT3	 neighbor;
		XMStoreSInt3(&neighbor, next + current);

		Block block = world_->GetBlock(neighbor);
		if (block.type != BlockType::INVALID_)
		{
			result.emplace_back(neighbor, useBlockLight ? block.GetBlockLightLevel() : block.GetSkyLightLevel());
		}
	}

	return result;
}
