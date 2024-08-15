#pragma once

#include "PortalWorld.h"
#include <glm/glm.hpp>
#include "GameBase.h"
#include "Leveler.h"
#include "BlueField.h"
#include <queue>

class ForceField
{
public:
	static std::vector<ForceField*> fields;
	static ALuint emancipationBuffer;
	static s_perTextureData* ptData;
	static s_texture* tex;
	static std::queue<pbWorldData*> destroyCList;


	int fieldRenderOffset;

	ForceField(PortalWorld* portalWorld, s_objectDataGrid* objData);

	static void staticInit();
	static void update();
	static void render(VkCommandBuffer cmdBuff, void* ptr);
};