#include "GameObjects/Dropper.h"
#include "SwapChain.h"
#include "ThinDrawer.h"
#include "LevelerTestCase.h"
#include "Gel.h"
#include "Overlayer.h"

std::vector<Dropper> Dropper::droppers;
std::set<Dropper*> Dropper::createList;
s_perTextureData* Dropper::ptData;
s_texture* Dropper::tex;

static b2Vec2 vertices[4] =
{
	b2Vec2(0.75f, 0.5f),
	b2Vec2(0.75f, -0.55f),
	b2Vec2(1.0f, -0.55f),
	b2Vec2(1.0f, -0.75f),
};

Dropper::Dropper(PortalWorld* portalWorld, s_objectDataGrid* objData)
{
	dropCreationPos = b2Vec2(objData->pos.x + 1.0f, objData->pos.y + 1.5f);

	b2BodyDef bDef;
	bDef.type = b2_staticBody;
	bDef.position = dropCreationPos;

	b2Body* body = portalWorld->CreateBody(&bDef);

	for (int i = 0; i < std::size(vertices) - 1; i++)
	{
		b2Vec2 v1 = vertices[i];
		b2Vec2 v2 = vertices[i + 1];

		b2EdgeShape eShape;
		eShape.SetTwoSided(v1, v2);

		b2Fixture* fix = body->CreateFixture(&eShape, 0.0f);

		v1.x *= -1.0f;
		v2.x *= -1.0f;

		eShape.SetTwoSided(v1, v2);

		fix = body->CreateFixture(&eShape, 0.0f);
	}

	startActive = objData->startActive;
	autoRespawn = objData->startPolarized;

	switch (objData->speed)
	{
	case 0:
	{
		linkedObject = new DefaultCube(portalWorld, dropCreationPos, this);
	}
	break;
	case 1:
	{
		linkedObject = new LaserCube(portalWorld, dropCreationPos, this);
	}
	break;
	case 2:
	{
		linkedObject = new Sphere(portalWorld, dropCreationPos, this);
	}
	break;
	default:
		printf("undefined dropper index\n");
		exit(1);
		break;
	}

	for (auto pair : linkedObject->wData->destroyCallbacks)
	{
		pair.first(linkedObject->wData->pbIndices[0], pair.second);
	}

	delete linkedObject->wData->pbIndices[0];

	if (autoRespawn)
	{
		linkedObject->wData->destroyCallbacks.push_back({destrFunc, this});
	}

	if (startActive)
	{
		createList.insert(this);
	}
}

Dropper::~Dropper()
{

}

Dropper* Dropper::createDropper(PortalWorld* portalWorld, s_objectDataGrid* objData)
{
	droppers.push_back(Dropper());

	Dropper* dropper = &droppers.back();

	new (dropper) Dropper(portalWorld, objData);

	return dropper;
}

void Dropper::enable()
{
	if (startActive) return;

	createList.insert(this);

	if (linkedObject->wData->pbIndices.size() != 0)
	{
		linkedObject->enterForceField(nullptr);
	}
}

void Dropper::disable()
{
	if (!startActive) return;

	createList.insert(this);

	if (linkedObject->wData->pbIndices.size() != 0)
	{
		linkedObject->enterForceField(nullptr);
	}
}

void Dropper::destrFunc(PortalBody* pb, void* ptr)
{
	Dropper* drp = (Dropper*)ptr;

	if (pb->worldIndex->pbIndices.size() <= 1)
	{
		createList.insert(drp);
	}
}

void Dropper::toDeath()
{
	createList = {};

	for (Dropper& drp : droppers)
	{
		if (drp.startActive)
		{
			createList.insert(&drp);
		}
	}
}

void Dropper::toLeveler()
{
	droppers = {};
	createList = {};
}

void Dropper::staticInit()
{
	constexpr int index = getLevelObjectIndex(CUBE_SPAWNER) - 4;

	ptData = &Leveler::object->perTextureData[index];
	tex = Leveler::object->ol->items[index + 4].tex;

	ThinDrawer::object->midDrawFuncs.push_back({ render, nullptr });
}

void Dropper::render(VkCommandBuffer cmdBuff, void* ptr)
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

void Dropper::update()
{
	for (auto iter = createList.begin(); iter != createList.end(); )
	{
		Dropper* dropper = *iter;
		pbWorldData* wData = dropper->linkedObject->wData;
		bodyData* bData;
		if (wData->pbIndices.size() == 0)
		{
			PortalBody* pb = Game::object->pWorld->createCloneBody0(wData, dropper->dropCreationPos, 0.0f, false);
			
			wData->destrTime = ThinDrawer::object->gameTime - 2.0f;
			wData->gravityScale = 0.0f;
		}
		else if ((bData = (bodyData*)wData->pbIndices[0]->body->GetUserData().pointer)->extraData == nullptr)
		{
			wData->destrTime += ThinDrawer::object->deltaTime * 2.0;
			if (wData->destrTime >= ThinDrawer::object->gameTime)
			{
				wData->destrTime = -20.0f;
				wData->gravityScale = 1.0f;

				bData->extraData = &dropper->linkedObject->pData;

				wData->pbIndices[0]->body->SetAwake(true);

				iter = createList.erase(iter);

				continue;
			}
		}

		++iter;
	}
}