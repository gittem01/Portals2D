#pragma once

#include "PortalWorld.h"
#include <glm/glm.hpp>
#include "GameBase.h"
#include "Leveler.h"
#include "BlueField.h"
#include <list>
#include "GameObjects/Transmitter.h"

class LaserEmitter;

class WallLaserReceiver : public Transmitter
{
public:
	static std::vector<WallLaserReceiver> wallLaserReceivers;
	static int numWallLaserReceivers;
	static std::list<WallLaserReceiver*> disableChecks;
	static std::list<WallLaserReceiver*> enableChecks;
	static s_perTextureData* ptData;
	static s_texture* tex;

	std::set<LaserEmitter*> enablers;
	b2Body* body;
	double disableTime;
	double enableTime;

	WallLaserReceiver(PortalWorld* portalWorld, s_objectDataGrid* objData);
	WallLaserReceiver() { };

	void enable(LaserEmitter* le);
	void disable(LaserEmitter* le);

	static void toDeath();
	static WallLaserReceiver* createWallLaserReceiver(PortalWorld* portalWorld, s_objectDataGrid* objData);
	static void staticInit();
	static void update();
	static void render(VkCommandBuffer cmdBuff, void* ptr);
};