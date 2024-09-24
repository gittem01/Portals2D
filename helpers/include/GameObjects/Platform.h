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
	static std::vector<Platform> platforms;
	static std::vector<uint8_t> loopingPlatforms;
	static std::vector<uint8_t> oneTimeMovePlatforms;

	b2Body* body;
	glm::ivec2 size;
	b2Vec2 basePos;
	b2Vec2 fwdDir;
	float dirMult;
	float limitDist;

	int platformRenderOffset;
	int edgeRenderIndex;
	uint8_t platformIndex;
	bool looping;

	Platform(PortalWorld* portalWorld, s_objectDataGrid* objData);
	Platform() {}

	void setMovementData(s_objectDataGrid* objData);

	void enable();
	void disable();

	static Platform* createPlatform(PortalWorld* portalWorld, s_objectDataGrid* objData);
	static void toDeath();
	static void toLeveler();
	static void staticInit();
	static void perTickUpdate(float dt);
	static void memMapFunc(void* ptr);
	static void render(VkCommandBuffer cmdBuff, void* ptr);
};