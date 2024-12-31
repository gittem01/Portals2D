#include "GameObjects/UseButton.h"
#include "Overlayer.h"

s_perTextureData* UseButton::ptData;
s_texture* UseButton::tex;
std::list<UseButton*> UseButton::checkButtons;

const float ySize = 1.35f;

b2Vec2 offsets[4] =
{
						b2Vec2(0.5f, ySize * 0.5f),
						b2Vec2(2.0f - ySize * 0.5f, 0.5f),
						b2Vec2(0.5f, 2.0f - ySize * 0.5f),
						b2Vec2(ySize * 0.5f, 0.5f)
};

b2Vec2 vertexPositions[4] =
{
	b2Vec2(+0.1f, -ySize * 0.5f),
	b2Vec2(+0.1f, +ySize * 0.5f),
	b2Vec2(-0.1f, +ySize * 0.5f),
	b2Vec2(-0.1f, -ySize * 0.5f),
};

float activeTimes[5] = { 2.0f, 4.0f, 8.0f, 16.0f, 3169140101.0f };

UseButton::UseButton(PortalWorld* portalWorld, s_objectDataGrid* objData)
{
	disableAfter = activeTimes[std::clamp((int)objData->speed, 0, (int)std::size(activeTimes))];

	b2BodyDef bDef;
	bDef.type = b2_staticBody;
	bDef.position = b2Vec2(objData->pos.x, objData->pos.y) + offsets[objData->rotation % 4];
	bDef.angle = objData->rotation * 0.5f * b2_pi;

	b2Body* body = portalWorld->CreateBody(&bDef);

	fixtureData* fData = new fixtureData();
	fData->type = F_USE_BUTTON;
	fData->data = (void*)this;
	fData->extraData = nullptr;

	for (int i = 0; i < 3; i++)
	{
		b2Vec2 pos = vertexPositions[i];
		b2Vec2 pos2 = vertexPositions[i + 1];
		b2EdgeShape shape;
		shape.SetTwoSided(pos, pos2);
		b2Fixture* fix = body->CreateFixture(&shape, 0.0f);
		//fix->SetSensor(true);

		fix->GetUserData().pointer = (uintptr_t)fData;
	}
}

void UseButton::enable()
{
	if (enabled) return;

	signalTime = Game::object->gameTime;
	checkButtons.push_back(this);

	Transmitter::enable();
}

void UseButton::toDeath()
{
	for (UseButton* button : checkButtons)
	{
		button->disable();
	}	
}

void UseButton::toLeveler()
{
	ThinDrawer* thinDrawer = Leveler::object->thinDrawer;

	if (thinDrawer->lineZ->numZs <= 0) return;

	s_lineZUbo* lineZUbo;
	vkMapMemory(thinDrawer->logicalDevice, thinDrawer->lineZ->uniformBuffers[0].memory, 0,
		thinDrawer->lineZ->numZs * sizeof(s_lineZUbo), 0, (void**)&lineZUbo);

	for (int i = 0; i < thinDrawer->lineZ->numZs; i++)
	{
		lineZUbo[i].colour = glm::vec4(0.0f, 0.5f, 1.0f, 1.0f);
	}

	vkUnmapMemory(thinDrawer->logicalDevice, thinDrawer->lineZ->uniformBuffers[0].memory);
}

void UseButton::staticInit()
{
	constexpr int index = getLevelObjectIndex(CLICK_BUTTON) - 4;

	ptData = &Leveler::object->perTextureData[index];
	tex = Leveler::object->ol->items[index + 4].tex;

	ThinDrawer::object->midDrawFuncs.push_back({ render, nullptr });
}

void UseButton::update()
{
	auto iter = checkButtons.begin();
	while (iter != checkButtons.end())
	{
		UseButton* button = *iter;
		if (Game::object->gameTime - button->signalTime > button->disableAfter)
		{
			button->signalTime = Game::object->gameTime;
			button->disable();
			iter = checkButtons.erase(iter);
		}
		else
		{
			iter++;
		}
	}
}

void UseButton::render(VkCommandBuffer cmdBuff, void* ptr)
{
	ManyTexture* mt = Leveler::object->ol->mt;
	ThinDrawer* td = Leveler::object->thinDrawer;

	const VkDeviceSize offsets = 0;

	if (ptData->currentOffset - ptData->offsetStart == 0)
	{
		return;
	}

	vkCmdBindPipeline(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS, mt->pipeline);

	vkCmdBindVertexBuffers(cmdBuff, 0, 1, &td->wallLight->buffers[0].buffer, &offsets);

	vkCmdBindDescriptorSets(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS,
		mt->pipelineLayout, 0, 1, &ShaderBase::sharedSet, 1, &td->camOffset);

	vkCmdBindDescriptorSets(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS,
		mt->pipelineLayout, 1, 1, &mt->descriptorSets[1], 0, VK_NULL_HANDLE);

	vkCmdBindDescriptorSets(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS,
		mt->pipelineLayout, 2, 1, &tex->set, 0, VK_NULL_HANDLE);

	vkCmdDraw(cmdBuff, td->wallLight->buffers[0].count, ptData->currentOffset - ptData->offsetStart, 0, ptData->offsetStart);
}