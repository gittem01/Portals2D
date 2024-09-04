#pragma once

#include "PortalWorld.h"
#include <glm/glm.hpp>
#include "GameBase.h"
#include "Leveler.h"
#include "GameObjects/Transmitter.h"

class CircleButton : public Transmitter
{
public:
	static std::vector<CircleButton*> circleButtons;
	static s_texture* bodyTex;

	s_renderBodyPsh bodyRenderData;
	double contactReportTime;
	b2Body* body;
	b2Vec2 rightDir;
	Sphere* lastEnabledSphere;

	CircleButton(PortalWorld* portalWorld, s_objectDataGrid* objData);

	static void toDeath();
	static void toLeveler();
	static void staticInit();
	static void update(double dt);
	static void render(VkCommandBuffer cmdBuff, void* ptr);
};