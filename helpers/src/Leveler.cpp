#include <Leveler.h>
#include <Game.h>
#include <ThinDrawer.h>
#include <Camera.h>

void renderGrid(VkCommandBuffer cmdBuff, void* ptr)
{
    Leveler* leveler = (Leveler*)ptr;
    ThinDrawer* td = leveler->thinDrawer;

    const VkDeviceSize offsets = 0;
    const uint32_t camOffset = ShaderBase::alignedSharedSize * td->lastSwapChainImageIndex;

    // Grid
    {
        vkCmdBindPipeline(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS, td->endlessGrid->pipeline);

        vkCmdBindVertexBuffers(cmdBuff, 0, 1, &td->wallLight->buffers[0]->buffer, &offsets);

        vkCmdBindDescriptorSets(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS,
            td->endlessGrid->pipelineLayout, 0, 1, &ShaderBase::sharedSet, 1, &camOffset);

        vkCmdDraw(cmdBuff, td->wallLight->buffers[0]->count, 200, 0, 0);
    }

    // GridSquare
    {
        vkCmdBindPipeline(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS, td->gridSquare->pipeline);

        vkCmdBindVertexBuffers(cmdBuff, 0, 1, &td->wallLight->buffers[0]->buffer, &offsets);

        vkCmdBindDescriptorSets(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS,
            td->gridSquare->pipelineLayout, 0, 1, &ShaderBase::sharedSet, 1, &camOffset);

        int x0 = std::floor(td->wh->cam->getMouseCoords().x);
        int y0 = std::floor(td->wh->cam->getMouseCoords().y);

        s_gridSquareData gsData;
        gsData.positionData[0] = x0 - 2;
        gsData.positionData[1] = y0 - 2;
        gsData.positionData[2] = 5;
        gsData.positionData[3] = 0;
        gsData.colour = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);

        vkCmdPushConstants(cmdBuff, td->gridSquare->pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(s_gridSquareData), (void*)&gsData);

        vkCmdDraw(cmdBuff, td->wallLight->buffers[0]->count, 25, 0, 0);

        //gsData.positionData[3] = 1;
        //gsData.colour = glm::vec4(1.0f, 1.0f, 1.0f, 0.5f);

        //vkCmdPushConstants(cmdBuff, td->gridSquare->pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(s_gridSquareData), (void*)&gsData);

        //vkCmdDraw(cmdBuff, td->wallLight->buffers[0]->count, 25, 0, 0);
    }
}

Leveler::~Leveler()
{
    for (auto iter = thinDrawer->lateDrawFuncs.begin(); iter != thinDrawer->lateDrawFuncs.end(); iter++)
    {
        if (iter->second == this)
        {
            thinDrawer->lateDrawFuncs.erase(iter);
            break;
        }
    }
}

Leveler::Leveler(Game* game)
{
	this->game = game;
    thinDrawer = game->thinDrawer;

	thinDrawer->lateDrawFuncs.push_back({ renderGrid, (void*)this});
}