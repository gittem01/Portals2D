#pragma once

#include "PortalWorld.h"
#include <glm/glm.hpp>
#include "GameBase.h"
#include "Leveler.h"
#include "HazardShader.h"

class HazardField
{
public:
	static std::vector<HazardField*> fields;
	static ALuint hazardBuffer;
	static s_perTextureData* ptData;
	static s_texture* tex;

	int fieldRenderOffset;

	HazardField(PortalWorld* portalWorld, s_objectDataGrid* objData);

	static void staticInit();
	static void update();
	static void render(VkCommandBuffer cmdBuff, void* ptr);
};