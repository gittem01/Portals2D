#pragma once

#include <DefaultObject.h>
#include <vulkan/vulkan.h>
#include <ShaderBase.h>

const float shootTimer = 0.2f;

class Turret : public DefaultObject
{
public:
	float midX;
	uint32_t shootGundex = 0;
	double lastShootTime = -10.0;
	float gunBacks[2] = { 0.0f, 0.0f };
	float openAmount = 0.0f;
	float rotation = 0.0f;
	s_texture* coverTex;
	s_texture* gunTex;

	s_buffers* coverVertexBuffer;

	uint32_t bodyFixDataIndex;

	Turret(PortalWorld* portalWorld);

	void shoot();
	void createBody();
	void createRenderings();

	static void draw(VkCommandBuffer cmdBuff, void* ptr);
};