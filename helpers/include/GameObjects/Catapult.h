#pragma once

#include "PortalWorld.h"
#include <glm/glm.hpp>
#include "GameBase.h"
#include "Leveler.h"

class Catapult
{
public:
	static std::vector<Catapult*> movingCatapults;
	static s_texture* headTex;
	static s_texture* bodyTex;
	static s_perTextureData* ptData;

	s_renderBodyPsh headRenderData;
	s_renderBodyPsh bodyRenderData;
	b2Vec2 sendSpeed;
	b2Body* body;
	double contactReportTime;

	Catapult(PortalWorld* portalWorld, s_objectDataGrid* objData);

	static void toDeath();
	static void toLeveler();
	static void staticInit();
	static void update();
	static void render(VkCommandBuffer cmdBuff, void* ptr);
};