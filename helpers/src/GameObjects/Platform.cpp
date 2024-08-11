#include "GameObjects/Platform.h"
#include "SwapChain.h"
#include "ThinDrawer.h"
#include "LevelerTestCase.h"

int Platform::numPlatforms = 0;
Platform* Platform::platforms[MAX_PLATFORMS] = { 0 };

glm::vec2 vertexMults[4][2] =
{
	{ glm::vec2(-0.5, +0.5), glm::vec2(+0.5, +0.5) },
	{ glm::vec2(+0.5, +0.5), glm::vec2(+0.5, -0.5) },
	{ glm::vec2(+0.5, -0.5), glm::vec2(-0.5, -0.5) },
	{ glm::vec2(-0.5, -0.5), glm::vec2(-0.5, +0.5) },
};

Platform::Platform(PortalWorld* portalWorld, s_objectDataGrid* objData)
{
	size = glm::ivec2(5, 2);
	glm::vec2 halfSize = glm::vec2(size) / 2.0f;

	b2BodyDef bDef;
	bDef.type = b2_kinematicBody;
	bDef.position.Set(objData->pos.x + halfSize.x, objData->pos.y + halfSize.y);
	body = portalWorld->CreateBody(&bDef);

	FixtureDataType pType;
	if (objData->platformType < 2)
		pType = F_EDGE;
	else if (objData->platformType == 2)
		pType = F_GEDGE;
	else
		pType = F_FEDGE;

	WALL_TYPE wType = (WALL_TYPE)(1 << objData->platformType);

	b2Fixture* fix;
	fixtureData* fData;
	s_edgeLocalData* eData;
	s_edgeGlobalData* gData;
	int initialOffset;
	int edgeIndex;
	createEdgeOneSided(b2Vec2(+halfSize.x, -halfSize.y), b2Vec2(+halfSize.x, +halfSize.y),
		b2Vec2(-halfSize.x, +halfSize.y), b2Vec2(-halfSize.x, -halfSize.y),
		pType, portalWorld, body);
	if (pType == F_EDGE)
	{
		fix = body->GetFixtureList();
		fData = (fixtureData*)fix->GetUserData().pointer;
		eData = (s_edgeLocalData*)fData->data;
		gData = eData->globalData;
		for (int i = 0; i < gData->unitWalls.size(); i++) gData->unitWalls[i].wallType = wType;
		gData->renderOfsset = ThinDrawer::object->wallGel->numWallGels;
		initialOffset = ThinDrawer::object->wallGel->numWallGels;
		ThinDrawer::object->wallGel->numWallGels += gData->unitWalls.size();
		edgeIndex = ThinDrawer::object->wallGel->numWallLines;
		LevelerTestCase::wallMemFill(b2Vec2(-halfSize.x, +halfSize.y), b2Vec2(+halfSize.x, +halfSize.y), edgeIndex, gData->renderOfsset, gData->unitWalls.size(), -1);
	}

	createEdgeOneSided(b2Vec2(-halfSize.x, -halfSize.y), b2Vec2(+halfSize.x, -halfSize.y),
		b2Vec2(+halfSize.x, +halfSize.y), b2Vec2(-halfSize.x, +halfSize.y),
		pType, portalWorld, body);
	if (pType == F_EDGE)
	{
		fix = body->GetFixtureList();
		fData = (fixtureData*)fix->GetUserData().pointer;
		eData = (s_edgeLocalData*)fData->data;
		gData = eData->globalData;
		for (int i = 0; i < gData->unitWalls.size(); i++) gData->unitWalls[i].wallType = wType;
		gData->renderOfsset = ThinDrawer::object->wallGel->numWallGels;
		ThinDrawer::object->wallGel->numWallGels += gData->unitWalls.size();
		edgeIndex = ThinDrawer::object->wallGel->numWallLines + 1;
		LevelerTestCase::wallMemFill(b2Vec2(+halfSize.x, +halfSize.y), b2Vec2(+halfSize.x, -halfSize.y), edgeIndex, gData->renderOfsset, gData->unitWalls.size(), edgeIndex - 1);
	}

	createEdgeOneSided(	b2Vec2(-halfSize.x, +halfSize.y), b2Vec2(-halfSize.x, -halfSize.y),
						b2Vec2(+halfSize.x, -halfSize.y), b2Vec2(+halfSize.x, +halfSize.y),
						pType, portalWorld, body);
	if (pType == F_EDGE)
	{
		fix = body->GetFixtureList();
		fData = (fixtureData*)fix->GetUserData().pointer;
		eData = (s_edgeLocalData*)fData->data;
		gData = eData->globalData;
		for (int i = 0; i < gData->unitWalls.size(); i++) gData->unitWalls[i].wallType = wType;
		gData->renderOfsset = ThinDrawer::object->wallGel->numWallGels;
		ThinDrawer::object->wallGel->numWallGels += gData->unitWalls.size();
		edgeIndex = ThinDrawer::object->wallGel->numWallLines + 2;
		LevelerTestCase::wallMemFill(b2Vec2(+halfSize.x, -halfSize.y), b2Vec2(-halfSize.x, -halfSize.y), edgeIndex, gData->renderOfsset, gData->unitWalls.size(), edgeIndex - 1);
	}

	createEdgeOneSided(	b2Vec2(+halfSize.x, +halfSize.y), b2Vec2(-halfSize.x, +halfSize.y),
						b2Vec2(-halfSize.x, -halfSize.y), b2Vec2(+halfSize.x, -halfSize.y),
						pType, portalWorld, body);
	if (pType == F_EDGE)
	{
		fix = body->GetFixtureList();
		fData = (fixtureData*)fix->GetUserData().pointer;
		eData = (s_edgeLocalData*)fData->data;
		gData = eData->globalData;
		for (int i = 0; i < gData->unitWalls.size(); i++) gData->unitWalls[i].wallType = wType;
		gData->renderOfsset = ThinDrawer::object->wallGel->numWallGels;
		ThinDrawer::object->wallGel->numWallGels += gData->unitWalls.size();
		edgeIndex = ThinDrawer::object->wallGel->numWallLines + 3;
		LevelerTestCase::wallMemFill(b2Vec2(-halfSize.x, -halfSize.y), b2Vec2(-halfSize.x, +halfSize.y), edgeIndex, gData->renderOfsset, gData->unitWalls.size(), edgeIndex - 1);

		LevelerTestCase::wallMemFill(b2Vec2(), b2Vec2(), edgeIndex - 3, initialOffset, -1, edgeIndex, true);

		edgeRenderIndex = ThinDrawer::object->wallGel->numWallLines;
		ThinDrawer::object->wallGel->numWallLines += 4;
	}
	else
	{
		edgeRenderIndex = -1;
	}

	//body->SetLinearVelocity(b2Vec2(1.0, 0.0));

	s_platformRectUbo platformData;
	s_platformRectUbo* platformUbo;
	vkMapMemory(ThinDrawer::object->logicalDevice, ThinDrawer::object->platformRectangle->uniformBuffers[0].memory,
		objData->offset * sizeof(s_platformRectUbo), sizeof(s_platformRectUbo), 0, (void**)&platformUbo);
	platformData = *platformUbo;
	vkUnmapMemory(ThinDrawer::object->logicalDevice, ThinDrawer::object->platformRectangle->uniformBuffers[0].memory);
	
	platformRenderOffset = objData->offset * sizeof(s_platformRectUbo);

	for (int i = 1; i < ThinDrawer::object->swapChain->imageCount; i++)
	{
		s_platformRectUbo* platformUbo2;
		int offset = i * sizeof(s_platformRectUbo) * MAX_PLATFORMS + platformRenderOffset;
		vkMapMemory(ThinDrawer::object->logicalDevice, ThinDrawer::object->platformRectangle->uniformBuffers[0].memory,
			offset, sizeof(s_platformRectUbo), 0, (void**)&platformUbo2);
		memcpy(platformUbo2, &platformData, sizeof(s_platformRectUbo));
		vkUnmapMemory(ThinDrawer::object->logicalDevice, ThinDrawer::object->platformRectangle->uniformBuffers[0].memory);
	}

	platforms[numPlatforms++] = this;
}

void Platform::staticInit()
{
	ThinDrawer::object->midDrawFuncs.push_back({ render, nullptr });
}

void Platform::render(VkCommandBuffer cmdBuff, void* ptr)
{
	VkDeviceSize offsets = 0;

	vkCmdBindPipeline(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS, ThinDrawer::object->platformRectangle->pipeline);

	vkCmdBindVertexBuffers(cmdBuff, 0, 1, &ThinDrawer::object->wallLight->buffers[0].buffer, &offsets);

	vkCmdBindDescriptorSets(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS,
		ThinDrawer::object->platformRectangle->pipelineLayout, 0, 1, &ShaderBase::sharedSet, 1, &ThinDrawer::object->camOffset);

	vkCmdBindDescriptorSets(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS,
		ThinDrawer::object->platformRectangle->pipelineLayout, 1, 1, &ThinDrawer::object->platformRectangle->descriptorSets[1], 0, VK_NULL_HANDLE);

	vkCmdDraw(cmdBuff, ThinDrawer::object->wallLight->buffers[0].count, ThinDrawer::object->platformRectangle->numPlatforms, 0,
		MAX_PLATFORMS * ThinDrawer::object->lastSwapChainImageIndex);
}

void Platform::update()
{
	ThinDrawer* td = ThinDrawer::object;

	for (int i = 0; i < numPlatforms; i++)
	{
		Platform* platform = platforms[i];

		b2Body* body = platform->body;

		for (int i = 0; (i < 4) && platform->edgeRenderIndex > -1; i++)
		{
			glm::vec2 gp0 = vertexMults[i][0] * glm::vec2(platform->size);
			glm::vec2 gp1 = vertexMults[i][1] * glm::vec2(platform->size);

			b2Vec2 p0 = body->GetWorldPoint(b2Vec2(gp0.x, gp0.y));
			b2Vec2 p1 = body->GetWorldPoint(b2Vec2(gp1.x, gp1.y));

			s_wallGelData* mapMem;
			vkMapMemory(td->logicalDevice, td->wallGel->uniformBuffers[0].memory, sizeof(s_wallGelData) * (platform->edgeRenderIndex + i), sizeof(s_wallGelData), 0, (void**)&mapMem);
			mapMem->positions = glm::vec4(p0.x, p0.y, p1.x, p1.y);
			vkUnmapMemory(td->logicalDevice, td->wallGel->uniformBuffers[0].memory);
		}

		b2Vec2 bodyPos = body->GetPosition();

		int offset = ThinDrawer::object->lastSwapChainImageIndex * sizeof(s_platformRectUbo) * MAX_PLATFORMS + platform->platformRenderOffset;
		s_platformRectUbo* platformUbo;
		vkMapMemory(ThinDrawer::object->logicalDevice, ThinDrawer::object->platformRectangle->uniformBuffers[0].memory,
			offset, sizeof(s_platformRectUbo), 0, (void**)&platformUbo);
		platformUbo->posSize.x = bodyPos.x;
		platformUbo->posSize.y = bodyPos.y;
		vkUnmapMemory(ThinDrawer::object->logicalDevice, ThinDrawer::object->platformRectangle->uniformBuffers[0].memory);
	}
}