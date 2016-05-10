#ifndef __GK2_ROOM_H_
#define __GK2_ROOM_H_

#include "gk2_applicationBase.h"
#include "gk2_meshLoader.h"
#include "gk2_camera.h"
#include "gk2_phongEffect.h"
#include "gk2_textureEffect.h"
#include "gk2_constantBuffer.h"
#include "gk2_colorTexEffect.h"
#include "gk2_multiTexEffect.h"
#include "gk2_environmentMapper.h"
#include "gk2_particles.h"
#include "gk2_vertices.h"

namespace gk2
{
	class Room : public ApplicationBase
	{
	public:
		explicit Room(HINSTANCE hInstance);
		virtual ~Room();

		static void* operator new(size_t size);
		static void operator delete(void* ptr);

	protected:
		bool LoadContent() override;
		void UnloadContent() override;

		void Update(float dt) override;
		void Render() override;

	private:
		static const float TABLE_H;
		static const float TABLE_TOP_H;
		static const float TABLE_R;
		static const DirectX::XMFLOAT4 TABLE_POS;
		static const DirectX::XMFLOAT4 LIGHT_POS[1];
		static const unsigned int BS_MASK;

		Mesh m_walls[6];
		Mesh m_teapot;
		Mesh m_debugSphere;
		Mesh m_box;
		Mesh m_lamp;
		Mesh m_chairSeat;
		Mesh m_chairBack;
		Mesh m_tableLegs[4];
		Mesh m_tableSide;
		Mesh m_tableTop;
		Mesh m_monitor;
		Mesh m_screen;
		Mesh m_robot[6];
		Mesh m_metal;
		DirectX::XMMATRIX transformMetal;

		DirectX::XMMATRIX m_projMtx;
		DirectX::XMMATRIX m_mirrorMtx;

		Camera m_camera;

		std::shared_ptr<CBMatrix> m_worldCB;
		std::shared_ptr<CBMatrix> m_viewCB;
		std::shared_ptr<CBMatrix> m_projCB;
		std::shared_ptr<ConstantBuffer<DirectX::XMFLOAT4, 2>> m_lightPosCB;
		std::shared_ptr<CBMatrix> m_textureCB;
		std::shared_ptr<CBMatrix> m_posterTexCB;
		std::shared_ptr<ConstantBuffer<DirectX::XMFLOAT4>> m_surfaceColorCB;
		std::shared_ptr<ConstantBuffer<DirectX::XMFLOAT4>> m_cameraPosCB;

		std::shared_ptr<ID3D11SamplerState> m_samplerWrap;
		std::shared_ptr<ID3D11SamplerState> m_samplerBorder;
		std::shared_ptr<ID3D11ShaderResourceView> m_wallTexture;
		std::shared_ptr<ID3D11ShaderResourceView> m_posterTexture;
		std::shared_ptr<ID3D11ShaderResourceView> m_perlinTexture;
		std::shared_ptr<ID3D11ShaderResourceView> m_woodTexture;

		std::shared_ptr<PhongEffect> m_phongEffect;
		std::shared_ptr<TextureEffect> m_textureEffect;
		std::shared_ptr<ColorTexEffect> m_colorTexEffect;
		std::shared_ptr<MultiTexEffect> m_multiTexEffect;
		std::shared_ptr<EnvironmentMapper> m_environmentMapper;
		std::shared_ptr<ParticleSystem> m_particles;
		std::shared_ptr<ID3D11InputLayout> m_layout;

		std::shared_ptr<ID3D11RasterizerState> m_rsCullFront;
		std::shared_ptr<ID3D11RasterizerState> m_rsCullBack;
		//Depth stencil state used to fill the stencil buffer
		std::shared_ptr<ID3D11DepthStencilState> m_dssWrite;
		//Depth stencil state used to perform stencil test when drawing mirrored scene
		std::shared_ptr<ID3D11DepthStencilState> m_dssTest;
		//Rasterizer state used to define front faces as counter-clockwise, used when drawing mirrored scene
		std::shared_ptr<ID3D11RasterizerState> m_rsCounterClockwise;
		std::shared_ptr<ID3D11BlendState> m_bsAlpha;
		std::shared_ptr<ID3D11DepthStencilState> m_dssNoWrite;

		void InitializeConstantBuffers();
		void InitializeTextures();
		void InitializeCamera();
		void InitializeRenderStates();
		void CreateScene();
		void UpdateCamera(const DirectX::XMMATRIX& view) const;
		void UpdateLamp(float dt);
		void UpdateRobot(float dt);

		void DrawScene();
		void DrawWalls(bool isMirror = false) const;
		void DrawTeapot() const;
		void DrawTableElement(Mesh& element) const;
		void DrawTableLegs(DirectX::XMVECTOR camVec);
		void DrawParticles(bool isMirror = false);
		void DrawRobot(bool isMirror = false);
		void DrawMetal();
		void DrawMirroredWorld();

		void inverse_kinematics(VertexPosNormal robotPosition, float & a1, float & a2, float & a3, float & a4, float & a5);
	};
}

#endif __GK2_ROOM_H_
