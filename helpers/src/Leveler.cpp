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

    clickStart[0] = -100000;

    thinDrawer->whiteWallShader->recreateUniforms(numWhiteWall);
    thinDrawer->blackWallShader->recreateUniforms(numBlackWall);

    thinDrawer->earlyDrawFuncs.push_back({ renderParticles, (void*)this });
    thinDrawer->lateDrawFuncs.push_back({ renderGrid, (void*)this });

    ol = new Overlayer(game);

    initOffsets();
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

void Leveler::initOffsets()
{
    gridMap.resize(ySize);
    for (int i = 0; i < ySize; i++)
    {
		gridMap[i].resize(xSize);
        for (int j = 0; j < xSize; j++)
        {
            gridMap[i][j] = { NONE_LO, 0 };
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
    if (!leveler->ol->itemDir)
    {
        vkCmdBindPipeline(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS, td->gridSquare->pipeline);

        vkCmdBindVertexBuffers(cmdBuff, 0, 1, &td->wallLight->buffers[0]->buffer, &offsets);

        vkCmdBindDescriptorSets(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS,
            td->gridSquare->pipelineLayout, 0, 1, &ShaderBase::sharedSet, 1, &camOffset);

        glm::ivec2 mp = leveler->getMouseGrid();
        int x0 = mp.x;
        int y0 = mp.y;

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

glm::ivec2 Leveler::getMouseGrid()
{
    glm::vec2 mousePos = wh->cam->getMouseCoords();
    int px = std::max(std::min((int)std::floor(mousePos.x), xSize_H - 1), -xSize_H);
    int py = std::max(std::min((int)std::floor(mousePos.y), ySize_H - 1), -ySize_H);

    return glm::ivec2(px, py);
}

void Leveler::update()
{
    ol->update();

    if (wh->mouseData[3] == 2)
    {
        clickStart[0] = -100000;
    }

    if (wh->mouseData[2] == 0)
    {
        if (clickStart[0] > -100000)
        {
            double st = glfwGetTime();
            
            glm::ivec2 mp = getMouseGrid();

            int minX = clickStart[0] > mp.x ? mp.x : clickStart[0];
            int maxX = clickStart[0] > mp.x ? clickStart[0] : mp.x;

            int minY = clickStart[1] > mp.y ? mp.y : clickStart[1];
            int maxY = clickStart[1] > mp.y ? clickStart[1] : mp.y;

            levelObject fillType = ol->items.at(ol->selectedItemIndex).objectType;

            fillGrid(minX, minY, maxX, maxY, fillType);

            printf("elp time: %f\n", glfwGetTime() - st);
        }

        clickStart[0] = -100000;
    }
    else if (wh->mouseData[2] == 2 && !ol->itemDir && ol->selectedItemIndex >= 0)
    {
        glm::ivec2 mousePos = getMouseGrid();
        clickStart[0] = mousePos.x;
        clickStart[1] = mousePos.y;
    }
}

inline void Leveler::setSquare(short x, short y, s_levelGridData data)
{
    gridMap[y + ySize_H][x + xSize_H] = data;
}

inline s_levelGridData Leveler::getSquare(short x, short y)
{
    return gridMap[y + ySize_H][x + xSize_H];
}

void Leveler::fillGrid(short minX, short minY, short maxX, short maxY, levelObject fillType)
{
    //vkDeviceWaitIdle(thinDrawer->logicalDevice);

    void(Leveler::*gridSquareAdder)(short x, short y);

    switch (fillType)
    {
    case WHITE_WALL:
        gridSquareAdder = &Leveler::addGridSquare_W;
        break;
    case BLACK_WALL:
        gridSquareAdder = &Leveler::addGridSquare_B;
        break;
    default:
        gridSquareAdder = &Leveler::addGridSquare_W;
        break;
    }

    for (int i = minY; i <= maxY; i++)
    {
        for (int j = minX; j <= maxX; j++)
        {
            s_levelGridData lgData = getSquare(j, i);

            if (lgData.objectType == fillType)
            {
                continue;
            }
            else if (lgData.objectType != NONE_LO)
            {
                removeGridSquare(j, i);
            }

            (this->*gridSquareAdder)(j, i);
        }
    }
}

void Leveler::addGridSquare_W(short x, short y)
{
    float randVal11 = (((float)rand() / RAND_MAX) - 0.5f) * 0.2f;
    float randVal22 = (((float)rand() / RAND_MAX)) * 0.0f;
    s_wallUbo_w wallUbo;
    wallUbo.posSize = glm::vec4(x + 0.5f, y + 0.5f, 1.0f, 1.0f);
    wallUbo.bgColour = glm::vec4(0.4f + randVal11, 0.4f + randVal11, 0.4f + randVal11, 1.0f);
    wallUbo.edgeColour = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
    wallUbo.dotColour = glm::vec4(0.1f + randVal11, 0.1f + randVal11, 0.1f + randVal11, 1.0f);
    wallUbo.data = glm::vec4(0.1f + randVal22, 0.06f, 2.0f, 1.0f);
    wallUbo.data2 = glm::vec4(0.0, 0.0, 1.0f, 0.0f);

    int offset = thinDrawer->whiteWallShader->wallCount * sizeof(s_wallUbo_w);
    void* pData;
    vkMapMemory(thinDrawer->logicalDevice, thinDrawer->whiteWallShader->uniformBuffers[0]->memory,
        offset, sizeof(s_wallUbo_w), 0, (void**)&pData);
    memcpy(pData, &wallUbo, sizeof(s_wallUbo_w));
    vkUnmapMemory(thinDrawer->logicalDevice, thinDrawer->whiteWallShader->uniformBuffers[0]->memory);
    thinDrawer->whiteWallShader->wallCount++;

    lastOffsetPos_W.push_back({ x, y });
    setSquare(x, y, { WHITE_WALL, offset });
}

void Leveler::addGridSquare_B(short x, short y)
{
    float rndVal = ((double)rand() / RAND_MAX + 2.0) / 3.0;
    s_wallUbo_b* wallUbo;
    int offset = thinDrawer->blackWallShader->wallCount * sizeof(s_wallUbo_b);
    vkMapMemory(thinDrawer->logicalDevice, thinDrawer->blackWallShader->uniformBuffers[0]->memory,
        offset, sizeof(s_wallUbo_b), 0, (void**)&wallUbo);
    wallUbo->posSize = glm::vec4(x + 0.5f, y + 0.5f, 1.0f, 1.0f);
    wallUbo->colour = glm::vec4(0.03, 0.03, 0.03, 1.0) * rndVal;
    wallUbo->data = glm::vec4(rand() > RAND_MAX / 2, 0.04, 1.0, 0.0);
    vkUnmapMemory(thinDrawer->logicalDevice, thinDrawer->blackWallShader->uniformBuffers[0]->memory);
    thinDrawer->blackWallShader->wallCount++;

    lastOffsetPos_B.push_back({ x, y });
    setSquare(x, y, { BLACK_WALL, offset });
}

void Leveler::removeGridSquare(short x, short y)
{
    s_levelGridData gData = getSquare(x, y);
    if (gData.objectType == NONE_LO) return;

    setSquare(x, y, { NONE_LO, 0 });

    switch (gData.objectType)
    {
    case WHITE_WALL:
    {
        s_gridSquare lastOffsetPos = lastOffsetPos_W.back();
        if (lastOffsetPos == s_gridSquare{ x, y })
        {
			lastOffsetPos_W.pop_back();
            thinDrawer->whiteWallShader->wallCount--;
            break;
		}

        int offsetPosIndex = gData.data.offset / sizeof(s_wallUbo_w);

        lastOffsetPos_W[offsetPosIndex] = lastOffsetPos;
        lastOffsetPos_W.pop_back();

        s_levelGridData gridWithLastOffset = getSquare(lastOffsetPos.x, lastOffsetPos.y);

        s_wallUbo_w *wUbo, wUboStager;
        vkMapMemory(thinDrawer->logicalDevice, thinDrawer->whiteWallShader->uniformBuffers[0]->memory,
            gridWithLastOffset.data.offset, sizeof(s_wallUbo_w), 0, (void**)&wUbo);
        memcpy((void*)&wUboStager, (void*)wUbo, sizeof(s_wallUbo_w));
        vkUnmapMemory(thinDrawer->logicalDevice, thinDrawer->whiteWallShader->uniformBuffers[0]->memory);

        vkMapMemory(thinDrawer->logicalDevice, thinDrawer->whiteWallShader->uniformBuffers[0]->memory,
            gData.data.offset, sizeof(s_wallUbo_w), 0, (void**)&wUbo);
        memcpy((void*)wUbo, (void*)&wUboStager, sizeof(s_wallUbo_w));
        vkUnmapMemory(thinDrawer->logicalDevice, thinDrawer->whiteWallShader->uniformBuffers[0]->memory);

        thinDrawer->whiteWallShader->wallCount--;

        setSquare(lastOffsetPos.x, lastOffsetPos.y, { WHITE_WALL, gData.data.offset });
    }
    break;

    case BLACK_WALL:
    {
        s_gridSquare lastOffsetPos = lastOffsetPos_B.back();
        if (lastOffsetPos == s_gridSquare{ x, y })
        {
            lastOffsetPos_B.pop_back();
            thinDrawer->blackWallShader->wallCount--;
            break;
        }

        int offsetPosIndex = gData.data.offset / sizeof(s_wallUbo_b);

        lastOffsetPos_B[offsetPosIndex] = lastOffsetPos;
        lastOffsetPos_B.pop_back();

        s_levelGridData gridWithLastOffset = getSquare(lastOffsetPos.x, lastOffsetPos.y);

        s_wallUbo_b* wUbo, wUboStager;
        vkMapMemory(thinDrawer->logicalDevice, thinDrawer->blackWallShader->uniformBuffers[0]->memory,
            gridWithLastOffset.data.offset, sizeof(s_wallUbo_b), 0, (void**)&wUbo);
        memcpy((void*)&wUboStager, (void*)wUbo, sizeof(s_wallUbo_b));
        vkUnmapMemory(thinDrawer->logicalDevice, thinDrawer->blackWallShader->uniformBuffers[0]->memory);

        vkMapMemory(thinDrawer->logicalDevice, thinDrawer->blackWallShader->uniformBuffers[0]->memory,
            gData.data.offset, sizeof(s_wallUbo_b), 0, (void**)&wUbo);
        memcpy((void*)wUbo, (void*)&wUboStager, sizeof(s_wallUbo_b));
        vkUnmapMemory(thinDrawer->logicalDevice, thinDrawer->blackWallShader->uniformBuffers[0]->memory);

        thinDrawer->blackWallShader->wallCount--;

        setSquare(lastOffsetPos.x, lastOffsetPos.y, { BLACK_WALL, gData.data.offset });
    }
    break;

    default:
        break;
    }
}