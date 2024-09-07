#pragma once

#include "PortalWorld.h"
#include <glm/glm.hpp>
#include <bitset>
#include "GameBase.h"
#include "Leveler.h"
#include "GameObjects/Receiver.h"

class Funnel : public Receiver
{
public:
	static std::vector<Funnel*> funnels;
	static s_perTextureData* ptData;
	static s_texture* tex;

	b2Vec2 funnelDir;
	b2Vec2 funnelPos;
	b2Vec2 rayPositions[2];

	Funnel(PortalWorld* portalWorld, s_objectDataGrid* objData);

	void ssignal();
	void usignal();

	static void staticInit();
	static void update();
	static void render(VkCommandBuffer cmdBuff, void* ptr);
};