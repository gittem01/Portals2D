#pragma once

#include "PortalWorld.h"
#include <glm/glm.hpp>
#include "GameBase.h"
#include "GameObjects/Transmitter.h"
#include <list>

class UseButton : public Transmitter
{
public:
	static s_perTextureData* ptData;
	static s_texture* tex;
	static std::list<UseButton*> checkButtons;

	double signalTime;
	float disableAfter;

	UseButton(PortalWorld* portalWorld, s_objectDataGrid* objData);

	void enable();

	static void toDeath();
	static void toLeveler();
	static void staticInit();
	static void update();
	static void render(VkCommandBuffer cmdBuff, void* ptr);
};