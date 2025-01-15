#pragma once

#include "PortalWorld.h"
#include <glm/glm.hpp>
#include "GameBase.h"
#include "Leveler.h"
#include "GameObjects/Receiver.h"
#include <set>

class Dropper : public Receiver
{
public:
	static std::vector<Dropper> droppers;
	static std::set<Dropper*> createList;
	static s_perTextureData* ptData;
	static s_texture* tex;

	DefaultObject* linkedObject;
	b2Vec2 dropCreationPos;
	bool startActive;
	bool autoRespawn;

	Dropper(PortalWorld* portalWorld, s_objectDataGrid* objData);
	Dropper() {};
	~Dropper();

	void enable();
	void disable();


	static Dropper* createDropper(PortalWorld* portalWorld, s_objectDataGrid* objData);
	static void destrFunc(PortalBody* pb, void* ptr);
	static void staticInit();
	static void toDeath();
	static void toLeveler();
	static void update();
	static void render(VkCommandBuffer cmdBuff, void* ptr);
};