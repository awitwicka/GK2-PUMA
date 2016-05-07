#include "gk2_camera.h"
#include <DirectXMath.h>
#include <stdio.h>
using namespace gk2;
using namespace DirectX;

Camera::Camera(float minDistance, float maxDistance, float distance)
	: m_angleX(0.0f), m_angleY(0.0f), m_zPos(distance), m_xPos(0.0f), m_yPos(0.0f)
{
	SetRange(minDistance, maxDistance);
}

void Camera::ClampDistance()
{
	/*if (m_xPos < m_minDistance)
		m_xPos = m_minDistance;
	if (m_xPos > m_maxDistance)
		m_xPos = m_maxDistance;*/
}

void Camera::SetRange(float minDistance, float maxDistance)
{
	if (minDistance < 0)
		minDistance = 0;
	if (maxDistance < minDistance)
		maxDistance = minDistance;
	m_minDistance = minDistance;
	m_maxDistance = maxDistance;
	ClampDistance();
}

void Camera::Zoom(float dx, float dz)
{
	XMVECTOR forward = getForwardDir();
	XMVECTOR right = getRightDir();
	XMVECTOR up = getUpDir();
	XMFLOAT3 temp;
	
	//XMStoreFloat3(&temp, forward*d + right * 0);
	XMStoreFloat3(&temp, forward*dz + up*dz);//+ up*dz
	m_zPos += temp.z;
	m_xPos += temp.x;
	m_yPos += temp.y;

	//	printf("%lf\n", temp.y);

	//ClampDistance();
}

void Camera::Rotate(float dx, float dy)
{
	m_angleX = XMScalarModAngle(m_angleX + dx);
	m_angleY = XMScalarModAngle(m_angleY + dy);
}

XMMATRIX Camera::GetViewMatrix() const
{
	XMMATRIX viewMtx;
	GetViewMatrix(viewMtx);
	return viewMtx;
}

void Camera::GetViewMatrix(XMMATRIX& viewMtx) const
{
	XMVECTOR forward = getForwardDir();
	XMVECTOR right = getRightDir();
	//	XMFLOAT3 temp;

	//	XMStoreFloat3(&temp, forward*m_zPos + right*m_xPos);
	//	XMFLOAT2 tr = XMFLOAT2(temp.x, temp.z);


	XMFLOAT3 up = XMFLOAT3(0.0f, 1.0f, 0.0f);
	XMFLOAT3 position = XMFLOAT3(m_xPos, m_yPos, m_zPos);
	XMFLOAT3 lookAt = XMFLOAT3(0, 0, 1);


	XMMATRIX rotationMatrix;

	float pitch = m_angleX;
	float yaw = -m_angleY;
	float roll = 0;
	rotationMatrix = XMMatrixRotationRollPitchYaw(pitch, yaw, roll);

	XMVECTOR lookAtVec = XMVector3TransformCoord(XMLoadFloat3(&lookAt), rotationMatrix);

	XMVECTOR upVec = XMVector3TransformCoord(XMLoadFloat3(&up), rotationMatrix);

	lookAtVec = XMLoadFloat3(&position) + lookAtVec;

	viewMtx = XMMatrixLookAtLH(lookAtVec, XMLoadFloat3(&position), upVec);

}

XMFLOAT4 Camera::GetPosition() const
{
	XMMATRIX viewMtx;
	GetViewMatrix(viewMtx);
	XMVECTOR det;
	viewMtx = XMMatrixInverse(&det, viewMtx);
	//auto alt = XMMatrixTranslation(0.0f, 0.0f, -m_distance) * XMMatrixRotationY(m_angleY) * XMMatrixRotationX(-m_angleX);
	XMFLOAT3 res(0.0f, 0.0f, 0.0f);
	auto transl = XMVector3TransformCoord(XMLoadFloat3(&res), viewMtx);
	XMStoreFloat3(&res, transl);
	return XMFLOAT4(res.x, res.y, res.z, 1.0f);
}


XMVECTOR Camera::getForwardDir() const
{
	XMVECTOR forward = XMVectorSet(0, 0, 1, 0);
	forward = XMVector3TransformNormal(forward, XMMatrixRotationY(-m_angleY));
	return forward;
}

XMVECTOR Camera::getRightDir() const
{
	XMVECTOR right = XMVectorSet(1, 0, 0, 0);
	right = XMVector3TransformNormal(right, XMMatrixRotationY(m_angleY));
	return right;
}

XMVECTOR Camera::getUpDir() const
{
	XMVECTOR up = XMVectorSet(0, 1, 0, 0);
	up = XMVector3TransformNormal(up, XMMatrixRotationX((XM_PI / 2) + m_angleX));
	XMFLOAT3 n3 = XMFLOAT3();
	XMStoreFloat3(&n3, up);
	up = XMVectorSet(0, n3.y,0,0);
	return up;
}
