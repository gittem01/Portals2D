#include <DefaultObjects/Turret.h>
#include <ThinDrawer.h>
#include <Portal.h>
#include <Game.h>

std::set<Turret*> Turret::turrets;
s_texture* Turret::coverTex;
s_texture* Turret::gunTex;
s_texture* Turret::glowTex;
s_buffers* Turret::coverVertexBuffer;
std::vector<uint32_t> Turret::soundBuffers;

Turret::Turret(PortalWorld* portalWorld) : DefaultObject(portalWorld)
{
	createBody();

	renderLaserDatas.resize(numLasers);

	turrets.insert(this);
}

void Turret::createBody()
{
	float ySize = 1.6967984934086628;
	float r = 0.15;
	b2Vec2 legPoints0[4] =
	{
		b2Vec2(-0.193, -0.19 * ySize),
		b2Vec2(-0.49, -0.35 * ySize),
		b2Vec2(-0.43, -0.38 * ySize),
		b2Vec2(-0.157, -0.22 * ySize)
	};
	b2Vec2 legPoints1[4] =
	{
		b2Vec2(-0.49, -0.35 * ySize),
		b2Vec2(-0.5, -0.5 * ySize),
		b2Vec2(-0.42, -0.5 * ySize),
		b2Vec2(-0.43, -0.36 * ySize)
	};

	b2Vec2 legPoints2[4] =
	{
		b2Vec2(0.1, -0.19 * ySize),
		b2Vec2(0.1, -0.23 * ySize),
		b2Vec2(0.23, -0.2 * ySize),
		b2Vec2(0.22, -0.16 * ySize)
	};
	b2Vec2 legPoints3[4] =
	{
		b2Vec2(0.22, -0.16 * ySize),
		b2Vec2(0.22, -0.2 * ySize),
		b2Vec2(0.44, -0.255 * ySize),
		b2Vec2(0.49, -0.23 * ySize)
	};
	b2Vec2 legPoints4[4] =
	{
		b2Vec2(0.44, -0.245 * ySize),
		b2Vec2(0.43, -0.5 * ySize),
		b2Vec2(0.5, -0.5 * ySize),
		b2Vec2(0.49, -0.23 * ySize)
	};

	b2Vec2 bodyPoints[8] =
	{
		b2Vec2(0.1, -0.27 * ySize),
		b2Vec2(0.24, -0.05 * ySize),
		b2Vec2(0.24, 0.22 * ySize),
		b2Vec2(0.1, 0.37 * ySize),
		b2Vec2(-0.195, 0.37 * ySize),
		b2Vec2(-0.284, 0.22 * ySize),
		b2Vec2(-0.304, -0.02 * ySize),
		b2Vec2(-0.194, -0.27 * ySize)
	};

	midX = -0.045;
	b2Vec2 bottomCirclePos = b2Vec2(midX, -0.235 * ySize);
	b2Vec2 topCirclePos = b2Vec2(midX, ySize - r - 0.1 - 0.5 * ySize);

	b2BodyDef bodyDef;
	bodyDef.type = b2_dynamicBody;
	bodyDef.position = b2Vec2(0.0, 0.0);

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

	PortalBody* portalBody = pWorld->createPortalBody(body);
	pBody = &portalBody->worldIndex->pbIndices;

	bodyData* bData = (bodyData*)body->GetUserData().pointer;
	g_PhysicsData* physicsData = (g_PhysicsData*)malloc(sizeof(g_PhysicsData));
	physicsData->type = (g_DataType)(DEFAULT_OBJECT_O | TURRET_O);
	physicsData->data = this;
	bData->extraData = physicsData;

	bodyFixDataIndex = 2;

	ThinDrawer::object->addPortalBody(portalBody, { "turretBody.png" });
	ThinDrawer::object->debug_addPortalBody(portalBody);
}

void Turret::staticInit()
{
	coverTex = IOHelper::loadTexture("turretCover.png", VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER);
	gunTex = IOHelper::loadTexture("turretGun.png", VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER);
	glowTex = IOHelper::loadTexture("glow.png", VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER);

	std::vector<s_vertex> coverVertices = 
	{
		{ { -0.5f, +0.5f }, { 0.0f, 1.0f } },
		{ { -0.5f, -0.5f }, { 0.0f, 0.0f } },
		{ { +0.5f, -0.5f }, { 1.0f, 0.0f } },
		
		{ { +0.5f, -0.5f }, { 1.0f, 0.0f } },
		{ { +0.5f, +0.5f }, { 1.0f, 1.0f } },
		{ { -0.5f, +0.5f }, { 0.0f, 1.0f } },
	};

	coverVertexBuffer = (s_buffers*)malloc(sizeof(s_buffers));
	coverVertexBuffer->count = coverVertices.size();
	ThinDrawer::object->bufferStage(coverVertices.data(), coverVertices.size() * sizeof(s_vertex), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, coverVertexBuffer);

	ThinDrawer::object->drawFuncs.push_back({ draw, nullptr });

	ALuint buffer = Sounder::object->loadSound("firee.wav");
	soundBuffers.push_back(buffer);
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
	if (ThinDrawer::object->gameTime - lastShootTime > shootTimer && openAmount > 0.999f)
	{
		ALuint source = Sounder::object->playSound(soundBuffers[0], 0.2 + 0.1 * rand() / (float)RAND_MAX);
		Sounder::object->setSoundPosition(source, glm::vec3(pBody->at(0)->body->GetPosition().x, pBody->at(0)->body->GetPosition().y, 0.0));

		lastShootTime = ThinDrawer::object->gameTime;
		shootTimes[shootGundex] = ThinDrawer::object->gameTime;
		

		float mirrMult = pBody->at(0)->isMirrored ? -1.0f : 1.0f;
		float effectiveRotation = rotation * mirrMult;
		glm::vec2 center = glm::vec2((midX + openAmount * 0.05) * mirrMult, 0.115f);

		b2Vec2 bPos = b2Vec2(-0.31 * mirrMult, 0.24 - shootGundex * 0.24);
		
		glm::vec2 res = rotateArroundPoint(center, *(glm::vec2*)&bPos, effectiveRotation);
		b2Vec2 fwdMov = b2Vec2(bPos.x - 100.0f * mirrMult, bPos.y);
		bPos = b2Vec2(res.x, res.y);
		res = rotateArroundPoint(glm::vec2(0.0f, 0.0f), *(glm::vec2*)&fwdMov, effectiveRotation + 0.2f * rand() / (float)RAND_MAX - 0.1f);
		fwdMov = b2Vec2(res.x, res.y);

		bPos = pBody->at(0)->body->GetWorldPoint(bPos);
		fwdMov = pBody->at(0)->body->GetWorldPoint(fwdMov);

		s_laserUbo laserData;
		laserData.colour = glm::vec4(0.8f, 0.8f, 0.0f, 1.0f);
		laserData.points = glm::vec4(bPos.x, bPos.y, fwdMov.x, fwdMov.y);
		laserData.data = glm::vec4(0.25f, 1.5f, 1.0f, ThinDrawer::object->gameTime);

		renderLaserDatas[lastLaserIndex % numLasers] = { laserData, shootGundex };
		lastLaserIndex++;

		shootGundex = (shootGundex + 1) % 2;
	}
}

void Turret::draw(VkCommandBuffer cmdBuff, void* ptr)
{
	ThinDrawer* drawer = ThinDrawer::object;

	VkDeviceSize offsets = 0;

	for (auto trt : turrets)
	{
		//trt->shoot();
		for (int i = 0; i < 2; i++)
		{
			double td = ThinDrawer::object->gameTime - trt->shootTimes[i];
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

		const uint32_t camOffset = ShaderBase::alignedSharedSize * drawer->lastSwapChainImageIndex;
		vkCmdBindDescriptorSets(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS,
			drawer->bodyShader->pipelineLayout, 0, 1, &ShaderBase::sharedSet, 1, &camOffset);

		vkCmdBindDescriptorSets(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS,
			drawer->bodyShader->pipelineLayout, 1, 1, &trt->coverTex->set, 0, VK_NULL_HANDLE);

		vkCmdBindVertexBuffers(cmdBuff, 0, 1, &trt->coverVertexBuffer->buffer, &offsets);

		/*trt->openAmount = sin(drawer->gameTime * 3.0f) * 0.5f + 0.5f;
		trt->openAmount = 1.0f;
		trt->rotation = sin(drawer->gameTime * 1.0f) * 0.5f;
		trt->rotation *= trt->openAmount;*/

		for (auto pBody : *trt->pBody)
		{
			s_renderBodyPsh pshData;
			pshData.data.y = -20.0f;

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
					portalCollision* pc = pbfData->pCollisions[i];
					if (pc->status < 0)
					{
						renderDisabled = true; i++;
						break;
					}

					pshData.portals[i] = glm::vec4(
						pc->portal->points[pc->side ^ 1].x, pc->portal->points[pc->side ^ 1].y,
						pc->portal->points[pc->side].x, pc->portal->points[pc->side].y);
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

				float timeMult = sin(drawer->gameTime * 3.0f) * 0.1f + 0.1f + 1.0f;
				pshData.posRot.z = pBody->body->GetAngle();
				pshData.movScale = glm::vec4(-0.23f * mirrMult, 0.11f, (0.3f * (1.0f + trt->openAmount * 0.3)) * timeMult, (0.3f * (1.0f + trt->openAmount * 0.3)) * timeMult);

				vkCmdBindDescriptorSets(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS,
					drawer->bodyShader->pipelineLayout, 1, 1, &trt->glowTex->set, 0, VK_NULL_HANDLE);

				vkCmdPushConstants(cmdBuff, drawer->bodyShader->pipelineLayout,
					VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(s_renderBodyPsh), &pshData);

				vkCmdDraw(cmdBuff, 6, 1, 0, 0);
			}
		}

		float mirrMult = trt->pBody->at(0)->isMirrored ? -1.0f : 1.0f;
		float effectiveRotation = trt->rotation * mirrMult;

		vkCmdBindPipeline(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS, drawer->laserPshShader->pipeline);

		vkCmdBindDescriptorSets(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS,
			drawer->laserPshShader->pipelineLayout, 0, 1, &ShaderBase::sharedSet, 1, &camOffset);

		vkCmdBindVertexBuffers(cmdBuff, 0, 1, &drawer->playerShader->buffers[0]->buffer, &offsets);

		b2Vec2 bPos = b2Vec2(mirrMult * -0.23f, 0.11f);
		b2Vec2 fwdMov = b2Vec2(bPos.x - 100.0f * mirrMult, bPos.y);
		glm::vec2 res = rotateArroundPoint(glm::vec2(0.0f, 0.0f), *(glm::vec2*)&fwdMov, effectiveRotation);
		fwdMov = b2Vec2(res.x, res.y);

		bPos = trt->pBody->at(0)->body->GetWorldPoint(bPos);
		fwdMov = trt->pBody->at(0)->body->GetWorldPoint(fwdMov);

		s_laserUbo laserData;
		laserData.colour = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);
		laserData.points = glm::vec4(bPos.x, bPos.y, fwdMov.x, fwdMov.y);
		laserData.data = glm::vec4(0.2f, 1.5f, 0.5f, 0.0f);
		vkCmdPushConstants(cmdBuff, drawer->laserPshShader->pipelineLayout,
			VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(s_laserUbo), &laserData);

		vkCmdDraw(cmdBuff, 6, 1, 0, 0);

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

				bPos = trt->pBody->at(0)->body->GetWorldPoint(bPos);
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

void Turret::update()
{
	for (auto trt : turrets)
	{
		//

		if (ThinDrawer::object->wh->keyData[GLFW_KEY_W] == 2)
		{
			trt->status = true;
		}
		if (ThinDrawer::object->wh->keyData[GLFW_KEY_S] == 2)
		{
			trt->status = false;
		}
		if (ThinDrawer::object->wh->keyData[GLFW_KEY_A])
		{
			trt->rotation -= Game::object->deltaTime;
			trt->rotation = std::max(trt->rotation, -rotationLimit);

		}
		if (ThinDrawer::object->wh->keyData[GLFW_KEY_D])
		{
			trt->rotation += Game::object->deltaTime;
			trt->rotation = std::min(trt->rotation, rotationLimit);
		}
		if (ThinDrawer::object->wh->keyData[GLFW_KEY_SPACE])
		{
			trt->shoot();
		}

		//
		if (trt->status)
		{
			trt->openAmount += 0.01f;
			trt->openAmount = std::min(trt->openAmount, 1.0f);
		}
		else
		{
			trt->openAmount -= 0.01f;
			trt->openAmount = std::max(trt->openAmount, 0.0f);
		}
	}
}