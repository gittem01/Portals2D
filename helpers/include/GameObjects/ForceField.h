#pragma once

#include "PortalWorld.h"
#include <glm/glm.hpp>
#include "GameBase.h"
#include "Leveler.h"
#include "BlueField.h"
#include <queue>
#include "GameObjects/Receiver.h"

class ForceField : public Receiver
{
public:
	static std::vector<ForceField*> fields;
	static ALuint emancipationBuffer;
	static s_perTextureData* ptData;
	static s_texture* tex;
	static std::queue<pbWorldData*> destroyCList;

	b2Body* body;
	int fieldRenderOffset;
	bool startEnabled;

	ForceField(PortalWorld* portalWorld, s_objectDataGrid* objData);
	~ForceField();

	void enable();
	void disable();

	static void toDeath();
	static void toLeveler();
	static void staticInit();
	static void update();
	static void render(VkCommandBuffer cmdBuff, void* ptr);
};