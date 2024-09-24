#include "GameObjects/Platform.h"
#include "SwapChain.h"
#include "ThinDrawer.h"
#include "LevelerTestCase.h"

int Platform::numPlatforms = 0;
std::vector<Platform> Platform::platforms;
std::vector<uint8_t> Platform::loopingPlatforms;
std::vector<uint8_t> Platform::oneTimeMovePlatforms;

glm::vec2 vertexMults[4][2] =
{
	{ glm::vec2(-0.5, +0.5), glm::vec2(+0.5, +0.5) },
	{ glm::vec2(+0.5, +0.5), glm::vec2(+0.5, -0.5) },
	{ glm::vec2(+0.5, -0.5), glm::vec2(-0.5, -0.5) },
	{ glm::vec2(-0.5, -0.5), glm::vec2(-0.5, +0.5) },
};

static b2Vec2 fwdDirs[4] = { { 0, -1.0f }, { +1.0f, 0 }, { 0, +1.0f }, { -1.0f, 0 } };

void fillWallGelData(b2Body* body, WALL_TYPE wType, int edgeIndex, int prevEdgeIndex)
{
	b2Fixture* fix = body->GetFixtureList();
	b2EdgeShape* edge = (b2EdgeShape*)fix->GetShape();

	fixtureData* fData = (fixtureData*)fix->GetUserData().pointer;
	s_edgeLocalData*  eData = (s_edgeLocalData*)fData->data;
	s_edgeGlobalData*  gData = eData->globalData;
	for (int i = 0; i < gData->unitWalls.size(); i++) gData->unitWalls[i].wallType = wType;
	gData->renderOffset = ThinDrawer::object->wallGel->numWallGels;
	ThinDrawer::object->wallGel->numWallGels += gData->unitWalls.size();
	LevelerTestCase::wallMemFill(edge->m_vertex1, edge->m_vertex2, edgeIndex, gData->renderOffset, gData->unitWalls.size(), prevEdgeIndex);
}

Platform::Platform(PortalWorld* portalWorld, s_objectDataGrid* objData)
{
	looping = objData->loop;
	platformIndex = numPlatforms - 1;
	setMovementData(objData);

	glm::vec2 halfSize = glm::vec2(size) / 2.0f;

	b2BodyDef bDef;
	bDef.type = b2_kinematicBody;
	bDef.position = basePos;
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
	int edgeRenderIndexStart = ThinDrawer::object->wallGel->numWallLines + (numPlatforms - 1) * 4;

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
		
		LevelerTestCase::wallMemFill(b2Vec2(), b2Vec2(), edgeRenderIndexStart, initialOffset, size.x * 10, edgeRenderIndexStart + 3, true);

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
	box.SetAsBox(halfSize.x - 0.03, halfSize.y - 0.03);
	b2Fixture* fix = body->CreateFixture(&box, 0.0f);

	fix->GetUserData().pointer = fix->GetNext()->GetUserData().pointer;

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

	if (objData->startActive)
	{
		body->SetLinearVelocity(platformSpeed * fwdDir);

		if (looping)
		{
			loopingPlatforms.push_back(platformIndex);
		}
		else
		{
			oneTimeMovePlatforms.push_back(platformIndex);
		}
	}
}

void Platform::setMovementData(s_objectDataGrid* objData)
{
	int rotation = objData->rotation % 4;

	size = objData->scale;

	int ind = (rotation % 2) ^ 1;
	limitDist = size[ind];
	size[ind] = objData->extraScale.x;

	limitDist -= (rotation % 2 == 0 ? size.y : size.x);

	basePos = b2Vec2(objData->pos.x + size.x * 0.5f, objData->pos.y + size.y * 0.5f);
	if (rotation == 0)
		basePos.y += limitDist;
	else if (rotation == 3)
		basePos.x += limitDist;

	fwdDir = fwdDirs[rotation];
	dirMult = platformSpeed;
}

void Platform::enable()
{
	if (looping)
	{
		loopingPlatforms.push_back(platformIndex);
	}
	else
	{
		body->SetLinearVelocity(platformSpeed * fwdDir);
		oneTimeMovePlatforms.push_back(platformIndex);
	}
}

void Platform::disable()
{
	for (uint8_t thisIndex = 0; thisIndex < loopingPlatforms.size(); thisIndex++)
	{
		if (loopingPlatforms[thisIndex] == platformIndex)
		{
			loopingPlatforms[thisIndex] = loopingPlatforms[loopingPlatforms.size() - 1];
			loopingPlatforms.pop_back();

			break;
		}
	}

	if (looping)
	{
		body->SetLinearVelocity(b2Vec2(0.0f, 0.0f));
	}
	else
	{
		body->SetLinearVelocity(-platformSpeed * fwdDir);
		oneTimeMovePlatforms.push_back(platformIndex);
	}
}

Platform* Platform::createPlatform(PortalWorld* portalWorld, s_objectDataGrid* objData)
{
	Platform* platform = &platforms[numPlatforms++];
	return new (platform) Platform(portalWorld, objData);
}

void Platform::toDeath()
{
	for (int i = 0; i < platforms.size(); i++)
	{
		Platform* platform = &platforms[i];

		platform->body->SetTransform(platform->basePos, 0.0f);
	}

	loopingPlatforms = std::vector<uint8_t>();
	oneTimeMovePlatforms = std::vector<uint8_t>();
}

void Platform::toLeveler()
{
	for (int i = 0; i < platforms.size(); i++)
	{
		Platform* platform = &platforms[i];

		s_platformRectUbo* platformUbo;
		vkMapMemory(ThinDrawer::object->logicalDevice, ThinDrawer::object->platformRectangle->uniformBuffers[0].memory,
			platform->platformRenderOffset, sizeof(s_platformRectUbo), 0, (void**)&platformUbo);
		platformUbo->posSize.x = platform->basePos.x;
		platformUbo->posSize.y = platform->basePos.y;
		vkUnmapMemory(ThinDrawer::object->logicalDevice, ThinDrawer::object->platformRectangle->uniformBuffers[0].memory);
	}

	numPlatforms = 0;
	platforms = std::vector<Platform>();
	loopingPlatforms = std::vector<uint8_t>();
	oneTimeMovePlatforms = std::vector<uint8_t>();
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

	vkCmdDraw(cmdBuff, 6, platforms.size() * 4, 0, ThinDrawer::object->wallGel->numWallLines +
		MAX_PLATFORMS * ThinDrawer::object->lastSwapChainImageIndex);
}

void Platform::memMapFunc(void* ptr)
{
	ThinDrawer* td = ThinDrawer::object;

	for (int i = 0; i < platforms.size(); i++)
	{
		Platform* platform = &platforms[i];

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
			mapMem->positions = glm::vec4(p1.x, p1.y, p0.x, p0.y);
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

void Platform::perTickUpdate(float dt)
{
	for (int i = 0; i < loopingPlatforms.size(); i++)
	{
		Platform* platform = &platforms[loopingPlatforms[i]];

		b2Body* body = platform->body;

		b2Vec2 velocity = platform->dirMult * platform->fwdDir;

		float posDiff = b2Dot(body->GetPosition() - platform->basePos, platform->fwdDir);

		float speed = b2Dot(velocity, platform->fwdDir);
		float moveAmnt = abs(speed) * dt + 0.001f;

		if (speed > 0.0f && posDiff + moveAmnt >= platform->limitDist)
		{
			platform->dirMult = -platformSpeed;
		}
		else if (speed < 0.0f && posDiff <= moveAmnt)
		{
			platform->dirMult = platformSpeed;
		}

		body->SetLinearVelocity(platform->dirMult * platform->fwdDir);
	}

	for (int i = 0; i < oneTimeMovePlatforms.size(); i++)
	{
		Platform* platform = &platforms[oneTimeMovePlatforms[i]];

		b2Body* body = platform->body;

		b2Vec2 velocity = platform->body->GetLinearVelocity();
		float speed = b2Dot(velocity, platform->fwdDir);
		float moveAmnt = speed * dt + 0.001f * (speed > 0.0 ? 1.0f : -1.0f);
		float posDiff = b2Dot(body->GetPosition() - platform->basePos, platform->fwdDir) + moveAmnt;

		if (platform->enabled && posDiff >= platform->limitDist)
		{
			body->SetLinearVelocity(b2Vec2(0.0f, 0.0f));
			b2Vec2 setPos = platform->basePos + platform->limitDist * platform->fwdDir;
			body->SetTransform(setPos, 0.0f);
			oneTimeMovePlatforms[i] = oneTimeMovePlatforms[oneTimeMovePlatforms.size() - 1];
			oneTimeMovePlatforms.pop_back();
			i--;
		}
		else if (!platform->enabled && posDiff <= 0.0f)
		{
			body->SetLinearVelocity(b2Vec2(0.0f, 0.0f));
			body->SetTransform(platform->basePos, 0.0f);
			oneTimeMovePlatforms[i] = oneTimeMovePlatforms[oneTimeMovePlatforms.size() - 1];
			oneTimeMovePlatforms.pop_back();
			i--;
		}
	}
}