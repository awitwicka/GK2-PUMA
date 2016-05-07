#ifndef __GK2_VERTICES_H_
#define __GK2_VERTICES_H_

#include <d3d11.h>
#include <DirectXMath.h>

namespace gk2
{
	struct VertexPos 
	{
		DirectX::XMFLOAT3 Pos;
		static const unsigned int LayoutElements = 1;
		static const D3D11_INPUT_ELEMENT_DESC Layout[LayoutElements];
	};

	struct VertexPosNormal
	{
		DirectX::XMFLOAT3 Pos;
		DirectX::XMFLOAT3 Normal;
		static const unsigned int LayoutElements = 2;
		static const D3D11_INPUT_ELEMENT_DESC Layout[LayoutElements];
	};

	struct VertexLineTriangles
	{
		DirectX::XMFLOAT3 Pos1;
		DirectX::XMFLOAT3 Pos2;
		unsigned short Triangle1[3];
		unsigned short Triangle2[3];
		static const unsigned int LayoutElements = 4;
		static const D3D11_INPUT_ELEMENT_DESC Layout[LayoutElements];
	};

}

#endif __GK2_VERTICES_H_
