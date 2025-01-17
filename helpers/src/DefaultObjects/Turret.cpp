#include <DefaultObjects/Turret.h>
#include <ThinDrawer.h>
#include <Portal.h>
#include <Game.h>
#include <BaseTestCase.h>

std::vector<Turret*> Turret::turrets;
s_texture* Turret::coverTex;
s_texture* Turret::gunTex;
s_texture* Turret::glowTex;
s_buffers* Turret::coverVertexBuffer;
std::vector<uint32_t> Turret::soundBuffers;

const b2Vec2 tEyePos = b2Vec2(-0.23f, 0.11f);
const float minCosX = 0.7;
size_t lastTurretUpdateIndex = 0;

Turret::Turret(PortalWorld* portalWorld, b2Vec2 pos, bool mirrored) : DefaultObject(portalWorld)
{
	this->mirrored = mirrored;

	createBody(pos);

	laserDir = wData->pbIndices[0]->isMirrored ? b2Vec2(1.0f, 0.0f) : b2Vec2(-1.0f, 0.0f);

	renderLaserDatas.resize(numLasers);

	turrets.push_back(this);
}

void Turret::createBody(b2Vec2 pos)
{
	float ySize = 1.6967984934086628;
	float r = 0.15;
	float xm = mirrored ? -1.0f : 1.0f;
	b2Vec2 legPoints0[4] =
	{
		b2Vec2(-0.193 * xm, -0.19 * ySize),
		b2Vec2(-0.49 * xm, -0.35 * ySize),
		b2Vec2(-0.43 * xm, -0.38 * ySize),
		b2Vec2(-0.157 * xm, -0.22 * ySize)
	};
	b2Vec2 legPoints1[4] =
	{
		b2Vec2(-0.49 * xm, -0.35 * ySize),
		b2Vec2(-0.5 * xm, -0.5 * ySize),
		b2Vec2(-0.42 * xm, -0.5 * ySize),
		b2Vec2(-0.43 * xm, -0.36 * ySize)
	};

	b2Vec2 legPoints2[4] =
	{
		b2Vec2(0.1 * xm, -0.19 * ySize),
		b2Vec2(0.1 * xm, -0.23 * ySize),
		b2Vec2(0.23 * xm, -0.2 * ySize),
		b2Vec2(0.22 * xm, -0.16 * ySize)
	};
	b2Vec2 legPoints3[4] =
	{
		b2Vec2(0.22 * xm, -0.16 * ySize),
		b2Vec2(0.22 * xm, -0.2 * ySize),
		b2Vec2(0.44 * xm, -0.255 * ySize),
		b2Vec2(0.49 * xm, -0.23 * ySize)
	};
	b2Vec2 legPoints4[4] =
	{
		b2Vec2(0.44 * xm, -0.245 * ySize),
		b2Vec2(0.43 * xm, -0.5 * ySize),
		b2Vec2(0.5 * xm, -0.5 * ySize),
		b2Vec2(0.49 * xm, -0.23 * ySize)
	};

	b2Vec2 bodyPoints[8] =
	{
		b2Vec2(0.1 * xm, -0.27 * ySize),
		b2Vec2(0.24 * xm, -0.05 * ySize),
		b2Vec2(0.24 * xm, 0.22 * ySize),
		b2Vec2(0.1 * xm, 0.37 * ySize),
		b2Vec2(-0.195 * xm, 0.37 * ySize),
		b2Vec2(-0.284 * xm, 0.22 * ySize),
		b2Vec2(-0.304 * xm, -0.02 * ySize),
		b2Vec2(-0.194 * xm, -0.27 * ySize)
	};

	midX = -0.045;
	b2Vec2 bottomCirclePos = b2Vec2(midX * xm, -0.235 * ySize);
	b2Vec2 topCirclePos = b2Vec2(midX * xm, ySize - r - 0.1 - 0.5 * ySize);

	b2BodyDef bodyDef;
	bodyDef.type = b2_dynamicBody;
	bodyDef.position = pos;

	initialPos = pos;

	b2Body* body = pWorld->CreateBody(&bodyDef);

	b2PolygonShape legShape0;
	legShape0.Set(legPoints0, 4);
	b2PolygonShape legShape1;
	legShape1.Set(legPoints1, 4);
	b2PolygonShape legShape2;
	legShape2.Set(legPoints2, 4);
	b2PolygonShape legShape3;
	legShape3.Set(legPoints3, 4);
	b2PolygonShape legShape4;
	legShape4.Set(legPoints4, 4);
	b2PolygonShape bodyShape;
	bodyShape.Set(bodyPoints, 8);
	b2CircleShape bottomCircle;
	bottomCircle.m_radius = r;
	bottomCircle.m_p = bottomCirclePos;
	b2CircleShape topCircle;
	topCircle.m_radius = r;
	topCircle.m_p = topCirclePos;

	body->CreateFixture(&legShape0, 1.0f);
	body->CreateFixture(&legShape1, 1.0f);
	body->CreateFixture(&legShape2, 1.0f);
	body->CreateFixture(&legShape3, 1.0f);
	body->CreateFixture(&legShape4, 1.0f);
	body->CreateFixture(&bodyShape, 1.0f);
	body->CreateFixture(&bottomCircle, 1.0f);
	body->CreateFixture(&topCircle, 1.0f);

	eyeBody = pWorld->createPortalBody(body);

	eyeBody->isMirrored = mirrored;
	wData = eyeBody->worldIndex;	

	if (mirrored)
	{
		wData->swapMirrored();
	}

	bodyData* bData = (bodyData*)body->GetUserData().pointer;
	pData.type = (g_DataType)(DEFAULT_OBJECT_O | TURRET_O);
	pData.data = this;
	bData->extraData = &pData;

	bodyFixDataIndex = 2;

	renderGroup = ThinDrawer::object->addPortalBody(eyeBody, { "turretBody.png" });
	ThinDrawer::object->debug_addPortalBody(eyeBody);
}

void Turret::recreate(bool unused_mirrored)
{
	if (wData->pbIndices.size() == 0)
	{
		PortalBody* portalBody = pWorld->createCloneBody0(wData, initialPos, 0.0f, mirrored);

		portalBody->worldIndex->gravityScale = 1.0f;
		portalBody->worldIndex->destrTime = -20.0f;

		bodyData* bData = (bodyData*)portalBody->body->GetUserData().pointer;
		bData->extraData = (void*)&pData;
	}
	else
	{
		DefaultObject::recreate(mirrored);
	}

	lastTrackTime = -10.0;
	lastTrackPtr = nullptr;
	tracking = false;

	shootGundex = 0;
	lastShootTime = -10.0;
	shootTimes[0] = -10.0f; shootTimes[1] = -10.0f;
	fallTime = -10.0;
	gunBacks[0] = 0.0f; gunBacks[1] = 0.0f;
	status = false;
	deactivated = false;
	openAmount = 0.0f;
	rotation = 0.0f;

	firstLaserIndex = 0;
	lastLaserIndex = 0;

	eyeBody = wData->pbIndices[0];
}

void Turret::renderInit()
{
	coverTex = IOHelper::loadTexture("turretCover.png", VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER);
	gunTex = IOHelper::loadTexture("turretGun.png", VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER);
	glowTex = IOHelper::loadTexture("glow.png", VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER);
	IOHelper::loadTexture("turretBody.png", VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER);

	std::vector<s_vertex> coverVertices =
	{
		{ { -0.5f, +0.5f }, { 0.0f, 0.0f } },
		{ { -0.5f, -0.5f }, { 0.0f, 1.0f } },
		{ { +0.5f, -0.5f }, { 1.0f, 1.0f } },

		{ { +0.5f, -0.5f }, { 1.0f, 1.0f } },
		{ { +0.5f, +0.5f }, { 1.0f, 0.0f } },
		{ { -0.5f, +0.5f }, { 0.0f, 0.0f } },
	};

	coverVertexBuffer = (s_buffers*)malloc(sizeof(s_buffers));
	coverVertexBuffer->count = coverVertices.size();
	ThinDrawer::object->bufferStage(coverVertices.data(), coverVertices.size() * sizeof(s_vertex), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, coverVertexBuffer);

	ALuint buffer = Sounder::object->loadSound("firee.wav");
	soundBuffers.push_back(buffer);
}

void Turret::staticInit()
{
	ThinDrawer::object->midDrawFuncs.push_back({ draw, nullptr });
}

glm::vec2 rotateArroundPoint(glm::vec2 c, glm::vec2 p, float angle)
{
	glm::vec2 v = p - c;
	float s = sin(angle);
	float c2 = cos(angle);
	return glm::vec2(c.x + v.x * c2 - v.y * s, c.y + v.x * s + v.y * c2);
}

void Turret::shoot()
{
	if (Game::object->gameTime - lastShootTime > shootTimer && openAmount > 0.999f)
	{
		ALuint source = Sounder::object->playSound(soundBuffers[0], 0.2 + 0.1 * rand() / (float)RAND_MAX);
		Sounder::object->setSoundPosition(source, glm::vec3(wData->pbIndices[0]->body->GetPosition().x, wData->pbIndices[0]->body->GetPosition().y, 0.0));

		lastShootTime = Game::object->gameTime;
		shootTimes[shootGundex] = Game::object->gameTime;

		float mirrMult = wData->pbIndices[0]->isMirrored ? -1.0f : 1.0f;
		float effectiveRotation = rotation * mirrMult;
		glm::vec2 center = glm::vec2((midX + openAmount * 0.05) * mirrMult, 0.115f);

		b2Vec2 bPos = b2Vec2(-0.31 * mirrMult, 0.24 - shootGundex * 0.24);
		
		glm::vec2 res = rotateArroundPoint(center, *(glm::vec2*)&bPos, effectiveRotation);
		b2Vec2 fwdMov = b2Vec2(bPos.x - 100.0f * mirrMult, bPos.y);
		bPos = b2Vec2(res.x, res.y);
		res = rotateArroundPoint(glm::vec2(0.0f, 0.0f), *(glm::vec2*)&fwdMov, effectiveRotation + 0.1f * rand() / (float)RAND_MAX - 0.05f);
		fwdMov = b2Vec2(res.x, res.y);

		bPos = wData->pbIndices[0]->body->GetWorldPoint(bPos);
		fwdMov = wData->pbIndices[0]->body->GetWorldPoint(fwdMov);
		b2Vec2 dir = fwdMov - bPos;

		if (wData->pbIndices[0]->bodyMaps.size() > 0)
		{
			for (int i = 0; i < wData->pbIndices[0]->bodyMaps.size(); i++)
			{
				bodyCollisionStatus bcs = wData->pbIndices[0]->bodyMaps[i];
				Portal* portal = bcs.connection->portal1;
				if (PortalWorld::getRealDist(portal->points[0], portal->points[1], bPos) > -0.01f)
				{
					bPos = PortalWorld::getRelativePortalPosition_W(bPos, bcs.connection);
					dir = -PortalWorld::getRelativePortalPosition_L(dir, bcs.connection);
					break;
				}
			}
		}

		pWorld->rayHandler->sendRay(bPos, dir, 120.0f, wData);

		for (rayData& rd : pWorld->rayHandler->rayResult)
		{
			s_laserUbo laserData;
			laserData.colour = glm::vec4(0.8f, 0.8f, 0.0f, 1.0f);
			laserData.points = glm::vec4(rd.startPos.x, rd.startPos.y, rd.endPos.x, rd.endPos.y);
			laserData.data = glm::vec4(0.25f, 1.5f, 1.0f, ThinDrawer::object->gameTime);

			renderLaserDatas[lastLaserIndex % numLasers] = { laserData, shootGundex };
			lastLaserIndex++;
		}

		if (pWorld->rayHandler->closestFixture)
		{
			bodyData* bData = (bodyData*)pWorld->rayHandler->closestFixture->GetBody()->GetUserData().pointer;
			if (bData && bData->extraData && ((g_PhysicsData*)bData->extraData)->type & PLAYER_O)
			{
				TestPlayer::object->health -= 0;
				b2Vec2 vel = TestPlayer::object->wData->pbIndices[0]->body->GetLinearVelocity();
				TestPlayer::object->wData->pbIndices[0]->body->SetLinearVelocity(0.75f * vel);
			}
			
			rayData lastRay = pWorld->rayHandler->rayResult[pWorld->rayHandler->rayResult.size() - 1];
			b2Vec2 rayEnd = lastRay.endPos;
			b2Vec2 lastDir = lastRay.endPos - lastRay.startPos; lastDir.Normalize();

			pWorld->rayHandler->closestFixture->GetBody()->ApplyForce(400.0f * lastDir, rayEnd, true);
		}

		shootGundex = (shootGundex + 1) % 2;
	}
}

void Turret::draw(VkCommandBuffer cmdBuff, void* ptr)
{
	ThinDrawer* drawer = ThinDrawer::object;

	VkDeviceSize offsets = 0;

	double timeNow = Game::object->gameTime;

	for (auto trt : turrets)
	{
		for (int i = 0; i < 2; i++)
		{
			double td = timeNow - trt->shootTimes[i];
			if (td < fullBackInTime)
			{
				trt->gunBacks[i] = sqrt((0.5 - 1.0 / (exp(td * 30.0) + 1.0)) * 2.0);
			}
			else if (td < fullBackInTime * 2.0)
			{
				trt->gunBacks[i] = 1.0 - (td - fullBackInTime) / fullBackInTime;
			}
			else
			{
				trt->gunBacks[i] = 0.0f;
			}
		}

		vkCmdBindPipeline(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS, drawer->bodyShader->pipeline);

		const uint32_t camOffset = ShaderBase::alignedSharedSize * drawer->frameIndex;
		vkCmdBindDescriptorSets(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS,
			drawer->bodyShader->pipelineLayout, 0, 1, &ShaderBase::sharedSet, 1, &camOffset);

		vkCmdBindDescriptorSets(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS,
			drawer->bodyShader->pipelineLayout, 1, 1, &trt->coverTex->set, 0, VK_NULL_HANDLE);

		vkCmdBindVertexBuffers(cmdBuff, 0, 1, &trt->coverVertexBuffer->buffer, &offsets);

		for (auto pBody : trt->wData->pbIndices)
		{
			s_renderBodyPsh pshData;
			pshData.data.y = pBody->worldIndex->destrTime;
			pshData.data.z = trt->renderGroup->gelIndex + 0.5f;

			float mirrMult = pBody->isMirrored ? -1.0f : 1.0f;
			float effectiveRotation = trt->rotation * mirrMult;
			pshData.posRot = glm::vec4(pBody->body->GetPosition().x, pBody->body->GetPosition().y, pBody->body->GetAngle() + effectiveRotation, pBody->isMirrored * -1.0f);
			bool renderDisabled = false;
			fixtureData* fixData = (fixtureData*)pBody->fixtures[trt->bodyFixDataIndex]->GetUserData().pointer;
			PBFixtureData* pbfData = (PBFixtureData*)fixData->data;
			if (pbfData->pCollisions.size() > 0)
			{
				for (int i = 0; i < pbfData->pCollisions.size(); i++)
				{
					portalCollision pc = pbfData->pCollisions[i];
					if (pc.status < 0)
					{
						renderDisabled = true; i++;
						break;
					}

					pshData.portals[i] = glm::vec4(
						pc.portal->points[pc.side ^ 1].x, pc.portal->points[pc.side ^ 1].y,
						pc.portal->points[pc.side].x, pc.portal->points[pc.side].y);
				}

				pshData.data.x = (float)pbfData->pCollisions.size();
			}
			else
			{
				pshData.data.x = 0.0f;
			}

			if (!renderDisabled)
			{
				glm::vec2 center = glm::vec2((trt->midX + trt->openAmount * 0.05) * mirrMult, 0.115f);
				glm::vec2 newCenter = rotateArroundPoint(glm::vec2(0.0f, 0.0f), center, -effectiveRotation);

				vkCmdBindDescriptorSets(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS,
					drawer->bodyShader->pipelineLayout, 1, 1, &trt->gunTex->set, 0, VK_NULL_HANDLE);

				pshData.movScale = glm::vec4((-0.05f - trt->openAmount * 0.125 + trt->gunBacks[0] * 0.1 * trt->openAmount) * mirrMult, 0.24f, 0.275, 0.275 * 0.234);
				glm::vec2 p = glm::vec2(pshData.movScale.x, pshData.movScale.y);
				p = rotateArroundPoint(glm::vec2(0.0f, 0.0f), p, -effectiveRotation);
				p = rotateArroundPoint(newCenter, p, effectiveRotation);
				pshData.movScale.x = p.x; pshData.movScale.y = p.y;

				vkCmdPushConstants(cmdBuff, drawer->bodyShader->pipelineLayout,
					VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(s_renderBodyPsh), &pshData);

				vkCmdDraw(cmdBuff, 6, 1, 0, 0);

				pshData.movScale.x = (-0.05f - trt->openAmount * 0.125 + trt->gunBacks[1] * 0.1 * trt->openAmount) * mirrMult;
				pshData.movScale.y = 0.0f;
				p = glm::vec2(pshData.movScale.x, pshData.movScale.y);
				p = rotateArroundPoint(glm::vec2(0.0f, 0.0f), p, -effectiveRotation);
				p = rotateArroundPoint(newCenter, p, effectiveRotation);
				pshData.movScale.x = p.x; pshData.movScale.y = p.y;

				vkCmdPushConstants(cmdBuff, drawer->bodyShader->pipelineLayout,
					VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(s_renderBodyPsh), &pshData);

				vkCmdDraw(cmdBuff, 6, 1, 0, 0);

				pshData.movScale = glm::vec4(newCenter.x, newCenter.y, 0.32f * (1.0 + trt->openAmount * 0.125), 0.55f * (1.0 + trt->openAmount * 0.125));
				vkCmdBindDescriptorSets(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS,
					drawer->bodyShader->pipelineLayout, 1, 1, &trt->coverTex->set, 0, VK_NULL_HANDLE);

				vkCmdPushConstants(cmdBuff, drawer->bodyShader->pipelineLayout,
					VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(s_renderBodyPsh), &pshData);

				vkCmdDraw(cmdBuff, 6, 1, 0, 0);

				if (!trt->deactivated)
				{
					float timeMult = sin(timeNow * 3.0f) * 0.1f + 0.1f + 1.0f;
					timeMult *= trt->fallTime > -5.0 ? std::max(2.4 - (timeNow - trt->fallTime), 0.0) / 2.4f : 1.0;
					pshData.posRot.z = pBody->body->GetAngle();
					pshData.movScale = glm::vec4(-0.23f * mirrMult, 0.11f, (0.3f * (1.0f + trt->openAmount * 0.3)) * timeMult, (0.3f * (1.0f + trt->openAmount * 0.3)) * timeMult);

					vkCmdBindDescriptorSets(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS,
						drawer->bodyShader->pipelineLayout, 1, 1, &trt->glowTex->set, 0, VK_NULL_HANDLE);

					pshData.data.z = -10.0;
					vkCmdPushConstants(cmdBuff, drawer->bodyShader->pipelineLayout,
						VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(s_renderBodyPsh), &pshData);

					vkCmdDraw(cmdBuff, 6, 1, 0, 0);
				}
			}
		}

		if (trt->wData->pbIndices.size() == 0) continue;

		float mirrMult = trt->eyeBody->isMirrored ? -1.0f : 1.0f;
		float effectiveRotation = trt->rotation * mirrMult;

		vkCmdBindPipeline(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS, drawer->laserPshShader->pipeline);

		vkCmdBindDescriptorSets(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS,
			drawer->laserPshShader->pipelineLayout, 0, 1, &ShaderBase::sharedSet, 1, &camOffset);

		vkCmdBindVertexBuffers(cmdBuff, 0, 1, &drawer->playerShader->buffers[0].buffer, &offsets);

		b2Vec2 bPos = b2Vec2(mirrMult * -0.23f, 0.11f);
		b2Vec2 fwdMov = b2Vec2(bPos.x - 100.0f * mirrMult, bPos.y);
		glm::vec2 res = rotateArroundPoint(glm::vec2(0.0f, 0.0f), *(glm::vec2*)&fwdMov, effectiveRotation);
		fwdMov = b2Vec2(res.x, res.y);

		bPos = trt->eyeBody->body->GetWorldPoint(bPos);
		fwdMov = trt->eyeBody->body->GetWorldPoint(fwdMov);

		float destrDiff = 1.0f;
		if (trt->wData->destrTime > 0.0)
		{
			destrDiff = std::max(1.5f - (float)(ThinDrawer::object->gameTime - trt->wData->destrTime), 0.0f);
		}

		s_laserUbo laserData;
		laserData.colour = glm::vec4(1.0f, 0.0f, 0.0f, destrDiff);
		laserData.data = glm::vec4(0.2f, 1.5f, 0.5f, 0.0f);

		if (trt->laserPoints.size() > 0)
		{
			for (centerLaser clData : trt->laserPoints)
			{
				memcpy(&laserData.points, &clData, sizeof(centerLaser));
				vkCmdPushConstants(cmdBuff, drawer->laserPshShader->pipelineLayout,
					VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(s_laserUbo), &laserData);

				vkCmdDraw(cmdBuff, 6, 1, 0, 0);
			}

			memcpy(&laserData.points, &trt->laserPoints.front().to, sizeof(b2Vec2));
			memcpy(&(laserData.points.z), &trt->laserPoints.front().to, sizeof(b2Vec2));
			laserData.data.x = 1.0f;
			vkCmdPushConstants(cmdBuff, drawer->laserPshShader->pipelineLayout,
				VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(s_laserUbo), &laserData);

			vkCmdDraw(cmdBuff, 6, 1, 0, 0);
		}

		uint32_t i = trt->firstLaserIndex;
		for (; i < trt->lastLaserIndex; i++)
		{
			std::pair<s_laserUbo, uint32_t> renderData = trt->renderLaserDatas[i % numLasers];
			s_laserUbo laserData = renderData.first;
			uint32_t laserIndex = renderData.second;
			float dt = ThinDrawer::object->gameTime - laserData.data.w;
			if (dt < 0.2f)
			{
				glm::vec2 center = glm::vec2((trt->midX + trt->openAmount * 0.05) * mirrMult, 0.115f);

				b2Vec2 bPos = b2Vec2(-0.31 * mirrMult, 0.24 - laserIndex * 0.24);

				glm::vec2 res = rotateArroundPoint(center, *(glm::vec2*)&bPos, effectiveRotation);
				bPos = b2Vec2(res.x, res.y);

				bPos = trt->wData->pbIndices[0]->body->GetWorldPoint(bPos);
				//laserData.points.x = bPos.x;
				//laserData.points.y = bPos.y;

				laserData.data.z = 1.0f - sin(dt * b2_pi * 2.5f);

				vkCmdPushConstants(cmdBuff, drawer->laserPshShader->pipelineLayout,
					VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(s_laserUbo), &laserData);

				vkCmdDraw(cmdBuff, 6, 1, 0, 0);
			}
			else
			{
				trt->firstLaserIndex++;
			}
		}
	}
}

void Turret::perTickUpdate(float dt)
{
	double timeNow = Game::object->gameTime;
	for (auto trt : turrets)
	{
		if (trt->wData->destrTime > 0.0f) continue;

		if (trt->deactivated)
		{
			trt->eyeBody = trt->wData->pbIndices[0];
			continue;
		}

		if (trt->fallTime > -5.0 && timeNow - trt->fallTime > 2.5)
		{
			trt->deactivated = true;
			trt->laserPoints.clear();
			trt->status = false;
			continue;
		}

		if (trt->status)
		{
			trt->openAmount += 0.002f;
			trt->openAmount = std::min(trt->openAmount, 1.0f);
		}
		else
		{
			trt->openAmount -= 0.002f;
			trt->openAmount = std::max(trt->openAmount, 0.0f);
		}

		PortalBody* tPb;
		if (trt->wData->pbIndices.size() == 1)
		{
			tPb = trt->wData->pbIndices[0];
		}
		else
		{
			tPb = trt->wData->pbIndices[0];
			b2Vec2 eyePos = b2Vec2(tEyePos.x * (tPb->isMirrored ? -1.0f : 1.0f), tEyePos.y);
			eyePos = tPb->body->GetWorldPoint(eyePos);
			if (tPb->bodyMaps.size() > 0)
			{
				portalConnection* pc = tPb->bodyMaps[0].connection;
				if (pc->portal1->getPointSide(eyePos) != pc->side1)
				{
					tPb = trt->wData->pbIndices[1];
				}
			}
		}

		if (abs(tPb->body->GetAngle()) > b2_pi * 0.4f)
		{
			if (trt->fallTime < -5.0)
			{
				trt->fallTime = timeNow;
			}
		}
		else
		{
			trt->fallTime = -10.0;
		}

		trt->eyeBody = tPb;

		const float turretBodyAngle = tPb->body->GetAngle();
		trt->fwd = (tPb->isMirrored ? 1.0f : -1.0f) * b2Vec2(cos(turretBodyAngle), sin(turretBodyAngle));

		ThinDrawer::object->registerDebugLine(glm::vec4(trt->eyePos.x, trt->eyePos.y, trt->eyePos.x, trt->eyePos.y),
			glm::vec4(0.0f, 1.0f, 0.0f, 1.0f), 0.1f, 0.5f);

		if (trt->lastTrackTime + 0.5 > timeNow)
		{
			b2Vec2 v = trt->trackPoint - trt->eyePos;

			ThinDrawer::object->registerDebugLine(glm::vec4(trt->trackPoint.x, trt->trackPoint.y, trt->trackPoint.x, trt->trackPoint.y),
				glm::vec4(1.0f, 0.0f, 0.0f, 1.0f), 0.1f, 0.5f);

			v.Normalize();

			float vAngle = PortalWorld::calcAngle2(trt->fwd) - PortalWorld::calcAngle2(v);
			vAngle = PortalWorld::normalizeAngle(vAngle);
			vAngle = std::clamp(vAngle, -1.1f, 1.1f);
			v = PortalWorld::rotateVec(trt->fwd, -vAngle);
			
			trt->laserDir = 0.99f * trt->laserDir + 0.01f * v;
			trt->status = true;
			trt->shoot();
		}
		else if (trt->fallTime > -5.0 && timeNow - trt->fallTime > 0.1)
		{
			trt->status = true;
			b2Vec2 randomDir = trt->fwd; randomDir = PortalWorld::rotateVec(randomDir, (rand() / (float)RAND_MAX) * 2.2f - 1.1f);
			trt->laserDir = trt->laserDir = 0.99f * trt->laserDir + 0.01f * randomDir;
			trt->shoot();
		}
		else
		{
			trt->laserDir = trt->laserDir = 0.99f * trt->laserDir + 0.01f * trt->fwd;
			trt->status = false;
		}
		
		if (trt->openAmount != 0.0f)
		{
			float angle = atan2(trt->laserDir.y, trt->laserDir.x);
			trt->rotation = (tPb->isMirrored ? -1.0f : 1.0f) * (angle - turretBodyAngle + b2_pi) + (tPb->isMirrored ? 1.0f : 0.0f) * b2_pi;
		}
		else
		{
			trt->rotation = 0.0f;
		}

		trt->laserPoints.clear();
		trt->pWorld->rayHandler->sendRay(trt->eyePos, trt->laserDir, 400.0f);
		for (rayData& rd : trt->pWorld->rayHandler->rayResult)
		{
			trt->laserPoints.push_back({ rd.startPos, rd.endPos });
		}

		trt->eyePos = b2Vec2(tEyePos.x * (tPb->isMirrored ? -1.0f : 1.0f), tEyePos.y);
		trt->eyePos = tPb->body->GetWorldPoint(trt->eyePos);
	}

	size_t numTurretUpdates = std::min((size_t)(dt * 2000.0f), turrets.size());

	PortalBody* pb;
	float mirrMult;
	b2Body* body;
	float bodyAngle;
	b2Vec2 pos;

	for (int i = TestPlayer::object->wData->pbIndices.size() - 1; i > -1; i--)
	{
		pb = TestPlayer::object->wData->pbIndices[i];
		mirrMult = pb->isMirrored ? -1.0f : 1.0f;
		body = pb->body;
		bodyAngle = body->GetAngle();
		pos = body->GetPosition();

		for (size_t i = lastTurretUpdateIndex; i < numTurretUpdates + lastTurretUpdateIndex; i++)
		{
			Turret* trt = turrets.at(i % turrets.size());

			if (trt->lastTrackPtr != pb && trt->lastTrackTime + 0.3 > timeNow || trt->wData->destrTime > 0.0f || trt->deactivated) continue;
			
			PortalBody* tPb = trt->eyeBody;

			b2Vec2 upVec = pos - tPb->body->GetPosition();
			upVec = b2Vec2(-upVec.y, upVec.x); upVec.Normalize();
			b2Vec2 lookPos = pos + sin(timeNow * 50.0f) * upVec;

			b2Vec2 diff = lookPos - trt->eyePos;
			float cosX = b2Dot(diff, trt->fwd) / diff.Length();

			if (cosX > minCosX)
			{
				lookPos.x += upVec.y; lookPos.y += -upVec.x;
				trt->pWorld->rayHandler->sendRay(trt->eyePos, lookPos - trt->eyePos, 100.0f);
				if (trt->pWorld->rayHandler->closestFixture)
				{
					bodyData* bData = (bodyData*)trt->pWorld->rayHandler->closestFixture->GetBody()->GetUserData().pointer;
					if (bData && bData->extraData && ((g_PhysicsData*)bData->extraData)->type & PLAYER_O)
					{
						trt->trackPoint = pos;
						trt->lastTrackPtr = pb;
						trt->lastTrackTime = timeNow;
						ThinDrawer::object->registerDebugLine(glm::vec4(trt->eyePos.x, trt->eyePos.y, lookPos.x, lookPos.y), glm::vec4(1.0f, 1.0f, 1.0f, 1.0f), 0.03f, 0.5f);
					}
				}
			}
		}
	}

	for (size_t i = lastTurretUpdateIndex; i < numTurretUpdates + lastTurretUpdateIndex; i++)
	{
		Turret* trt = turrets.at(i % turrets.size());

		if (trt->wData->destrTime > 0.0f || trt->deactivated) continue;

		PortalBody* tPb = trt->eyeBody;

		for (Portal* p : Game::object->pWorld->portals)
		{
			if (p->connections->size() == 0 || p->getPointSide(trt->eyePos) != p->connections->at(0)->side1 ||
				trt->lastTrackPtr != p && trt->lastTrackTime + 0.3 > timeNow)
			{
				continue;
			}

			b2Vec2 pos2 = PortalWorld::getRelativePortalPositionReverse_W(pos, p->connections->at(0));
			b2Vec2 pos2Res = pos2;
			b2Vec2 upVec = pos2 - tPb->body->GetPosition();
			upVec = b2Vec2(-upVec.y, upVec.x); upVec.Normalize();
			pos2 = pos2 + sin(timeNow * 50.0f) * upVec;

			float cross1 = b2Cross(p->points[0] - trt->eyePos, pos2 - trt->eyePos);
			float cross2 = b2Cross(pos2 - trt->eyePos, p->points[1] - trt->eyePos);

			if (std::signbit(cross1) == std::signbit(cross2))
			{
				b2Vec2 diff = pos2 - trt->eyePos;
				float cosX = b2Dot(diff, trt->fwd) / diff.Length();

				if (cosX > minCosX)
				{
					pos2.x += upVec.y; pos2.y += -upVec.x;
					trt->pWorld->rayHandler->sendRay(trt->eyePos, pos2 - trt->eyePos, 100.0f);
					if (trt->pWorld->rayHandler->closestFixture)
					{
						bodyData* bData = (bodyData*)trt->pWorld->rayHandler->closestFixture->GetBody()->GetUserData().pointer;
						if (bData && bData->extraData && ((g_PhysicsData*)bData->extraData)->type & PLAYER_O)
						{
							trt->trackPoint = pos2Res;
							trt->lastTrackPtr = p;
							trt->lastTrackTime = timeNow;
							ThinDrawer::object->registerDebugLine(glm::vec4(trt->eyePos.x, trt->eyePos.y, pos2.x, pos2.y), glm::vec4(1.0f, 1.0f, 1.0f, 1.0f), 0.03f, 0.5f);
							break;
						}
					}
				}
			}
		}
	}

	lastTurretUpdateIndex += numTurretUpdates;
	if (turrets.size() > 0)
		lastTurretUpdateIndex %= turrets.size();
}