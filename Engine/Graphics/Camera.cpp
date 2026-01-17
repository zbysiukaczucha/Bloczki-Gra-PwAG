#include "Camera.h"
#include <algorithm>

#include "../Core/CollisionSystem.h"
#include "../Core/Input.h"
#include "../World/World.h"

Camera::Camera() :
	position_(0, 0, 0),
	forward_(0, 0, 1),
	up_(0, 1, 0),
	right_(1, 0, 0),
	pitch_(0.0f),
	yaw_(0.0f),
	viewMatrixDirty_(true),
	projMatrixDirty_(true),
	frustumDirty_(true)
{
}

bool Camera::Initialize(float fovDegrees, float aspectRatio, float nearZ, float farZ)
{
	fov_		 = fovDegrees;
	aspectRatio_ = aspectRatio;
	nearZ_		 = nearZ;
	farZ_		 = farZ;

	return true;
}

void Camera::OnWindowResize(float aspectRatio)
{
	// Re-create projection matrix
	aspectRatio_ = aspectRatio;

	projMatrixDirty_ = true;
	frustumDirty_	 = true;
}

void Camera::UpdateVectors()
{
	using namespace DirectX;

	XMVECTOR rotation	  = XMQuaternionRotationRollPitchYaw(pitch_, yaw_, 0.0f);
	XMVECTOR defaultFront = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
	XMVECTOR front		  = XMVector3Rotate(defaultFront, rotation);

	XMVECTOR worldUp = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
	XMVECTOR right	 = XMVector3Normalize(XMVector3Cross(worldUp, front));
	XMVECTOR up		 = XMVector3Normalize(XMVector3Cross(front, right));

	XMStoreFloat3(&forward_, front);
	XMStoreFloat3(&up_, up);
	XMStoreFloat3(&right_, right);
}

void Camera::UpdateFrustum()
{
	using namespace DirectX;

	XMVECTOR rotation	  = XMQuaternionRotationRollPitchYaw(pitch_, yaw_, 0.0f);
	cameraFrustum_.Origin = position_;
	XMStoreFloat4(&cameraFrustum_.Orientation, rotation);
	frustumDirty_ = false;
}

void Camera::Move(DirectX::XMFLOAT3 newPosition)
{
	position_		 = newPosition;
	viewMatrixDirty_ = true;
	frustumDirty_	 = true;
}

void Camera::Rotate(float dYaw, float dPitch)
{
	pitch_ += dPitch;
	yaw_   += dYaw;

	float limit = DirectX::XM_PIDIV2 - 0.01;
	pitch_		= std::clamp(pitch_, -limit, limit);
	UpdateVectors();
	viewMatrixDirty_ = true;
	frustumDirty_	 = true;
}

void Camera::CreateProjectionMatrix()
{
	using namespace DirectX;
	float	 fovRadians = XMConvertToRadians(fov_);
	XMMATRIX proj		= XMMatrixPerspectiveFovLH(fovRadians, aspectRatio_, nearZ_, farZ_);
	XMStoreFloat4x4(&projectionMatrix_, proj);
	projMatrixDirty_ = false;
}

void Camera::CreateViewMatrix()
{
	using namespace DirectX;
	const XMVECTOR position = XMLoadFloat3(&position_);
	const XMVECTOR forward	= XMLoadFloat3(&forward_);
	const XMVECTOR up		= XMLoadFloat3(&up_);
	// const XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

	XMStoreFloat4x4(&viewMatrix_, XMMatrixLookAtLH(position, position + forward, up));
	viewMatrixDirty_ = false;
}

void Camera::CreateFrustum()
{
	DirectX::BoundingFrustum::CreateFromMatrix(cameraFrustum_, GetProjectionMatrix());
}

DirectX::XMMATRIX Camera::GetViewMatrix()
{
	using namespace DirectX;

	// If view matrix needs recalculating
	if (viewMatrixDirty_) [[likely]]
	{
		CreateViewMatrix();
	}

	return XMLoadFloat4x4(&viewMatrix_);
}

DirectX::XMMATRIX Camera::GetProjectionMatrix()
{
	using namespace DirectX;

	// If projection matrix needs recalculating
	if (projMatrixDirty_) [[unlikely]]
	{
		CreateProjectionMatrix();
		CreateFrustum();
	}

	return XMLoadFloat4x4(&projectionMatrix_);
}

DirectX::XMMATRIX Camera::GetProjectionViewMatrix()
{
	using namespace DirectX;

	const auto projectionMatrix = GetProjectionMatrix();
	const auto viewMatrix		= GetViewMatrix();

	return projectionMatrix * viewMatrix;
}

const DirectX::BoundingFrustum& Camera::GetFrustum()
{
	if (frustumDirty_) [[likely]]
	{
		UpdateFrustum();
	}

	return cameraFrustum_;
}

DirectX::XMVECTOR Camera::GetPosition() const
{
	return XMLoadFloat3(&position_);
}
