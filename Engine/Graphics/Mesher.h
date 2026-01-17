#pragma once
#include <array>
#include <cstdint>
#include <d3d11.h>
#include <optional>
#include <span>
#include <vector>
#include <wrl/client.h>

#include "../World/BlockFace.h"
#include "../World/ChunkContext.h"
#include "MeshData.h"

class Chunk;

// One per thread
class Mesher
{
public:
	Mesher();
	[[nodiscard]] MeshGPUData CreateMesh(const ChunkContext& context, ID3D11Device* device);

private:
	[[nodiscard]] bool IsFaceExposed(const ChunkContext& context,
									 DirectX::XMUINT3	 block,
									 BlockFace			 face,
									 std::uint8_t&		 outLightLevel) const;

	// Doesn't take neighboring chunks into consideration, used for shadow proxies
	[[nodiscard]] bool IsFaceExposed(std::span<const Block> chunk, DirectX::XMUINT3 blockPos, BlockFace face);

	[[nodiscard]] std::array<std::uint8_t, 6> GetBlockMaterialIndices(
		BlockFace blockDirection) const; // handles rotated blocks

	void CreateChunkBuffers(Chunk* chunk);

	void CreateFace(DirectX::XMUINT3 block, BlockFace face, std::uint32_t materialIdx, std::uint8_t lightLevel);
	void CreateSimpleFace(DirectX::XMUINT3 block, BlockFace face);

	/**
	 *
	 * @param block the coordinates of the block we're looking for the neighbors of
	 * @param direction the direction we're looking in
	 * @return position of the neighbor in the desired direction or nullopt if the neighbor were be outside the current
	 * block's chunk
	 */
	[[nodiscard]] std::optional<DirectX::XMUINT3> GetNeighborBlockPosition(DirectX::XMUINT3 block,
																		   BlockFace		direction) const;

	// Memory pools
	std::vector<Vertex>		  vertexCache_;
	std::vector<SimpleVertex> simpleVertexCache_;
	std::vector<uint32_t>	  indexCache_;
};
