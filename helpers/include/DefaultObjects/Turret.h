#pragma once

#include <DefaultObject.h>
#include <vulkan/vulkan.h>
#include <ShaderBase.h>
#include <LaserShader.h>

const float shootTimer = 0.06f;
const float fullBackInTime = shootTimer;

class Turret : public DefaultObject
{
public:
	float midX;
	uint32_t shootGundex = 0;
	double lastShootTime = -10.0;
	double shootTimes[2] = { -10.0, -10.0 };
	float gunBacks[2] = { 0.0f, 0.0f };
	float openAmount = 0.0f;
	float rotation = 0.0f;

	s_texture* coverTex;
	s_texture* gunTex;
	s_texture* glowTex;

	s_buffers* coverVertexBuffer;
	uint32_t bodyFixDataIndex;

	std::vector<uint32_t> soundBuffers;

	std::vector<std::pair<s_laserUbo, uint32_t>> renderLaserDatas;

	Turret(PortalWorld* portalWorld);

	void shoot();
	void createBody();
	void createRenderings();

	static void draw(VkCommandBuffer cmdBuff, void* ptr);
};