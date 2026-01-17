#include "Player.h"

#include <algorithm>

#include "Input.h"

#include <dinput.h>

#include "../World/World.h"
#include "CollisionSystem.h"
bool Player::Initialize(CollisionSystem* collisionSystem,
						World*			 world,
						Input*			 input,
						float			 fovDegrees,
						float			 aspectRation,
						float			 nearZ,
						float			 farZ)
{
	collisionSystem_ = collisionSystem;
	world_			 = world;
	input_			 = input;

	if (collisionSystem_ == nullptr || world_ == nullptr || input_ == nullptr)
	{
		return false;
	}

	playerPosition_ = {0.0f, 100.0f, 0.f};
	camera_.Move({playerPosition_.x, playerPosition_.y + CAMERA_OFFSET, playerPosition_.z});

	std::ranges::fill(blockBar_, BlockType::INVALID_);

	blockBar_[0] = BlockType::Dirt;
	blockBar_[1] = BlockType::Grass;
	blockBar_[2] = BlockType::Stone;
	blockBar_[3] = BlockType::Cobblestone;
	blockBar_[4] = BlockType::Log;
	blockBar_[5] = BlockType::Glowstone;
	blockBar_[6] = BlockType::DiamondBlock;
	blockBar_[7] = BlockType::IronBlock;
	// blockBar_[8] = BlockType::Dirt;


	return camera_.Initialize(fovDegrees, aspectRation, nearZ, farZ);
}

void Player::OnTick(const float deltaTime)
{
	using namespace DirectX;


	// Recreate collision
	XMVECTOR playerPos = XMLoadFloat3(&playerPosition_);
	// offset so that the box is centered on the player
	XMVECTOR offset = XMVectorSet(0.0f, 0.9f, 0.0f, 1.0f);
	XMFLOAT3 offsetPlayerPos;
	XMStoreFloat3(&offsetPlayerPos, offset + playerPos);

	playerBounds_ = BoundingBox(offsetPlayerPos, XMFLOAT3(0.3f, 0.9f, 0.3f));

	HandleInput(deltaTime);
	HandleMouseInput(deltaTime);
}

void Player::HandleInput(const float deltaTime)
{
	using namespace DirectX;

	XMVECTOR forward = camera_.GetForward();
	forward			 = XMVectorSetY(forward, 0.0f); // flatten forward vector
	XMVECTOR right	 = camera_.GetRight();
	XMVECTOR up		 = XMVectorSet(0.0f, 1.0f, 0.0f, 1.0f);

	int forwardMultiplier = 0;
	int rightMultiplier	  = 0;
	int upMultiplier	  = 0;

	// Handle movement
	if (input_->IsKeyPressed(DIK_W))
	{
		forwardMultiplier += 1;
	}
	if (input_->IsKeyPressed(DIK_S))
	{
		forwardMultiplier -= 1;
	}
	if (input_->IsKeyPressed(DIK_A))
	{
		rightMultiplier -= 1;
	}
	if (input_->IsKeyPressed(DIK_D))
	{
		rightMultiplier += 1;
	}
	if (input_->IsKeyPressed(DIK_SPACE))
	{
		upMultiplier += 1;
	}
	if (input_->IsKeyPressed(DIK_LSHIFT))
	{
		upMultiplier -= 1;
	}

	XMVECTOR moveDirection = XMVectorZero();
	moveDirection		   = XMVectorAdd(moveDirection, XMVectorScale(forward, static_cast<float>(forwardMultiplier)));
	moveDirection		   = XMVectorAdd(moveDirection, XMVectorScale(right, static_cast<float>(rightMultiplier)));
	moveDirection		   = XMVectorAdd(moveDirection, XMVectorScale(up, static_cast<float>(upMultiplier)));
	moveDirection		   = XMVector3Normalize(moveDirection);

	// Apply movement speed now
	moveDirection = XMVectorScale(moveDirection, MOVEMENT_SPEED);

	// Check if we should do any movement
	if (XMVectorGetX(XMVector3LengthSq(moveDirection)) > 0.0f)
	{
		XMFLOAT3 finalMoveDir;
		XMStoreFloat3(&finalMoveDir, moveDirection);
		HandleMovement(deltaTime, finalMoveDir);
	}

	// Pass time
	if (input_->IsKeyPressed(DIK_LEFTARROW))
	{
		world_->PassWorldTime(-0.001f);
	}
	if (input_->IsKeyPressed(DIK_RIGHTARROW))
	{
		world_->PassWorldTime(0.001f);
	}
}

void Player::HandleMovement(const float deltaTime, const DirectX::XMFLOAT3 movementDirection)
{
	using namespace DirectX;

	XMVECTOR finalMovementVector  = collisionSystem_->ResolveMovement(playerBounds_, movementDirection, deltaTime);
	XMVECTOR playerPos			  = XMLoadFloat3(&playerPosition_);
	playerPos					 += finalMovementVector;

	XMStoreFloat3(&playerPosition_, playerPos);

	static const XMVECTOR cameraOffset = XMVectorSet(0.0, CAMERA_OFFSET, 0.0, 1.0f);
	XMFLOAT3			  newCamPosition;
	XMStoreFloat3(&newCamPosition, playerPos + cameraOffset);
	// Update camera position
	camera_.Move(newCamPosition);
}

void Player::HandleMouseInput(const float deltaTime)
{
	using namespace DirectX;
	auto [dYaw, dPitch, dScroll] = input_->GetMouseDelta();
	if (dYaw != 0 || dPitch != 0)
	{
		float dYawF	  = dYaw * MOUSE_SENSITIVITY;
		float dPitchF = dPitch * MOUSE_SENSITIVITY;
		camera_.Rotate(dYawF, dPitchF);
	}
	// Change current blockBar block
	if (dScroll != 0)
	{
		if (dScroll > 0)
		{
			const int nextIndex	  = currentBlockBarIndex_ - 1;
			currentBlockBarIndex_ = (nextIndex) < 0 ? blockBar_.size() - 1 : nextIndex;
		}
		else
		{
			const int nextIndex	  = currentBlockBarIndex_ + 1;
			currentBlockBarIndex_ = (nextIndex) >= blockBar_.size() ? 0 : nextIndex;
		}
	}

	// Perform raycast for blocks
	XMFLOAT3 cameraPos;
	XMStoreFloat3(&cameraPos, camera_.GetPosition());
	XMFLOAT3 forward;
	XMStoreFloat3(&forward, camera_.GetForward());

	lastBlockRaycastResult_ = collisionSystem_->BlockRaycast(cameraPos, forward, MAX_BLOCK_INTERACTION_RANGE);

	// Prevent insta-breaking ray of destruction
	static float breakTimer = 0.0f;
	static float placeTimer = 0.0f;

	// Check the mouse keys
	if (input_->IsKeyPressed(MouseButton::LEFT))
	{
		if (breakTimer >= 0.35f || breakTimer == 0.0f)
		{
			breakTimer = 0.0f;
			if (lastBlockRaycastResult_.success)
			{
				world_->SetBlock(lastBlockRaycastResult_.blockPosition, BlockType::Air, BlockFace::North);
			}
		}
		breakTimer += deltaTime;
	}
	else
	{
		breakTimer = 0.0f;
	}
	if (input_->IsKeyPressed(MouseButton::RIGHT))
	{
		if (placeTimer >= 1.0f || placeTimer == 0.0f)
		{
			placeTimer = 0.0f;
			if (lastBlockRaycastResult_.success)
			{
				XMVECTOR hitBlockCoords = XMLoadSInt3(&lastBlockRaycastResult_.blockPosition);
				XMVECTOR offset			= XMLoadSInt3(&lastBlockRaycastResult_.faceNormal);
				XMINT3	 placedBlockCoords;
				XMStoreSInt3(&placedBlockCoords, hitBlockCoords + offset);
				world_->SetBlock(placedBlockCoords, blockBar_[currentBlockBarIndex_], BlockFace::North);
			}
		}
		placeTimer += deltaTime;
	}
	else
	{
		placeTimer = 0.0f;
	}
}
