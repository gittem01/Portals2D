#include <Leveler.h>
#include <Game.h>
#include <ThinDrawer.h>
#include <Camera.h>
#include <Overlayer.h>

Leveler::Leveler(Game* game)
{
    this->game = game;
    thinDrawer = game->thinDrawer;
    wh = thinDrawer->wh;

    ol = new Overlayer(game);

    clickStart[0] = -100000;

    thinDrawer->earlyDrawFuncs.push_back({ renderParticles, (void*)this });
    thinDrawer->lateDrawFuncs.push_back({ renderGrid, (void*)this });
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

void Leveler::renderParticles(VkCommandBuffer cmdBuff, void* ptr)
{
    Leveler* leveler = (Leveler*)ptr;
    ThinDrawer* td = leveler->thinDrawer;

    VkDeviceSize offsets = 0;
    uint32 dynamicOffsets = 0;
    const uint32_t camOffset = ShaderBase::alignedSharedSize * td->lastSwapChainImageIndex;

    vkCmdBindPipeline(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS, td->dualParticleShader->pipeline);

    vkCmdBindVertexBuffers(cmdBuff, 0, 1, &td->dualParticleShader->buffers[0]->buffer, &offsets);

    vkCmdBindDescriptorSets(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS,
        td->dualParticleShader->pipelineLayout, 0, 1, &ShaderBase::sharedSet, 1, &camOffset);

    vkCmdBindDescriptorSets(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS,
        td->dualParticleShader->pipelineLayout, 1, 1, &td->dualParticleShader->descriptorSets[1], 1, &dynamicOffsets);

    vkCmdDraw(cmdBuff, td->dualParticleShader->buffers[0]->count, 300, 0, 0);

    dynamicOffsets = td->dualParticleShader->alignedSize;
    vkCmdBindDescriptorSets(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS,
        td->dualParticleShader->pipelineLayout, 1, 1, &td->dualParticleShader->descriptorSets[1], 1, &dynamicOffsets);

    vkCmdDraw(cmdBuff, td->dualParticleShader->buffers[0]->count, 300, 0, 0);
}

void Leveler::renderGrid(VkCommandBuffer cmdBuff, void* ptr)
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

        if (leveler->clickStart[0] > -50000)
        {
            gsData.positionData[0] = leveler->clickStart[0] > x0 ? x0 : leveler->clickStart[0];
            gsData.positionData[1] = leveler->clickStart[1] > y0 ? y0 : leveler->clickStart[1];
            gsData.positionData[2] = abs(x0 - leveler->clickStart[0]) + 1;
            gsData.positionData[3] = 1;
            gsData.colour = glm::vec4(1.0f, 0.5f, 0.0f, 1.0f);

            vkCmdPushConstants(cmdBuff, td->gridSquare->pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(s_gridSquareData), (void*)&gsData);

            int drawCount = (abs(x0 - leveler->clickStart[0]) + 1) * (abs(y0 - leveler->clickStart[1]) + 1);
            vkCmdDraw(cmdBuff, td->wallLight->buffers[0]->count, drawCount, 0, 0);
        }
    }
}

void Leveler::update()
{
    if (wh->mouseData[3] == 2)
    {
        clickStart[0] = -100000;
    }

    if (wh->mouseData[2] == 0)
    {
        if (clickStart[0] > -50000)
        {
            double st = glfwGetTime();
            glm::vec2 mousePos = wh->cam->getMouseCoords();
            int px = std::floor(mousePos.x);
            int py = std::floor(mousePos.y);

            int minX = clickStart[0] > px ? px : clickStart[0];
            int maxX = clickStart[0] > px ? clickStart[0] : px;

            int minY = clickStart[1] > py ? py : clickStart[1];
            int maxY = clickStart[1] > py ? clickStart[1] : py;

            for (int i = minX; i <= maxX; i++)
            {
                for (int j = minY; j <= maxY; j++)
                {
                    auto iter = gridMap.find({(short)i, (short)j});
                    if (iter != gridMap.end())
                    {
                        continue;
                    }

                    gridMap[{(short)i, (short)j}] = {WHITE_WALL, nullptr};

                    float randVal11 = (((float)rand() / RAND_MAX) - 0.5f) * 0.2f;
                    float randVal22 = (((float)rand() / RAND_MAX)) * 0.0f;
                    s_wallUbo_w wallUbo;
                    wallUbo.posSize = glm::vec4(i + 0.5f, j + 0.5f, 1.0f, 1.0f);
                    wallUbo.bgColour = glm::vec4(0.4f + randVal11, 0.4f + randVal11, 0.4f + randVal11, 1.0f);
                    wallUbo.edgeColour = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
                    wallUbo.dotColour = glm::vec4(0.1f + randVal11, 0.1f + randVal11, 0.1f + randVal11, 1.0f);
                    wallUbo.data = glm::vec4(0.05f + randVal22, 0.06f, 2.0f, 1.0f);
                    wallUbo.data2 = glm::vec4(0.0, 0.0, 1.0f, 0.0f);

                    void* pData;
                    CHECK_RESULT_VK(vkMapMemory(thinDrawer->logicalDevice, thinDrawer->whiteWallShader->uniformBuffers[0]->memory,
                        thinDrawer->whiteWallShader->wallCount * sizeof(s_wallUbo_w), sizeof(s_wallUbo_w), 0, (void**)&pData));
                    memcpy(pData, &wallUbo, sizeof(s_wallUbo_w));
                    vkUnmapMemory(thinDrawer->logicalDevice, thinDrawer->whiteWallShader->uniformBuffers[0]->memory);
                    thinDrawer->whiteWallShader->wallCount++;
                }
            }

            printf("elp time: %f\n", glfwGetTime() - st);
        }

        clickStart[0] = -100000;
    }
    else if (wh->mouseData[2] == 2)
    {
        glm::vec2 mousePos = wh->cam->getMouseCoords();
        clickStart[0] = std::floor(mousePos.x);
        clickStart[1] = std::floor(mousePos.y);
    }
}