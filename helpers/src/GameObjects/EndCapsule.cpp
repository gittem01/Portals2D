#include "GameObjects/EndCapsule.h"
#include "SwapChain.h"
#include "ThinDrawer.h"
#include "LevelerTestCase.h"
#include "Gel.h"
#include "Overlayer.h"

EndCapsule* EndCapsule::object = nullptr;

static b2Vec2 vertices[2] =
{
	b2Vec2(-0.5f, +1.0f),
	b2Vec2(+0.5f, +1.0f),
};

EndCapsule::EndCapsule(PortalWorld* portalWorld, b2Vec2 pos)
{
	this->pos = pos + b2Vec2(0.5f, 1.0f);

	b2BodyDef bDef;
	bDef.type = b2_staticBody;
	bDef.position = this->pos;

	b2Body* body = portalWorld->CreateBody(&bDef);

	b2Vec2 v1 = vertices[0];
	b2Vec2 v2 = vertices[1];

	b2EdgeShape eShape;
	eShape.SetTwoSided(v1, v2);

	b2Fixture* fix = body->CreateFixture(&eShape, 0.0f);

	v1.y *= -1.0f;
	v2.y *= -1.0f;

	eShape.SetTwoSided(v1, v2);

	fix = body->CreateFixture(&eShape, 0.0f);

	fData.type = F_END_CAPSULE;
	fData.data = this;
	fData.extraData = nullptr;

	// sensor edges
	v1 = vertices[0];
	v2 = vertices[0]; v2.y *= -1.0f;
	eShape.SetTwoSided(v1, v2);
	fix = body->CreateFixture(&eShape, 0.0f);
	fix->GetUserData().pointer = (uintptr_t)&fData;

	v1.x *= -1.0f; v2.x *= -1.0f;
	eShape.SetTwoSided(v1, v2);
	fix = body->CreateFixture(&eShape, 0.0f);
	fix->GetUserData().pointer = (uintptr_t)&fData;

    ThinDrawer::object->lateDrawFuncs.push_back({ render, this });

	tex = IOHelper::loadTexture("endCapsule.png");

	EndCapsule::object = this;
}

EndCapsule::~EndCapsule()
{

}


void EndCapsule::render(VkCommandBuffer cmdBuff, void* ptr)
{
    EndCapsule* ec = (EndCapsule*)ptr;
	ThinDrawer* td = Leveler::object->thinDrawer;

	const VkDeviceSize offsets = 0;

    vkCmdBindPipeline(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS, td->bodyShader->pipeline);

    vkCmdBindDescriptorSets(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS,
        td->bodyShader->pipelineLayout, 0, 1, &ShaderBase::sharedSet, 1, &td->camOffset);

    vkCmdBindDescriptorSets(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS,
        td->bodyShader->pipelineLayout, 1, 1, &ec->tex->set, 0, VK_NULL_HANDLE);

    vkCmdBindVertexBuffers(cmdBuff, 0, 1, &Turret::coverVertexBuffer->buffer, &offsets);

    s_renderBodyPsh pshConst;
    pshConst.posRot = glm::vec4(ec->pos.x, ec->pos.y, 0.0f, 1.0f);
    pshConst.movScale = glm::vec4(0.0f, 0.0f, 1.0f, 2.0f);
    pshConst.data = glm::vec4(0.0f, -20.0f, -20.0f, 0.0f);

    vkCmdPushConstants(cmdBuff, td->bodyShader->pipelineLayout,
        VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(s_renderBodyPsh), &pshConst);

    vkCmdDraw(cmdBuff, 6, 1, 0, 0);
}