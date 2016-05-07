#ifndef __GK2_CAMERA_H_
#define __GK2_CAMERA_H_

#include <DirectXMath.h>

namespace gk2
{
	class Camera
	{
	public:
		Camera(float minDistance, float maxDistance, float distance = 0.0f);

		void SetRange(float minDistance, float maxDistance);
		void Zoom(float dx, float dz);
		void Rotate(float dx, float dy);
		DirectX::XMMATRIX GetViewMatrix() const;
		void GetViewMatrix(DirectX::XMMATRIX& viewMatrix) const;
		DirectX::XMFLOAT4 GetPosition() const;
		DirectX::XMVECTOR getForwardDir() const;
		DirectX::XMVECTOR getRightDir() const;
		DirectX::XMVECTOR getUpDir() const;

	private:
		float m_angleX;
		float m_angleY;
		float m_yPos;
		float m_zPos;
		float m_xPos;
		float m_minDistance;
		float m_maxDistance;

		void ClampDistance();
	};
}

#endif __GK2_CAMERA_H_
