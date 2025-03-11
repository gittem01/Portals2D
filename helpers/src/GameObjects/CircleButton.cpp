#include "GameObjects/CircleButton.h"
#include "Overlayer.h"

std::vector<CircleButton*> CircleButton::circleButtons;
s_texture* CircleButton::bodyTex;

static b2Vec2 bodyOffsets[4] =
{
						b2Vec2(1.0f, 0.085f),
						b2Vec2(0.915f, 1.0f),
						b2Vec2(1.0f, 0.915f),
						b2Vec2(0.085f, 1.0f),
};

static b2Vec2 bodyVertices[7] =
{
	b2Vec2(0.775f,	-0.08f),
	b2Vec2(0.65f,	+0.09f),
	b2Vec2(0.6f,	+0.09f),

	b2Vec2(0.5f,	+0.06f),
	b2Vec2(0.4f,	+0.02f),
	b2Vec2(0.3f,	-0.01f),
	b2Vec2(0.1f,	-0.04f),
};

static b2Vec2 rightDirs[2] =
{
	b2Vec2(1.0f, 0.0f),
	b2Vec2(0.0f, 1.0f),
};

CircleButton::CircleButton(PortalWorld* portalWorld, s_objectDataGrid* objData)
{
	b2BodyDef bDef;
	bDef.type = b2_kinematicBody;
	bDef.position = b2Vec2(objData->pos.x, objData->pos.y) + bodyOffsets[objData->rotation % 4];
	bDef.angle = objData->rotation * 0.5f * b2_pi;

	contactReportTime = -10.0;

	bDef.type = b2_staticBody;
	bDef.position = b2Vec2(objData->pos.x, objData->pos.y) + bodyOffsets[objData->rotation % 4];

	body = portalWorld->CreateBody(&bDef);
	rightDir = rightDirs[objData->rotation % 2];

	const int vSize = std::size(bodyVertices);
	for (int i = 0; i < vSize - 1; i++)
	{
		b2EdgeShape edge;

		edge.m_vertex1 = bodyVertices[i];
		edge.m_vertex2 = bodyVertices[i + 1];
		body->CreateFixture(&edge, 0.0f);

		edge.m_vertex2 = bodyVertices[i]; edge.m_vertex2.x *= -1.0f;
		edge.m_vertex1 = bodyVertices[i + 1]; edge.m_vertex1.x *= -1.0f;
		body->CreateFixture(&edge, 0.0f);
	}

	b2EdgeShape edge;

	edge.m_vertex1 = bodyVertices[vSize - 1];
	edge.m_vertex2 = bodyVertices[vSize - 1]; edge.m_vertex2.x *= -1.0f;
	body->CreateFixture(&edge, 0.0f);

	bodyRenderData.posRot = glm::vec4(bDef.position.x, bDef.position.y, bDef.angle, 1.0f);
	bodyRenderData.movScale = glm::vec4(0.0f, 0.0f, 1.6f, 0.2f);
	bodyRenderData.data.x = 0.0f; bodyRenderData.data.y = -20.0f;
	bodyRenderData.data.z = -10.0;

	lastEnabledSphere = nullptr;

	circleButtons.push_back(this);
}

void CircleButton::toDeath()
{
	for (CircleButton* cb : circleButtons)
	{
		cb->disable();

		if (cb->lastEnabledSphere)
			cb->lastEnabledSphere->renderGroup->currentImgIndex = 0;
	}
}

void CircleButton::toLeveler()
{
	CircleButton::circleButtons.clear();
}

void CircleButton::staticInit()
{
	bodyTex = IOHelper::loadTexture("circleButton.png", VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);

	ThinDrawer::object->midDrawFuncs.push_back({ render, nullptr });
}

void CircleButton::update(double dt)
{
	for (CircleButton* cb : circleButtons)
	{
		b2Body* body = cb->body;

		bool down = false;

		if (cb->lastEnabledSphere)
			cb->lastEnabledSphere->renderGroup->currentImgIndex = 0;

		for (b2ContactEdge* contactEdge = body->GetContactList(); contactEdge; contactEdge = contactEdge->next)
		{
			b2Contact* contact = contactEdge->contact;
			if (!contact->IsTouching()) continue;

			b2Body* otherBody = contact->GetFixtureA()->GetBody() == body ? contact->GetFixtureB()->GetBody() : contact->GetFixtureA()->GetBody();

			bodyData* bdData = (bodyData*)otherBody->GetUserData().pointer;
			if (!bdData) continue;

			g_PhysicsData* pData = (g_PhysicsData*)bdData->extraData;
			if (!pData || !(pData->type & SPHERE_CUBE_O))
			{
				continue;
			}

			b2Vec2 posDiff = otherBody->GetPosition() - body->GetPosition();

			if (abs(b2Dot(posDiff, cb->rightDir)) > 0.5f) continue;

			down = true;
			if (!cb->enabled)
			{
				Transmitter::enableSound(body->GetPosition());
			}

			cb->enable();
			
			float angVel = otherBody->GetAngularVelocity();
			angVel -= angVel * dt * 20.0f;
			otherBody->SetAngularVelocity(angVel);
			
			b2Vec2 linVel = otherBody->GetLinearVelocity();
			float speed = linVel.Normalize();
			speed -= speed * dt * 20.0f;
			linVel = speed * linVel;
			//otherBody->SetLinearVelocity(linVel);
			

			cb->contactReportTime = Game::object->gameTime;

			cb->lastEnabledSphere = (Sphere*)pData->data;
			cb->lastEnabledSphere->renderGroup->currentImgIndex = 1;

			break;
		}

		if (!down && Game::object->gameTime - cb->contactReportTime > 0.3)
		{
			if (cb->enabled)
			{
				Transmitter::disableSound(body->GetPosition());
			}

			cb->disable();

			cb->lastEnabledSphere = nullptr;
		}
	}
}

void CircleButton::render(VkCommandBuffer cmdBuff, void* ptr)
{
	ThinDrawer* td = Leveler::object->thinDrawer;

	const VkDeviceSize offsets = 0;

	vkCmdBindPipeline(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS, td->bodyShader->pipeline);

	vkCmdBindDescriptorSets(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS,
		td->bodyShader->pipelineLayout, 0, 1, &ShaderBase::sharedSet, 1, &td->camOffset);

	vkCmdBindVertexBuffers(cmdBuff, 0, 1, &Turret::coverVertexBuffer->buffer, &offsets);

	vkCmdBindDescriptorSets(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS,
		td->bodyShader->pipelineLayout, 1, 1, &bodyTex->set, 0, VK_NULL_HANDLE);

	for (CircleButton* button : circleButtons)
	{
		vkCmdPushConstants(cmdBuff, td->bodyShader->pipelineLayout,
			VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(s_renderBodyPsh), &button->bodyRenderData);

		vkCmdDraw(cmdBuff, 6, 1, 0, 0);
	}
}