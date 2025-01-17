#pragma once

#include "PortalWorld.h"
#include "GameBase.h"
#include "Leveler.h"

class EndCapsule
{
public:
	static EndCapsule* object;

	s_texture* tex;
	b2Vec2 pos;
	fixtureData fData;

	EndCapsule(PortalWorld* portalWorld, b2Vec2 pos);
	~EndCapsule();

	static void render(VkCommandBuffer cmdBuff, void* ptr);
};