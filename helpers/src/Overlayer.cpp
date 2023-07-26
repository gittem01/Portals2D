#include <Overlayer.h>
#include <Game.h>
#include <ThinDrawer.h>

Overlayer::Overlayer(Game* game)
{
	this->game = game;
	this->thinDrawer = game->thinDrawer;
	this->wh = thinDrawer->wh;
	this->as = thinDrawer->arrowShader;
    this->il = thinDrawer->itemList;

    cam.zoom = 1.0;
    cam.baseX = 160.0;
    cam.defaultXSides = glm::vec2(-80.0, +80.0);
    cam.defaultYSides = glm::vec2(-45.0, +45.0);
    cam.init(glm::vec2(0.0, 0.0), wh);

    thinDrawer->memMapFuncs.push_back({ maper, (void*)this });
	thinDrawer->lateDrawFuncs.push_back({ draw, (void*)this });
}

void Overlayer::maper(void* ptr)
{
    Overlayer* ol = (Overlayer*)ptr;
    ThinDrawer* td = ol->thinDrawer;
    WindowHandler* wh = ol->wh;

    s_sharedUniformData* sharedData;
    CHECK_RESULT_VK(vkMapMemory(td->logicalDevice, ShaderBase::sharedOrthoUniform->memory, ShaderBase::alignedSharedSize * (td->lastSwapChainImageIndex + 8),
        ShaderBase::alignedSharedSize, 0, (void**)&sharedData));

    sharedData->orthoMatrix = ol->cam.camMatrix;
    sharedData->cameraPos = glm::vec4(
        ol->cam.pos.x, ol->cam.pos.y,
        ol->cam.xSides.y - ol->cam.xSides.x, ol->cam.ySides.y - ol->cam.ySides.x);
    sharedData->data.x = ol->cam.zoom;
    sharedData->data.y = td->gameTime;

    glm::vec2 worldMouse = ol->cam.getMouseCoords();
    sharedData->data.z = worldMouse.x;
    sharedData->data.w = worldMouse.y;

    vkUnmapMemory(td->logicalDevice, ShaderBase::sharedOrthoUniform->memory);
}

void Overlayer::draw(VkCommandBuffer cmdBuff, void* ptr)
{
    Overlayer* ol = (Overlayer*)ptr;
    ThinDrawer* td = ol->thinDrawer;

    const VkDeviceSize offsets = 0;
    const uint32_t camOffset = ShaderBase::alignedSharedSize * (td->lastSwapChainImageIndex + 8);

    {
        s_itemData iData;
        iData.data = glm::vec4(0.0, 0.0, 0.0, 0.0);
        iData.posSize = glm::vec4(-68.0, 0.0, 18.0, 70.0);

        vkCmdBindPipeline(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS, ol->il->pipeline);

        vkCmdBindVertexBuffers(cmdBuff, 0, 1, &td->wallLight->buffers[0]->buffer, &offsets);

        vkCmdBindDescriptorSets(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS,
            ol->il->pipelineLayout, 0, 1, &ShaderBase::sharedSet, 1, &camOffset);

        vkCmdPushConstants(cmdBuff, ol->il->pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(s_itemData), (void*)&iData);

        vkCmdDraw(cmdBuff, td->wallLight->buffers[0]->count, 1, 0, 0);
    }

    {
        s_arrowData aData;
        aData.color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
        aData.data = glm::vec4(-0.5, 0.5, -0.0, 0.5);
        aData.posSize = glm::vec4(-78.5, 0.0, 3.0, 3.0);
        aData.data2.x = 0.0;

        vkCmdBindPipeline(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS, ol->as->pipeline);

        vkCmdBindVertexBuffers(cmdBuff, 0, 1, &td->wallLight->buffers[0]->buffer, &offsets);

        vkCmdBindDescriptorSets(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS,
            ol->as->pipelineLayout, 0, 1, &ShaderBase::sharedSet, 1, &camOffset);

        vkCmdPushConstants(cmdBuff, ol->as->pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(s_arrowData), (void*)&aData);

        vkCmdDraw(cmdBuff, td->wallLight->buffers[0]->count, 1, 0, 0);
    }
}

void Overlayer::update()
{

}