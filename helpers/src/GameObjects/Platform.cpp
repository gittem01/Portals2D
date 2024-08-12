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

void fillWallGelData(b2Body* body, WALL_TYPE wType, int edgeIndex, int prevEdgeIndex)
{
	b2Fixture* fix = body->GetFixtureList();
	b2EdgeShape* edge = (b2EdgeShape*)fix->GetShape();

	fixtureData* fData = (fixtureData*)fix->GetUserData().pointer;
	s_edgeLocalData*  eData = (s_edgeLocalData*)fData->data;
	s_edgeGlobalData*  gData = eData->globalData;
	for (int i = 0; i < gData->unitWalls.size(); i++) gData->unitWalls[i].wallType = wType;
	gData->renderOfsset = ThinDrawer::object->wallGel->numWallGels;
	ThinDrawer::object->wallGel->numWallGels += gData->unitWalls.size();
	LevelerTestCase::wallMemFill(edge->m_vertex2, edge->m_vertex1, edgeIndex, gData->renderOfsset, gData->unitWalls.size(), prevEdgeIndex);
}

Platform::Platform(PortalWorld* portalWorld, s_objectDataGrid* objData)
{
	size = objData->scale;
	size[((objData->rotation % 4) % 2) ^ 1] = objData->extraScale.x;

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

	int initialOffset = -1;
	int edgeRenderIndexStart = ThinDrawer::object->wallGel->numWallLines + numPlatforms * 4;

	createEdgeOneSided(b2Vec2(+halfSize.x, -halfSize.y), b2Vec2(+halfSize.x, +halfSize.y),
		b2Vec2(-halfSize.x, +halfSize.y), b2Vec2(-halfSize.x, -halfSize.y),
		pType, portalWorld, body);
	if (pType == F_EDGE)
	{
		initialOffset = ThinDrawer::object->wallGel->numWallGels;
		fillWallGelData(body, wType, edgeRenderIndexStart, -1);
	}

	createEdgeOneSided(b2Vec2(-halfSize.x, -halfSize.y), b2Vec2(+halfSize.x, -halfSize.y),
		b2Vec2(+halfSize.x, +halfSize.y), b2Vec2(-halfSize.x, +halfSize.y),
		pType, portalWorld, body);
	if (pType == F_EDGE)
	{
		fillWallGelData(body, wType, edgeRenderIndexStart + 1, edgeRenderIndexStart);
	}

	createEdgeOneSided(	b2Vec2(-halfSize.x, +halfSize.y), b2Vec2(-halfSize.x, -halfSize.y),
						b2Vec2(+halfSize.x, -halfSize.y), b2Vec2(+halfSize.x, +halfSize.y),
						pType, portalWorld, body);
	if (pType == F_EDGE)
	{
		fillWallGelData(body, wType, edgeRenderIndexStart + 2, edgeRenderIndexStart + 1);
	}

	createEdgeOneSided(	b2Vec2(+halfSize.x, +halfSize.y), b2Vec2(-halfSize.x, +halfSize.y),
						b2Vec2(-halfSize.x, -halfSize.y), b2Vec2(+halfSize.x, -halfSize.y),
						pType, portalWorld, body);
	if (pType == F_EDGE)
	{
		fillWallGelData(body, wType, edgeRenderIndexStart + 3, edgeRenderIndexStart + 2);
		
		LevelerTestCase::wallMemFill(b2Vec2(), b2Vec2(), edgeRenderIndexStart, initialOffset, -1, edgeRenderIndexStart + 3, true);

		edgeRenderIndex = edgeRenderIndexStart;

		for (int i = 1; i < ThinDrawer::object->swapChain->imageCount; i++)
		{
			for (int j = 0; j < 4; j++)
			{
				s_wallGelData wgData;
				s_wallGelData* mapMem;
				vkMapMemory(ThinDrawer::object->logicalDevice, ThinDrawer::object->wallGel->uniformBuffers[0].memory,
					sizeof(s_wallGelData) * (edgeRenderIndex + j), sizeof(s_wallGelData), 0, (void**)&mapMem);
				wgData = *mapMem;
				vkUnmapMemory(ThinDrawer::object->logicalDevice, ThinDrawer::object->wallGel->uniformBuffers[0].memory);

				vkMapMemory(ThinDrawer::object->logicalDevice, ThinDrawer::object->wallGel->uniformBuffers[0].memory,
					sizeof(s_wallGelData) * (edgeRenderIndex + j + MAX_PLATFORMS * i), sizeof(s_wallGelData), 0, (void**)&mapMem);
				*mapMem = wgData;
				vkUnmapMemory(ThinDrawer::object->logicalDevice, ThinDrawer::object->wallGel->uniformBuffers[0].memory);
			}
		}
	}
	else
	{
		edgeRenderIndex = -1;
	}

	b2PolygonShape box;
	box.SetAsBox(halfSize.x - 0.01, halfSize.y - 0.01);
	body->CreateFixture(&box, 0.0f);

	body->SetLinearVelocity(b2Vec2(1.0, 0.0));

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
	ThinDrawer::object->memMapFuncs.push_back({ memMapFunc, nullptr });
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

	// gels
	vkCmdBindPipeline(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS, ThinDrawer::object->wallGel->pipeline);

	vkCmdBindDescriptorSets(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS,
		ThinDrawer::object->wallGel->pipelineLayout, 0, 1, &ShaderBase::sharedSet, 1, &ThinDrawer::object->camOffset);

	vkCmdBindDescriptorSets(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS,
		ThinDrawer::object->wallGel->pipelineLayout, 1, 1, &ThinDrawer::object->wallGel->descriptorSets[1], 0, nullptr);

	vkCmdDraw(cmdBuff, 6, numPlatforms * 4, 0, ThinDrawer::object->wallGel->numWallLines +
		MAX_PLATFORMS * ThinDrawer::object->lastSwapChainImageIndex);
}

void Platform::memMapFunc(void* ptr)
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
			vkMapMemory(td->logicalDevice, td->wallGel->uniformBuffers[0].memory,
				sizeof(s_wallGelData) * (platform->edgeRenderIndex + i + MAX_PLATFORMS * td->lastSwapChainImageIndex), sizeof(s_wallGelData), 0, (void**)&mapMem);
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

void Platform::update()
{

}