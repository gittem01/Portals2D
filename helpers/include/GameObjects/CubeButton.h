#pragma once

#include "PortalWorld.h"
#include <glm/glm.hpp>
#include "GameBase.h"
#include "Leveler.h"
#include "GameObjects/Transmitter.h"

class CubeButton : public Transmitter
{
public:
	static std::vector<CubeButton*> cubeButtons;
	static s_texture* buttonTex;
	static s_texture* bodyTex;

	s_renderBodyPsh buttonRenderData;
	s_renderBodyPsh bodyRenderData;
	b2Vec2 fwdDir;
	b2Vec2 initialPos;
	b2Body* button;
	bodyData bData;
	double contactReportTime;

	CubeButton(PortalWorld* portalWorld, s_objectDataGrid* objData);

	void enable();

	static void toDeath();
	static void toLeveler();
	static void staticInit();
	static void update();
	static void render(VkCommandBuffer cmdBuff, void* ptr);
};