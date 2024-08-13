#pragma once

#include "PortalWorld.h"
#include <glm/glm.hpp>
#include "GameBase.h"
#include "Leveler.h"
#include "BlueField.h"

class ForceField
{
public:
	static std::vector<ForceField*> fields;

	int fieldRenderOffset;

	ForceField(PortalWorld* portalWorld, s_objectDataGrid* objData);

	static void staticInit();
	static void memMapFunc(void* ptr);
	static void render(VkCommandBuffer cmdBuff, void* ptr);
};