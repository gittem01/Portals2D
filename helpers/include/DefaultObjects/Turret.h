#pragma once

#include <DefaultObject.h>
#include <vulkan/vulkan.h>
#include <ShaderBase.h>

class Turret : public DefaultObject
{
public:
	float midY;
	s_texture* coverTex;
	s_texture* gunTex;

	s_buffers* coverVertexBuffer;

	uint32_t bodyFixDataIndex;

	Turret(PortalWorld* portalWorld);

	void createBody();
	void createRenderings();

	static void draw(VkCommandBuffer cmdBuff, void* ptr);
};