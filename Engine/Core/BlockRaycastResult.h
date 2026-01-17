#pragma once
#include <DirectXMath.h>


struct BlockRaycastResult
{
	bool			success = false;
	DirectX::XMINT3 blockPosition;
	DirectX::XMINT3 faceNormal = {0, 0, 0};
	float			distance   = -1;
};
