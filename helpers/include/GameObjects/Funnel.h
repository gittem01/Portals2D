#pragma once

#include "PortalWorld.h"
#include <glm/glm.hpp>
#include <unordered_map>
#include "GameBase.h"
#include "Leveler.h"
#include "GameObjects/Receiver.h"

class FunnelRay;

class Funnel : public Receiver
{
public:
	static std::vector<DefaultObject*> handleObjects;
	static int numHandleObjects;
	static std::vector<Funnel> funnels;
	static int numFunnels;
	static s_perTextureData* ptData;
	static s_texture* tex;
	static FunnelRay funnelRay;
	static bool extraRenderChecker[2];

	s_funnelUbo data;
	s_funnelUbo data2;
	b2Vec2 rayPositions[4];
	b2Vec2 funnelDir;
	b2Vec2 funnelPos;
	b2Vec2 lastStartPos2;
	b2Vec2 funnelDir2;
	int extraRenderIndex;
	int funnelRenderOffset;
	float rayLen;
	float lastLen;
	float lastLen2;
	float relativeSpeed;
	float baseAngle;
	float turnAngle;
	float turnAngle2;
	int reqMapCount;
	int signalCount2;
	int arrIndex;

	Funnel(PortalWorld* portalWorld, s_objectDataGrid* objData);
	Funnel() {};
	
	void ssignal();
	void usignal();

	void ssignal2();
	void usignal2();

	static Funnel* createFunnel(PortalWorld* portalWorld, s_objectDataGrid* objData);

	static void toLeveler();
	static void toDeath();
	static void staticInit();
	static void update();
	static void perTickUpdate();
	static void memMap(void* ptr);
	static void render(VkCommandBuffer cmdBuff, void* ptr);
};

class FunnelRay : public b2RayCastCallback
{
public:
	FunnelRay()
	{

	}

    float minFraction = 1.0f;
	float rayLen;

    b2Fixture* closestFixture;
    uint32_t filterMask;
	std::vector<std::pair<float, DefaultObject*>> objMap;

    void sendRay(b2Vec2 rayStart, b2Vec2 dirVec, float rayLength, uint32_t filterMask=0);

private:
    float ReportFixture(b2Fixture* fixture, const b2Vec2& point, const b2Vec2& normal, float fraction);
};