#include "Mesher.h"

#include <limits>
#include <optional>

#include "../World/BlockData.h"
#include "../World/BlockDatabase.h"
#include "../World/Chunk.h"

Mesher::Mesher()
{
	// Worst-case scenario vectors
	constexpr std::size_t MAX_BLOCKS  = Chunk::CHUNK_SIZE * Chunk::CHUNK_SIZE * Chunk::CHUNK_SIZE;
	constexpr std::size_t MAX_FACES	  = MAX_BLOCKS * 6; // assuming that somehow every block's face will be visible
	constexpr std::size_t MAX_VERTS	  = MAX_FACES * 4;	// four verts per face
	constexpr std::size_t MAX_INDICES = MAX_FACES * 6;	// 6 indices per face, a face is two triangles

	vertexCache_.reserve(MAX_VERTS);
	simpleVertexCache_.reserve(MAX_VERTS);
	indexCache_.reserve(MAX_INDICES);
}

MeshGPUData Mesher::CreateMesh(const ChunkContext& context, ID3D11Device* device)
{
	// reset cache data, keep size
	vertexCache_.clear();
	indexCache_.clear();

	// Run meshing;
	BlockDatabase& database = BlockDatabase::GetDatabase();
	auto&		   blocks	= context.mainChunk;
	for (std::uint32_t z = 0, i = 0; z < Chunk::CHUNK_SIZE; ++z)
	{
		for (std::uint32_t y = 0; y < Chunk::CHUNK_SIZE; ++y)
		{
			for (std::uint32_t x = 0; x < Chunk::CHUNK_SIZE; ++x, ++i)
			{
				assert(blocks[i].type != BlockType::INVALID_);
				if (blocks[i].type == BlockType::Air)
				{
					continue;
				}

				for (auto face : ALL_BLOCKFACES)
				{
					auto		 materialIndices = database.GetBlockData(blocks[i].type)->textureIndices;
					std::uint8_t lightLevel		 = 0;
					if (IsFaceExposed(context, {x, y, z}, face, lightLevel))
					{
						CreateFace({x, y, z}, face, materialIndices[static_cast<std::uint32_t>(face)], lightLevel);
					}
				}
			}
		}
	}

	MeshGPUData mesh;
	mesh.indexCount = static_cast<std::uint32_t>(indexCache_.size());
	//
	D3D11_BUFFER_DESC vertexBufferDesc = {};
	vertexBufferDesc.Usage			   = D3D11_USAGE_IMMUTABLE;
	vertexBufferDesc.ByteWidth		   = sizeof(Vertex) * vertexCache_.size();
	vertexBufferDesc.BindFlags		   = D3D11_BIND_VERTEX_BUFFER;

	D3D11_SUBRESOURCE_DATA vertexBufferData = {};
	vertexBufferData.pSysMem				= vertexCache_.data();

	HRESULT result = device->CreateBuffer(&vertexBufferDesc, &vertexBufferData, &mesh.vertexBuffer);
	if (FAILED(result))
	{
		return {};
	}

	D3D11_BUFFER_DESC indexBufferDesc = {};
	indexBufferDesc.Usage			  = D3D11_USAGE_IMMUTABLE;
	indexBufferDesc.ByteWidth		  = sizeof(uint32_t) * indexCache_.size();
	indexBufferDesc.BindFlags		  = D3D11_BIND_INDEX_BUFFER;

	D3D11_SUBRESOURCE_DATA indexBufferData = {};
	indexBufferData.pSysMem				   = indexCache_.data();
	result = device->CreateBuffer(&indexBufferDesc, &indexBufferData, &mesh.indexBuffer);
	if (FAILED(result))
	{
		return {};
	}


	// Now make the shadow proxy
	simpleVertexCache_.clear();
	indexCache_.clear();

	for (std::uint32_t z = 0, i = 0; z < Chunk::CHUNK_SIZE; ++z)
	{
		for (std::uint32_t y = 0; y < Chunk::CHUNK_SIZE; ++y)
		{
			for (std::uint32_t x = 0; x < Chunk::CHUNK_SIZE; ++x, ++i)
			{
				if (blocks[i].type == BlockType::Air)
				{
					continue;
				}

				for (auto face : ALL_BLOCKFACES)
				{
					if (IsFaceExposed(blocks, {x, y, z}, face))
					{
						CreateSimpleFace({x, y, z}, face);
					}
				}
			}
		}
	}

	mesh.shadowProxyIndexCount = static_cast<std::uint32_t>(indexCache_.size());

	D3D11_BUFFER_DESC shadowProxyVertexBufferDesc = {};
	shadowProxyVertexBufferDesc.Usage			  = D3D11_USAGE_IMMUTABLE;
	shadowProxyVertexBufferDesc.ByteWidth		  = sizeof(SimpleVertex) * simpleVertexCache_.size();
	shadowProxyVertexBufferDesc.BindFlags		  = D3D11_BIND_VERTEX_BUFFER;

	D3D11_SUBRESOURCE_DATA shadowProxyVertexBufferData = {};
	shadowProxyVertexBufferData.pSysMem				   = simpleVertexCache_.data();

	result = device->CreateBuffer(&shadowProxyVertexBufferDesc,
								  &shadowProxyVertexBufferData,
								  &mesh.shadowProxyVertexBuffer);
	if (FAILED(result))
	{
		return {};
	}

	D3D11_BUFFER_DESC shadowProxyIndexBufferDesc = {};
	shadowProxyIndexBufferDesc.Usage			 = D3D11_USAGE_IMMUTABLE;
	shadowProxyIndexBufferDesc.ByteWidth		 = sizeof(uint32_t) * indexCache_.size();
	shadowProxyIndexBufferDesc.BindFlags		 = D3D11_BIND_INDEX_BUFFER;

	D3D11_SUBRESOURCE_DATA shadowProxyIndexBufferData = {};
	shadowProxyIndexBufferData.pSysMem				  = indexCache_.data();
	result											  = device->CreateBuffer(&shadowProxyIndexBufferDesc,
									 &shadowProxyIndexBufferData,
									 &mesh.shadowProxyIndexBuffer);
	if (FAILED(result))
	{
		return {};
	}

	return mesh;
}

bool Mesher::IsFaceExposed(const ChunkContext& context,
						   DirectX::XMUINT3	   block,
						   BlockFace		   face,
						   std::uint8_t&	   outLightLevel) const
{
	constexpr std::size_t strideX = 1;
	constexpr std::size_t strideY = Chunk::CHUNK_SIZE;
	constexpr std::size_t strideZ = Chunk::CHUNK_SIZE * Chunk::CHUNK_SIZE;

	constexpr std::size_t strideU = 1;
	constexpr std::size_t strideV = Chunk::CHUNK_SIZE;
	outLightLevel				  = 0b11110000;

	// temporary, will be reassigned in the if-statement
	BlockType neighborType = BlockType::INVALID_;

	// If block not on the edges, consider just the main chunk
	auto neighborOpt = GetNeighborBlockPosition(block, face);
	bool hasNeighbor = context.hasNeighbors[static_cast<std::uint8_t>(face)];

	if (neighborOpt.has_value())
	{
		// Chunk*			 chunk	  = context.mainChunk;
		const auto&				blocks	 = context.mainChunk;
		const DirectX::XMUINT3& neighbor = neighborOpt.value();
		// neighborType			  = chunk->GetBlock(neighbor);

		auto x = neighbor.x * strideX;
		auto y = neighbor.y * strideY;
		auto z = neighbor.z * strideZ;

		neighborType  = blocks[x + y + z].type;
		outLightLevel = blocks[x + y + z].lightLevel;
	}
	else if (hasNeighbor) // chunk edge face, need to take neighboring chunks into account
	{
		// Chunk*			 neighborChunk = nullptr;
		const ChunkContext::ChunkSlice* neighborChunkSlice = nullptr;
		// DirectX::XMUINT3				neighbor		   = block;

		std::size_t neighborU = 0;
		std::size_t neighborV = 0;

		switch (face)
		{
			case BlockFace::North:
			{
				neighborChunkSlice = &context.northNeighbor;
				assert(block.z == Chunk::CHUNK_SIZE - 1);

				neighborU = block.x;
				neighborV = block.y;
				break;
			}
			case BlockFace::South:
			{
				neighborChunkSlice = &context.southNeighbor;
				assert(block.z == 0);
				neighborU = block.x;
				neighborV = block.y;
				break;
			}
			case BlockFace::East:
			{
				neighborChunkSlice = &context.eastNeighbor;
				assert(block.x == Chunk::CHUNK_SIZE - 1);
				neighborU = block.z;
				neighborV = block.y;

				break;
			}
			case BlockFace::West:
			{
				neighborChunkSlice = &context.westNeighbor;
				assert(block.x == 0);
				neighborU = block.z;
				neighborV = block.y;
				break;
			}
			case BlockFace::Top:
			{
				neighborChunkSlice = &context.topNeighbor;
				assert(block.y == Chunk::CHUNK_SIZE - 1);
				neighborU = block.x;
				neighborV = block.z;
				break;
			}
			case BlockFace::Bottom:
			{
				neighborChunkSlice = &context.bottomNeighbor;
				assert(block.y == 0);
				neighborU = block.x;
				neighborV = block.z;
				break;
			}
		}

		if (neighborChunkSlice == nullptr)
		{
			// No neighbor chunk, block exposed
			// If chunk generation were to be implemented, might want to make an assert here, but for the static world
			// for now it's fine
			return true;
		}

		neighborU *= strideU;
		neighborV *= strideV;

		neighborType  = (*neighborChunkSlice)[neighborU + neighborV].type;
		outLightLevel = (*neighborChunkSlice)[neighborU + neighborV].lightLevel;
	}
	else
	{
		// No neighbor in the same chunk in that direction, neighbor chunk is empty
		return true;
	}

	assert(neighborType != BlockType::INVALID_);
	if (neighborType == BlockType::Air) // special case, block database doesn't have an entry for air
	{
		return true;
	}

	const BlockDatabase& database = BlockDatabase::GetDatabase();
	const BlockData*	 data	  = database.GetBlockData(neighborType);
	assert(data != nullptr);

	if (data->isSolid == false || data->isTransparent == true)
	{
		return true;
	}

	return false; // false by default, will make it visually clear something's wrong if there's suddenly a faceless
				  // block
}

bool Mesher::IsFaceExposed(std::span<const Block> chunk, DirectX::XMUINT3 blockPos, BlockFace face)
{
	constexpr std::size_t strideX = 1;
	constexpr std::size_t strideY = Chunk::CHUNK_SIZE;
	constexpr std::size_t strideZ = Chunk::CHUNK_SIZE * Chunk::CHUNK_SIZE;

	auto neighborOpt = GetNeighborBlockPosition(blockPos, face);
	if (neighborOpt.has_value())
	{
		const DirectX::XMUINT3& neighbor = neighborOpt.value();

		auto x = neighbor.x * strideX;
		auto y = neighbor.y * strideY;
		auto z = neighbor.z * strideZ;

		assert((x + y + z) < chunk.size());

		BlockType neighborType = chunk[x + y + z].type;
		if (neighborType != BlockType::Air)
		{
			return false;
		}
	}

	return true;
}

std::array<std::uint8_t, 6> Mesher::GetBlockMaterialIndices(BlockFace blockDirection) const
{

	return {};
}

void Mesher::CreateChunkBuffers(Chunk* chunk)
{
}

void Mesher::CreateFace(DirectX::XMUINT3 block, BlockFace face, std::uint32_t materialIdx, std::uint8_t lightLevel)
{
	std::uint32_t baseIndex = vertexCache_.size();

	DirectX::XMFLOAT3 normal;
	DirectX::XMFLOAT3 tangent;
	DirectX::XMFLOAT3 bitangent;
	DirectX::XMFLOAT3 topLeft;
	DirectX::XMFLOAT3 topRight;
	DirectX::XMFLOAT3 bottomLeft;
	DirectX::XMFLOAT3 bottomRight;


	// block-space coordinates, coordinates centered on 0,0,0, so vertices need to be +/- 0.5
	float v = 0.5f;

	switch (face)
	{
		case BlockFace::North:
		{
			normal	  = {0.0f, 0.0f, 1.0f};
			tangent	  = {-1.0f, 0.0f, 0.0f};
			bitangent = {0.0f, -1.0f, 0.0f};

			topRight	= {-v, v, v};
			bottomRight = {-v, -v, v};
			bottomLeft	= {v, -v, v};
			topLeft		= {v, v, v};

			break;
		}
		case BlockFace::South:
		{
			normal	  = {0.0f, 0.0f, -1.0f};
			tangent	  = {1.0f, 0.0f, 0.0f};
			bitangent = {0.0f, -1.0f, 0.0f};

			topRight	= {v, v, -v};
			bottomRight = {v, -v, -v};
			bottomLeft	= {-v, -v, -v};
			topLeft		= {-v, v, -v};
			break;
		}
		case BlockFace::East:
		{
			normal	  = {1.0f, 0.0f, 0.0f};
			tangent	  = {0.0f, 0.0f, 1.0f};
			bitangent = {0.0f, -1.0f, 0.0f};

			topRight	= {v, v, v};
			bottomRight = {v, -v, v};
			bottomLeft	= {v, -v, -v};
			topLeft		= {v, v, -v};
			break;
		}
		case BlockFace::West:
		{
			normal	  = {-1.0f, 0.0f, 0.0f};
			tangent	  = {0.0f, 0.0f, -1.0f};
			bitangent = {0.0f, -1.0f, 0.0f};

			topRight	= {-v, v, -v};
			bottomRight = {-v, -v, -v};
			bottomLeft	= {-v, -v, v};
			topLeft		= {-v, v, v};
			break;
		}
		case BlockFace::Top:
		{
			normal	  = {0.0f, 1.0f, 0.0f};
			tangent	  = {1.0f, 0.0f, 0.0f};
			bitangent = {0.0f, 0.0f, -1.0f};

			topRight	= {v, v, v};
			bottomRight = {v, v, -v};
			bottomLeft	= {-v, v, -v};
			topLeft		= {-v, v, v};
			break;
		}
		case BlockFace::Bottom:
		{
			normal	  = {0.0f, -1.0f, 0.0f};
			tangent	  = {1.0f, 0.0f, 0.0f};
			bitangent = {0.0f, 0.0f, 1.0f};

			topRight	= {v, -v, -v};
			bottomRight = {v, -v, v};
			bottomLeft	= {-v, -v, v};
			topLeft		= {-v, -v, -v};
			break;
		}
	}

	float x = static_cast<float>(block.x);
	float y = static_cast<float>(block.y);
	float z = static_cast<float>(block.z);

	DirectX::XMVECTOR blockCenter = DirectX::XMVectorSet(x + 0.5f, y + 0.5f, z + 0.5f, 1.0f);
	auto			  tr		  = DirectX::XMLoadFloat3(&topRight);
	auto			  br		  = DirectX::XMLoadFloat3(&bottomRight);
	auto			  bl		  = DirectX::XMLoadFloat3(&bottomLeft);
	auto			  tl		  = DirectX::XMLoadFloat3(&topLeft);

	tr = DirectX::XMVectorAdd(tr, blockCenter);
	br = DirectX::XMVectorAdd(br, blockCenter);
	bl = DirectX::XMVectorAdd(bl, blockCenter);
	tl = DirectX::XMVectorAdd(tl, blockCenter);

	DirectX::XMStoreFloat3(&topRight, tr);
	DirectX::XMStoreFloat3(&bottomRight, br);
	DirectX::XMStoreFloat3(&bottomLeft, bl);
	DirectX::XMStoreFloat3(&topLeft, tl);

	vertexCache_
		.emplace_back(topLeft, normal, tangent, bitangent, DirectX::XMFLOAT2{0.0f, 0.0f}, materialIdx, lightLevel);
	vertexCache_
		.emplace_back(topRight, normal, tangent, bitangent, DirectX::XMFLOAT2{1.0f, 0.0f}, materialIdx, lightLevel);
	vertexCache_
		.emplace_back(bottomLeft, normal, tangent, bitangent, DirectX::XMFLOAT2{0.0f, 1.0f}, materialIdx, lightLevel);
	vertexCache_
		.emplace_back(bottomRight, normal, tangent, bitangent, DirectX::XMFLOAT2{1.0f, 1.0f}, materialIdx, lightLevel);

	indexCache_.push_back(baseIndex + 0);
	indexCache_.push_back(baseIndex + 1);
	indexCache_.push_back(baseIndex + 2);

	indexCache_.push_back(baseIndex + 1);
	indexCache_.push_back(baseIndex + 3);
	indexCache_.push_back(baseIndex + 2);
}

void Mesher::CreateSimpleFace(DirectX::XMUINT3 block, BlockFace face)
{
	std::uint32_t baseIndex = simpleVertexCache_.size();

	DirectX::XMFLOAT3 topLeft;
	DirectX::XMFLOAT3 topRight;
	DirectX::XMFLOAT3 bottomLeft;
	DirectX::XMFLOAT3 bottomRight;

	float v = 0.5f;

	switch (face)
	{
		case BlockFace::North:
		{
			topRight	= {-v, v, v};
			bottomRight = {-v, -v, v};
			bottomLeft	= {v, -v, v};
			topLeft		= {v, v, v};

			break;
		}
		case BlockFace::South:
		{
			topRight	= {v, v, -v};
			bottomRight = {v, -v, -v};
			bottomLeft	= {-v, -v, -v};
			topLeft		= {-v, v, -v};
			break;
		}
		case BlockFace::East:
		{
			topRight	= {v, v, v};
			bottomRight = {v, -v, v};
			bottomLeft	= {v, -v, -v};
			topLeft		= {v, v, -v};
			break;
		}
		case BlockFace::West:
		{
			topRight	= {-v, v, -v};
			bottomRight = {-v, -v, -v};
			bottomLeft	= {-v, -v, v};
			topLeft		= {-v, v, v};
			break;
		}
		case BlockFace::Top:
		{
			topRight	= {v, v, v};
			bottomRight = {v, v, -v};
			bottomLeft	= {-v, v, -v};
			topLeft		= {-v, v, v};
			break;
		}
		case BlockFace::Bottom:
		{
			topRight	= {v, -v, -v};
			bottomRight = {v, -v, v};
			bottomLeft	= {-v, -v, v};
			topLeft		= {-v, -v, -v};
			break;
		}
	}

	float x = static_cast<float>(block.x);
	float y = static_cast<float>(block.y);
	float z = static_cast<float>(block.z);

	DirectX::XMVECTOR blockCenter = DirectX::XMVectorSet(x + 0.5f, y + 0.5f, z + 0.5f, 1.0f);
	auto			  tr		  = DirectX::XMLoadFloat3(&topRight);
	auto			  br		  = DirectX::XMLoadFloat3(&bottomRight);
	auto			  bl		  = DirectX::XMLoadFloat3(&bottomLeft);
	auto			  tl		  = DirectX::XMLoadFloat3(&topLeft);

	tr = DirectX::XMVectorAdd(tr, blockCenter);
	br = DirectX::XMVectorAdd(br, blockCenter);
	bl = DirectX::XMVectorAdd(bl, blockCenter);
	tl = DirectX::XMVectorAdd(tl, blockCenter);

	DirectX::XMStoreFloat3(&topRight, tr);
	DirectX::XMStoreFloat3(&bottomRight, br);
	DirectX::XMStoreFloat3(&bottomLeft, bl);
	DirectX::XMStoreFloat3(&topLeft, tl);

	simpleVertexCache_.emplace_back(topLeft);
	simpleVertexCache_.emplace_back(topRight);
	simpleVertexCache_.emplace_back(bottomLeft);
	simpleVertexCache_.emplace_back(bottomRight);

	indexCache_.push_back(baseIndex + 0);
	indexCache_.push_back(baseIndex + 1);
	indexCache_.push_back(baseIndex + 2);

	indexCache_.push_back(baseIndex + 1);
	indexCache_.push_back(baseIndex + 3);
	indexCache_.push_back(baseIndex + 2);
}

std::optional<DirectX::XMUINT3> Mesher::GetNeighborBlockPosition(DirectX::XMUINT3 block, BlockFace direction) const
{
	assert(Chunk::CHUNK_SIZE <= static_cast<std::size_t>((std::numeric_limits<std::int32_t>::max)()));
	std::int32_t x = static_cast<std::int32_t>(block.x);
	std::int32_t y = static_cast<std::int32_t>(block.y);
	std::int32_t z = static_cast<std::int32_t>(block.z);

	DirectX::XMUINT3 neighborBlock = block;
	switch (direction)
	{
		case BlockFace::North:
		{
			z += 1;
			break;
		}
		case BlockFace::South:
		{
			z -= 1;
			break;
		}
		case BlockFace::East:
		{
			x += 1;
			break;
		}
		case BlockFace::West:
		{
			x -= 1;
			break;
		}
		case BlockFace::Top:
		{
			y += 1;
			break;
		}
		case BlockFace::Bottom:
		{
			y -= 1;
			break;
		}
	}

	bool xOutsideChunk = x < 0 || x >= Chunk::CHUNK_SIZE;
	bool yOutsideChunk = y < 0 || y >= Chunk::CHUNK_SIZE;
	bool zOutsideChunk = z < 0 || z >= Chunk::CHUNK_SIZE;

	if (xOutsideChunk || yOutsideChunk || zOutsideChunk)
	{
		return std::nullopt;
	}

	return DirectX::XMUINT3{static_cast<std::uint32_t>(x),
							static_cast<std::uint32_t>(y),
							static_cast<std::uint32_t>(z)};
}
