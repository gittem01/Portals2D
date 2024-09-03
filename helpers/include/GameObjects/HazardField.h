#pragma once

#include "PortalWorld.h"
#include <glm/glm.hpp>
#include "GameBase.h"
#include "Leveler.h"
#include "HazardShader.h"
#include "GameObjects/Receiver.h"

class HazardField : public Receiver
{
public:
	static std::vector<HazardField*> fields;
	static ALuint hazardBuffer;
	static s_perTextureData* ptData;
	static s_texture* tex;

	b2Body* body;
	int fieldRenderOffset;
	bool startEnabled;

	HazardField(PortalWorld* portalWorld, s_objectDataGrid* objData);

	void ssignal();
	void usignal();

	static void toDeath();
	static void toLeveler();
	static void staticInit();
	static void update();
	static void render(VkCommandBuffer cmdBuff, void* ptr);
};