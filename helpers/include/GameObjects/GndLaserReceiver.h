#pragma once

#include "PortalWorld.h"
#include <glm/glm.hpp>
#include "GameBase.h"
#include "Leveler.h"
#include "BlueField.h"
#include <list>
#include "GameObjects/Transmitter.h"

#define MAX_GND_LASER_RECEIVERS 64

class LaserEmitter;

class GndLaserReceiver : public Transmitter
{
public:
	static std::vector<GndLaserReceiver> gndLaserReceivers;
	static std::list<GndLaserReceiver*> disableChecks;
	static std::list<GndLaserReceiver*> enableChecks;
	static s_perTextureData* ptData;
	static s_texture* tex;
	static int numGndLaserReceivers;

	std::set<LaserEmitter*> enablers;
	b2Body* body;
	double disableTime;
	double enableTime;
	int arrIndex;

	GndLaserReceiver(PortalWorld* portalWorld, s_objectDataGrid* objData);
	GndLaserReceiver() {};

	void enable(LaserEmitter* le);
	void disable(LaserEmitter* le);

	static GndLaserReceiver* create(PortalWorld* portalWorld, s_objectDataGrid* objData);
	static void staticInit();
	static void update();
	static void render(VkCommandBuffer cmdBuff, void* ptr);
};