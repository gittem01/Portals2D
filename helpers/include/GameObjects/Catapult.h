#pragma once

#include "PortalWorld.h"
#include <glm/glm.hpp>
#include "GameBase.h"
#include "Leveler.h"
#include <set>

class Catapult
{
public:
	static std::vector<Catapult*> catapults;
	static std::set<DefaultObject*> hitObjects;
	static s_texture* headTex;
	static s_texture* bodyTex;
	static ALuint buffer;

	s_renderBodyPsh headRenderData;
	s_renderBodyPsh bodyRenderData;
	b2Vec2 sendVelocity;
	float sendSpeed;
	b2Body* body;

	double lastHitTime;

	b2Vec2 rotationOrigin;
	float rotationDir;
	float baseRotation;
	b2Vec2 moveDir;
	b2Vec2 basePos;

	s_renderBodyPsh catapultRenderData;

	Catapult(PortalWorld* portalWorld, s_objectDataGrid* objData);

	void sendIt(DefaultObject* dObj, b2Body* body, float mult);

	static void toDeath();
	static void toLeveler();
	static void staticInit();
	static void update();
	static void render(VkCommandBuffer cmdBuff, void* ptr);
};