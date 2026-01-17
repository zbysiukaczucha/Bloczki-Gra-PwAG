#pragma once
#include <DirectXMath.h>
#include <utility>
#include <vector>
#include "../Graphics/Vertex.h"
#include "../World/BlockDatabase.h"
#include "../World/BlockFace.h"

namespace Utils::Test
{
	inline std::pair<std::vector<Vertex>, std::vector<uint32_t>> CreateTestCube(
		BlockType blockType = BlockType::Cobblestone)
	{
		std::vector<Vertex>		   vertices;
		std::vector<std::uint32_t> indices;

		// Helper to push a face
		auto AddFace = [&](DirectX::XMFLOAT3 normal,
						   DirectX::XMFLOAT3 tangent,
						   DirectX::XMFLOAT3 bitangent,
						   DirectX::XMFLOAT3 tr,
						   DirectX::XMFLOAT3 tl,
						   DirectX::XMFLOAT3 bl,
						   DirectX::XMFLOAT3 br,
						   uint32_t			 matID)
		{
			uint16_t baseIndex = static_cast<uint32_t>(vertices.size());

			// Push 4 vertices for this face
			// UVs: 0,0 (TopLeft) -> 1,1 (BottomRight)
			vertices.push_back({tl, normal, tangent, bitangent, {0.0f, 0.0f}, matID}); // 0: Top-Left
			vertices.push_back({tr, normal, tangent, bitangent, {1.0f, 0.0f}, matID}); // 1: Top-Right
			vertices.push_back({bl, normal, tangent, bitangent, {0.0f, 1.0f}, matID}); // 2: Bottom-Left
			vertices.push_back({br, normal, tangent, bitangent, {1.0f, 1.0f}, matID}); // 3: Bottom-Right

			// Push 2 triangles (Clockwise winding)
			indices.push_back(baseIndex + 0);
			indices.push_back(baseIndex + 1);
			indices.push_back(baseIndex + 2);

			indices.push_back(baseIndex + 1);
			indices.push_back(baseIndex + 3);
			indices.push_back(baseIndex + 2);
		};

		// Define coordinates (Unit cube centered at 0,0,0)
		float s = 0.5f;

		// FRONT Face (Normal -Z)
		BlockDatabase&	 blockDatabase = BlockDatabase::GetDatabase();
		const BlockData* data		   = blockDatabase.GetBlockData(blockType);
		if (data == nullptr)
		{
			return {};
		}

		auto matID = data->textureIndices[0]; // cobblestone should have same index for all faces;

		auto northMatID	 = data->textureIndices[static_cast<uint8_t>(BlockFace::North)];
		auto southMatID	 = data->textureIndices[static_cast<uint8_t>(BlockFace::South)];
		auto westMatID	 = data->textureIndices[static_cast<uint8_t>(BlockFace::West)];
		auto eastMatID	 = data->textureIndices[static_cast<uint8_t>(BlockFace::East)];
		auto topMatID	 = data->textureIndices[static_cast<uint8_t>(BlockFace::Top)];
		auto bottomMatID = data->textureIndices[static_cast<uint8_t>(BlockFace::Bottom)];

		// --------------------------------------------------------
		// TANGENT/BITANGENT LOGIC
		// Tangent   = Direction of U (Texture X)
		// Bitangent = Direction of V (Texture Y)
		// --------------------------------------------------------

		// FRONT Face (Normal -Z)
		// U goes Right (+X), V goes Down (-Y)
		AddFace({0.0f, 0.0f, -1.0f},
				{1.0f, 0.0f, 0.0f},
				{0.0f, -1.0f, 0.0f},
				{s, s, -s},
				{-s, s, -s},
				{-s, -s, -s},
				{s, -s, -s},
				southMatID);

		// BACK Face (Normal +Z)
		// U goes Left (-X) to wrap correctly, V goes Down (-Y)
		AddFace({0.0f, 0.0f, 1.0f},
				{-1.0f, 0.0f, 0.0f},
				{0.0f, -1.0f, 0.0f},
				{-s, s, s},
				{s, s, s},
				{s, -s, s},
				{-s, -s, s},
				northMatID);

		// LEFT Face (Normal -X)
		// U goes Forward (+Z), V goes Down (-Y)
		AddFace({-1.0f, 0.0f, 0.0f},
				{0.0f, 0.0f, -1.0f},
				{0.0f, -1.0f, 0.0f},
				{-s, s, -s},
				{-s, s, s},
				{-s, -s, s},
				{-s, -s, -s},
				westMatID);

		// RIGHT Face (Normal +X)
		// U goes Backward (-Z), V goes Down (-Y)
		AddFace({1.0f, 0.0f, 0.0f},
				{0.0f, 0.0f, 1.0f},
				{0.0f, -1.0f, 0.0f},
				{s, s, s},
				{s, s, -s},
				{s, -s, -s},
				{s, -s, s},
				eastMatID);

		// TOP Face (Normal +Y)
		// U goes Right (+X), V goes Forward (-Z) - Standard mapping
		AddFace({0.0f, 1.0f, 0.0f},
				{1.0f, 0.0f, 0.0f},
				{0.0f, 0.0f, -1.0f},
				{s, s, s},
				{-s, s, s},
				{-s, s, -s},
				{s, s, -s},
				topMatID);

		// BOTTOM Face (Normal -Y)
		// U goes Right (+X), V goes Backward (+Z)
		AddFace({0.0f, -1.0f, 0.0f},
				{1.0f, 0.0f, 0.0f},
				{0.0f, 0.0f, 1.0f},
				{s, -s, -s},
				{-s, -s, -s},
				{-s, -s, s},
				{s, -s, s},
				bottomMatID);
		return {vertices, indices};
	}
} // namespace Utils::Test
