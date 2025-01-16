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
	static std::set<Dropper*> pushList;

	static s_perTextureData* ptData;
	static s_texture* tex;

	DefaultObject* linkedObject;
	b2Body* pushBody;
	b2Vec2 dropCreationPos;
	float pushBegin;
	float pushEnd;
	float createAddition;
	bool startActive;
	bool autoRespawn;

	Dropper(PortalWorld* portalWorld, s_objectDataGrid* objData);
	Dropper() {};
	~Dropper();

	void enable();
	void disable();
	void pushStart();

	static Dropper* createDropper(PortalWorld* portalWorld, s_objectDataGrid* objData);
	static void destrFunc(PortalBody* pb, void* ptr);
	static void staticInit();
	static void toDeath();
	static void toLeveler();
	static void update();
	static void render(VkCommandBuffer cmdBuff, void* ptr);
};