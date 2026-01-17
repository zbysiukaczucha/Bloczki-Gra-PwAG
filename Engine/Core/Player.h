#pragma once
#include <array>


#include "../Graphics/Camera.h"


enum class BlockType : std::size_t;
class World;
class CollisionSystem;
class Input;

class Player
{
	static constexpr float MOVEMENT_SPEED			   = 10.0f;
	static constexpr float MOUSE_SENSITIVITY		   = 0.001f;
	static constexpr float HALF_CAMERA_OFFSET		   = 0.8f;
	static constexpr float CAMERA_OFFSET			   = HALF_CAMERA_OFFSET * 2;
	static constexpr float MAX_BLOCK_INTERACTION_RANGE = 5.0f;

public:
	Player() = default;

	bool Initialize(CollisionSystem* collisionSystem,
					World*			 world,
					Input*			 input,
					float			 fovDegrees,
					float			 aspectRatio,
					float			 nearZ,
					float			 farZ);

	void OnTick(float deltaTime);

private:
	void HandleInput(float deltaTime);
	void HandleMovement(float deltaTime, DirectX::XMFLOAT3 movementDirection);
	void HandleMouseInput(float deltaTime);

	static constexpr std::size_t BLOCK_BAR_SIZE = 8;

	Camera								  camera_;
	CollisionSystem*					  collisionSystem_;
	World*								  world_;
	Input*								  input_;
	DirectX::BoundingBox				  playerBounds_;
	BlockRaycastResult					  lastBlockRaycastResult_;
	DirectX::XMFLOAT3					  playerPosition_;
	std::array<BlockType, BLOCK_BAR_SIZE> blockBar_;
	std::uint8_t						  currentBlockBarIndex_ = 0;

public:
	// Getters
	[[nodiscard]] Camera&					GetCamera() { return camera_; }
	[[nodiscard]] const BlockRaycastResult& GetLastBlockRaycastResult() const { return lastBlockRaycastResult_; }
	[[nodiscard]] auto						GetBlockBarIndex() const { return currentBlockBarIndex_; }
	[[nodiscard]] auto&						GetBlockBar() const { return blockBar_; }
};
