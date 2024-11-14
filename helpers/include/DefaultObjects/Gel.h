#pragma once

#include <DefaultObject.h>
#include <SplashShader.h>

#define GEL_COLLISION_IGNORE (F_FEDGE | F_GELLER)

class Gel : public DefaultObject
{
	friend class Geller;

private:
	int gelIndex;
	int gelType;
	glm::vec4 gelColor;
	float gelRadius;

	Gel* nearGels[16];
	int numNearGels = 0;
	
	static std::vector<splashData> splashEffects;

public:
	static std::vector<Gel*> gels;
	static std::vector<Gel> gelPool;
	static std::vector<Gel*> freeGels;
	static int gelCount;
	static int splashCount;

	Gel(PortalWorld* portalWorld, glm::vec2 pos, glm::vec2 velocity, float radius, int gelType);
	Gel() {}

	static void getRangeData(b2Vec2 thisPos, b2Fixture* fix, int range[2], s_edgeGlobalData** data);
	void splash(b2Contact* contact, b2Fixture* otherFix);
	void recreate(bool mirrored=false) override;

	static void toDeath();
	static void toLeveler();
	static void staticInit();
	static s_edgeGlobalData* getEdgeData(fixtureData* fData);
	static bool bounceAt(b2Body* body, b2Fixture* edgeFix, b2Vec2 pos);
	static void generateGel(glm::vec2 pos, glm::vec2 velocity, float radius, int colorIndex);
	static void update();
	static void perFrameUpdate();
	static void render(VkCommandBuffer cmdBuff, void* ptr);
	void endContact(b2Contact* contact, b2Fixture* otherFix);
	void beginContact(b2Contact* contact, b2Fixture* otherFix, b2Fixture* thisFix);
};