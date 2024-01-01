#include <DefaultObjects/Turret.h>
#include <ThinDrawer.h>
#include <Portal.h>

Turret::Turret(PortalWorld* portalWorld) : DefaultObject(portalWorld)
{
	createBody();
	createRenderings();
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

	bodyFixDataIndex = 2;

	ThinDrawer::object->addPortalBody(portalBody, { "turretBody.png" });
	ThinDrawer::object->debug_addPortalBody(portalBody);
}

void Turret::createRenderings()
{
	coverTex = IOHelper::loadTexture("turretCover.png", VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER);
	gunTex = IOHelper::loadTexture("turretGun.png", VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER);

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

	ThinDrawer::object->drawFuncs.push_back({ draw, this });
}

void Turret::shoot()
{
	if (ThinDrawer::object->gameTime - lastShootTime > shootTimer)
	{
		lastShootTime = ThinDrawer::object->gameTime;
		shootTimes[shootGundex] = ThinDrawer::object->gameTime;
		shootGundex = (shootGundex + 1) % 2;
	}
}

glm::vec2 rotateArroundPoint(glm::vec2 c, glm::vec2 p, float angle)
{
	glm::vec2 v = p - c;
	float s = sin(angle);
	float c2 = cos(angle);
	return glm::vec2(c.x + v.x * c2 - v.y * s, c.y + v.x * s + v.y * c2);
}

void Turret::draw(VkCommandBuffer cmdBuff, void* ptr)
{
	Turret* trt = (Turret*)ptr;
	ThinDrawer* drawer = ThinDrawer::object;

	VkDeviceSize offsets = 0;

	trt->shoot();
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

	trt->openAmount = sin(drawer->gameTime * 3.0f) * 0.5f + 0.5f;
	trt->openAmount = 1.0f;
	trt->rotation = sin(drawer->gameTime * 1.0f) * 0.5f;
	trt->rotation *= trt->openAmount;

	for (auto pBody : *trt->pBody)
	{
		s_renderBodyPsh pshData;

		float mirrMult = pBody->isMirrored ? -1.0f : 1.0f;
		float effectiveRotation = trt->rotation * mirrMult;
		pshData.numPortals;
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

			pshData.numPortals = pbfData->pCollisions.size();
		}
		else
		{
			pshData.numPortals = 0;
		}

		if (!renderDisabled)
		{
			glm::vec2 center = glm::vec2((trt->midX + trt->openAmount * 0.05) * mirrMult, 0.115f);
			vkCmdBindVertexBuffers(cmdBuff, 0, 1, &trt->coverVertexBuffer->buffer, &offsets);

			vkCmdBindDescriptorSets(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS,
				drawer->bodyShader->pipelineLayout, 1, 1, &trt->gunTex->set, 0, VK_NULL_HANDLE);

			glm::vec2 newCenter = rotateArroundPoint(glm::vec2(0.0f, 0.0f), center, -effectiveRotation);

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
		}
	}
}