#include "GameObjects/CubeButton.h"
#include "Overlayer.h"

#define ENABLERS (LASER_CUBE_O | DEFAULT_CUBE_O)

std::vector<CubeButton*> CubeButton::cubeButtons;
s_texture* CubeButton::buttonTex;
s_texture* CubeButton::bodyTex;

static b2Vec2 buttonOffsets[4] =
{
						b2Vec2(1.0f, 0.2f),
						b2Vec2(0.8f, 1.0f),
						b2Vec2(1.0f, 0.8f),
						b2Vec2(0.2f, 1.0f),
};

static b2Vec2 bodyOffsets[4] =
{
						b2Vec2(1.0f, 0.1f),
						b2Vec2(0.9f, 1.0f),
						b2Vec2(1.0f, 0.9f),
						b2Vec2(0.1f, 1.0f),
};

static b2Vec2 scale = b2Vec2(0.5f, 0.1f);

static b2Vec2 buttonVertices[12] =
{
	b2Vec2(+0.5f,	-0.02f),
	b2Vec2(+0.5f,	+0.1f),
	b2Vec2(+0.43f,	+0.1f),
	b2Vec2(+0.36f,	+0.03f),
	b2Vec2(+0.11f,	+0.03f),
	b2Vec2(+0.03f,	+0.09f),

	b2Vec2(-0.03f,	+0.09f),
	b2Vec2(-0.11f,	+0.03f),
	b2Vec2(-0.36f,	+0.03f),
	b2Vec2(-0.43f,	+0.1f),
	b2Vec2(-0.5f,	+0.1f),
	b2Vec2(-0.5f,	-0.02f),
};

static b2Vec2 bodyVertices[3] =
{
	b2Vec2(0.8f,	-0.1f),
	b2Vec2(0.6f,	+0.1f),
	b2Vec2(0.475f,	+0.1f),
};

static b2Vec2 fwdDirs[4] =
{
	b2Vec2(0.0f, 1.0f),
	b2Vec2(-1.0f, 0.0f),
	b2Vec2(0.0f, -1.0f),
	b2Vec2(+1.0f, 0.0f)
};


CubeButton::CubeButton(PortalWorld* portalWorld, s_objectDataGrid* objData)
{
	b2BodyDef bDef;
	bDef.type = b2_kinematicBody;
	bDef.position = b2Vec2(objData->pos.x, objData->pos.y) + buttonOffsets[objData->rotation % 4];
	bDef.angle = objData->rotation * 0.5f * b2_pi;

	button = portalWorld->CreateBody(&bDef);
	initialPos = bDef.position;
	fwdDir = fwdDirs[objData->rotation % 4];
	contactReportTime = -10.0;

	for (int i = 0; i < 11; i++)
	{
		b2Vec2 v0 = buttonVertices[i];
		b2Vec2 v1 = buttonVertices[(i + 1) % 12];

		b2EdgeShape edge;
		edge.m_vertex1 = v0;
		edge.m_vertex2 = v1;

		b2Fixture* fix = button->CreateFixture(&edge, 0.0f);
		fix->SetRestitution(0.0f);
	}

	buttonRenderData.posRot = glm::vec4(bDef.position.x, bDef.position.y, bDef.angle, 1.0f);
	buttonRenderData.movScale = glm::vec4(0.0f, 0.0f, scale.x * 2.0f, scale.y * 2.0f);
	buttonRenderData.data.x = 0.0f; buttonRenderData.data.y = -20.0f;
	buttonRenderData.data.z = -10.0;

	bDef.type = b2_staticBody;
	bDef.position = b2Vec2(objData->pos.x, objData->pos.y) + bodyOffsets[objData->rotation % 4];

	b2Body* body = portalWorld->CreateBody(&bDef);

	for (int i = 0; i < 2; i++)
	{
		b2EdgeShape edge;

		edge.m_vertex1 = bodyVertices[i];
		edge.m_vertex2 = bodyVertices[i + 1];
		body->CreateFixture(&edge, 0.0f);

		edge.m_vertex1 = bodyVertices[i]; edge.m_vertex1.x *= -1.0f;
		edge.m_vertex2 = bodyVertices[i + 1]; edge.m_vertex2.x *= -1.0f;
		body->CreateFixture(&edge, 0.0f);
	}

	bodyRenderData.posRot = glm::vec4(bDef.position.x, bDef.position.y, bDef.angle, 1.0f);
	bodyRenderData.movScale = glm::vec4(0.0f, 0.0f, 1.6f, 0.2f);
	bodyRenderData.data.x = 0.0f; bodyRenderData.data.y = -20.0f;
	bodyRenderData.data.z = -10.0;

	cubeButtons.push_back(this);
}

void CubeButton::toDeath()
{
	for (CubeButton* db : cubeButtons)
	{
		db->disable();

		db->button->SetTransform(db->initialPos, 0.0f);
		db->button->SetLinearVelocity(b2Vec2(0.0f, 0.0f));
	}
}

void CubeButton::toLeveler()
{
	CubeButton::cubeButtons.clear();
}

void CubeButton::staticInit()
{
	bodyTex = IOHelper::loadTexture("downButtonBody.png", VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);
	buttonTex = IOHelper::loadTexture("cubeButtonButton.png", VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);

	ThinDrawer::object->midDrawFuncs.push_back({ render, nullptr });
}

void CubeButton::update()
{
	for (CubeButton* db : cubeButtons)
	{
		b2Body* body = db->button;

		bool down = false;

		for (b2ContactEdge* contactEdge = body->GetContactList(); contactEdge; contactEdge = contactEdge->next)
		{
			b2Contact* contact = contactEdge->contact;
			if (!contact->IsTouching()) continue;

			b2Body* otherBody = contact->GetFixtureA()->GetBody() == body ? contact->GetFixtureB()->GetBody() : contact->GetFixtureA()->GetBody();

			bodyData* bdData = (bodyData*)otherBody->GetUserData().pointer;
			if (!bdData) continue;

			g_PhysicsData* pData = (g_PhysicsData*)bdData->extraData;
			if (!pData || !(pData->type & ENABLERS))
			{
				continue;
			}

			b2Vec2 posDiff = otherBody->GetPosition() - db->initialPos;
			b2Vec2 rightFwdDir = b2Vec2(-db->fwdDir.y, db->fwdDir.x);

			if (abs(b2Dot(posDiff, rightFwdDir)) > 0.7f) continue;

			body->SetLinearVelocity(0.25f * -db->fwdDir);

			down = true;
			db->contactReportTime = Game::object->gameTime;

			break;
		}

		if (!down && Game::object->gameTime - db->contactReportTime > 0.25)
		{
			body->SetLinearVelocity(b2Vec2(0.25f * db->fwdDir));
		}

		float moveDir = b2Dot(body->GetLinearVelocity(), db->fwdDir);
		float posAmnt = b2Dot(db->initialPos - body->GetPosition(), db->fwdDir);

		if ((body->GetPosition() - db->initialPos).LengthSquared() > 0.006 && moveDir < -0.1f ||
			posAmnt < 0.0f && moveDir > 0.1f)
		{
			body->SetLinearVelocity(b2Vec2(0.0f, 0.0f));
		}

		if (posAmnt > 0.07)
		{
			db->enable();
		}
		else if (posAmnt < 0.001f)
		{
			db->disable();
		}
	}
}

void CubeButton::render(VkCommandBuffer cmdBuff, void* ptr)
{
	ThinDrawer* td = Leveler::object->thinDrawer;

	const VkDeviceSize offsets = 0;

	vkCmdBindPipeline(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS, td->bodyShader->pipeline);

	vkCmdBindDescriptorSets(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS,
		td->bodyShader->pipelineLayout, 0, 1, &ShaderBase::sharedSet, 1, &td->camOffset);

	vkCmdBindVertexBuffers(cmdBuff, 0, 1, &Turret::coverVertexBuffer->buffer, &offsets);

	vkCmdBindDescriptorSets(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS,
		td->bodyShader->pipelineLayout, 1, 1, &buttonTex->set, 0, VK_NULL_HANDLE);

	for (CubeButton* button : cubeButtons)
	{
		b2Vec2 bodyPos = button->button->GetPosition();
		button->buttonRenderData.posRot.x = bodyPos.x;
		button->buttonRenderData.posRot.y = bodyPos.y;

		vkCmdPushConstants(cmdBuff, td->bodyShader->pipelineLayout,
			VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(s_renderBodyPsh), &button->buttonRenderData);

		vkCmdDraw(cmdBuff, 6, 1, 0, 0);
	}

	vkCmdBindDescriptorSets(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS,
		td->bodyShader->pipelineLayout, 1, 1, &bodyTex->set, 0, VK_NULL_HANDLE);

	for (CubeButton* button : cubeButtons)
	{
		vkCmdPushConstants(cmdBuff, td->bodyShader->pipelineLayout,
			VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(s_renderBodyPsh), &button->bodyRenderData);

		vkCmdDraw(cmdBuff, 6, 1, 0, 0);
	}
}