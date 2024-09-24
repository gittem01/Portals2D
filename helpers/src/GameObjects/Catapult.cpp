#include "GameObjects/Catapult.h"
#include "Overlayer.h"
#include "PolynomialShader.h"

#define JUMPERS (TURRET_O | PLAYER_O | SPHERE_CUBE_O | LASER_CUBE_O | DEFAULT_CUBE_O)

std::vector<Catapult*> Catapult::movingCatapults;
s_texture* Catapult::headTex;
s_texture* Catapult::bodyTex;
s_perTextureData* Catapult::ptData;

static b2Vec2 bodyOffsets[4] =
{
						b2Vec2(0.5f, 0.0f),
						b2Vec2(1.0f, 0.5f),
						b2Vec2(0.5f, 1.0f),
						b2Vec2(0.0f, 0.5f),
};

static b2Vec2 fwdDirs[4] =
{
	b2Vec2(0.0f, 1.0f),
	b2Vec2(-1.0f, 0.0f),
	b2Vec2(0.0f, -1.0f),
	b2Vec2(+1.0f, 0.0f)
};

Catapult::Catapult(PortalWorld* portalWorld, s_objectDataGrid* objData)
{
	PortalWorld* pWorld = Game::object->pWorld;
	int index = objData->extraOffsets[0];

	s_polynomUbo& prData = ThinDrawer::object->polynomShader->datas[index].data;
	float gravity = abs(pWorld->GetGravity().y);

	if (prData.data.x < -111111111.111f)
	{
		float a = prData.equation.x;
		float b = prData.equation.y;
		float c = prData.equation.z;

		float startX = prData.data.y;
		float xLen = prData.data.z;

		float upX = -b / (2.0f * a);
		float upY = a * upX * upX + b * upX + c;

		b2Vec2 p0 = b2Vec2(objData->pos.x + 0.5f, objData->pos.y + 0.5f);

		float ySpeed;
		float xSpeed;

		float yDiff = upY - p0.y;
		if (yDiff < 0.001f)
		{
			float endY = a * (startX + xLen) * (startX + xLen) + b * (startX + xLen) + c;
			yDiff = p0.y - endY;
			float fallTime = sqrtf(2.0f * yDiff / gravity);
			
			ySpeed = 0.0f;
			xSpeed = xLen / fallTime;
		}
		else
		{
			ySpeed = sqrtf(2.0f * gravity * yDiff);
			if (upX > p0.x && xLen < 0.0f)
			{
				ySpeed *= -1.0f;
			}
			xSpeed = (upX - p0.x) / (ySpeed / gravity);
		}

		sendSpeed = b2Vec2(xSpeed, ySpeed);
	}
	else
	{
		float startY = prData.data.y;
		float yLen = prData.data.z;

		float ySpeed;
		if (yLen < 0.0f)
		{
			ySpeed = std::min(yLen, 10.0f);
		}
		else
		{
			ySpeed = sqrtf(2.0f * gravity * yLen);
		}

		sendSpeed = b2Vec2(0.0f, ySpeed);
	}
	
	b2BodyDef bDef;
	bDef.type = b2_kinematicBody;
	bDef.position = b2Vec2(objData->pos.x, objData->pos.y) + bodyOffsets[objData->rotation % 4];
	bDef.angle = objData->rotation * 0.5f * b2_pi;

	body = portalWorld->CreateBody(&bDef);
	contactReportTime = -10.0;

	b2Vec2 scale = b2Vec2(0.5f, 0.1f);
	b2PolygonShape shape;
	shape.SetAsBox(scale.x, scale.y);

	b2Fixture* fix = body->CreateFixture(&shape, 0.0f);

	fixtureData* fData = new fixtureData();
	fData->type = F_CATAPULT;
	fData->data = (void*)this;
	fData->extraData = nullptr;

	fix->GetUserData().pointer = (uintptr_t)fData;
}

void Catapult::toDeath()
{
	
}

void Catapult::toLeveler()
{
	movingCatapults.clear();
}

void Catapult::staticInit()
{
	/*bodyTex = IOHelper::loadTexture("CatapultBody.png", VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);
	buttonTex = IOHelper::loadTexture("CatapultButton.png", VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);*/

	constexpr int index = getLevelObjectIndex(CATAPULT) - 4;

	ptData = &Leveler::object->perTextureData[index];
	bodyTex = Leveler::object->ol->items[index + 4].tex;

	ThinDrawer::object->midDrawFuncs.push_back({ render, nullptr });
}

void Catapult::update()
{
	
}

void Catapult::render(VkCommandBuffer cmdBuff, void* ptr)
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
		mt->pipelineLayout, 2, 1, &bodyTex->set, 0, VK_NULL_HANDLE);

	vkCmdDraw(cmdBuff, td->wallLight->buffers[0].count, ptData->currentOffset - ptData->offsetStart, 0, ptData->offsetStart);
}