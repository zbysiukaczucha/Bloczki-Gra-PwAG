#pragma once
#include <DirectXCollision.h>
#include <DirectXMath.h>

#include "../Core/BlockRaycastResult.h"


class Camera
{
	static constexpr float MOVEMENT_SPEED	 = 2.0f;
	static constexpr float MOUSE_SENSITIVITY = 0.001f;

public:
	Camera();
	bool Initialize(float fovDegrees, float aspectRatio, float nearZ, float farZ);
	void OnWindowResize(float aspectRatio);

	void Move(DirectX::XMFLOAT3 newPosition);
	void Rotate(float dYaw, float dPitch);

private:
	void UpdateVectors();
	void UpdateFrustum();
	void CreateProjectionMatrix();
	void CreateViewMatrix();
	void CreateFrustum();

	DirectX::XMFLOAT3 position_;
	DirectX::XMFLOAT3 forward_;
	DirectX::XMFLOAT3 up_;
	DirectX::XMFLOAT3 right_;

	// Euler angles in radians
	float pitch_;
	float yaw_;

	float fov_;
	float aspectRatio_;
	float nearZ_;
	float farZ_;

	DirectX::XMFLOAT4X4		 projectionMatrix_;
	DirectX::XMFLOAT4X4		 viewMatrix_;
	DirectX::BoundingFrustum cameraFrustum_;

	bool projMatrixDirty_;
	bool viewMatrixDirty_;
	bool frustumDirty_;

	DirectX::BoundingBox playerBoundingBox_;
	BlockRaycastResult	 blockRaycastResult_;

public:
	// Getters
	[[nodiscard]] DirectX::XMMATRIX				  GetViewMatrix();
	[[nodiscard]] DirectX::XMMATRIX				  GetProjectionMatrix();
	[[nodiscard]] DirectX::XMMATRIX				  GetProjectionViewMatrix();
	[[nodiscard]] const DirectX::BoundingFrustum& GetFrustum();
	[[nodiscard]] DirectX::XMVECTOR				  GetPosition() const;
	[[nodiscard]] const BlockRaycastResult&		  GetBlockRaycastResult() const { return blockRaycastResult_; };
	[[nodiscard]] DirectX::XMVECTOR				  GetForward() const { return DirectX::XMLoadFloat3(&forward_); };
	[[nodiscard]] DirectX::XMVECTOR				  GetUp() const { return DirectX::XMLoadFloat3(&up_); };
	[[nodiscard]] DirectX::XMVECTOR				  GetRight() const { return DirectX::XMLoadFloat3(&right_); };
};
