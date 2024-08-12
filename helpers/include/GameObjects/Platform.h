#pragma once

#include "PortalWorld.h"
#include <glm/glm.hpp>
#include "GameBase.h"
#include "Leveler.h"
#include "PlatformRectangle.h"

const float platformSpeed = 1.5f;

class Platform
{
public:
	static int numPlatforms;
	static Platform* platforms[MAX_PLATFORMS];

	b2Body* body;
	glm::ivec2 size;
	b2Vec2 basePos;
	b2Vec2 fwdDir;
	float limitDist;

	int platformRenderOffset;
	int edgeRenderIndex;

	Platform(PortalWorld* portalWorld, s_objectDataGrid* objData);
	void setMovementData(s_objectDataGrid* objData);

	static void staticInit();
	static void update();
	static void memMapFunc(void* ptr);
	static void render(VkCommandBuffer cmdBuff, void* ptr);
};