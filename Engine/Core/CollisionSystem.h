#pragma once
#include <DirectXCollision.h>
#include <DirectXMath.h>

#include "BlockRaycastResult.h"

class World;
class CollisionSystem
{
public:
	CollisionSystem() = default;

	bool Initialize(World* world);

	/**
	 *
	 * @param entityBox the bounding box of an entity performing the movement
	 * @param entityVelocity velocity of the entity
	 * @param deltaTime
	 * @return the position the entity is allowed to be in
	 */
	DirectX::XMVECTOR ResolveMovement(const DirectX::BoundingBox& entityBox,
									  DirectX::XMFLOAT3			  entityVelocity,
									  float						  deltaTime) const;

	BlockRaycastResult BlockRaycast(DirectX::XMFLOAT3 origin, DirectX::XMFLOAT3 direction, float maxDistance) const;

private:
	World* world_;

	bool CheckBlockCollision(const DirectX::BoundingBox& box) const;
};
