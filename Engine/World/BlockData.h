#pragma once
#include <DirectXMath.h>
#include <string>

struct BlockData
{
	std::string	 blockName;
	bool		 isTransparent		= false;
	bool		 isSolid			= true;
	std::uint8_t lightEmissionLevel = 0;

	DirectX::XMFLOAT3 lightColor	 = DirectX::XMFLOAT3(1.0f, 1.0f, 1.0f);
	float			  lightIntensity = 0.0f;

	std::size_t textureIndices[6]{}; /* Format: Idx 0 = North
												Idx 1 = South
												Idx 2 = West
												Idx 3 = East
												Idx 4 = Top
												Idx 5 = Bottom*/
};
