#pragma once
#include <DirectXCollision.h>
#include <DirectXMath.h>
#include <array>
#include <d3d11.h>
#include <vector>
#include <wrl/client.h>

#include "Block.h"
#include "BlockFace.h"
#include "BlockType.h"

class Chunk
{
public:
	friend class World;
	static constexpr std::size_t CHUNK_SIZE = 16;
	Chunk()									= delete;
	Chunk(DirectX::XMINT3 chunkWorldPos);

	[[nodiscard]] Block GetBlock(DirectX::XMUINT3 block) const;							 // Chunk-space coordinates
	[[nodiscard]] Block GetBlock(std::size_t x, std::size_t y, std::size_t z) const;	 // Chunk-space coordinates
	bool SetBlockType(std::size_t x, std::size_t y, std::size_t z, BlockType blockType); // Chunk-space coordinates
	bool SetBlockType(DirectX::XMUINT3 block, BlockType blockType);						 // Chunk-space coordinates

	bool SetSkyLightLevel(DirectX::XMUINT3 block, std::uint8_t lightLevel);
	bool SetBlockLightLevel(DirectX::XMUINT3 block, std::uint8_t lightLevel);
	bool SetSkyLightLevel(std::size_t x, std::size_t y, std::size_t z, std::uint8_t lightLevel);
	bool SetBlockLightLevel(std::size_t x, std::size_t y, std::size_t z, std::uint8_t lightLevel);
	void ClearDirtyState() { dirty_ = false; }

	/**
	 *
	 * @param outBlocks array to copy the slice into
	 * @param direction the face of the chunk that will be turned into a slice. IMPORTANT: the direction is considered
	 * from the point of view of the chunk that this method gets called on. E.g. if we want to get a slice of a chunk's
	 * northern neighbor, then from the perspective of the neighbor, the neighbor returns its southern slice
	 */
	void GetBorderSlice(std::array<Block, CHUNK_SIZE * CHUNK_SIZE>& outBlocks, BlockFace direction) const;

	/**
	 *
	 * @param block chunk-space coordinates
	 * @return empty if the block doesn't lie on any of the chunk's borders/block lies outside the chunk
	 * otherwise returns all neighboring directions (up to 3, since a block cannot border more than 3 chunks)
	 */
	static std::vector<BlockFace> IsBlockOnBorder(DirectX::XMINT3 block);

	/**
	 *
	 * @param x chunk-space block x
	 * @param y chunk-space block y
	 * @param z chunk-space block z
	 * @return empty if the block doesn't lie on any of the chunk's borders/block lies outside the chunk
	 * otherwise returns all neighboring directions (up to 3, since a block cannot border more than 3 chunks)
	 */
	static std::vector<BlockFace> IsBlockOnBorder(std::size_t x, std::size_t y, std::size_t z);

private:
	std::array<Block, CHUNK_SIZE * CHUNK_SIZE * CHUNK_SIZE> blocks_;

	bool				 dirty_;
	DirectX::XMINT3		 chunkWorldPos_;
	DirectX::XMFLOAT4X4	 chunkWorldMatrix_;
	DirectX::BoundingBox chunkBounds_;

	Microsoft::WRL::ComPtr<ID3D11Buffer> vertexBuffer_;
	Microsoft::WRL::ComPtr<ID3D11Buffer> indexBuffer_;
	std::uint32_t						 indexCount_;

	/*
	 * This is used for shadowmaps
	 * With the max-culling meshing, the sun can view the world from a POV that makes little/very low quality shadows
	 * this can lead to light leaks etc.
	 * Shadow proxy mesh treats the chunks as if they were always rendered in a complete void, not taking into account
	 * neighboring chunks
	 */

	Microsoft::WRL::ComPtr<ID3D11Buffer> shadowProxyVertexBuffer_;
	Microsoft::WRL::ComPtr<ID3D11Buffer> shadowProxyIndexBuffer_;
	std::uint32_t						 shadowProxyIndexCount_;

public:
	// Getters
	[[nodiscard]] Microsoft::WRL::ComPtr<ID3D11Buffer> GetVertexBuffer() const { return vertexBuffer_; }
	[[nodiscard]] Microsoft::WRL::ComPtr<ID3D11Buffer> GetIndexBuffer() const { return indexBuffer_; }

	[[nodiscard]] Microsoft::WRL::ComPtr<ID3D11Buffer> GetShadowProxyVertexBuffer() const
	{
		return shadowProxyVertexBuffer_.Get();
	}
	[[nodiscard]] Microsoft::WRL::ComPtr<ID3D11Buffer> GetShadowProxyIndexBuffer() const
	{
		return shadowProxyIndexBuffer_.Get();
	}

	// Copy-getter, use only when necessary
	[[nodiscard]] std::array<Block, CHUNK_SIZE * CHUNK_SIZE * CHUNK_SIZE> GetBlocks() const { return blocks_; }
	[[nodiscard]] std::uint32_t											  GetIndexCount() const { return indexCount_; };
	[[nodiscard]] std::uint32_t		   GetShadowProxyIndexCount() const { return shadowProxyIndexCount_; };
	[[nodiscard]] DirectX::XMINT3	   GetChunkWorldPos() const { return chunkWorldPos_; }
	[[nodiscard]] DirectX::XMMATRIX	   GetWorldMatrix() const { return DirectX::XMLoadFloat4x4(&chunkWorldMatrix_); }
	[[nodiscard]] DirectX::BoundingBox GetChunkBounds() const { return chunkBounds_; }


	void SetVertexBuffer(const Microsoft::WRL::ComPtr<ID3D11Buffer>& vertexBuffer) { vertexBuffer_ = vertexBuffer; }
	void SetIndexBuffer(const Microsoft::WRL::ComPtr<ID3D11Buffer>& indexBuffer) { indexBuffer_ = indexBuffer; }
	void SetIndexCount(std::uint32_t indexCount) { indexCount_ = indexCount; };

	void SetShadowProxyVertexBuffer(const Microsoft::WRL::ComPtr<ID3D11Buffer>& vertexBuffer)
	{
		shadowProxyVertexBuffer_ = vertexBuffer;
	}
	void SetShadowProxyIndexBuffer(const Microsoft::WRL::ComPtr<ID3D11Buffer>& indexBuffer)
	{
		shadowProxyIndexBuffer_ = indexBuffer;
	}
	void SetShadowProxyIndexCount(std::uint32_t indexCount) { shadowProxyIndexCount_ = indexCount; };
};
