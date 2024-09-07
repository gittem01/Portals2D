#pragma once

#include "PortalWorld.h"
#include <glm/glm.hpp>
#include <bitset>
#include "GameBase.h"
#include "Leveler.h"
#include "GameObjects/WallLaserReceiver.h"
#include "GameObjects/GndLaserReceiver.h"
#include "GameObjects/Receiver.h"

typedef struct
{
	b2Vec2 startPos;
	b2Vec2 endPos;
} s_simpleRay;

class LaserEmitter : public Receiver
{
public:
	static std::vector<LaserEmitter> laserEmitters;
	static int numLaserEmitters;
	static s_perTextureData* ptData;
	static s_texture* tex;

	std::vector<s_simpleRay> laserRays;
	std::set<pbWorldData*> collidedLaserCubes;
	std::bitset<MAX_GND_LASER_RECEIVERS> gndLaserMasks;
	std::bitset<MAX_GND_LASER_RECEIVERS> enableSet;
	WallLaserReceiver* lastWallReceiver;
	b2Vec2 laserDir;
	b2Vec2 laserPos;
	b2Vec2 endPos;
	float endAngle;
	bool lastLaserCube;

	LaserEmitter(PortalWorld* portalWorld, s_objectDataGrid* objData);
	LaserEmitter() {};

	void ssignal();
	void usignal();

	void handleGroundReceivers();

	static void createLaserEmitter(PortalWorld* portalWorld, s_objectDataGrid* objData);
	static void staticInit();
	static void update();
	static void render(VkCommandBuffer cmdBuff, void* ptr);
};