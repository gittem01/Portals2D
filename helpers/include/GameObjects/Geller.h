#pragma once

#include "PortalWorld.h"
#include <glm/glm.hpp>
#include "GameBase.h"
#include "Leveler.h"

class Geller
{
public:
	static std::vector<Geller> gellers;
	static s_perTextureData* ptData[4];
	static s_texture* tex[4];

	glm::vec2 gelCreationPos;
	double lastGelCreationTime;
	int gellerIndex;
	char gellerRotation;
	bool enabled;

	Geller(PortalWorld* portalWorld, s_objectDataGrid* objData);
	~Geller();

	static void createGeller(PortalWorld* portalWorld, s_objectDataGrid* objData);
	static void staticInit();
	static void update();
	static void render(VkCommandBuffer cmdBuff, void* ptr);
};