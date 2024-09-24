#pragma once

#include "PortalWorld.h"
#include <glm/glm.hpp>
#include "GameBase.h"
#include "Leveler.h"
#include "GameObjects/Receiver.h"

class Geller : public Receiver
{
public:
	static std::vector<Geller> gellers;
	static int numGellers;
	static s_perTextureData* ptData[4];
	static s_texture* tex[4];

	glm::vec2 gelCreationPos;
	double lastGelCreationTime;
	int gellerIndex;
	char gellerRotation;

	Geller(PortalWorld* portalWorld, s_objectDataGrid* objData);
	Geller() {};
	~Geller();

	void ssignal();
	void usignal();

	static void createGeller(PortalWorld* portalWorld, s_objectDataGrid* objData);
	static void staticInit();
	static void update();
	static void render(VkCommandBuffer cmdBuff, void* ptr);
};