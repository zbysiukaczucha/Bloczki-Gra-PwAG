#include "CollisionSystem.h"
#include <algorithm>
#include <cmath>

#include "../World/World.h"
bool CollisionSystem::Initialize(World* world)
{
	if (world == nullptr)
	{
		return false;
	}

	world_ = world;
	return true;
}


DirectX::XMVECTOR CollisionSystem::ResolveMovement(const DirectX::BoundingBox& entityBox,
												   DirectX::XMFLOAT3		   entityVelocity,
												   float					   deltaTime) const
{
	DirectX::XMFLOAT3 entitySize = entityBox.Extents;

	DirectX::XMFLOAT3 position	 = entityBox.Center;
	DirectX::XMFLOAT3 moveAmount = {entityVelocity.x * deltaTime,
									entityVelocity.y * deltaTime,
									entityVelocity.z * deltaTime};

	// --- X AXIS ---
	if (std::fabs(moveAmount.x) > 0.0001f)
	{
		DirectX::XMFLOAT3 nextPos  = position;
		nextPos.x				  += moveAmount.x;

		// Create a test box at the new X location
		DirectX::BoundingBox testBox(nextPos, entitySize);

		if (CheckBlockCollision(testBox) == false)
		{
			position.x = nextPos.x; // Valid move
		}
	}

	// --- Y AXIS ---
	if (std::fabs(moveAmount.y) > 0.0001f)
	{
		DirectX::XMFLOAT3 nextPos  = position;
		nextPos.y				  += moveAmount.y;

		// Create a test box at the new X location
		DirectX::BoundingBox testBox(nextPos, entitySize);

		if (CheckBlockCollision(testBox) == false)
		{
			position.y = nextPos.y; // Valid move
		}
	}

	// --- Z AXIS ---
	if (std::fabs(moveAmount.z) > 0.0001f)
	{
		DirectX::XMFLOAT3 nextPos  = position;
		nextPos.z				  += moveAmount.z;

		// Create a test box at the new X location
		DirectX::BoundingBox testBox(nextPos, entitySize);

		if (CheckBlockCollision(testBox) == false)
		{
			position.z = nextPos.z; // Valid move
		}
	}

	// Return final valid movement vector (NewPos - OldPos)
	const DirectX::XMFLOAT3& originalPosition = entityBox.Center;

	return DirectX::XMVectorSet(position.x - originalPosition.x,
								position.y - originalPosition.y,
								position.z - originalPosition.z,
								1.0f);
}

BlockRaycastResult CollisionSystem::BlockRaycast(DirectX::XMFLOAT3 origin,
												 DirectX::XMFLOAT3 direction,
												 float			   maxDistance) const
{
	// Using an integer grid of blocks, make the origin into a block on the grid
	int x = static_cast<int>(std::floorf(origin.x));
	int y = static_cast<int>(std::floorf(origin.y));
	int z = static_cast<int>(std::floorf(origin.z));

	int stepX = (direction.x > 0) ? 1 : -1;
	int stepY = (direction.y > 0) ? 1 : -1;
	int stepZ = (direction.z > 0) ? 1 : -1;

	// Avoiding division by 0
	float deltaDistX = (direction.x == 0) ? 1e30f : std::fabs(1.0f / direction.x);
	float deltaDistY = (direction.y == 0) ? 1e30f : std::fabs(1.0f / direction.y);
	float deltaDistZ = (direction.z == 0) ? 1e30f : std::fabs(1.0f / direction.z);

	float sideDistX, sideDistY, sideDistZ;
	if (direction.x < 0)
	{
		sideDistX = (origin.x - x) * deltaDistX;
	}
	else
	{
		sideDistX = (x + 1.0f - origin.x) * deltaDistX;
	}

	if (direction.y < 0)
	{
		sideDistY = (origin.y - y) * deltaDistY;
	}
	else
	{
		sideDistY = (y + 1.0f - origin.y) * deltaDistY;
	}

	if (direction.z < 0)
	{
		sideDistZ = (origin.z - z) * deltaDistZ;
	}
	else
	{
		sideDistZ = (z + 1.0f - origin.z) * deltaDistZ;
	}

	BlockRaycastResult result{false, {0, 0, 0}, {0, 0, 0}, 0.0f};
	float			   currentDist = 0.0f;
	while (currentDist < maxDistance)
	{
		if (sideDistX < sideDistY && sideDistX < sideDistZ)
		{
			sideDistX		  += deltaDistX;
			x				  += stepX;
			currentDist		   = sideDistX - deltaDistX; // Update total distance
			result.faceNormal  = {-stepX, 0, 0};		 // We hit the side perpendicular to X
		}
		else if (sideDistY < sideDistZ)
		{
			sideDistY		  += deltaDistY;
			y				  += stepY;
			currentDist		   = sideDistY - deltaDistY;
			result.faceNormal  = {0, -stepY, 0};
		}
		else
		{
			sideDistZ		  += deltaDistZ;
			z				  += stepZ;
			currentDist		   = sideDistZ - deltaDistZ;
			result.faceNormal  = {0, 0, -stepZ};
		}

		//  Check if we hit a block
		if (world_->IsBlockSolid(DirectX::XMINT3(x, y, z)))
		{
			result.success		 = true;
			result.blockPosition = {x, y, z};
			result.distance		 = currentDist;
			return result;
		}
	}

	// Missed everything (reached max distance)
	return result;
}

bool CollisionSystem::CheckBlockCollision(const DirectX::BoundingBox& box) const
{

	// Get box corners
	DirectX::XMFLOAT3 minPos = {box.Center.x - box.Extents.x,
								box.Center.y - box.Extents.y,
								box.Center.z - box.Extents.z};

	DirectX::XMFLOAT3 maxPos = {box.Center.x + box.Extents.x,
								box.Center.y + box.Extents.y,
								box.Center.z + box.Extents.z};

	int minX = static_cast<int>(std::floorf(minPos.x));
	int minY = static_cast<int>(std::floorf(minPos.y));
	int minZ = static_cast<int>(std::floorf(minPos.z));

	int maxX = static_cast<int>(std::floorf(maxPos.x - 0.001f));
	int maxY = static_cast<int>(std::floorf(maxPos.y - 0.001f));
	int maxZ = static_cast<int>(std::floorf(maxPos.z - 0.001f));

	for (int x = minX; x <= maxX; ++x)
	{
		for (int y = minY; y <= maxY; ++y)
		{
			for (int z = minZ; z <= maxZ; ++z)
			{
				if (world_->IsBlockSolid(DirectX::XMINT3(x, y, z)))
				{
					DirectX::BoundingBox block;
					block.Center  = DirectX::XMFLOAT3(x + 0.5f, y + 0.5f, z + 0.5f);
					block.Extents = DirectX::XMFLOAT3(0.5f, 0.5f, 0.5f);

					if (box.Intersects(block))
					{
						return true;
					}
				}
			}
		}
	}

	// No solid blocks found
	return false;
}
