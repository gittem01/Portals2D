#pragma once

#include "PortalWorld.h"
#include <glm/glm.hpp>
#include "GameBase.h"
#include "Leveler.h"
#include "LightBridge.h"
#include "GameObjects/Receiver.h"

class LightRay;

typedef struct
{
	b2Vec2 p0;
	b2Vec2 p1;
} s_lightRay;

typedef struct
{
	b2Vec2 p1v1;
	b2Vec2 p1v2;
	b2Vec2 p2v1;
	b2Vec2 p2v2;
} portalFrames;

class LightBridge : public Receiver
{
public:
	static std::vector<LightBridge> lightBridges;
	static std::vector<LightBridge*> perTickChecks;
	static int numLightBridges;
	static s_perTextureData* ptData;
	static s_texture* tex;
	static LightRay lightRay;

	s_bridgeUbo data;
	std::vector<s_lightRay> renderLightPositions;
	std::vector<b2Body*> bodies;
	portalFrames frames;
	s_edgeGlobalData eDatas[2];
	b2Body* bridgeBody;
	b2Body* capBody;
	b2Body* endBody;
	b2Body* outBody;
	b2Vec2 lightDir;
	b2Vec2 lightPos;
	b2Vec2 lightLasers[2];
	b2Vec2 endPos;
	float lastLen;
	float maxSend;
	float totalSize;
	int reqMapCount;
	int renderOffset;
	int extraRenderIndex;
	int wallRenderIndex;
	int wallGelStartOffset1;
	int wallGelStartOffset2;

	LightBridge(PortalWorld* portalWorld, s_objectDataGrid* objData);
	LightBridge() {};

	void ssignal();
	void usignal();

	static void toDeath();
	static void toLeveler();
	static LightBridge* createLightBridge(PortalWorld* portalWorld, s_objectDataGrid* objData);
	static void staticInit();
	static void perTickUpdate();
	static void update();
	static void memMap(void* ptr);
	static void render(VkCommandBuffer cmdBuff, void* ptr);
};

class LightRay : public b2RayCastCallback
{
public:
	LightRay()
	{

	}

	float minFraction = 1.0f;
	float rayLen;
	int portalCounter;

	b2Fixture* closestFixture;
	uint32_t filterMask;

	void sendRay(b2Vec2 rayStart, b2Vec2 dirVec, float rayLength, uint32_t filterMask = 0, int portalCounter=0);

private:
	float ReportFixture(b2Fixture* fixture, const b2Vec2& point, const b2Vec2& normal, float fraction);
};