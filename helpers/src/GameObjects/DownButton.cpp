#include "GameObjects/DownButton.h"
#include "Overlayer.h"

#define ENABLERSINV (TURRET_O | GEL_O)

std::vector<DownButton*> DownButton::downButtons;
s_texture* DownButton::buttonTex;
s_texture* DownButton::bodyTex;

b2Vec2 buttonOffsets[4] =
{
						b2Vec2(1.0f, 0.2f),
						b2Vec2(0.8f, 1.0f),
						b2Vec2(1.0f, 0.8f),
						b2Vec2(0.2f, 1.0f),
};

b2Vec2 bodyOffsets[4] =
{
						b2Vec2(1.0f, 0.1f),
						b2Vec2(0.9f, 1.0f),
						b2Vec2(1.0f, 0.9f),
						b2Vec2(0.1f, 1.0f),
};

b2Vec2 bodyVertices[4] =
{
	b2Vec2(-0.8f, -0.1f),
	b2Vec2(+0.8f, -0.1f),
	b2Vec2(+0.6f, 0.1f),
	b2Vec2(-0.6f, 0.1f)
};

static b2Vec2 fwdDirs[4] =
{
	b2Vec2(0.0f, 1.0f),
	b2Vec2(-1.0f, 0.0f),
	b2Vec2(0.0f, -1.0f),
	b2Vec2(+1.0f, 0.0f)
};

DownButton::DownButton(PortalWorld* portalWorld, s_objectDataGrid* objData)
{
	b2BodyDef bDef;
	bDef.type = b2_kinematicBody;
	bDef.position = b2Vec2(objData->pos.x, objData->pos.y) + buttonOffsets[objData->rotation % 4];
	bDef.angle = objData->rotation * 0.5f * b2_pi;

	button = portalWorld->CreateBody(&bDef);
	initialPos = bDef.position;
	fwdDir = fwdDirs[objData->rotation % 4];
	contactReportTime = -10.0;

	b2Vec2 scale = b2Vec2(0.5f, 0.1f);
	b2PolygonShape shape;
	shape.SetAsBox(scale.x, scale.y);

	b2Fixture* fix = button->CreateFixture(&shape, 0.0f);

	buttonRenderData.posRot = glm::vec4(bDef.position.x, bDef.position.y, bDef.angle, 1.0f);
	buttonRenderData.movScale = glm::vec4(0.0f, 0.0f, scale.x * 2.0f, scale.y * 2.0f);
	buttonRenderData.data.x = 0.0f; buttonRenderData.data.y = -20.0f;
	buttonRenderData.data.z = -10.0;

	bDef.type = b2_staticBody;
	bDef.position = b2Vec2(objData->pos.x, objData->pos.y) + bodyOffsets[objData->rotation % 4];
	
	b2Body* body = portalWorld->CreateBody(&bDef);

	shape.Set(bodyVertices, 4);

	fix = body->CreateFixture(&shape, 0.0f);

	bodyRenderData.posRot = glm::vec4(bDef.position.x, bDef.position.y, bDef.angle, 1.0f);
	bodyRenderData.movScale = glm::vec4(0.0f, 0.0f, 1.6f, 0.2f);
	bodyRenderData.data.x = 0.0f; bodyRenderData.data.y = -20.0f;
	bodyRenderData.data.z = -10.0;

	downButtons.push_back(this);
}

void DownButton::toDeath()
{
	for (DownButton* db : downButtons)
	{
		db->disable();

		db->button->SetTransform(db->initialPos, 0.0f);
		db->button->SetLinearVelocity(b2Vec2(0.0f, 0.0f));
	}
}

void DownButton::toLeveler()
{
	DownButton::downButtons.clear();
}

void DownButton::staticInit()
{
	bodyTex = IOHelper::loadTexture("downButtonBody.png", VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);
	buttonTex = IOHelper::loadTexture("DownButtonButton.png", VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);

	ThinDrawer::object->midDrawFuncs.push_back({ render, nullptr });
}

void DownButton::update()
{
	for (DownButton* db : downButtons)
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
			if (!pData || pData->type & ENABLERSINV)
			{
				continue;
			}

			b2Vec2 posDiff = otherBody->GetPosition() - db->initialPos;
			b2Vec2 rightFwdDir = b2Vec2(-db->fwdDir.y, db->fwdDir.x);

			float maxX = pData->type & PLAYER_O ? 0.6f : 0.7f;

			if (abs(b2Dot(posDiff, rightFwdDir)) > maxX) continue;

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

		if ((body->GetPosition() - db->initialPos).LengthSquared() > 0.008 && moveDir < -0.1f ||
			posAmnt < 0.0f && moveDir > 0.1f)
		{
			body->SetLinearVelocity(b2Vec2(0.0f, 0.0f));
		}

		if (posAmnt > 0.08)
		{
			if (!db->enabled)
			{
				Transmitter::enableSound(body->GetPosition());
			}

			db->enable();
		}
		else if (posAmnt < 0.001f)
		{
			if (db->enabled)
			{
				Transmitter::disableSound(body->GetPosition());
			}

			db->disable();
		}
	}
}

void DownButton::render(VkCommandBuffer cmdBuff, void* ptr)
{
	ThinDrawer* td = Leveler::object->thinDrawer;

	const VkDeviceSize offsets = 0;

	vkCmdBindPipeline(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS, td->bodyShader->pipeline);

	vkCmdBindDescriptorSets(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS,
		td->bodyShader->pipelineLayout, 0, 1, &ShaderBase::sharedSet, 1, &td->camOffset);

	vkCmdBindVertexBuffers(cmdBuff, 0, 1, &Turret::coverVertexBuffer->buffer, &offsets);

	vkCmdBindDescriptorSets(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS,
		td->bodyShader->pipelineLayout, 1, 1, &buttonTex->set, 0, VK_NULL_HANDLE);

	for (DownButton* button : downButtons)
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

	for (DownButton* button : downButtons)
	{
		vkCmdPushConstants(cmdBuff, td->bodyShader->pipelineLayout,
			VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(s_renderBodyPsh), &button->bodyRenderData);

		vkCmdDraw(cmdBuff, 6, 1, 0, 0);
	}
}