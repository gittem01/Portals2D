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

class LightBridge : public Receiver
{
public:
	static std::vector<LightBridge> lightBridges;
	static int numLightBridges;
	static s_perTextureData* ptData;
	static s_texture* tex;
	static LightRay lightRay;

	s_bridgeUbo data;
	b2Vec2 lightDir;
	b2Vec2 lightPos;
	b2Vec2 lightLasers[2];
	b2Vec2 endPos;
	std::vector<s_lightRay> renderLightPositions;
	float lastLen;
	float maxSend;
	float totalSize;
	int reqMapCount;
	int renderOffset;
	int extraRenderIndex;

	LightBridge(PortalWorld* portalWorld, s_objectDataGrid* objData);
	LightBridge() {};

	void ssignal();
	void usignal();

	static void toDeath();
	static void toLeveler();
	static LightBridge* createLightBridge(PortalWorld* portalWorld, s_objectDataGrid* objData);
	static void staticInit();
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