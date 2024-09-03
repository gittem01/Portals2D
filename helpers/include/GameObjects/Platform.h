#pragma once

#include "PortalWorld.h"
#include <glm/glm.hpp>
#include "GameBase.h"
#include "Leveler.h"
#include "PlatformRectangle.h"
#include "GameObjects/Receiver.h"

const float platformSpeed = 2.0f;

class Platform : public Receiver
{
public:
	static int numPlatforms;
	static Platform* platforms[MAX_PLATFORMS];

	b2Body* body;
	glm::ivec2 size;
	b2Vec2 basePos;
	b2Vec2 fwdDir;
	float dirMult;
	float limitDist;

	int platformRenderOffset;
	int edgeRenderIndex;
	bool startEnabled;

	Platform(PortalWorld* portalWorld, s_objectDataGrid* objData);
	void setMovementData(s_objectDataGrid* objData);

	void ssignal();
	void usignal();

	static void toDeath();
	static void toLeveler();
	static void staticInit();
	static void update();
	static void memMapFunc(void* ptr);
	static void render(VkCommandBuffer cmdBuff, void* ptr);
};