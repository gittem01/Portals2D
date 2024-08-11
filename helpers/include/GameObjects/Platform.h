#pragma once

#include "PortalWorld.h"
#include <glm/glm.hpp>
#include "GameBase.h"
#include "Leveler.h"
#include "PlatformRectangle.h"

class Platform
{
public:
	static int numPlatforms;
	static Platform* platforms[MAX_PLATFORMS];

	b2Body* body;
	glm::ivec2 size;
	int platformRenderOffset;
	int edgeRenderIndex;

	Platform(PortalWorld* portalWorld, s_objectDataGrid* objData);

	static void staticInit();
	static void update();
	static void render(VkCommandBuffer cmdBuff, void* ptr);
};