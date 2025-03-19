#include "GameObjects/Platform.h"
#include "SwapChain.h"
#include "ThinDrawer.h"
#include "LevelerTestCase.h"

int Platform::numPlatforms = 0;
std::vector<Platform> Platform::platforms;
std::vector<uint16_t> Platform::loopingPlatforms;
std::vector<uint16_t> Platform::oneTimeMovePlatforms;
std::vector<uint16_t> Platform::soundMakingPlatforms;

glm::vec2 vertexMults[4][2] =
{
	{ glm::vec2(-0.5, +0.5), glm::vec2(+0.5, +0.5) },
	{ glm::vec2(+0.5, +0.5), glm::vec2(+0.5, -0.5) },
	{ glm::vec2(+0.5, -0.5), glm::vec2(-0.5, -0.5) },
	{ glm::vec2(-0.5, -0.5), glm::vec2(-0.5, +0.5) },
};

static b2Vec2 fwdDirs[4] = { { 0, -1.0f }, { +1.0f, 0 }, { 0, +1.0f }, { -1.0f, 0 } };

static float platformSpeeds[4] = { 1.0f, 2.0f, 4.0f, 8.0f };

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
	platformSpeed = platformSpeeds[std::clamp((int)objData->speed, 0, (int)std::size(platformSpeeds))];
	setMovementData(objData);

	soundData.baseVolume = 10.0f;
	soundData.buffer = Sounder::object->loadSound("platformloop.wav");
	soundData.currentVolume = 0.0f;
	soundData.enabled = startEnabled;
	soundData.playing = false;
	soundData.pos0 = glm::vec2(basePos.x, basePos.y);
	soundData.source = 0;
	soundData.isPoint = true;

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

	float inLen = 0.01f;

	createEdgeOneSided(b2Vec2(+halfSize.x, -halfSize.y), b2Vec2(+halfSize.x, +halfSize.y),
		b2Vec2(-halfSize.x, +halfSize.y), b2Vec2(-halfSize.x, -halfSize.y),
		pType, portalWorld, body);
	if (pType == F_EDGE)
	{
		initialOffset = ThinDrawer::object->wallGel->numWallGels;
		fillWallGelData(body, wType, edgeRenderIndexStart, -1);
	}

	//b2Fixture* lastFix = body->GetFixtureList();
	//b2EdgeShape* eShape = (b2EdgeShape*)lastFix->GetShape();
	//b2EdgeShape edge;
	//edge.SetOneSided(eShape->m_vertex0, eShape->m_vertex1 + inLen * b2Vec2(-1.0f, -1.0f),
	//	eShape->m_vertex2 + inLen * b2Vec2(1.0f, -1.0f), eShape->m_vertex3);

	//b2Fixture* fix = body->CreateFixture(&edge, 0.0f);
	//fix->GetUserData().pointer = lastFix->GetUserData().pointer;

	createEdgeOneSided(b2Vec2(-halfSize.x, -halfSize.y), b2Vec2(+halfSize.x, -halfSize.y),
		b2Vec2(+halfSize.x, +halfSize.y), b2Vec2(-halfSize.x, +halfSize.y),
		pType, portalWorld, body);
	
	if (pType == F_EDGE)
	{
		fillWallGelData(body, wType, edgeRenderIndexStart + 1, edgeRenderIndexStart);
	}

	//lastFix = body->GetFixtureList();
	//eShape = (b2EdgeShape*)lastFix->GetShape();
	//edge.SetOneSided(eShape->m_vertex0, eShape->m_vertex1 + inLen * b2Vec2(-1.0f, 1.0f),
	//	eShape->m_vertex2 + inLen * b2Vec2(-1.0f, -1.0f), eShape->m_vertex3);

	//fix = body->CreateFixture(&edge, 0.0f);
	//fix->GetUserData().pointer = lastFix->GetUserData().pointer;

	createEdgeOneSided(	b2Vec2(-halfSize.x, +halfSize.y), b2Vec2(-halfSize.x, -halfSize.y),
						b2Vec2(+halfSize.x, -halfSize.y), b2Vec2(+halfSize.x, +halfSize.y),
						pType, portalWorld, body);

	if (pType == F_EDGE)
	{
		fillWallGelData(body, wType, edgeRenderIndexStart + 2, edgeRenderIndexStart + 1);
	}

	//lastFix = body->GetFixtureList();
	//eShape = (b2EdgeShape*)lastFix->GetShape();
	//edge.SetOneSided(eShape->m_vertex0, eShape->m_vertex1 + inLen * b2Vec2(1.0f, 1.0f),
	//	eShape->m_vertex2 + inLen * b2Vec2(-1.0f, 1.0f), eShape->m_vertex3);

	//fix = body->CreateFixture(&edge, 0.0f);
	//fix->GetUserData().pointer = lastFix->GetUserData().pointer;

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
					sizeof(s_wallGelData) * (edgeRenderIndex + j + MAX_PLATFORMS * i * 4), sizeof(s_wallGelData), 0, (void**)&mapMem);
				*mapMem = wgData;
				vkUnmapMemory(ThinDrawer::object->logicalDevice, ThinDrawer::object->wallGel->uniformBuffers[0].memory);
			}
		}
	}
	else
	{
		edgeRenderIndex = -1;
	}

	//lastFix = body->GetFixtureList();
	//eShape = (b2EdgeShape*)lastFix->GetShape();
	//edge.SetOneSided(eShape->m_vertex0, eShape->m_vertex1 + inLen * b2Vec2(1.0f, -1.0f),
	//	eShape->m_vertex2 + inLen * b2Vec2(1.0f, 1.0f), eShape->m_vertex3);

	//fix = body->CreateFixture(&edge, 0.0f);
	//fix->GetUserData().pointer = lastFix->GetUserData().pointer;

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

	startEnabled = objData->startActive;
	if (startEnabled)
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

void Platform::startMakingSound()
{
	soundMakingPlatforms.push_back(platformIndex);
	soundData.playing = true;

	soundData.source = Sounder::object->playSound(soundData.buffer, soundData.currentVolume + 0.001);
	alSourcei(soundData.source, AL_LOOPING, AL_TRUE);
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
	if (looping)
	{
		// bitmaps ?, use a flag index?, no this is fine
		for (uint8_t thisIndex = 0; thisIndex < loopingPlatforms.size(); thisIndex++)
		{
			if (loopingPlatforms[thisIndex] == platformIndex)
			{
				loopingPlatforms[thisIndex] = loopingPlatforms[loopingPlatforms.size() - 1];
				loopingPlatforms.pop_back();

				break;
			}
		}

		float posDiff = b2Dot(body->GetPosition() - basePos, fwdDir);

		if (posDiff >= limitDist)
		{
			b2Vec2 setPos = basePos + limitDist * fwdDir;
			body->SetTransform(setPos, 0.0f);
		}
		else if (posDiff <= 0.0f)
		{
			body->SetLinearVelocity(b2Vec2(0.0f, 0.0f));
			body->SetTransform(basePos, 0.0f);
		}

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
	loopingPlatforms = {};
	oneTimeMovePlatforms = {};
	for (int platformIndex : soundMakingPlatforms)
	{
		Platform* platform = &platforms[platformIndex];
		platform->soundData.playing = false;
		platform->soundData.currentVolume = 0.0f;

		alSourcei(platform->soundData.source, AL_LOOPING, AL_FALSE);
		alSourceStop(platform->soundData.source);
	}

	soundMakingPlatforms = {};

	for (int i = 0; i < platforms.size(); i++)
	{
		Platform* platform = &platforms[i];

		platform->body->SetTransform(platform->basePos, 0.0f);

		if (platform->startEnabled)
		{
			if (platform->looping)
			{
				loopingPlatforms.push_back(platform->platformIndex);
			}
			else
			{
				oneTimeMovePlatforms.push_back(platform->platformIndex);
			}

			platform->body->SetLinearVelocity(platform->platformSpeed * platform->fwdDir);
			platform->dirMult = platform->platformSpeed;
		}
		else
		{
			platform->body->SetLinearVelocity(b2Vec2(0.0f, 0.0f));
		}
	}
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

	for (int platformIndex : soundMakingPlatforms)
	{
		Platform* platform = &platforms[platformIndex];
		platform->soundData.playing = false;
		platform->soundData.currentVolume = 0.0f;

		alSourcei(platform->soundData.source, AL_LOOPING, AL_FALSE);
		alSourceStop(platform->soundData.source);
	}

	soundMakingPlatforms = {};

	numPlatforms = 0;
	platforms = std::vector<Platform>();
	loopingPlatforms = {};
	oneTimeMovePlatforms = {};
}

void Platform::staticInit()
{
	ThinDrawer::object->drawFuncs.push_back({ render, nullptr });
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
		MAX_PLATFORMS * ThinDrawer::object->frameIndex);

	// gels
	vkCmdBindPipeline(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS, ThinDrawer::object->wallGel->pipeline);

	vkCmdBindDescriptorSets(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS,
		ThinDrawer::object->wallGel->pipelineLayout, 0, 1, &ShaderBase::sharedSet, 1, &ThinDrawer::object->camOffset);

	vkCmdBindDescriptorSets(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS,
		ThinDrawer::object->wallGel->pipelineLayout, 1, 1, &ThinDrawer::object->wallGel->descriptorSets[1], 0, nullptr);

	vkCmdDraw(cmdBuff, 6, platforms.size() * 4, 0, ThinDrawer::object->wallGel->numWallLines +
		MAX_PLATFORMS * ThinDrawer::object->frameIndex);
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
				sizeof(s_wallGelData) * (platform->edgeRenderIndex + i + MAX_PLATFORMS * td->frameIndex * 4), sizeof(s_wallGelData), 0, (void**)&mapMem);
			mapMem->positions = glm::vec4(p1.x, p1.y, p0.x, p0.y);
			vkUnmapMemory(td->logicalDevice, td->wallGel->uniformBuffers[0].memory);
		}

		b2Vec2 bodyPos = body->GetPosition();

		int offset = ThinDrawer::object->frameIndex * sizeof(s_platformRectUbo) * MAX_PLATFORMS + platform->platformRenderOffset;
		s_platformRectUbo* platformUbo;
		vkMapMemory(ThinDrawer::object->logicalDevice, ThinDrawer::object->platformRectangle->uniformBuffers[0].memory,
			offset, sizeof(s_platformRectUbo), 0, (void**)&platformUbo);
		platformUbo->posSize.x = bodyPos.x;
		platformUbo->posSize.y = bodyPos.y;
		vkUnmapMemory(ThinDrawer::object->logicalDevice, ThinDrawer::object->platformRectangle->uniformBuffers[0].memory);
	}

	for (int i = 0; i < soundMakingPlatforms.size(); i++)
	{
		uint16_t platformIndex = soundMakingPlatforms[i];

		Platform* platform = &platforms[platformIndex];
		bool enabled = platform->body->GetLinearVelocity().LengthSquared() > 0.001f;

		b2Vec2 platformPosb2 = platform->body->GetPosition();
		glm::vec2 platformPos = glm::vec2(platformPosb2.x, platformPosb2.y);
		glm::vec2 camPos = glm::vec2(Sounder::object->listenerPos.x, -Sounder::object->listenerPos.y);

		float len = glm::length(platformPos - camPos);

		gridSoundData* soundData = &platform->soundData;

		float soundAdd = soundData->baseVolume * ThinDrawer::object->deltaTime;

		if (len > Sounder::object->soundRadius || !enabled)
		{
			if (!enabled)
			{
				soundData->currentVolume = glm::max(0.0f, soundData->currentVolume - soundAdd * 2.0f);

				if (soundData->currentVolume > 0.001f)
				{
					goto ifOut;
				}
			}

			alSourcei(soundData->source, AL_LOOPING, AL_FALSE);
			Sounder::object->stopSound(soundData->source);

			soundData->playing = false;
			soundData->currentVolume = 0.0f;

			soundMakingPlatforms[i] = soundMakingPlatforms.back();
			soundMakingPlatforms.pop_back();

			i--;

			continue;
		}

	ifOut:

		if (enabled)
		{
			soundData->currentVolume += soundAdd;
			soundData->currentVolume = glm::min(soundData->currentVolume, soundData->baseVolume);
		}

		float soundMult = glm::clamp(1.0f - len / Sounder::object->soundRadius, 0.0f, 1.0f);

		Sounder::object->setSoundVolume(soundData->source, soundData->currentVolume * soundMult);
		Sounder::object->setSoundPosition(soundData->source, glm::vec3(platformPos.x, platformPos.y, 0.0));

		ThinDrawer::object->registerDebugLine(glm::vec4(platformPos.x, platformPos.y, platformPos.x, platformPos.y), glm::vec4(1.0f), 0.2f, 0.5f);
	}
}

void Platform::perTickUpdate(float dt)
{
	static double timeSinceSoundExec = 0.0f;

	bool exec;
	if (timeSinceSoundExec >= 0.1f)
	{
		exec = true;
		timeSinceSoundExec = 0.0f;
	}
	else
	{
		exec = false;
		timeSinceSoundExec += dt;
	}

	float squaredRadius;
	b2Vec2 camPos;
	if (exec)
	{
		camPos = b2Vec2(Sounder::object->listenerPos.x, -Sounder::object->listenerPos.y);
		squaredRadius = Sounder::object->soundRadius * Sounder::object->soundRadius;
	}

	for (int i = 0; i < loopingPlatforms.size(); i++)
	{
		Platform* platform = &platforms[loopingPlatforms[i]];

		b2Body* body = platform->body;
		b2Vec2 bodyPos = body->GetPosition();

		if (exec && !platform->soundData.playing)
		{
			float len = b2DistanceSquared(bodyPos, camPos);

			if (len < squaredRadius)
			{
				platform->startMakingSound();
			}
		}

		b2Vec2 velocity = platform->dirMult * platform->fwdDir;

		float posDiff = b2Dot(bodyPos - platform->basePos, platform->fwdDir);

		float speed = b2Dot(velocity, platform->fwdDir);
		float moveAmnt = abs(speed) * dt + 0.001f;
		if (velocity.y < -2.5f)
		{
			moveAmnt -= 0.075f;
		}

		if (speed > 0.0f && posDiff + moveAmnt >= platform->limitDist)
		{
			platform->dirMult = -platform->platformSpeed;
		}
		else if (speed < 0.0f && posDiff <= moveAmnt)
		{
			platform->dirMult = platform->platformSpeed;
		}

		body->SetLinearVelocity(platform->dirMult * platform->fwdDir);
	}

	for (int i = 0; i < oneTimeMovePlatforms.size(); i++)
	{
		Platform* platform = &platforms[oneTimeMovePlatforms[i]];

		b2Body* body = platform->body;

		b2Vec2 bodyPos = body->GetPosition();

		if (exec && !platform->soundData.playing)
		{
			float len = b2DistanceSquared(bodyPos, camPos);

			if (len < squaredRadius)
			{
				platform->startMakingSound();
			}
		}

		b2Vec2 velocity = platform->body->GetLinearVelocity();
		float speed = b2Dot(velocity, platform->fwdDir);
		float moveAmnt = abs(speed) * dt + 0.001f;

		if (velocity.y < -2.5f)
		{
			moveAmnt -= 0.075f;
		}

		float posDiff = b2Dot(bodyPos - platform->basePos, platform->fwdDir) + moveAmnt;

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