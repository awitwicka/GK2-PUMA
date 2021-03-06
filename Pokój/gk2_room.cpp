﻿#include "gk2_room.h"
#include "gk2_window.h"
#include "gk2_textureGenerator.h"
#include "gk2_utils.h"
#include <array>



using namespace std;
using namespace gk2;
using namespace DirectX;

const float Room::TABLE_H = 1.0f;
const float Room::TABLE_TOP_H = 0.1f;
const float Room::TABLE_R = 1.5f;
const XMFLOAT4 Room::TABLE_POS = XMFLOAT4(0.5f, -0.96f, 0.5f, 1.0f);
const XMFLOAT4 Room::LIGHT_POS[1] = {XMFLOAT4(-1.0f, 2.6f, 0.0f, 1.0f)};
const unsigned int Room::BS_MASK = 0xffffffff;


Room::Room(HINSTANCE hInstance)
	: ApplicationBase(hInstance), m_camera(0.01f, 100.0f)
{

}

Room::~Room()
{

}

void* Room::operator new(size_t size)
{
	return Utils::New16Aligned(size);
}

void Room::operator delete(void* ptr)
{
	Utils::Delete16Aligned(ptr);
}

void Room::InitializeConstantBuffers()
{
	m_projCB.reset(new CBMatrix(m_device));
	m_viewCB.reset(new CBMatrix(m_device));
	m_worldCB.reset(new CBMatrix(m_device));
	m_lightPosCB.reset(new ConstantBuffer<XMFLOAT4, 2>(m_device));
	m_textureCB.reset(new CBMatrix(m_device));
	m_posterTexCB.reset(new CBMatrix(m_device));
	m_surfaceColorCB.reset(new ConstantBuffer<XMFLOAT4>(m_device));
	m_cameraPosCB.reset(new ConstantBuffer<XMFLOAT4>(m_device));
}

void Room::InitializeCamera()
{
	auto s = getMainWindow()->getClientSize();
	auto ar = static_cast<float>(s.cx) / s.cy;
	m_projMtx = XMMatrixPerspectiveFovLH(XM_PIDIV4, ar, 0.01f, 100.0f);
	m_projCB->Update(m_context, m_projMtx);
	m_camera.Zoom(0,5);
	UpdateCamera(m_camera.GetViewMatrix());
}

void Room::InitializeTextures()
{
	constexpr auto WOOD_TEX_WIDTH = 64;
	constexpr auto WOOD_TEX_HEIGHT = 512;
	constexpr auto WOOD_TEX_BPP = 4;
	constexpr auto WOOD_TEX_STRIDE = WOOD_TEX_WIDTH * WOOD_TEX_BPP;
	constexpr auto WOOD_TEX_SIZE = WOOD_TEX_STRIDE * WOOD_TEX_HEIGHT;
	m_wallTexture = m_device.CreateShaderResourceView(L"resources/textures/brick_wall.jpg", m_context);
	m_posterTexture = m_device.CreateShaderResourceView(L"resources/textures/lautrec_divan.jpg", m_context);
	auto sd = m_device.DefaultSamplerDesc();
	sd.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	sd.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	m_samplerWrap = m_device.CreateSamplerState(sd);
	sd.AddressU = D3D11_TEXTURE_ADDRESS_BORDER;
	sd.AddressV = D3D11_TEXTURE_ADDRESS_BORDER;
	//sd.MipLODBias = 4.0f;
	m_samplerBorder = m_device.CreateSamplerState(sd);
	m_perlinTexture = m_device.CreateShaderResourceView(L"resources/textures/perlin.jpg", m_context);

	auto texDesc = m_device.DefaultTexture2DDesc();
	texDesc.Width = WOOD_TEX_WIDTH;
	texDesc.Height = WOOD_TEX_HEIGHT;
	texDesc.BindFlags |= D3D11_BIND_RENDER_TARGET;
	texDesc.MiscFlags = D3D11_RESOURCE_MISC_GENERATE_MIPS;
	auto woodTexture = m_device.CreateTexture2D(texDesc);
	m_woodTexture = m_device.CreateShaderResourceView(woodTexture);
	array<BYTE, WOOD_TEX_SIZE> data;
	auto d = data.data();
	TextureGenerator txGen(6, 0.35f);
	for (auto i = 0; i < WOOD_TEX_HEIGHT; ++i)
	{
		auto x = i / static_cast<float>(WOOD_TEX_HEIGHT);
		for (auto j = 0; j < WOOD_TEX_WIDTH; ++j)
		{
			auto y = j / static_cast<float>(WOOD_TEX_WIDTH);
			auto c = txGen.Wood(x, y);
			auto ic = static_cast<BYTE>(c * 239);
			*(d++) = ic;
			ic = static_cast<BYTE>(c * 200);
			*(d++) = ic;
			ic = static_cast<BYTE>(c * 139);
			*(d++) = ic;
			*(d++) = 255;
		}
	}
	m_context->UpdateSubresource(woodTexture.get(), 0, nullptr, data.data(), WOOD_TEX_STRIDE, WOOD_TEX_SIZE);
	m_context->GenerateMips(m_woodTexture.get());
}

void Room::CreateScene()
{
	MeshLoader loader(m_device);
	//robot
	m_robot[0] = loader.LoadTxtMesh(L"resources/meshes/mesh1.txt");
	m_robot[1] = loader.LoadTxtMesh(L"resources/meshes/mesh2.txt");
	m_robot[2] = loader.LoadTxtMesh(L"resources/meshes/mesh3.txt");
	m_robot[3] = loader.LoadTxtMesh(L"resources/meshes/mesh4.txt");
	m_robot[4] = loader.LoadTxtMesh(L"resources/meshes/mesh5.txt");
	m_robot[5] = loader.LoadTxtMesh(L"resources/meshes/mesh6.txt");
	//metal
	m_metal = loader.GetQuad(2.0f);
	auto metal = XMMatrixTranslation(0.0f, 0.0f, 2.0f);
	float deg30toRad = 0.523599;
	//transformMetal = XMMatrixRotationX(deg30toRad) * metal * XMMatrixRotationY(-XM_PIDIV2) * XMMatrixTranslation(0.5f, -(2-(sqrt(3)/2)), 0.0f);
	transformMetal = XMMatrixRotationX(deg30toRad) * XMMatrixRotationY(-XM_PIDIV2) * XMMatrixTranslation(-1.5f, 0.0f, 0.0f);
	m_metal.setWorldMatrix(transformMetal);
	XMVECTOR det;
	m_mirrorMtx = XMMatrixInverse(&det, transformMetal) * XMMatrixScaling(1, 1, -1) * transformMetal;
	//walls
	m_walls[0] = loader.GetQuad(4.0f);
	for (auto i = 1; i < 6; ++i)
		m_walls[i] = m_walls[0];
	auto wall = XMMatrixTranslation(0.0f, 0.0f, 2.0f);
	float a = 0;
	for (auto i = 0; i < 4; ++i, a += XM_PIDIV2)
		m_walls[i].setWorldMatrix(wall * XMMatrixRotationY(a) * XMMatrixTranslation(0.0f,1.0f,0.0f));
	m_walls[4].setWorldMatrix(wall * XMMatrixRotationX(XM_PIDIV2)* XMMatrixTranslation(0.0f, 1.0f, 0.0f));
	m_walls[5].setWorldMatrix(wall * XMMatrixRotationX(-XM_PIDIV2)* XMMatrixTranslation(0.0f, 1.0f, 0.0f));
	//lamp
	m_lamp = loader.LoadMesh(L"resources/meshes/lamp.mesh");
	UpdateLamp(0.0f);
	//table 
	m_tableLegs[0] = loader.GetCylinder(4, 9, 0.1f, TABLE_H - TABLE_TOP_H);
	for (auto i = 1; i < 4; ++i)
		m_tableLegs[i] = m_tableLegs[0];
	a = XM_PIDIV4;
	for (auto i = 0; i < 4; ++i, a += XM_PIDIV2)
		m_tableLegs[i].setWorldMatrix(XMMatrixTranslation(0.0f, 0.0f, TABLE_R - 0.35f) * XMMatrixRotationY(a) *
			XMMatrixTranslation(TABLE_POS.x, TABLE_POS.y - (TABLE_H + TABLE_TOP_H) / 2, TABLE_POS.z));
	m_tableSide = loader.GetCylinder(1, 16, TABLE_R, TABLE_TOP_H);
	m_tableSide.setWorldMatrix(XMMatrixRotationY(XM_PIDIV4/4) *
							   XMMatrixTranslation(TABLE_POS.x, TABLE_POS.y - TABLE_TOP_H / 2, TABLE_POS.z));
	m_tableTop = loader.GetDisc(16, TABLE_R);
	m_tableTop.setWorldMatrix(XMMatrixRotationY(XM_PIDIV4/4) *
							  XMMatrixTranslation(TABLE_POS.x, TABLE_POS.y, TABLE_POS.z));
	m_lightPosCB->Update(m_context, LIGHT_POS);
	m_textureCB->Update(m_context, XMMatrixScaling(0.25f, 0.25f, 1.0f) * XMMatrixTranslation(0.5f, 0.5f, 0.0f));
	
	m_posterTexCB->Update(m_context, XMMatrixScaling(0.25f, -0.25f, 1.0f) * XMMatrixTranslation(0.2f, 0.0f, 0.0f) *
									 XMMatrixRotationZ(XM_PIDIV2 / 9) * XMMatrixScaling(4.0f, 3.0f, 1.0f) *
									 XMMatrixTranslation(0.5f, 0.5f, 0.5f));
}

void Room::InitializeRenderStates()
{
	auto rsDesc = m_device.DefaultRasterizerDesc();
	rsDesc.CullMode = D3D11_CULL_FRONT;
	m_rsCullFront = m_device.CreateRasterizerState(rsDesc);

	rsDesc.CullMode = D3D11_CULL_BACK;
	m_rsCullBack = m_device.CreateRasterizerState(rsDesc);
	///////////////////MIRROR

	D3D11_DEPTH_STENCIL_DESC dssDesc = m_device.DefaultDepthStencilDesc();

	/***********************dssWRITE ***************/
	dssDesc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
	dssDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_REPLACE;
	dssDesc.StencilEnable = true;

	dssDesc.BackFace.StencilFunc = D3D11_COMPARISON_NEVER;

	dssDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;

	//Setup depth stencil state for writing
	m_dssWrite = m_device.CreateDepthStencilState(dssDesc);
	/********************** dssTest *********************/
	dssDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
	dssDesc.FrontFace.StencilFunc = D3D11_COMPARISON_EQUAL;
	dssDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;


	//Setup depth stencil state for testing
	m_dssTest = m_device.CreateDepthStencilState(dssDesc);

	//D3D11_RASTERIZER_DESC rsDesc = m_device.DefaultRasterizerDesc();
	rsDesc.FrontCounterClockwise = true;

	//Set rasterizer state front face to ccw
	m_rsCounterClockwise = m_device.CreateRasterizerState(rsDesc);

	/////////
	D3D11_BLEND_DESC bsDesc = m_device.DefaultBlendDesc();
	//Setup alpha blending  http://www.gamedev.net/topic/657801-directx-11-alpha-blending/
	bsDesc.RenderTarget[0].BlendEnable = true;
	bsDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
	bsDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
	bsDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	bsDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
	bsDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
	bsDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
	m_bsAlpha = m_device.CreateBlendState(bsDesc);

/*	auto dssDesc = m_device.DefaultDepthStencilDesc();
	dssDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;*/
	m_dssNoWrite = m_device.CreateDepthStencilState(dssDesc);
}

bool Room::LoadContent()
{
	InitializeConstantBuffers();
	InitializeCamera();
	InitializeTextures();
	InitializeRenderStates();
	CreateScene();
	m_phongEffect.reset(new PhongEffect(m_device, m_layout));
	m_phongEffect->SetProjMtxBuffer(m_projCB);
	m_phongEffect->SetViewMtxBuffer(m_viewCB);
	m_phongEffect->SetWorldMtxBuffer(m_worldCB);
	m_phongEffect->SetLightPosBuffer(m_lightPosCB);
	m_phongEffect->SetSurfaceColorBuffer(m_surfaceColorCB);

	m_textureEffect.reset(new TextureEffect(m_device, m_layout));
	m_textureEffect->SetProjMtxBuffer(m_projCB);
	m_textureEffect->SetViewMtxBuffer(m_viewCB);
	m_textureEffect->SetWorldMtxBuffer(m_worldCB);
	m_textureEffect->SetTextureMtxBuffer(m_textureCB);
	m_textureEffect->SetSamplerState(m_samplerWrap);
	m_textureEffect->SetTexture(m_wallTexture);

	m_colorTexEffect.reset(new ColorTexEffect(m_device, m_layout));
	m_colorTexEffect->SetProjMtxBuffer(m_projCB);
	m_colorTexEffect->SetViewMtxBuffer(m_viewCB);
	m_colorTexEffect->SetWorldMtxBuffer(m_worldCB);
	m_colorTexEffect->SetTextureMtxBuffer(m_textureCB);
	m_colorTexEffect->SetSamplerState(m_samplerWrap);
	m_colorTexEffect->SetTexture(m_perlinTexture);
	m_colorTexEffect->SetSurfaceColorBuffer(m_surfaceColorCB);

	m_multiTexEffect.reset(new MultiTexEffect(m_device, m_layout));
	m_multiTexEffect->SetProjMtxBuffer(m_projCB);
	m_multiTexEffect->SetViewMtxBuffer(m_viewCB);
	m_multiTexEffect->SetWorldMtxBuffer(m_worldCB);
	m_multiTexEffect->Set1stTextureMtxBuffer(m_textureCB);
	m_multiTexEffect->Set2ndTextureMtxBuffer(m_posterTexCB);
	m_multiTexEffect->SetSamplerState(m_samplerBorder);
	m_multiTexEffect->Set1stTexture(m_wallTexture);
	m_multiTexEffect->Set2ndTexture(m_posterTexture);

	m_environmentMapper.reset(new EnvironmentMapper(m_device, m_layout, m_context, 0.4f, 8.0f,
													XMFLOAT3(-1.3f, -0.74f, -0.6f)));
	m_environmentMapper->SetProjMtxBuffer(m_projCB);
	m_environmentMapper->SetViewMtxBuffer(m_viewCB);
	m_environmentMapper->SetWorldMtxBuffer(m_worldCB);
	m_environmentMapper->SetSamplerState(m_samplerWrap);
	m_environmentMapper->SetCameraPosBuffer(m_cameraPosCB);
	m_environmentMapper->SetSurfaceColorBuffer(m_surfaceColorCB);

	m_particles.reset(new ParticleSystem(m_device, m_context, XMFLOAT3(0.0f, 0.0f, 0.f)));
	m_particles->SetViewMtxBuffer(m_viewCB);
	m_particles->SetProjMtxBuffer(m_projCB);
	m_particles->SetSamplerState(m_samplerWrap);
	return true;
}

void Room::UnloadContent()
{

}

void Room::UpdateCamera(const XMMATRIX& view) const
{
	/*XMMATRIX view;
	m_camera.GetViewMatrix(view);*/
	m_viewCB->Update(m_context, view);
	m_cameraPosCB->Update(m_context, m_camera.GetPosition());
}

void Room::UpdateLamp(float dt)
{
	auto lamp = XMMatrixTranslation(0.0f, -0.4f, 0.0f)*XMMatrixTranslation(-1.0f, 2.0f, 0.0f) * XMMatrixTranslation(0.0f, 1.0f, 0.0f);
	m_lamp.setWorldMatrix(lamp);
}

void gk2::Room::UpdateRobot(float dt) //dt -> ile czasu up³yne³o
{
	static auto time = 0.0f;
	time += dt;
	float a1, a2, a3, a4, a5;
	VertexPosNormal newPos;

	float circleRadius = 1/2.0f;
	float x = circleRadius*cos(time*XM_2PI);
	float y = circleRadius*sin(time*XM_2PI);
	//transform from x, y, z=0 to metal plane
	XMFLOAT3 pos = XMFLOAT3(x, y, 0);
	XMFLOAT4 n4 = XMFLOAT4(pos.x, pos.y, pos.z, 1.0);
	XMStoreFloat3(&pos, XMVector3TransformCoord(XMLoadFloat4(&n4), transformMetal));
	//set position/normal of robot arm
	ParticleSystem * a = m_particles.get();
	a->SetEmitterPos(pos);

	XMStoreFloat3(&pos, XMVector3TransformCoord(XMLoadFloat4(&n4), /*XMMatrixTranslation(-0.5f, 1.0f, -0.5f)*/transformMetal));
	newPos.Pos = pos;
	
	//vector4 posit = TranslateMatrix44(-1.5, 0.25, 0) * RotateRadMatrix44(vector3(1, 1, 0), time) * vector4(0, 0, 0.5, 1);
	//newPos.Pos = XMFLOAT3(posit.x, posit.y, posit.z);
	newPos.Normal = XMFLOAT3(sqrt(3), 1, 0);
	inverse_kinematics(newPos, a1, a2, a3, a4, a5);

	XMMATRIX robot[6];
	//float l1 = .91f, l2 = .81f, l3 = .33f, dy = .27f, dz = .26f;
	//auto robot_startpos = XMMatrixTranslation(0.0f, -1.0f, 0.0f) *XMMatrixScaling(1.0, 1.0, -1.0);
	//robot[0] = robot_startpos;
	//robot[1] = XMMatrixRotationY(a1) * robot[0];
	//robot[2] = XMMatrixTranslation(0, -dy, 0) * XMMatrixRotationZ(a2) * XMMatrixTranslation(0, +dy, 0) * robot[1];
	//robot[3] = XMMatrixTranslation(l1, -dy, 0) * XMMatrixRotationZ(a3) * XMMatrixTranslation(-l1, dy, 0) * robot[2];
	//robot[4] = XMMatrixTranslation(l1 + l2, -dy, 0) * XMMatrixRotationZ(a5) * XMMatrixTranslation(-(l1 + l2), dy, 0) * robot[3];	
	//robot[5] = XMMatrixTranslation(0, -dy, dz) * XMMatrixRotationX(a4) * XMMatrixTranslation(0, dy, -dz) * robot[4];
	robot[0] = XMMatrixTranslation(0.0f, 0.0f, 0.0f);
	robot[1] = XMMatrixRotationY(a1) * robot[0];
	robot[2] = XMMatrixTranslation(0, -0.27, 0) * XMMatrixRotationZ(a2) * XMMatrixTranslation(0, 0.27, 0) * robot[1];
	robot[3] = XMMatrixTranslation(0.91f, -0.27f, 0.26f) * XMMatrixRotationZ(a3) * XMMatrixTranslation(-0.91f, 0.27f, -0.26f) * robot[2];
	robot[4] = XMMatrixTranslation(0, -0.27, 0.26f) * XMMatrixRotationX(a4) * XMMatrixTranslation(0, 0.27, -0.26f) * robot[3];
	robot[5] = XMMatrixTranslation(1.72f, -0.27f, 0) * XMMatrixRotationZ(a5) * XMMatrixTranslation(-1.72f, 0.27f, 0) * robot[4];
	for (auto i = 0; i < 6; i++) {
		m_robot[i].setWorldMatrix(robot[i]);
	}
}

void Room::Update(float dt)
{
	UpdateLamp(dt);
	UpdateRobot(dt);
	static MouseState prevState;
	MouseState currentState;
	if (m_mouse->GetState(currentState))
	{
		bool change = true;
		if (prevState.isButtonDown(0))
		{
			auto d = currentState.getMousePositionChange();
			m_camera.Rotate(d.y/300.f, d.x/300.f);
		}
		else if (prevState.isButtonDown(1))
		{
			auto d = currentState.getMousePositionChange();
			m_camera.Zoom(0, d.y/10.0f);
		}
		else
			change = false;
		prevState = currentState;
		if (change) {
			XMMATRIX view;
			m_camera.GetViewMatrix(view);
			UpdateCamera(view);
		}
	}
	
	KeyboardState keyboardState;
	if (m_keyboard->GetState(keyboardState)) {
		bool change = true;
		if (keyboardState.isKeyDown(DIK_W))
		{
			m_camera.Zoom(0, -dt*5.f);
		}
		if (keyboardState.isKeyDown(DIK_S))
		{
			m_camera.Zoom(0, dt*5.f);
		}
		if (keyboardState.isKeyDown(DIK_A))
		{
			m_camera.Zoom(dt*4.f,0);
		}
		if (keyboardState.isKeyDown(DIK_D))
		{
			m_camera.Zoom(-dt*4.f,0);
		}
		if (keyboardState.isKeyDown(DIK_Q))
		{
			m_camera.updateYPos(-dt*4.f);
		}
		if (keyboardState.isKeyDown(DIK_E))
		{
			m_camera.updateYPos(dt*4.f);
		}
		if (change) {
			XMMATRIX view;
			m_camera.GetViewMatrix(view);
			UpdateCamera(view);
		}
	}
	m_particles->Update(m_context, dt, m_camera.GetPosition());
}

void Room::DrawWalls(bool isMirror) const
{
	if (isMirror)
	{
		m_context->RSSetState(m_rsCounterClockwise.get());
	}
	//Draw floor
	m_textureCB->Update(m_context, XMMatrixScaling(0.25f, 4.0f, 1.0f) * XMMatrixTranslation(0.5f, 0.5f, 0.0f));
	m_textureEffect->SetTexture(m_woodTexture);
	m_textureEffect->Begin(m_context);
	m_worldCB->Update(m_context, m_walls[4].getWorldMatrix());
	m_walls[4].Render(m_context);
	m_textureEffect->End();

	//Draw ceiling
	m_surfaceColorCB->Update(m_context, XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f));
	m_textureCB->Update(m_context, XMMatrixScaling(0.25f, 0.25f, 0.25f) * XMMatrixTranslation(0.5f, 0.5f, 0.0f));
	m_colorTexEffect->Begin(m_context);
	m_worldCB->Update(m_context, m_walls[5].getWorldMatrix());
	m_walls[5].Render(m_context);
	m_colorTexEffect->End();
	m_surfaceColorCB->Update(m_context, XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f));

	//draw back wall
	m_multiTexEffect->Begin(m_context);
	m_worldCB->Update(m_context, m_walls[0].getWorldMatrix());
	m_walls[0].Render(m_context);
	m_multiTexEffect->End();

	//draw remaining walls
	m_textureEffect->SetTexture(m_wallTexture);
	m_textureEffect->Begin(m_context);
	for (auto i = 1; i < 4; ++i)

	{
		m_worldCB->Update(m_context, m_walls[i].getWorldMatrix());
		m_walls[i].Render(m_context);
	}
	m_textureEffect->End();
	if (isMirror)
	{
		m_context->RSSetState(nullptr);
	}
}

void Room::DrawTeapot() const
{
	//TODO: Replace with environment map effect
	m_phongEffect->Begin(m_context);
	m_surfaceColorCB->Update(m_context, XMFLOAT4(0.8f, 0.7f, 0.65f, 1.0f));

	m_worldCB->Update(m_context, m_teapot.getWorldMatrix());
	m_teapot.Render(m_context);
	/*m_worldCB->Update(m_context, m_debugSphere.getWorldMatrix());
	m_debugSphere.Render(m_context);*/

	m_surfaceColorCB->Update(m_context, XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f));
	m_phongEffect->End();
}

void Room::DrawTableElement(Mesh& element) const
{
	m_worldCB->Update(m_context, element.getWorldMatrix());
	m_context->RSSetState(m_rsCullFront.get());
	element.Render(m_context);
	m_context->RSSetState(nullptr);
	element.Render(m_context);
}

void Room::DrawTableLegs(XMVECTOR camVec)
{
	XMFLOAT4 v(1.0f, 0.0f, 0.0f, 0.0f);
	auto plane1 = XMLoadFloat4(&v);
	v = XMFLOAT4(0.0f, 0.0f, 1.0f, 0.0f);
	auto plane2 = XMLoadFloat4(&v);
	auto left = XMVector3Dot(camVec, plane1).m128_f32[0] > 0;
	auto back = XMVector3Dot(camVec, plane2).m128_f32[0] > 0;
	if (left)
	{
		if (back)
		{
			DrawTableElement(m_tableLegs[2]);
			DrawTableElement(m_tableLegs[3]);
			DrawTableElement(m_tableLegs[1]);
			DrawTableElement(m_tableLegs[0]);
		}
		else
		{
			DrawTableElement(m_tableLegs[3]);
			DrawTableElement(m_tableLegs[2]);
			DrawTableElement(m_tableLegs[0]);
			DrawTableElement(m_tableLegs[1]);
		}
	}
	else
	{

		if (back)
		{
			DrawTableElement(m_tableLegs[1]);
			DrawTableElement(m_tableLegs[0]);
			DrawTableElement(m_tableLegs[2]);
			DrawTableElement(m_tableLegs[3]);
		}
		else
		{
			DrawTableElement(m_tableLegs[0]);
			DrawTableElement(m_tableLegs[1]);
			DrawTableElement(m_tableLegs[3]);
			DrawTableElement(m_tableLegs[2]);
		}
	}
}

void Room::DrawParticles(bool isMirror)
{/*
	m_context->OMSetBlendState(m_bsAlpha.get(), nullptr, BS_MASK);
	m_context->OMSetDepthStencilState(m_dssNoWrite.get(), 0);

	m_particles->Render(m_context);*/
/*	m_surfaceColorCB->Update(m_context, XMFLOAT4(0.1f, 0.1f, 0.1f, 0.9f));
	auto v = m_camera.GetPosition();
	auto camVec = XMVector3Normalize(XMLoadFloat4(&v) - XMLoadFloat4(&TABLE_POS));
	v = XMFLOAT4(0.0f, 1.0f, 0.0f, 0.0f);
	auto plane = XMLoadFloat4(&v);
	if (XMVector3Dot(camVec, plane).m128_f32[0] > 0)
	{
		m_phongEffect->Begin(m_context);
		m_context->RSSetState(m_rsCullFront.get());
		m_worldCB->Update(m_context, m_tableSide.getWorldMatrix());
		//m_tableSide.Render(m_context);
		m_context->RSSetState(nullptr);
		//DrawTableLegs(camVec);
		m_worldCB->Update(m_context, m_tableSide.getWorldMatrix());
		//m_tableSide.Render(m_context);
		//DrawTableElement(m_tableTop);
		m_phongEffect->End();
		m_particles->Render(m_context);
	}
	else
	{
		m_particles->Render(m_context);
		m_phongEffect->Begin(m_context);
		//DrawTableElement(m_tableTop);
		m_context->RSSetState(m_rsCullFront.get());
		m_worldCB->Update(m_context, m_tableSide.getWorldMatrix());
		//m_tableSide.Render(m_context);
		m_context->RSSetState(nullptr);
		//DrawTableLegs(camVec);
		m_worldCB->Update(m_context, m_tableSide.getWorldMatrix());
		//m_tableSide.Render(m_context);
		m_phongEffect->End();
	}
	*/ /*
	m_surfaceColorCB->Update(m_context, XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f));
	m_context->OMSetDepthStencilState(nullptr, 0);
	m_context->OMSetBlendState(nullptr, nullptr, BS_MASK);*/
	m_context->OMSetBlendState(m_bsAlpha.get(), nullptr, BS_MASK);
	//m_context->OMSetDepthStencilState(m_dssNoWrite.get(), 0);
	m_particles->Render(m_context);
	//m_context->OMSetDepthStencilState(nullptr, 0);
	m_context->OMSetBlendState(nullptr, nullptr, BS_MASK);
}

void gk2::Room::DrawRobot(bool isMirror)
{
	//TODO: Replace with environment map effect
	m_phongEffect->Begin(m_context);
	m_surfaceColorCB->Update(m_context, XMFLOAT4(0.8f, 0.7f, 0.65f, 1.0f));
	if (isMirror)
		m_context->RSSetState(m_rsCullBack.get());

	for (auto i = 0; i < 6; i++) 
	{
		m_worldCB->Update(m_context, m_robot[i].getWorldMatrix());
		m_robot[i].Render(m_context);
	}
	if (isMirror)
		m_context->RSSetState(nullptr);
	m_surfaceColorCB->Update(m_context, XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f));
	m_phongEffect->End();
}

void gk2::Room::DrawMetal()
{
		m_context->OMSetBlendState(m_bsAlpha.get(), nullptr, BS_MASK);
		m_surfaceColorCB->Update(m_context, XMFLOAT4(0.8, 0.85, 0.8, 0.8));
		m_worldCB->Update(m_context, m_metal.getWorldMatrix());
		m_metal.Render(m_context);
		m_context->OMSetBlendState(nullptr, nullptr, BS_MASK);

}

void gk2::Room::DrawMirroredWorld() 
{
	m_phongEffect->Begin(m_context);

	//substitute with draw metal 
	m_context->OMSetDepthStencilState(m_dssWrite.get(), 1);
	
	m_worldCB->Update(m_context, m_metal.getWorldMatrix());
	m_metal.Render(m_context);
	
	m_context->OMSetDepthStencilState(m_dssTest.get(),1);
	
	m_context->RSSetState(m_rsCounterClockwise.get());
	XMMATRIX viewMatrix = m_mirrorMtx * m_camera.GetViewMatrix();
	UpdateCamera(viewMatrix);

	DrawMetal();	
	DrawWalls();	
	DrawRobot();
	m_worldCB->Update(m_context, m_lamp.getWorldMatrix());
	m_lamp.Render(m_context);
	
	m_context->RSSetState(nullptr);
	DrawParticles(true);
	m_phongEffect->End();
	m_context->OMSetDepthStencilState(nullptr, 0);
	UpdateCamera(m_camera.GetViewMatrix());
}

void gk2::Room::inverse_kinematics(VertexPosNormal robotPosition, float & a1, float & a2, float & a3, float & a4, float & a5)
{
	float l1 = .91f, l2 = .81f, l3 = .33f, dy = .27f, dz = .26f;
	//normalize Normal
	auto normalized = XMLoadFloat3(&robotPosition.Normal);
	normalized = XMVector3Normalize(normalized);
	XMStoreFloat3(&robotPosition.Normal, normalized);

	//calculations
	auto l3normal = robotPosition.Normal;
	l3normal.x *= l3;
	l3normal.y *= l3;
	l3normal.z *= l3;
	auto pos1 = XMFLOAT3(robotPosition.Pos);
	pos1.x += l3normal.x;
	pos1.y += l3normal.y;
	pos1.z += l3normal.z;

	float e = sqrtf(pos1.z*pos1.z + pos1.x*pos1.x - dz*dz);
	a1 = atan2(pos1.z, -pos1.x) + atan2(dz, e);

	auto pos2 = XMFLOAT3(e, pos1.y - dy, .0f);
	a3 = -acosf(min(1.0f, (pos2.x*pos2.x + pos2.y*pos2.y - l1*l1 - l2*l2) / (2.0f*l1*l2)));

	float k = l1 + l2 * cosf(a3), l = l2 * sinf(a3);
	a2 = -atan2(pos2.y, sqrtf(pos2.x*pos2.x + pos2.z*pos2.z)) - atan2(l, k);
	
	XMFLOAT3 normal1;
	XMStoreFloat3(&normal1, XMVector3TransformNormal(XMLoadFloat3(&robotPosition.Normal), XMMatrixRotationY(-a1)));
	XMStoreFloat3(&normal1, XMVector3TransformNormal(XMLoadFloat3(&normal1), XMMatrixRotationZ(-(a2 + a3))));

	a5 = acosf(normal1.x);
	a4 = atan2(normal1.z, normal1.y);
}

void Room::DrawScene()
{
	DrawMirroredWorld();
	m_phongEffect->Begin(m_context);
	
	DrawMetal();
	m_worldCB->Update(m_context, m_lamp.getWorldMatrix());
	m_lamp.Render(m_context);
	DrawWalls();
	DrawRobot();
	

	m_phongEffect->End();

	DrawParticles(false);
}

void Room::Render()
{
	if (m_context == nullptr)
		return;

	//TODO: Render scene to each environment cube map face

	ResetRenderTarget();
	m_projCB->Update(m_context, m_projMtx);
	UpdateCamera(m_camera.GetViewMatrix());
	//Clear buffers
	float clearColor[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
	m_context->ClearRenderTargetView(m_backBuffer.get(), clearColor);
	m_context->ClearDepthStencilView(m_depthStencilView.get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
	DrawScene();
	m_swapChain->Present(0, 0);
}
