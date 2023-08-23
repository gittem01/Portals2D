#include <Leveler.h>
#include <Game.h>
#include <ThinDrawer.h>
#include <Camera.h>
#include <Overlayer.h>
#include <Sounder.h>
#include <SwapChain.h>

// for math things
#include <PortalWorld.h>

Leveler* Leveler::object;

glm::vec2 rotationMults[4] = { {+0.0f, -1.0f}, {+1.0f, +0.0f}, {0.0f, +1.0f}, {-1.0f, 0.0f} };
const int maxPlatformDiff = 4;

Leveler::Leveler(Game* game)
{
    object = this;

    this->game = game;
    thinDrawer = game->thinDrawer;
    wh = thinDrawer->wh;
    mt = thinDrawer->manyTexture;

    numSwapChainImages = thinDrawer->swapChain->imageCount;

    clickStart[0] = -100000;

    operationList = new s_gridOperationList();
    currentOperation = operationList;

    thinDrawer->whiteWallShader->recreateUniforms(numWhiteWall * 2);
    thinDrawer->blackWallShader->recreateUniforms(numBlackWall * 2);
    thinDrawer->wallLight->recreateUniforms(numWhiteWall * 2);
    thinDrawer->diagonalLiner->recreateUniforms(numWhiteWall * 2);

    //thinDrawer->earlyDrawFuncs.push_back({ renderParticles, (void*)this });
    thinDrawer->midDrawFuncs.push_back({ renderLasers, (void*)this });
    thinDrawer->midDrawFuncs.push_back({ renderFunnels, (void*)this });
    thinDrawer->lateDrawFuncs.push_back({ renderGrid, (void*)this });

    ol = new Overlayer(game);

    thinDrawer->diagonalLiner->testFill();

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

void rupFiller(std::vector<std::vector<std::vector<s_shaderGridData>>>& renderGridUp)
{
    renderGridUp.resize(2);

    int offs = 0;
    for (int l = 0; l < renderGridUp.size(); l++)
    {
        renderGridUp[l].resize(WallBase::gridCount);
        for (int i = 0; i < WallBase::gridCount; i++)
        {
            renderGridUp[l][i].resize(WallBase::gridCount);
            for (int j = 0; j < WallBase::gridCount; j++)
            {
                renderGridUp[l][i][j].numElements = 0;
                renderGridUp[l][i][j].offsetStart = offs;
                renderGridUp[l][i][j].offsetEnd = offs;
                offs += WallBase::gridDivider * WallBase::gridDivider;
            }
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

    lastOffsetPos_W.resize(WallBase::gridCount);
    lastOffsetPos_B.resize(WallBase::gridCount);
    lastOffsetPos_G.resize(WallBase::gridCount);
    for (int i = 0; i < WallBase::gridCount; i++)
    {
        lastOffsetPos_W[i].resize(WallBase::gridCount);
        lastOffsetPos_B[i].resize(WallBase::gridCount);
        lastOffsetPos_G[i].resize(WallBase::gridCount);
    }

    rupFiller(thinDrawer->whiteWallShader->renderGridUp);
    rupFiller(thinDrawer->blackWallShader->renderGridUp);
    rupFiller(thinDrawer->wallLight->renderGridUp);
    rupFiller(thinDrawer->diagonalLiner->renderGridUp);

    textureRenderOffset = mt->lastAvailablePos;
    mt->lastAvailablePos += numSwapChainImages;

    perTextureData.resize(5);
    lastOffsetPos_M.resize(perTextureData.size());
    for (int i = 0; i < perTextureData.size(); i++)
    {
        perTextureData[i].offsetStart = mt->lastAvailablePos;
        perTextureData[i].currentOffset = mt->lastAvailablePos;
        mt->lastAvailablePos += perTextureSpace * numSwapChainImages;
    }

    const float sz = 2.5f;
    for (int i = -80; i < 80; i++)
    {
        for (int j = -80; j < 80; j++)
        {
            float rndVal = ((double)rand() / RAND_MAX + 2.0) / 3.0;
            if (rndVal > 0.9f)
            {
                float randVal11 = (((float)rand() / RAND_MAX) - 0.5f) * 0.01f;
                float randVal22 = (((float)rand() / RAND_MAX)) * 0.0f;
                s_wallUbo_w wallUbo;
                wallUbo.posSize = glm::vec4(i * sz, j * sz, sz, sz);
                wallUbo.bgColour = glm::vec4(0.1f + randVal11, 0.1f + randVal11, 0.1f + randVal11, 1.0f);
                wallUbo.edgeColour = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
                wallUbo.dotColour = glm::vec4(0.01f + randVal11, 0.01f + randVal11, 0.01f + randVal11, 1.0f);
                wallUbo.data = glm::vec4(0.3f + randVal22, 0.01f, 2.0f, 1.0f);
                wallUbo.data2 = glm::vec4(0.0, 0.0, 0.8f, 0.0f);

                s_gridPositionShader gridPos = thinDrawer->whiteWallShader->getGridSquare(wallUbo.posSize.x, wallUbo.posSize.y);
                s_shaderGridData* gData = &thinDrawer->whiteWallShader->renderGridUp[0][gridPos.y][gridPos.x];
                gData->numElements++;

                void* pData;
                CHECK_RESULT_VK(vkMapMemory(thinDrawer->whiteWallShader->logicalDevice, thinDrawer->whiteWallShader->uniformBuffers[0]->memory,
                    gData->offsetEnd * sizeof(s_wallUbo_w), sizeof(s_wallUbo_w), 0, (void**)&pData));
                memcpy(pData, &wallUbo, sizeof(s_wallUbo_w));
                vkUnmapMemory(thinDrawer->whiteWallShader->logicalDevice, thinDrawer->whiteWallShader->uniformBuffers[0]->memory);
                gData->offsetEnd++;

                continue;
            }
            else if (rndVal > 0.89f)
            {
                float ang = rand() > RAND_MAX / 2 ? 0.0 : b2_pi * 0.5;
                float ang2 = rand() > RAND_MAX / 2 ? 0.0 : b2_pi * 0.5;

                float r0 = (((float)rand() / RAND_MAX) - 0.5f);
                float g0 = (((float)rand() / RAND_MAX) - 0.5f);
                float b0 = (((float)rand() / RAND_MAX) - 0.5f);

                s_gridPositionShader gridPos = thinDrawer->wallLight->getGridSquare(i * sz, j * sz);
                s_shaderGridData* gData = &thinDrawer->wallLight->renderGridUp[0][gridPos.y][gridPos.x];
                gData->numElements++;
                
                s_walLightlUbo* pData;
                vkMapMemory(thinDrawer->wallLight->logicalDevice, thinDrawer->wallLight->uniformBuffers[0]->memory,
                    gData->offsetEnd * sizeof(s_walLightlUbo), sizeof(s_walLightlUbo), 0, (void**)&pData);
                pData->color = glm::vec4(1.0 + r0, 1.0 + g0, 1.0 + b0, 1.0);
                pData->posSize = glm::vec4(i * sz, j * sz, sz, sz);
                pData->data = glm::vec4(sz * 0.6, sz * 0.3, 0.3, -1.5);
                pData->data2 = glm::vec4(ang, ang2, 0.8, 1.0);
                vkUnmapMemory(thinDrawer->wallLight->logicalDevice, thinDrawer->wallLight->uniformBuffers[0]->memory);
                gData->offsetEnd++;
                continue;
            }

            s_gridPositionShader gridPos = thinDrawer->blackWallShader->getGridSquare(i * sz, j * sz);
            s_shaderGridData* gData = &thinDrawer->blackWallShader->renderGridUp[0][gridPos.y][gridPos.x];
            gData->numElements++;

            s_wallUbo_b* wallUbo;
            vkMapMemory(thinDrawer->blackWallShader->logicalDevice, thinDrawer->blackWallShader->uniformBuffers[0]->memory,
                gData->offsetEnd * sizeof(s_wallUbo_b), sizeof(s_wallUbo_b), 0, (void**)&wallUbo);
            wallUbo->posSize = glm::vec4(i * sz, j * sz, sz, sz);
            wallUbo->colour = glm::vec4(0.05, 0.05, 0.05, 1.0) * rndVal;
            wallUbo->data = glm::vec4(rand() > RAND_MAX / 2, 0.1, 0.8, 0.0);
            vkUnmapMemory(thinDrawer->blackWallShader->logicalDevice, thinDrawer->blackWallShader->uniformBuffers[0]->memory);
            gData->offsetEnd++;
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

void Leveler::renderLasers(VkCommandBuffer cmdBuff, void* ptr)
{
    Leveler* leveler = (Leveler*)ptr;
    ThinDrawer* td = leveler->thinDrawer;

    VkDeviceSize offsets = 0;
    const uint32_t camOffset = ShaderBase::alignedSharedSize * td->lastSwapChainImageIndex;

    vkCmdBindPipeline(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS, td->laserShader->pipeline);

    vkCmdBindVertexBuffers(cmdBuff, 0, 1, &td->wallLight->buffers[0]->buffer, &offsets);

    vkCmdBindDescriptorSets(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS,
        td->laserShader->pipelineLayout, 0, 1, &ShaderBase::sharedSet, 1, &camOffset);

    vkCmdBindDescriptorSets(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS,
        td->laserShader->pipelineLayout, 1, 1, &td->laserShader->descriptorSets[1], 0, VK_NULL_HANDLE);

    vkCmdDraw(cmdBuff, td->wallLight->buffers[0]->count, td->laserShader->numLasers, 0, 0);

    int laserIndex = getLevelObjectIndex(LASERER);

    if (leveler->ol->selectedItemIndex == laserIndex)
    {
        const s_gridItemData& item = leveler->ol->items[leveler->ol->selectedItemIndex];

        glm::ivec2 mp = leveler->getMouseGrid();
        int objRot = leveler->selectedObjectRotation;

        short scaleX = item.gridScale[objRot % 2];
        short scaleY = item.gridScale[(objRot + 1) % 2];

        glm::vec2 startPoint;
        glm::vec2 endPoint;

        leveler->sendObjectRay(item, mp.x, mp.y, objRot, scaleX, scaleY, 0.5, 0.0, (levelObject)(WHITE_WALL | BLACK_WALL), startPoint, endPoint);

        int laserOffset = td->laserShader->numLasers + td->lastSwapChainImageIndex;

        s_laserUbo* laserUbo;
        vkMapMemory(td->laserShader->logicalDevice, td->laserShader->uniformBuffers[0]->memory,
            laserOffset * sizeof(s_laserUbo), sizeof(s_laserUbo), 0, (void**)&laserUbo);
        laserUbo->points = glm::vec4(startPoint.x, startPoint.y, endPoint.x, endPoint.y);
        laserUbo->colour = glm::vec4(1.0, 0.0, 0.0, 1.0);
        laserUbo->data = glm::vec4(0.4, 11.0, 36.0, 0.0);

        VkMappedMemoryRange memoryRange = {};
        memoryRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
        memoryRange.memory = td->laserShader->uniformBuffers[0]->memory;
        memoryRange.offset = laserOffset * sizeof(s_laserUbo);
        memoryRange.size = sizeof(s_laserUbo);

        vkFlushMappedMemoryRanges(td->laserShader->logicalDevice, 1, &memoryRange);

        vkUnmapMemory(td->laserShader->logicalDevice, td->laserShader->uniformBuffers[0]->memory);

        vkCmdDraw(cmdBuff, td->wallLight->buffers[0]->count, 1, 0, laserOffset);
    }
}

void Leveler::renderFunnels(VkCommandBuffer cmdBuff, void* ptr)
{
    Leveler* leveler = (Leveler*)ptr;
    ThinDrawer* td = leveler->thinDrawer;

    VkDeviceSize offsets = 0;
    const uint32_t camOffset = ShaderBase::alignedSharedSize * td->lastSwapChainImageIndex;

    vkCmdBindPipeline(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS, td->funnelShader->pipeline);
    vkCmdBindVertexBuffers(cmdBuff, 0, 1, &td->debugCircleShader->buffers[0]->buffer, &offsets);

    vkCmdBindDescriptorSets(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS,
        td->funnelShader->pipelineLayout, 0, 1, &ShaderBase::sharedSet, 1, &camOffset);

    vkCmdBindDescriptorSets(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS,
        td->funnelShader->pipelineLayout, 1, 1, &td->funnelShader->descriptorSets[1], 0, VK_NULL_HANDLE);

    vkCmdDraw(cmdBuff, td->debugCircleShader->buffers[0]->count, td->funnelShader->numFunnels, 0, 0);

    int funnelIndex = getLevelObjectIndex(FUNNEL);

    if (leveler->ol->selectedItemIndex == funnelIndex)
    {
        const s_gridItemData& item = leveler->ol->items[leveler->ol->selectedItemIndex];

        glm::ivec2 mp = leveler->getMouseGrid();
        int objRot = leveler->selectedObjectRotation;

        short scaleX = item.gridScale[objRot % 2];
        short scaleY = item.gridScale[(objRot + 1) % 2];

        float xSub = ((objRot + 1) % 2) * -1.0;
        float ySub = (objRot % 2) * -1.0;

        glm::vec2 startPoint;
        glm::vec2 endPoint;

        leveler->sendObjectRay(item, mp.x + xSub, mp.y + ySub, objRot, scaleX, scaleY, 0.1, 0.0, (levelObject)(WHITE_WALL | BLACK_WALL | GLASS_WALL), startPoint, endPoint);

        int funnelOffset = td->funnelShader->numFunnels + td->lastSwapChainImageIndex;

        s_funnelUbo* funnelUbo;
        vkMapMemory(td->funnelShader->logicalDevice, td->funnelShader->uniformBuffers[0]->memory,
            funnelOffset * sizeof(s_funnelUbo), sizeof(s_funnelUbo), 0, (void**)&funnelUbo);

        funnelUbo->pointts = glm::vec4(startPoint.x, startPoint.y, endPoint.x, endPoint.y);
        float diffLen = glm::length(endPoint - startPoint);
        funnelUbo->colour = glm::vec4(0.0, 0.3, 1.0, 1.0);
        funnelUbo->colour2 = glm::vec4(0.0, 0.3, 1.0, 0.0);
        funnelUbo->data = glm::vec4(1.75, 0.0, 0.0, -1.0);
        funnelUbo->data2 = glm::vec4(diffLen + 10.0, diffLen - 1.0, diffLen, 0.0);

        vkUnmapMemory(td->funnelShader->logicalDevice, td->funnelShader->uniformBuffers[0]->memory);

        VkMappedMemoryRange memoryRange = {};
        memoryRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
        memoryRange.memory = td->funnelShader->uniformBuffers[0]->memory;
        memoryRange.offset = funnelOffset * sizeof(s_funnelUbo);
        memoryRange.size = sizeof(s_funnelUbo);

        vkFlushMappedMemoryRanges(td->funnelShader->logicalDevice, 1, &memoryRange);

        vkUnmapMemory(td->funnelShader->logicalDevice, td->funnelShader->uniformBuffers[0]->memory);

        vkCmdDraw(cmdBuff, td->wallLight->buffers[0]->count, 1, 0, funnelOffset);
    }
}

void renderTextureds(VkCommandBuffer cmdBuff, Leveler* leveler)
{
    ManyTexture* mt = leveler->ol->mt;
    ThinDrawer* td = leveler->thinDrawer;

    const VkDeviceSize offsets = 0;
    const uint32_t camOffset = ShaderBase::alignedSharedSize * td->lastSwapChainImageIndex;

    int i = 3;
    for (auto& data : leveler->perTextureData)
    {
        i++;
        if (data.currentOffset - data.offsetStart == 0)
        {
            continue;
        }

        vkCmdBindPipeline(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS, mt->pipeline);

        vkCmdBindVertexBuffers(cmdBuff, 0, 1, &td->wallLight->buffers[0]->buffer, &offsets);

        vkCmdBindDescriptorSets(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS,
            mt->pipelineLayout, 0, 1, &ShaderBase::sharedSet, 1, &camOffset);

        vkCmdBindDescriptorSets(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS,
            mt->pipelineLayout, 1, 1, &mt->descriptorSets[1], 0, VK_NULL_HANDLE);

        vkCmdBindDescriptorSets(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS,
            mt->pipelineLayout, 2, 1, &leveler->ol->items[i].texSet, 0, VK_NULL_HANDLE);

        vkCmdDraw(cmdBuff, td->wallLight->buffers[0]->count, data.currentOffset - data.offsetStart, 0, data.offsetStart);
    }
}

void renderSingleDrawSelection(VkCommandBuffer cmdBuff, Leveler* leveler)
{
    ManyTexture* mt = leveler->ol->mt;
    ThinDrawer* td = leveler->thinDrawer;

    const VkDeviceSize offsets = 0;
    const uint32_t camOffset = ShaderBase::alignedSharedSize * td->lastSwapChainImageIndex;

    glm::ivec2 mp = leveler->getMouseGrid();
    const s_gridItemData& item = leveler->ol->items[leveler->ol->selectedItemIndex];

    int objRot = leveler->selectedObjectRotation;

    short scaleX = item.gridScale[objRot % 2];
    short scaleY = item.gridScale[(objRot % 2) ^ 1];
    glm::vec2 extraAdd = rotationMults[objRot] * item.gridRenderTranslation.y;

    short startX = mp.x - scaleX / 2;
    short startY = mp.y - scaleY / 2;
    short endX = startX + scaleX;
    short endY = startY + scaleY;

    vkCmdBindPipeline(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS, td->gridSquare->pipeline);

    vkCmdBindVertexBuffers(cmdBuff, 0, 1, &td->wallLight->buffers[0]->buffer, &offsets);

    vkCmdBindDescriptorSets(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS,
        td->gridSquare->pipelineLayout, 0, 1, &ShaderBase::sharedSet, 1, &camOffset);

    s_gridSquareData gsData;
    gsData.positionData[2] = 1;
    gsData.positionData[3] = 1;
    for (int y = startY; y < endY; y++)
    {
        for (int x = startX; x < endX; x++)
        {
            gsData.positionData[0] = x;
            gsData.positionData[1] = y;

            if (leveler->checkGridOccupancy(gsData.positionData[0], gsData.positionData[1]))
            {
                gsData.colour = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);
            }
            else
            {
                gsData.colour = glm::vec4(0.0f, 1.0f, 0.0f, 1.0f);
            }

            vkCmdPushConstants(cmdBuff, td->gridSquare->pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(s_gridSquareData), (void*)&gsData);

            vkCmdDraw(cmdBuff, td->wallLight->buffers[0]->count, 1, 0, 0);
        }
    }

    if (item.requiredSideFill != -1)
    {
        switch ((objRot + item.requiredSideFill) % 4)
        {
        case 0:
            startY = startY - 1;
            endY = startY + 1;
            break;
        case 1:
            startX = endX;
            endX = startX + 1;
            break;
        case 2:
            startY = endY;
            endY = startY + 1;
            break;
        case 3:
            startX = startX - 1;
            endX = startX + 1;
            break;
        default:
            break;
        }

        for (int y = startY; y < endY; y++)
        {
            for (int x = startX; x < endX; x++)
            {
                gsData.positionData[0] = x;
                gsData.positionData[1] = y;

                if (leveler->isInBounds(x, y) && leveler->getSquare(x, y).objectType & (WHITE_WALL | BLACK_WALL))
                {
                    gsData.colour = glm::vec4(0.0f, 1.0f, 0.0f, 1.0f);
                }
                else
                {
                    gsData.colour = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);
                }

                vkCmdPushConstants(cmdBuff, td->gridSquare->pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(s_gridSquareData), (void*)&gsData);

                vkCmdDraw(cmdBuff, td->wallLight->buffers[0]->count, 1, 0, 0);
            }
        }
    }

    int renderIndex = leveler->textureRenderOffset + td->lastSwapChainImageIndex;
    s_manyTextureData* mtd;
    vkMapMemory(mt->logicalDevice, mt->uniformBuffers[0]->memory, renderIndex * sizeof(s_manyTextureData),
        sizeof(s_manyTextureData), 0, (void**)&mtd);

    float hAdd = scaleX % 2 == 1 ? 0.5 : 0.0;
    float vAdd = scaleY % 2 == 1 ? 0.5 : 0.0;

    mtd->posSize = glm::vec4(mp.x + hAdd + extraAdd.x, mp.y + vAdd + extraAdd.y, item.gridRenderScale, item.gridRenderScale);
    mtd->data.x = leveler->selectedObjectRotation * b2_pi * 0.5;
    vkUnmapMemory(mt->logicalDevice, mt->uniformBuffers[0]->memory);

    vkCmdBindPipeline(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS, mt->pipeline);

    vkCmdBindVertexBuffers(cmdBuff, 0, 1, &td->wallLight->buffers[0]->buffer, &offsets);

    vkCmdBindDescriptorSets(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS,
        mt->pipelineLayout, 0, 1, &ShaderBase::sharedSet, 1, &camOffset);

    vkCmdBindDescriptorSets(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS,
        mt->pipelineLayout, 1, 1, &mt->descriptorSets[1], 0, VK_NULL_HANDLE);

    vkCmdBindDescriptorSets(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS,
        mt->pipelineLayout, 2, 1, &item.texSet, 0, VK_NULL_HANDLE);

    vkCmdDraw(cmdBuff, td->wallLight->buffers[0]->count, 1, 0, renderIndex);
}

glm::vec2 rotateVec(glm::vec2 vec, float angle)
{
    float x = cos(angle) * vec.x - sin(angle) * vec.y;
    float y = sin(angle) * vec.x + cos(angle) * vec.y;

    return { x, y };
}

void getPlatformLastStateSquares(glm::ivec2 p0, glm::ivec2 p1, glm::ivec2 mp, glm::ivec2& lp0, glm::ivec2& lp1, glm::vec2& absStart, glm::vec2& absEnd, glm::vec2& absDown)
{
    short downY = std::min(p0.y, p1.y); short downYDiff = downY - mp.y;
    short upY = std::max(p0.y, p1.y); short upYDiff = mp.y - upY;
    short leftX = std::min(p0.x, p1.x); short leftXDiff = leftX - mp.x;
    short rightX = std::max(p0.x, p1.x); short rightXDiff = mp.x - rightX;

    int maxDiff = std::max(std::max(downYDiff, upYDiff), std::max(leftXDiff, rightXDiff));

    if (maxDiff <= 0)
    {
        lp0 = glm::ivec2(leftX, downY); lp1 = glm::ivec2(rightX, upY);
        absStart = glm::vec2(leftX, downY); absEnd = glm::vec2(rightX + 1, upY + 1);
        absDown = rotateVec(absEnd - absStart, glm::pi<float>() * 0.5) * 0.25f;
    }
    else if (downYDiff == maxDiff)
    {
        lp0 = glm::ivec2(leftX, downY - maxDiff); lp1 = glm::ivec2(rightX, downY - 1);
        absStart = glm::vec2((leftX + rightX + 1.0f) * 0.5, upY + 1.0f); absEnd = glm::vec2(absStart.x, downY - maxDiff);
        absDown = rotateVec(glm::normalize(absEnd - absStart) * 0.5f, glm::pi<float>() * 0.5) * (float)(rightX - leftX + 1);
    }
    else if (rightXDiff == maxDiff)
    {
        lp0 = glm::ivec2(rightX + 1, downY); lp1 = glm::ivec2(rightX + maxDiff, upY);
        absStart = glm::vec2(leftX, (downY + upY + 1.0f) * 0.5); absEnd = glm::vec2(rightX + maxDiff + 1, absStart.y);
        absDown = rotateVec(glm::normalize(absEnd - absStart) * 0.5f, glm::pi<float>() * 0.5) * (float)(upY - downY + 1);
    }
    else if (upYDiff == maxDiff)
    {
        lp0 = glm::ivec2(leftX, upY + 1); lp1 = glm::ivec2(rightX, upY + maxDiff);
        absStart = glm::vec2((leftX + rightX + 1.0f) * 0.5, downY); absEnd = glm::vec2(absStart.x, upY + maxDiff + 1);
        absDown = rotateVec(glm::normalize(absEnd - absStart) * 0.5f, glm::pi<float>() * 0.5) * (float)(rightX - leftX + 1);
    }
    else if (leftXDiff == maxDiff)
    {
        lp0 = glm::ivec2(leftX - maxDiff, downY); lp1 = glm::ivec2(leftX - 1, upY);
        absStart = glm::vec2(rightX + 1, (downY + upY + 1.0f) * 0.5); absEnd = glm::vec2(leftX - maxDiff, absStart.y);
        absDown = rotateVec(glm::normalize(absEnd - absStart) * 0.5f, glm::pi<float>() * 0.5) * (float)(upY - downY + 1);
    }
}

void renderPlatforms(VkCommandBuffer cmdBuff, Leveler* leveler)
{
    ThinDrawer* td = leveler->thinDrawer;

    glm::ivec2 mp = leveler->getMouseGrid();

    const VkDeviceSize offsets = 0;
    const uint32_t camOffset = ShaderBase::alignedSharedSize * td->lastSwapChainImageIndex;

    // lines
    vkCmdBindPipeline(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS, td->debugLineShader->pipeline);
    vkCmdBindVertexBuffers(cmdBuff, 0, 1, &td->debugCircleShader->buffers[0]->buffer, &offsets);

    vkCmdBindDescriptorSets(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS,
        td->debugLineShader->pipelineLayout, 0, 1, &ShaderBase::sharedSet, 1, &camOffset);

    vkCmdBindDescriptorSets(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS,
        td->debugLineShader->pipelineLayout, 1, 1, &td->debugLineShader->descriptorSets[1], 0, VK_NULL_HANDLE);

    vkCmdDraw(cmdBuff, td->debugCircleShader->buffers[0]->count, td->debugLineShader->numLines, 0, 0);

    // platform
    vkCmdBindPipeline(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS, td->platformRectangle->pipeline);

    vkCmdBindVertexBuffers(cmdBuff, 0, 1, &td->wallLight->buffers[0]->buffer, &offsets);

    vkCmdBindDescriptorSets(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS,
        td->platformRectangle->pipelineLayout, 0, 1, &ShaderBase::sharedSet, 1, &camOffset);

    vkCmdBindDescriptorSets(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS,
        td->platformRectangle->pipelineLayout, 1, 1, &td->platformRectangle->descriptorSets[1], 0, VK_NULL_HANDLE);

    vkCmdDraw(cmdBuff, td->wallLight->buffers[0]->count, td->platformRectangle->numPlatforms, 0, 0);

    if (leveler->ol->selectedItemIndex < 0) return;

    const s_gridItemData& item = leveler->ol->items[leveler->ol->selectedItemIndex];

    if (!(item.objectType & PLATFORM)) return;

    if (leveler->consqClickVec.size() == 0)
    {
        s_gridSquareData gsData;
        gsData.positionData[2] = 1;
        gsData.positionData[3] = 1;

        gsData.positionData[0] = mp.x;
        gsData.positionData[1] = mp.y;

        if (leveler->checkGridOccupancy(mp.x, mp.y))
        {
            gsData.colour = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);
        }
        else
        {
            gsData.colour = glm::vec4(0.0f, 1.0f, 0.0f, 1.0f);
        }

        vkCmdBindPipeline(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS, td->gridSquare->pipeline);

        vkCmdBindVertexBuffers(cmdBuff, 0, 1, &td->wallLight->buffers[0]->buffer, &offsets);

        vkCmdBindDescriptorSets(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS,
            td->gridSquare->pipelineLayout, 0, 1, &ShaderBase::sharedSet, 1, &camOffset);

        vkCmdPushConstants(cmdBuff, td->gridSquare->pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(s_gridSquareData), (void*)&gsData);

        vkCmdDraw(cmdBuff, td->wallLight->buffers[0]->count, 1, 0, 0);
    }
    else if (leveler->consqClickVec.size() == 1)
    {
        glm::ivec2 p0 = leveler->consqClickVec[0];

        int xDiff = std::max(std::min(mp.x - p0.x, maxPlatformDiff), -maxPlatformDiff);
        int yDiff = std::max(std::min(mp.y - p0.y, maxPlatformDiff), -maxPlatformDiff);

        glm::ivec2 p1 = p0 + glm::ivec2(xDiff, yDiff);

        int minX = std::min(p0.x, p1.x); int maxX = std::max(p0.x, p1.x);
        int minY = std::min(p0.y, p1.y); int maxY = std::max(p0.y, p1.y);

        int xSize = maxX - minX + 1;
        int ySize = maxY - minY + 1;

        float midX = (minX + maxX) * 0.5 + 0.5;
        float midY = (minY + maxY) * 0.5 + 0.5;

        s_platformRectUbo* platformUbo;
        vkMapMemory(td->logicalDevice, td->platformRectangle->uniformBuffers[0]->memory, td->platformRectangle->numPlatforms * sizeof(s_platformRectUbo),
            sizeof(s_platformRectUbo), 0, (void**)&platformUbo);
        platformUbo->posSize = glm::vec4(midX, midY, xSize, ySize);
        platformUbo->inColour = glm::vec4(0.6, 0.6, 0.6, 0.5);
        platformUbo->outColour = glm::vec4(1.0, 1.0, 1.0, 1.0);
        platformUbo->data = glm::vec4(0.0, 0.1, 1.0, 1.0);
        vkUnmapMemory(td->logicalDevice, td->platformRectangle->uniformBuffers[0]->memory);

        vkCmdBindPipeline(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS, td->platformRectangle->pipeline);

        vkCmdBindVertexBuffers(cmdBuff, 0, 1, &td->wallLight->buffers[0]->buffer, &offsets);

        vkCmdBindDescriptorSets(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS,
            td->platformRectangle->pipelineLayout, 0, 1, &ShaderBase::sharedSet, 1, &camOffset);

        vkCmdBindDescriptorSets(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS,
            td->platformRectangle->pipelineLayout, 1, 1, &td->platformRectangle->descriptorSets[1], 0, VK_NULL_HANDLE);

        vkCmdDraw(cmdBuff, td->wallLight->buffers[0]->count, 1, 0, td->platformRectangle->numPlatforms);


        vkCmdBindPipeline(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS, td->gridSquare->pipeline);

        vkCmdBindVertexBuffers(cmdBuff, 0, 1, &td->wallLight->buffers[0]->buffer, &offsets);

        vkCmdBindDescriptorSets(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS,
            td->gridSquare->pipelineLayout, 0, 1, &ShaderBase::sharedSet, 1, &camOffset);

        s_gridSquareData gsData;
        gsData.positionData[2] = 1;
        gsData.positionData[3] = 1;
        for (int y = minY; y <= maxY; y++)
        {
            for (int x = minX; x <= maxX; x++)
            {
                gsData.positionData[0] = x;
                gsData.positionData[1] = y;

                if (leveler->checkGridOccupancy(gsData.positionData[0], gsData.positionData[1]))
                {
                    gsData.colour = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);
                }
                else
                {
                    gsData.colour = glm::vec4(0.0f, 1.0f, 0.5f, 1.0f);
                }

                vkCmdPushConstants(cmdBuff, td->gridSquare->pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(s_gridSquareData), (void*)&gsData);

                vkCmdDraw(cmdBuff, td->wallLight->buffers[0]->count, 1, 0, 0);
            }
        }
    }
    else if (leveler->consqClickVec.size() == 2)
    {
        vkCmdBindPipeline(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS, td->platformRectangle->pipeline);

        vkCmdBindVertexBuffers(cmdBuff, 0, 1, &td->wallLight->buffers[0]->buffer, &offsets);

        vkCmdBindDescriptorSets(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS,
            td->platformRectangle->pipelineLayout, 0, 1, &ShaderBase::sharedSet, 1, &camOffset);

        vkCmdBindDescriptorSets(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS,
            td->platformRectangle->pipelineLayout, 1, 1, &td->platformRectangle->descriptorSets[1], 0, VK_NULL_HANDLE);

        vkCmdDraw(cmdBuff, td->wallLight->buffers[0]->count, 1, 0, td->platformRectangle->numPlatforms);


        vkCmdBindPipeline(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS, td->gridSquare->pipeline);

        vkCmdBindVertexBuffers(cmdBuff, 0, 1, &td->wallLight->buffers[0]->buffer, &offsets);

        vkCmdBindDescriptorSets(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS,
            td->gridSquare->pipelineLayout, 0, 1, &ShaderBase::sharedSet, 1, &camOffset);

        glm::ivec2 lp0;
        glm::ivec2 lp1;

        glm::vec2 absStart;
        glm::vec2 absEnd;
        glm::vec2 absDown;

        getPlatformLastStateSquares(leveler->consqClickVec[0], leveler->consqClickVec[1], mp, lp0, lp1, absStart, absEnd, absDown);
        
        s_gridSquareData gsData;
        gsData.positionData[2] = 1;
        gsData.positionData[3] = 1;
        for (int y = lp0.y; y <= lp1.y; y++)
        {
            for (int x = lp0.x; x <= lp1.x; x++)
            {
                gsData.positionData[0] = x;
                gsData.positionData[1] = y;

                if (leveler->checkGridOccupancy(x, y))
                {
                    gsData.colour = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);
                }
                else
                {
                    gsData.colour = glm::vec4(0.0f, 0.7f, 0.7f, 1.0f);
                }

                vkCmdPushConstants(cmdBuff, td->gridSquare->pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(s_gridSquareData), (void*)&gsData);

                vkCmdDraw(cmdBuff, td->wallLight->buffers[0]->count, 1, 0, 0);
			}
		}

        s_debugLineUbo* lineUbo;
        s_debugLineUbo copyLine;
        copyLine.colour = glm::vec4(1.0, 1.0, 1.0, 1.0);
        copyLine.data.y = 0.02; copyLine.data.z = 0.25;
        copyLine.data.x = 0.4; copyLine.data.w = 0.2;
        vkMapMemory(td->logicalDevice, td->debugLineShader->uniformBuffers[0]->memory, td->debugLineShader->numLines * sizeof(s_debugLineUbo),
            sizeof(s_debugLineUbo) * 3, 0, (void**)&lineUbo);
        *lineUbo = copyLine;
        lineUbo->positions = glm::vec4(absStart.x, absStart.y, absEnd.x, absEnd.y);

        lineUbo++;
        *lineUbo = copyLine;
        lineUbo->positions = glm::vec4(absStart.x - absDown.x, absStart.y - absDown.y, absStart.x + absDown.x, absStart.y + absDown.y);

        lineUbo++;
        *lineUbo = copyLine;
        lineUbo->positions = glm::vec4(absEnd.x - absDown.x, absEnd.y - absDown.y, absEnd.x + absDown.x, absEnd.y + absDown.y);

        vkUnmapMemory(td->logicalDevice, td->debugLineShader->uniformBuffers[0]->memory);

        vkCmdBindPipeline(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS, td->debugLineShader->pipeline);
        vkCmdBindVertexBuffers(cmdBuff, 0, 1, &td->debugCircleShader->buffers[0]->buffer, &offsets);

        vkCmdBindDescriptorSets(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS,
            td->debugLineShader->pipelineLayout, 0, 1, &ShaderBase::sharedSet, 1, &camOffset);

        vkCmdBindDescriptorSets(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS,
            td->debugLineShader->pipelineLayout, 1, 1, &td->debugLineShader->descriptorSets[1], 0, VK_NULL_HANDLE);

        vkCmdDraw(cmdBuff, td->debugCircleShader->buffers[0]->count, 3, 0, td->debugLineShader->numLines);
    }
}

void Leveler::renderGrid(VkCommandBuffer cmdBuff, void* ptr)
{
    Leveler* leveler = (Leveler*)ptr;
    ManyTexture* mt = leveler->ol->mt;
    ThinDrawer* td = leveler->thinDrawer;

    const VkDeviceSize offsets = 0;
    const uint32_t camOffset = ShaderBase::alignedSharedSize * td->lastSwapChainImageIndex;

    renderTextureds(cmdBuff, leveler);

    renderPlatforms(cmdBuff, leveler);

    if (leveler->ol->selectedItemIndex < 0) return;

    glm::ivec2 mp = leveler->getMouseGrid();

    const s_gridItemData& item = leveler->ol->items[leveler->ol->selectedItemIndex];
    const s_levelGridData& lgd = leveler->getSquare(mp.x, mp.y);

    if (item.objectType & SINGLE_DRAWS)
    {
        renderSingleDrawSelection(cmdBuff, leveler);
    }

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
        if (item.objectType & GRID_DRAWS && !(lgd.objectType & (SINGLE_DRAWS | PLATFORM) && item.objectType & ERASER))
        {
            vkCmdBindPipeline(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS, td->gridSquare->pipeline);

            vkCmdBindVertexBuffers(cmdBuff, 0, 1, &td->wallLight->buffers[0]->buffer, &offsets);

            vkCmdBindDescriptorSets(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS,
                td->gridSquare->pipelineLayout, 0, 1, &ShaderBase::sharedSet, 1, &camOffset);

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
        else if (lgd.objectType & (SINGLE_DRAWS | PLATFORM) && item.objectType & ERASER)
        {
            s_objectDataGrid* objectData = (s_objectDataGrid*)lgd.data.ptr;

            s_gridItemData& selectedItem = leveler->ol->items[getLevelObjectIndex(objectData->objectType)];

            short scaleX = objectData->scale.x;
            short scaleY = objectData->scale.y;

            vkCmdBindPipeline(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS, td->gridSquare->pipeline);

            vkCmdBindVertexBuffers(cmdBuff, 0, 1, &td->wallLight->buffers[0]->buffer, &offsets);

            vkCmdBindDescriptorSets(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS,
                td->gridSquare->pipelineLayout, 0, 1, &ShaderBase::sharedSet, 1, &camOffset);

            s_gridSquareData gsData;
            gsData.positionData[2] = 1;
            gsData.positionData[3] = 1;
            for (int y = 0; y < scaleY; y++)
            {
                for (int x = 0; x < scaleX; x++)
                {
                    gsData.positionData[0] = x + objectData->pos.x;
                    gsData.positionData[1] = y + objectData->pos.y;
                    gsData.colour = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);

                    vkCmdPushConstants(cmdBuff, td->gridSquare->pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(s_gridSquareData), (void*)&gsData);

                    vkCmdDraw(cmdBuff, td->wallLight->buffers[0]->count, 1, 0, 0);
                }
            }
        }
    }
}

void Leveler::calcMouseGrid()
{
    glm::vec2 mousePos = wh->cam->getMouseCoords();
    int px = keepXInBounds(std::floor(mousePos.x));
    int py = keepYInBounds(std::floor(mousePos.y));

    mouseGrid = glm::ivec2(px, py);
}

inline glm::ivec2 Leveler::getMouseGrid()
{
    return mouseGrid;
}

bool Leveler::checkGridOccupancy(short x, short y)
{
    if (keepInBounds({ x, y }) != glm::i16vec2(x, y)) return true;

    return getSquare(x, y).objectType & OCCUPIERS ? true : false;
}

short Leveler::keepXInBounds(short x)
{
    return std::min(std::max(x, (short)-xSize_H), (short)(xSize_H - 1));
}

short Leveler::keepYInBounds(short y)
{
    return std::min(std::max(y, (short)-ySize_H), (short)(ySize_H - 1));
}

bool Leveler::isInBounds(short x, short y)
{
    return x >= -xSize_H && x < xSize_H && y >= -ySize_H && y < ySize_H;
}

glm::i16vec2 Leveler::keepInBounds(glm::i16vec2 pos)
{
    return glm::i16vec2(keepXInBounds(pos.x), keepYInBounds(pos.y));
}

void Leveler::setObjectOperationTrack(operationType opType, s_objectDataGrid* objectData)
{
    s_gridOperationList* tempNextOp = currentOperation->next;
    currentOperation->next = new s_gridOperationList();
    currentOperation->next->prev = currentOperation;
    currentOperation = currentOperation->next;

    for (; tempNextOp;)
    {
        s_gridOperationList* del = tempNextOp;
        tempNextOp = tempNextOp->next;
        delete del;
    }

    currentOperation->objectOperation = new s_gridOperation_o();
    currentOperation->objectOperation->operationType = opType;
    currentOperation->objectOperation->objData = objectData;
}

void Leveler::update()
{
    calcMouseGrid();

    ol->update();

    s_gridItemData* item = ol->selectedItemIndex >= 0 ? &ol->items[ol->selectedItemIndex] : nullptr;

    if (wh->mouseData[3] == 2 || wh->keyData[GLFW_KEY_ESCAPE] == 2)
    {
        clickStart[0] = -100000;
        consqClickVec.clear();
    }

    if (wh->mouseData[2] == 0)
    {
        if (clickStart[0] > -100000)
        {
            double st = glfwGetTime();
            
            glm::ivec2 mp = getMouseGrid();

            int minX = clickStart.x > mp.x ? mp.x : clickStart.x;
            int maxX = clickStart.x > mp.x ? clickStart.x : mp.x;

            int minY = clickStart.y > mp.y ? mp.y : clickStart.y;
            int maxY = clickStart.y > mp.y ? clickStart.y : mp.y;

            levelObject fillType = item->objectType;

            fillGrid(minX, minY, maxX, maxY, fillType);

            printf("Fill Time: %f\n", glfwGetTime() - st);
        }

        clickStart.x = -100000;
    }
    else if (clickStart.x > -100000)
    {
        glm::ivec2 mousePos = getMouseGrid();
        if (mousePos != lastGrid)
        {
            ALuint source = ol->sounder->playSound(ol->bopSound, 0.001);
            alSourcei(source, AL_SOURCE_RELATIVE, AL_TRUE);
        }

        lastGrid = mousePos;
    }
    else if (wh->mouseData[2] == 2 && !ol->itemDir && item)
    {
        if (item->objectType & GRID_DRAWS)
        {
            glm::ivec2 mousePos = getMouseGrid();

            const s_levelGridData& lgd = getSquare(mousePos.x, mousePos.y);
            if (item->objectType == ERASER && lgd.objectType & (SINGLE_DRAWS | PLATFORM))
            {
                s_objectDataGrid* objectData = (s_objectDataGrid*)lgd.data.ptr;
                removeObject(objectData->pos.x, objectData->pos.y);
                setObjectOperationTrack(REMOVE, objectData);

                ALuint source = ol->sounder->playSound(ol->removeSound, 0.01);
                alSourcei(source, AL_SOURCE_RELATIVE, AL_TRUE);
            }
            else
            {
                clickStart = mousePos;
                lastGrid = clickStart;
            }
        }
        else if (item->objectType & SINGLE_DRAWS)
        {
            glm::ivec2 mousePos = getMouseGrid();

            const s_gridItemData& item = ol->items[ol->selectedItemIndex];
            short scaleX = item.gridScale[selectedObjectRotation % 2];
            short scaleY = item.gridScale[(selectedObjectRotation % 2) ^ 1];

            s_objectDataGrid* objectData = putObject(item, mousePos.x - scaleX / 2, mousePos.y - scaleY / 2, scaleX, scaleY, selectedObjectRotation, 0);

            if (objectData)
            {
                setObjectOperationTrack(ADD, objectData);
            }
        }
        else if (item->objectType & PLATFORM)
        {
            putPlatformOnClick();
        }
    }

    if (wh->keyData[wh->pIndLeftCommand] && wh->keyData[GLFW_KEY_Z] == 2)
    {
        double st = glfwGetTime();

        takeBack(wh->keyData[GLFW_KEY_LEFT_SHIFT] != 0);

        printf("Rev Time: %f\n", glfwGetTime() - st);
    }

    if (wh->keyData[GLFW_KEY_Q] == 2 && item && item->canRotate && item->objectType & SINGLE_DRAWS)
    {
        selectedObjectRotation = (selectedObjectRotation + 1) % 4;
    }
}

void Leveler::putPlatformOnClick()
{
    glm::ivec2 mp = getMouseGrid();

    if (consqClickVec.size() == 0)
    {
        if (!checkGridOccupancy(mp.x, mp.y))
        {
            consqClickVec.push_back(mp);
        }
    }
    else if (consqClickVec.size() == 1)
    {
        glm::ivec2 p0 = consqClickVec[0];

        int xDiff = std::max(std::min(mp.x - p0.x, maxPlatformDiff), -maxPlatformDiff);
        int yDiff = std::max(std::min(mp.y - p0.y, maxPlatformDiff), -maxPlatformDiff);

        glm::ivec2 p1 = p0 + glm::ivec2(xDiff, yDiff);

        int minX = std::min(p0.x, p1.x); int maxX = std::max(p0.x, p1.x);
        int minY = std::min(p0.y, p1.y); int maxY = std::max(p0.y, p1.y);

        for (int y = minY; y <= maxY; y++)
        {
            for (int x = minX; x <= maxX; x++)
            {
                if (checkGridOccupancy(x, y))
                {
					return;
				}
			}
		}

        consqClickVec.push_back(p1);
    }
    else
    {
        glm::ivec2 lp0;
        glm::ivec2 lp1;

        glm::vec2 absStart;
        glm::vec2 absEnd;
        glm::vec2 absDown;

        glm::ivec2 p0 = consqClickVec[0];
        glm::ivec2 p1 = consqClickVec[1];

        getPlatformLastStateSquares(p0, p1, mp, lp0, lp1, absStart, absEnd, absDown);

        if (abs(absDown.x) > 0.0001f && abs(absDown.y) > 0.0001f)
        {
			return;
		}

        for (int y = lp0.y; y <= lp1.y; y++)
        {
            for (int x = lp0.x; x <= lp1.x; x++)
            {
                if (checkGridOccupancy(x, y))
                {
                    return;
                }
            }
        }

        s_gridSquare pos, size;
        int pSize, rotation;

        short downY = std::min(p0.y, p1.y); short downYDiff = downY - mp.y;
        short upY = std::max(p0.y, p1.y); short upYDiff = mp.y - upY;
        short leftX = std::min(p0.x, p1.x); short leftXDiff = leftX - mp.x;
        short rightX = std::max(p0.x, p1.x); short rightXDiff = mp.x - rightX;

        short maxDiff = std::max(std::max(downYDiff, upYDiff), std::max(leftXDiff, rightXDiff));

        if (downYDiff == maxDiff)
        {
            pos = { leftX, (short)(downY - maxDiff) }; size = { (short)(rightX - leftX + 1), (short)(maxDiff + upY - downY + 1) }; pSize = upY - downY + 1; rotation = 0;
        }
        else if (rightXDiff == maxDiff)
        {
            pos = { leftX, downY }; size = { (short)(maxDiff + rightX - leftX + 1), (short)(upY - downY + 1) }; pSize = rightX - leftX + 1; rotation = 1;
        }
        else if (upYDiff == maxDiff)
        {
            pos = { leftX, downY }; size = { (short)(rightX - leftX + 1), (short)(maxDiff + upY - downY + 1) }; pSize = upY - downY + 1; rotation = 2;
        }
        else if (leftXDiff == maxDiff)
        {
            pos = { (short)(leftX - maxDiff), downY }; size = { (short)(maxDiff + rightX - leftX + 1), (short)(upY - downY + 1) }; pSize = rightX - leftX + 1; rotation = 3;
        }

        s_objectDataGrid* objectData = putPlatform(pos, size, pSize, rotation);

        setObjectOperationTrack(ADD, objectData);

        consqClickVec.clear();
    }
}

s_objectDataGrid* Leveler::putPlatform(s_gridSquare pos, s_gridSquare size, int pSize, int rotation)
{
    glm::vec2 lineStart;
    glm::vec2 lineEnd;
    float lineWidth;
    glm::vec2 platformPos;
    glm::vec2 platformSize;

    switch (rotation % 4)
    {
    case 0:
        lineStart = { pos.x + size.x * 0.5f, pos.y }; lineEnd = { lineStart.x, pos.y + size.y }; lineWidth = size.x;
        platformPos = { lineEnd.x, lineEnd.y - pSize * 0.5f }; platformSize = { size.x, pSize };
		break;
    case 1:
        lineStart = { pos.x, pos.y + size.y * 0.5f }; lineEnd = { pos.x + size.x, lineStart.y }; lineWidth = size.y;
        platformPos = { pos.x + pSize * 0.5f, lineStart.y }; platformSize = { pSize, size.y };
        break;
    case 2:
        lineStart = { pos.x + size.x * 0.5f, pos.y }; lineEnd = { lineStart.x, pos.y + size.y }; lineWidth = size.x;
        platformPos = { lineStart.x, pos.y + pSize * 0.5f }; platformSize = { size.x, pSize };
        break;
    case 3:
        lineStart = { pos.x, pos.y + size.y * 0.5f }; lineEnd = { pos.x + size.x, lineStart.y }; lineWidth = size.y;
        platformPos = { lineEnd.x - pSize * 0.5f, lineStart.y }; platformSize = { pSize, size.y };
		break;
    }

    glm::vec2 rightLine = rotateVec(glm::normalize(lineEnd - lineStart) * 0.5f, glm::pi<float>() * 0.5) * lineWidth;

    putBarrierOnMemories({ thinDrawer->platformRectangle, thinDrawer->debugLineShader });

    s_platformRectUbo* platformUbo;
    vkMapMemory(thinDrawer->logicalDevice, thinDrawer->platformRectangle->uniformBuffers[0]->memory,
        thinDrawer->platformRectangle->numPlatforms * sizeof(s_platformRectUbo), sizeof(s_platformRectUbo), 0, (void**)&platformUbo);
    platformUbo->posSize = glm::vec4(platformPos.x, platformPos.y, platformSize.x, platformSize.y);
    platformUbo->inColour = glm::vec4(0.6, 0.6, 0.6, 0.5);
    platformUbo->outColour = glm::vec4(1.0, 1.0, 1.0, 1.0);
    platformUbo->data = glm::vec4(0.0, 0.1, 1.0, 1.0);
    vkUnmapMemory(thinDrawer->logicalDevice, thinDrawer->platformRectangle->uniformBuffers[0]->memory);

    s_debugLineUbo* lineUbo;
    s_debugLineUbo copyLine;
    copyLine.colour = glm::vec4(1.0, 1.0, 1.0, 1.0);
    copyLine.data.y = 0.02; copyLine.data.z = 0.25;
    copyLine.data.x = 0.4; copyLine.data.w = 0.2;
    vkMapMemory(thinDrawer->logicalDevice, thinDrawer->debugLineShader->uniformBuffers[0]->memory,
        thinDrawer->debugLineShader->numLines * sizeof(s_debugLineUbo), sizeof(s_debugLineUbo) * 3, 0, (void**)&lineUbo);
    
    *lineUbo = copyLine;
    lineUbo->positions = glm::vec4(lineStart.x, lineStart.y, lineEnd.x, lineEnd.y);

    lineUbo++;
    *lineUbo = copyLine;
    lineUbo->positions = glm::vec4(lineStart.x - rightLine.x, lineStart.y - rightLine.y, lineStart.x + rightLine.x, lineStart.y + rightLine.y);

    lineUbo++;
    *lineUbo = copyLine;
    lineUbo->positions = glm::vec4(lineEnd.x - rightLine.x, lineEnd.y - rightLine.y, lineEnd.x + rightLine.x, lineEnd.y + rightLine.y);

    vkUnmapMemory(thinDrawer->logicalDevice, thinDrawer->debugLineShader->uniformBuffers[0]->memory);

    s_objectDataGrid* objectData = new s_objectDataGrid();
    s_gridData uData; uData.ptr = (void*)objectData;

    for (int i = 0; i < size.y; i++)
    {
        for (int j = 0; j < size.x; j++)
        {
            int x = pos.x + j; int y = pos.y + i;
            const s_levelGridData& currentGridData = getSquare(x, y);
            setSquare(x, y, { (levelObject)(PLATFORM | currentGridData.objectType), uData });
        }
    }

    objectData->offset = thinDrawer->platformRectangle->numPlatforms;
    objectData->objectType = PLATFORM;
    objectData->rotation = rotation;
    objectData->pos = { pos.x, pos.y };
    objectData->scale = { size.x, size.y };
    objectData->extraScale = { pSize, 0 };
    objectData->extraOffsets.push_back(thinDrawer->debugLineShader->numLines);

    levelObjects.insert(objectData);

    lastOffsetPos_Platform.push_back({ pos.x, pos.y });

    thinDrawer->platformRectangle->numPlatforms++;
    thinDrawer->debugLineShader->numLines += 3;

    return objectData;
}

void Leveler::removePlatform(short x, short y)
{
    const s_levelGridData& lgd = getSquare(x, y);

    s_objectDataGrid* objectData = (s_objectDataGrid*)lgd.data.ptr;
    int objIndex = getLevelObjectIndex(objectData->objectType);

    const s_gridItemData& item = ol->items[objIndex];
    short sizeX = objectData->scale.x;
    short sizeY = objectData->scale.y;

    putBarrierOnMemories({ thinDrawer->platformRectangle, thinDrawer->debugLineShader });

    for (int i = 0; i < sizeY; i++)
    {
        for (int j = 0; j < sizeX; j++)
        {
            const s_levelGridData& currentGridData = getSquare(x + j, y + i);
            setSquare(x + j, y + i, { (LevelObject)(currentGridData.objectType & (PLATFORM ^ -1ll)), 0 });
        }
    }

    levelObjects.erase(objectData);

    auto& reqVec = lastOffsetPos_Platform;
    s_gridSquare lastOffsetSquare = reqVec.back();

    thinDrawer->debugLineShader->numLines -= 3;
    thinDrawer->platformRectangle->numPlatforms--;

    if (lastOffsetSquare.asGlm() == objectData->pos)
    {
        reqVec.pop_back();
        return;
    }

    const s_levelGridData& lastSquareData = getSquare(lastOffsetSquare.x, lastOffsetSquare.y);
    s_objectDataGrid* lastObjectData = (s_objectDataGrid*)lastSquareData.data.ptr;

    int thisSquareOffsetIndex = objectData->offset;
    reqVec[thisSquareOffsetIndex] = reqVec.back();
    reqVec.pop_back();

    int lastPlatformOffset = lastObjectData->offset * sizeof(s_platformRectUbo); int thisPlatformOffset = objectData->offset * sizeof(s_platformRectUbo);
    int lastLineOffset = lastObjectData->extraOffsets[0] * sizeof(s_debugLineUbo); int thisLineOffset = objectData->extraOffsets[0] * sizeof(s_debugLineUbo);

    moveVkMem(thinDrawer->platformRectangle, lastPlatformOffset, thisPlatformOffset, sizeof(s_platformRectUbo));
    moveVkMem(thinDrawer->debugLineShader, lastLineOffset, thisLineOffset, sizeof(s_debugLineUbo) * 3);

    lastObjectData->offset = objectData->offset;
    lastObjectData->extraOffsets[0] = objectData->extraOffsets[0];
}

s_objectDataGrid* Leveler::putObject(const s_gridItemData& item, short startX, short startY, short scaleX, short scaleY, int rotation, int pSize)
{
    if (item.objectType & PLATFORM)
    {
        return putPlatform({ startX, startY }, { scaleX, scaleY }, pSize, rotation);
    }

    glm::vec2 extraAdd = rotationMults[rotation] * item.gridRenderTranslation.y;

    short endX = startX + scaleX;
    short endY = startY + scaleY;

    short centerX = startX + scaleX / 2;
    short centerY = startY + scaleY / 2;

    glm::i16vec4 sideArea = { startX, endX, startY, endY };

    for (int y = startY; y < endY; y++)
    {
        for (int x = startX; x < endX; x++)
        {
            if (checkGridOccupancy(x, y))
            {
                return nullptr;
            }
        }
    }

    levelObject depType = NONE_LO;

    if (item.requiredSideFill != -1)
    {
        switch ((rotation + item.requiredSideFill) % 4)
        {
        case 0:
            sideArea.z = sideArea.z - 1;
            sideArea.w = sideArea.z + 1;
            depType = DOWN_DEPENDENT;
            break;
        case 1:
            sideArea.x = sideArea.y;
            sideArea.y = sideArea.x + 1;
            depType = RIGHT_DEPENDENT;
            break;
        case 2:
            sideArea.z = sideArea.w;
            sideArea.w = sideArea.z + 1;
            depType = UP_DEPENDENT;
            break;
        case 3:
            sideArea.x = sideArea.x - 1;
            sideArea.y = sideArea.x + 1;
            depType = LEFT_DEPENDENT;
            break;
        default:
            break;
        }

        for (int y = sideArea.z; y < sideArea.w; y++)
        {
            for (int x = sideArea.x; x < sideArea.y; x++)
            {
                if (isInBounds(x, y) && getSquare(x, y).objectType & (WHITE_WALL | BLACK_WALL))
                {

                }
                else
                {
                    return nullptr;
                }
            }
        }
    }

    s_objectDataGrid* objectData = new s_objectDataGrid();
    s_gridData uData; uData.ptr = (void*)objectData;
    for (int y = startY; y < endY; y++)
    {
        for (int x = startX; x < endX; x++)
        {
            const s_levelGridData& currentGridData = getSquare(x, y);
            setSquare(x, y, { (levelObject)(item.objectType | currentGridData.objectType), uData });
        }
    }

    for (int y = sideArea.z; y < sideArea.w; y++)
    {
        for (int x = sideArea.x; x < sideArea.y; x++)
        {
            const s_levelGridData& currentGridData = getSquare(x, y);
            setSquare(x, y, { (levelObject)(depType | currentGridData.objectType), currentGridData.data });
        }
    }

    putBarrierOnMemories({ thinDrawer->manyTexture });

    int index = getLevelObjectIndex(item.objectType) - 4;
    s_perTextureData* pdData = &perTextureData[index];

    int renderIndex = pdData->currentOffset;
    lastOffsetPos_M[index].push_back({ startX, startY });
    s_manyTextureData* mtd;
    vkMapMemory(mt->logicalDevice, mt->uniformBuffers[0]->memory, renderIndex * sizeof(s_manyTextureData),
        sizeof(s_manyTextureData), 0, (void**)&mtd);

    float hAdd = scaleX % 2 == 1 ? 0.5 : 0.0;
    float vAdd = scaleY % 2 == 1 ? 0.5 : 0.0;

    mtd->posSize = glm::vec4(centerX + hAdd + extraAdd.x, centerY + vAdd + extraAdd.y, item.gridRenderScale, item.gridRenderScale);
    mtd->data.x = rotation * b2_pi * 0.5;
    vkUnmapMemory(mt->logicalDevice, mt->uniformBuffers[0]->memory);

    objectData->offset = pdData->currentOffset;
    objectData->objectType = item.objectType;
    objectData->rotation = rotation;
    objectData->pos = { startX, startY };
    objectData->scale = { scaleX, scaleY };

    afterPutObject(item, objectData, scaleX, scaleY);

    levelObjects.insert(objectData);
    pdData->currentOffset += item.numTextureObjects;

    return objectData;
}

bool Leveler::extraPutCheck(const s_gridItemData& item, short startX, short startY, short scaleX, short scaleY, int rotation, int pSize)
{
    switch (item.objectType)
    {
    case HAZARDER:
    case FORCE_FIELD:
    {
        glm::vec2 startPoint;
        glm::vec2 endPoint;
        sendObjectRay(item, startX, startY, rotation, scaleX, scaleY, 0.5, 1.5, (LevelObject)GRID_DRAWS, startPoint, endPoint);
    }
        break;

    default:
        break;
    }
}

void Leveler::sendObjectRay(const s_gridItemData& item, short x, short y, short rotation, short scaleX, short scaleY, float startOffset, float endOffset,
    levelObject objectMask, glm::vec2& startPoint, glm::vec2& endPoint)
{
    glm::i16vec2 startPosition;
    glm::i16vec2 frwdDir;

    switch (rotation)
    {
    case 0:
        startPosition = { x, y };
        frwdDir = { 0, 1 };
        break;
    case 1:
        startPosition = { x, y };
        frwdDir = { -1, 0 };
        break;
    case 2:
        startPosition = { x + scaleX - 1, y };
        frwdDir = { 0, -1 };
        break;
    case 3:
        startPosition = { x, y + scaleY - 1 };
        frwdDir = { 1, 0 };
        break;
    default:
        break;
    }

    glm::i16vec2 rightDir = { frwdDir.y, -frwdDir.x };

    int finalX;
    int finalY;
    int roadSize = 0;

    while (1)
    {
        roadSize += 1;
        for (int i = 0; i < item.gridScale.x; i++)
        {
            finalX = startPosition.x + frwdDir.x * roadSize + rightDir.x * i;
            finalY = startPosition.y + frwdDir.y * roadSize + rightDir.y * i;

            if (!isInBounds(finalX, finalY))
            {
                goto end;
            }

            const s_levelGridData& lgd = getSquare(finalX, finalY);
            if (lgd.objectType & objectMask)
            {
                goto end;
            }
        }
    }
end:
    startPoint = glm::vec2(
        startPosition.x + 0.5 + 0.5f * rightDir.x * (scaleX - 1.0) - (0.5f - startOffset) * frwdDir.x,
        startPosition.y + 0.5 + 0.5f * rightDir.y * (scaleY - 1.0) - (0.5f - startOffset) * frwdDir.y
    );

    endPoint = glm::vec2(startPoint.x + frwdDir.x * (roadSize - (startOffset + endOffset)), startPoint.y + frwdDir.y * (roadSize - (startOffset + endOffset)));
}

void Leveler::afterPutObject(const s_gridItemData& item, s_objectDataGrid* objectData, short scaleX, short scaleY)
{
    short x = objectData->pos.x;
    short y = objectData->pos.y;

    switch (item.objectType & SINGLE_DRAWS)
    {
    case LASERER:
    {
        lastOffsetPos_Laser.push_back({ x, y });
        objectData->extraOffsets.push_back(thinDrawer->laserShader->numLasers);

        glm::vec2 startPoint;
        glm::vec2 endPoint;
        sendObjectRay(item, x, y, objectData->rotation, scaleX, scaleY, 0.5, 0.0, (levelObject)(WHITE_WALL | BLACK_WALL), startPoint, endPoint);

        ShaderBase* shader = thinDrawer->laserShader;
        s_laserUbo* laserUbo;
        vkMapMemory(shader->logicalDevice, shader->uniformBuffers[0]->memory,
            objectData->extraOffsets[0] * sizeof(s_laserUbo), sizeof(s_laserUbo), 0, (void**)&laserUbo);
        laserUbo->points = glm::vec4(startPoint.x, startPoint.y, endPoint.x, endPoint.y);
        laserUbo->colour = glm::vec4(1.0, 0.0, 0.0, 1.0);
        laserUbo->data = glm::vec4(0.4, 11.0, 36.0, 0.0);
        vkUnmapMemory(shader->logicalDevice, shader->uniformBuffers[0]->memory);

        thinDrawer->laserShader->numLasers++;
    }
    break;

    case FUNNEL:
    {
        lastOffsetPos_Funnel.push_back({ x, y });
        objectData->extraOffsets.push_back(thinDrawer->funnelShader->numFunnels);

        glm::vec2 startPoint;
        glm::vec2 endPoint;
        sendObjectRay(item, x, y, objectData->rotation, scaleX, scaleY, 0.1, 0.0, (levelObject)(WHITE_WALL | BLACK_WALL | GLASS_WALL), startPoint, endPoint);

        ShaderBase* shader = thinDrawer->funnelShader;
        s_funnelUbo* funnelUbo;
        vkMapMemory(shader->logicalDevice, shader->uniformBuffers[0]->memory,
            objectData->extraOffsets[0] * sizeof(s_funnelUbo), sizeof(s_funnelUbo), 0, (void**)&funnelUbo);

        funnelUbo->pointts = glm::vec4(startPoint.x, startPoint.y, endPoint.x, endPoint.y);
        float diffLen = glm::length(endPoint - startPoint);
        funnelUbo->colour = glm::vec4(0.0, 0.3, 1.0, 1.0);
        funnelUbo->colour2 = glm::vec4(0.0, 0.3, 1.0, 0.0);
        funnelUbo->data = glm::vec4(1.75, game->gameTime, 0.0, -1.0);
        funnelUbo->data2 = glm::vec4(-10.0, diffLen - 1.0, diffLen, game->gameTime);

        vkUnmapMemory(shader->logicalDevice, shader->uniformBuffers[0]->memory);

        thinDrawer->funnelShader->numFunnels++;
    }
    case HAZARDER:
    {

    }
        break;
    break;

    default:
        break;
    }
}

void Leveler::removeObject(short x, short y)
{
    const s_levelGridData& lgd = getSquare(x, y);

    if (lgd.objectType & PLATFORM)
    {
        removePlatform(x, y);
    }

    if (!(lgd.objectType & SINGLE_DRAWS))
    {
		return;
	}

    putBarrierOnMemories({ thinDrawer->manyTexture });

    s_objectDataGrid* objectData = (s_objectDataGrid*)lgd.data.ptr;
    int objIndex = getLevelObjectIndex(objectData->objectType);

    const s_gridItemData& item = ol->items[objIndex];
    short sizeX = objectData->scale.x;
    short sizeY = objectData->scale.y;

    glm::i16vec4 sideArea = { x, x + sizeX, y, y + sizeY };

    levelObject depType = NONE_LO;
    if (item.requiredSideFill != -1)
    {
        switch ((objectData->rotation + item.requiredSideFill) % 4)
        {
        case 0:
            sideArea.z = sideArea.z - 1;
            sideArea.w = sideArea.z + 1;
            depType = (levelObject)(DOWN_DEPENDENT ^ -1ll);
            break;
        case 1:
            sideArea.x = sideArea.y;
            sideArea.y = sideArea.x + 1;
            depType = (levelObject)(RIGHT_DEPENDENT ^ -1ll);
            break;
        case 2:
            sideArea.z = sideArea.w;
            sideArea.w = sideArea.z + 1;
            depType = (levelObject)(UP_DEPENDENT ^ -1ll);
            break;
        case 3:
            sideArea.x = sideArea.x - 1;
            sideArea.y = sideArea.x + 1;
            depType = (levelObject)(LEFT_DEPENDENT ^ -1ll);
            break;
        default:
            break;
        }
    }

    for (int i = 0; i < sizeY; i++)
    {
        for (int j = 0; j < sizeX; j++)
        {
            const s_levelGridData& currentGridData = getSquare(x + j, y + i);
            setSquare(x + j, y + i, { (LevelObject)(currentGridData.objectType & (lgd.objectType ^ -1ll)), 0});
        }
    }

    for (int y = sideArea.z; y < sideArea.w; y++)
    {
        for (int x = sideArea.x; x < sideArea.y; x++)
        {
            const s_levelGridData& currentGridData = getSquare(x, y);
            setSquare(x, y, { (levelObject)(depType & currentGridData.objectType), currentGridData.data });
        }
    }

    levelObjects.erase(objectData);

    afterRemoveObject(item, objectData, sizeX, sizeY);

    auto& reqVec = lastOffsetPos_M[objIndex - 4];
    s_gridSquare lastOffsetSquare = reqVec.back();

    perTextureData[objIndex - 4].currentOffset -= item.numTextureObjects;

    if (lastOffsetSquare.asGlm() == objectData->pos)
    {
        reqVec.pop_back();
        return;
    }
    
    const s_levelGridData& lastSquareData = getSquare(lastOffsetSquare.x, lastOffsetSquare.y);
    s_objectDataGrid* lastObjectData = (s_objectDataGrid*)lastSquareData.data.ptr;

    int thisSquareOffsetIndex = (objectData->offset - perTextureData[objIndex - 4].offsetStart) / item.numTextureObjects;
    reqVec[thisSquareOffsetIndex] = reqVec.back();
    reqVec.pop_back();

    moveVkMem(thinDrawer->manyTexture, lastObjectData->offset * sizeof(s_manyTextureData), objectData->offset * sizeof(s_manyTextureData),
        sizeof(s_manyTextureData) * item.numTextureObjects);

    lastObjectData->offset = objectData->offset;
}

void Leveler::afterRemoveObject(const s_gridItemData& item, s_objectDataGrid* objectData, short scaleX, short scaleY)
{
    switch (objectData->objectType)
    {
    case LASERER:
    {
        s_gridSquare lastOffsetSquare = lastOffsetPos_Laser.back();
        lastOffsetPos_Laser.pop_back();

        thinDrawer->laserShader->numLasers--;

        if (lastOffsetSquare.asGlm() == objectData->pos)
        {
            return;
        }

        const s_levelGridData& lastSquareData = getSquare(lastOffsetSquare.x, lastOffsetSquare.y);
        s_objectDataGrid* lastObjectData = (s_objectDataGrid*)lastSquareData.data.ptr;

        int thisSquareOffsetIndex = objectData->extraOffsets[0];
        lastOffsetPos_Laser[thisSquareOffsetIndex] = lastOffsetSquare;

        moveVkMem(thinDrawer->laserShader, lastObjectData->extraOffsets[0] * sizeof(s_laserUbo),
            objectData->extraOffsets[0] * sizeof(s_laserUbo), sizeof(s_laserUbo));

        lastObjectData->extraOffsets[0] = objectData->extraOffsets[0];
    }
        break;
    case FUNNEL:
    {
        s_gridSquare lastOffsetSquare = lastOffsetPos_Funnel.back();
        lastOffsetPos_Funnel.pop_back();

        thinDrawer->funnelShader->numFunnels--;

        if (lastOffsetSquare.asGlm() == objectData->pos)
        {
            return;
        }

        const s_levelGridData& lastSquareData = getSquare(lastOffsetSquare.x, lastOffsetSquare.y);
        s_objectDataGrid* lastObjectData = (s_objectDataGrid*)lastSquareData.data.ptr;

        int thisSquareOffsetIndex = objectData->extraOffsets[0];
        lastOffsetPos_Funnel[thisSquareOffsetIndex] = lastOffsetSquare;

        moveVkMem(thinDrawer->funnelShader, lastObjectData->extraOffsets[0] * sizeof(s_funnelUbo),
            objectData->extraOffsets[0] * sizeof(s_funnelUbo), sizeof(s_funnelUbo));

        lastObjectData->extraOffsets[0] = objectData->extraOffsets[0];
    }
        break;

    default:
        break;
    }
}

void Leveler::takeBack(bool shiftPressed)
{
    if (shiftPressed)
    {
        if (currentOperation->next)
        {
            currentOperation = currentOperation->next;

            if (!currentOperation->objectOperation)
            {
                putBarrierOnMemories({ thinDrawer->whiteWallShader, thinDrawer->blackWallShader, thinDrawer->diagonalLiner, thinDrawer->laserShader });

                for (int i = 0; i < currentOperation->operations.size(); i++)
                {
                    auto operation = currentOperation->operations[i];
                    switch (operation.operationType)
                    {
                    case REMOVE:
                        removeGridSquare(operation.pos.x, operation.pos.y);
                        break;

                    case ADD:
                        addGridSquare(operation.pos.x, operation.pos.y, operation.objectType);
                        break;

                    default:
                        break;
                    }
                }

                afterFillRayCheck(currentOperation->region.x, currentOperation->region.z, currentOperation->region.y, currentOperation->region.w);
                sideUpdate(currentOperation->region.x, currentOperation->region.y, currentOperation->region.z, currentOperation->region.w);
            }
            else
            {
                glm::i16vec2 sq = currentOperation->objectOperation->objData->pos;
                switch (currentOperation->objectOperation->operationType)
                {
                case REMOVE:
                    removeObject(sq.x, sq.y);
                    break;

                case ADD:
                {
                    s_objectDataGrid* objectData = currentOperation->objectOperation->objData;
                    int objIndex = getLevelObjectIndex(objectData->objectType);
                    const s_gridItemData& item = ol->items[objIndex];

                    putObject(item, sq.x, sq.y, objectData->scale.x, objectData->scale.y, objectData->rotation, objectData->extraScale.x);
                }
                    break;

                default:
                    break;
                }
            }
        }
    }
    else if (currentOperation->prev)
    {
        if (!currentOperation->objectOperation)
        {
            putBarrierOnMemories({ thinDrawer->whiteWallShader, thinDrawer->blackWallShader, thinDrawer->diagonalLiner, thinDrawer->laserShader });

            for (int i = currentOperation->operations.size() - 1; i >= 0; i--)
            {
                auto operation = currentOperation->operations[i];
                switch (operation.operationType)
                {
                case ADD:
                    removeGridSquare(operation.pos.x, operation.pos.y);
                    break;

                case REMOVE:
                    addGridSquare(operation.pos.x, operation.pos.y, operation.objectType);
                    break;

                default:
                    break;
                }
            }

            afterFillRayCheck(currentOperation->region.x, currentOperation->region.z, currentOperation->region.y, currentOperation->region.w);
            sideUpdate(currentOperation->region.x, currentOperation->region.y, currentOperation->region.z, currentOperation->region.w);
        }
        else
        {
            glm::i16vec2 sq = currentOperation->objectOperation->objData->pos;
            switch (currentOperation->objectOperation->operationType)
            {
            case REMOVE:
            {
                s_objectDataGrid* objectData = currentOperation->objectOperation->objData;
                int objIndex = getLevelObjectIndex(objectData->objectType);
                const s_gridItemData& item = ol->items[objIndex];

                putObject(item, sq.x, sq.y, objectData->scale.x, objectData->scale.y, objectData->rotation, objectData->extraScale.x);
            }
                break;

            case ADD:
                removeObject(sq.x, sq.y);
                break;

            default:
                break;
            }
        }
        
        currentOperation = currentOperation->prev;
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

void Leveler::putBarrierOnMemories(std::vector<ShaderBase*> shaders)
{
    VkCommandBuffer cmd = thinDrawer->createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, thinDrawer->uploadPool, true);

    VkBufferMemoryBarrier bufferBarrier = {};
    bufferBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
    bufferBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT; // Previous writes from shaders
    bufferBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT; // Future reads/writes from shaders
    bufferBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    bufferBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    bufferBarrier.offset = 0;
    bufferBarrier.size = VK_WHOLE_SIZE;

    for (int i = 0; i < shaders.size(); i++)
    {
        bufferBarrier.buffer = shaders[i]->uniformBuffers[0]->buffer;

        vkCmdPipelineBarrier(
            cmd,
            VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            0,
            0, nullptr,
            1, &bufferBarrier,
            0, nullptr
        );
    }

    thinDrawer->flushCommandBuffer(cmd);
}

void Leveler::fillGrid(short minX, short minY, short maxX, short maxY, levelObject fillType)
{
    s_gridOperationList* tempNextOp = currentOperation->next;
    currentOperation->next = new s_gridOperationList();

    void(Leveler::*gridSquareAdder)(short x, short y);

    switch (fillType)
    {
    case WHITE_WALL:
        gridSquareAdder = &Leveler::addGridSquare_W;
        break;
    case BLACK_WALL:
        gridSquareAdder = &Leveler::addGridSquare_B;
        break;
    case GLASS_WALL:
        gridSquareAdder = &Leveler::addGridSquare_G;
        break;
    case FENCE_WALL:
        gridSquareAdder = &Leveler::addGridSquare_F;
		break;
    case ERASER:
        gridSquareAdder = &Leveler::addGridSquare_E;
		break;
    default:
        return;
    }

    // guaranteed version
    //vkDeviceWaitIdle(thinDrawer->logicalDevice);

    // promising version!!!!!!!!!!!!!!!!!!!
    putBarrierOnMemories({ thinDrawer->whiteWallShader, thinDrawer->blackWallShader, thinDrawer->diagonalLiner, thinDrawer->laserShader });

    for (short i = minY; i <= maxY; i++)
    {
        for (short j = minX; j <= maxX; j++)
        {
            s_levelGridData lgData = getSquare(j, i);

            if (lgData.objectType == fillType || lgData.objectType & (SINGLE_DRAWS | PLATFORM) ||
                (lgData.objectType & GRID_DEPS && !(fillType & (BLACK_WALL | WHITE_WALL))))
            {
                continue;
            }
            else if (lgData.objectType != NONE_LO)
            {
                removeGridSquare(j, i);
                currentOperation->next->operations.push_back({ { j, i }, REMOVE, lgData.objectType });
            }

            (this->*gridSquareAdder)(j, i);
            if (fillType != ERASER)
            {
                currentOperation->next->operations.push_back({ { j, i }, ADD, fillType });
            }
        }
    }

    if (currentOperation->next->operations.size() == 0)
    {
        delete currentOperation->next;
        currentOperation->next = nullptr;
    }
    else
    {
        sideUpdate(minX, maxX, minY, maxY);

        currentOperation->next->prev = currentOperation;
        currentOperation = currentOperation->next;
        currentOperation->region = { minX, maxX, minY, maxY };

        for (; tempNextOp;)
        {
            s_gridOperationList* del = tempNextOp;
            tempNextOp = tempNextOp->next;
            delete del;
        }

        afterFillRayCheck(minX, minY, maxX, maxY);
    }
}

void Leveler::afterFillRayCheck(short minX, short minY, short maxX, short maxY)
{
    int extra = 3;
    minX -= extra; minY -= extra;
    maxX += extra; maxY += extra;

    for (s_objectDataGrid* objectData : levelObjects)
    {
        glm::i16vec2 pos = objectData->pos;
        bool cond0 = pos.x >= minX && pos.x <= maxX && pos.y >= minY && pos.y <= maxY;
        bool cond1 = pos.x > minX && pos.x < maxX && pos.y < minY && objectData->rotation == 0;
        bool cond2 = pos.x > minX && pos.x < maxX && pos.y > maxY && objectData->rotation == 2;
        bool cond4 = pos.y > minY && pos.y < maxY && pos.x > maxX && objectData->rotation == 1;
        bool cond3 = pos.y > minY && pos.y < maxY && pos.x < minX && objectData->rotation == 3;
        
        if (cond0 || cond1 || cond2 || cond3 || cond4)
        {
            s_gridItemData& item = ol->items[getLevelObjectIndex(objectData->objectType)];
            short scaleX = item.gridScale[objectData->rotation % 2];
            short scaleY = item.gridScale[(objectData->rotation % 2) ^ 1];

            switch (item.objectType & SINGLE_DRAWS)
            {
            case LASERER:
            {
                glm::vec2 startPoint;
                glm::vec2 endPoint;
                sendObjectRay(item, pos.x, pos.y, objectData->rotation, scaleX, scaleY, 0.5, 0.0, (levelObject)(WHITE_WALL | BLACK_WALL), startPoint, endPoint);

                s_laserUbo* laserUbo;
                vkMapMemory(thinDrawer->laserShader->logicalDevice, thinDrawer->laserShader->uniformBuffers[0]->memory,
                    objectData->extraOffsets[0] * sizeof(s_laserUbo), sizeof(s_laserUbo), 0, (void**)&laserUbo);
                laserUbo->points = glm::vec4(startPoint.x, startPoint.y, endPoint.x, endPoint.y);
                vkUnmapMemory(thinDrawer->laserShader->logicalDevice, thinDrawer->laserShader->uniformBuffers[0]->memory);
            }
                break;

            case FUNNEL:
            {
                glm::vec2 startPoint;
                glm::vec2 endPoint;
                sendObjectRay(item, pos.x, pos.y, objectData->rotation, scaleX, scaleY, 0.1, 0.0, (levelObject)(WHITE_WALL | BLACK_WALL | GLASS_WALL), startPoint, endPoint);

                s_funnelUbo* funnelUbo;
                vkMapMemory(thinDrawer->funnelShader->logicalDevice, thinDrawer->funnelShader->uniformBuffers[0]->memory,
                    objectData->extraOffsets[0] * sizeof(s_funnelUbo), sizeof(s_funnelUbo), 0, (void**)&funnelUbo);
                float diffBefore = glm::length(glm::vec2(funnelUbo->pointts.z, funnelUbo->pointts.w) - glm::vec2(funnelUbo->pointts.x, funnelUbo->pointts.y));
                float diffLen = glm::length(endPoint - startPoint);
                funnelUbo->pointts = glm::vec4(startPoint.x, startPoint.y, endPoint.x, endPoint.y);
                funnelUbo->data2.y = diffLen - 1.0; funnelUbo->data2.z = diffLen;
                if (diffBefore < diffLen && funnelUbo->data.w < 0.0)
                {
                    float data2WChange = game->gameTime - funnelUbo->data2.w;
                    funnelUbo->data2.x = diffBefore; funnelUbo->data2.w = game->gameTime;
                    funnelUbo->data.y -= data2WChange;
                }

                vkUnmapMemory(thinDrawer->funnelShader->logicalDevice, thinDrawer->funnelShader->uniformBuffers[0]->memory);
            }
                break;
            
            default:
                break;
            }
		}
    }
}

void Leveler::sideUpdate(short minX, short maxX, short minY, short maxY)
{
    const short leftX = std::max(minX - 1, -xSize_H);
    const short rightX = std::min(maxX + 1, xSize_H - 1);
    const short botY = std::max(minY - 1, -ySize_H);
    const short topY = std::min(maxY + 1, ySize_H - 1);
    
    int regions[4][4] =
    {
        { leftX, rightX, botY, botY + 1 },
        { rightX - 1, rightX, botY, topY },
        { leftX, rightX, topY - 1, topY },
        { leftX, leftX + 1, botY, topY }
    };

    const glm::i8vec2 gridAdders[8] =
    {
        { +0, -1 }, { +1, +0 }, { +0, +1 }, { -1, +0 },
        { -1, -1 }, { +1, -1 }, { +1, +1 }, { -1, +1 },
    };

    for (int y = botY; y <= topY; y++)
    {
        for (int x = leftX; x <= rightX; x++)
        {
            const s_levelGridData& squareData = getSquare(x, y);
            if ((squareData.objectType & (WHITE_WALL | BLACK_WALL)) == 0)
            {
                continue;
            }

            int sideData = SIDE_ENABLE;
            for (int j = 0; j < 8; j++)
            {
                int xPos = x + gridAdders[j].x;
                int yPos = y + gridAdders[j].y;
                if (xPos < -xSize_H || xPos > xSize_H - 1 || yPos < -ySize_H || yPos > ySize_H - 1)
                {
                    sideData |= SIDE_ENABLE | (DOWN_LINE << j);
				    continue;
                }
                if (!(getSquare(xPos, yPos).objectType & (WHITE_WALL | BLACK_WALL)))
                {
                    sideData |= SIDE_ENABLE | (DOWN_LINE << j);
				}
			}
            
            if (sideData == 0)
            {
                sideData = SIDE_ENABLE;
            }

            if (squareData.objectType & WHITE_WALL)
            {
                s_wallUbo_w* wData;
                vkMapMemory(thinDrawer->logicalDevice, thinDrawer->whiteWallShader->uniformBuffers[0]->memory,
                    squareData.data.offset, sizeof(s_wallUbo_w), 0, (void**)&wData);
                wData->sideData.x = sideData;
                vkUnmapMemory(thinDrawer->logicalDevice, thinDrawer->whiteWallShader->uniformBuffers[0]->memory);
            }
            else if (squareData.objectType & BLACK_WALL)
            {
                s_wallUbo_b* wData;
                vkMapMemory(thinDrawer->logicalDevice, thinDrawer->blackWallShader->uniformBuffers[0]->memory,
                    squareData.data.offset, sizeof(s_wallUbo_w), 0, (void**)&wData);
                wData->sideData.x = sideData;
                vkUnmapMemory(thinDrawer->logicalDevice, thinDrawer->blackWallShader->uniformBuffers[0]->memory);
            }
        }
    }
}

void Leveler::addGridSquare(short x, short y, levelObject objectType)
{
    switch (objectType & GRID_DRAWS)
    {
    case WHITE_WALL:
        addGridSquare_W(x, y);
		break;
    case BLACK_WALL:
		addGridSquare_B(x, y);
		break;
    case GLASS_WALL:
        addGridSquare_G(x, y);
		break;
    case FENCE_WALL:
        addGridSquare_F(x, y);
		break;
    default:
        break;
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
    wallUbo.sideData.x = SIDE_DISABLE;

    s_gridPositionShader pos = thinDrawer->whiteWallShader->getGridSquare(wallUbo.posSize.x, wallUbo.posSize.y);
    s_shaderGridData* gridData = &thinDrawer->whiteWallShader->renderGridUp[1][pos.y][pos.x];
    gridData->numElements++;

    int offset = gridData->offsetEnd * sizeof(s_wallUbo_w);
    gridData->offsetEnd++;

    void* pData;
    vkMapMemory(thinDrawer->logicalDevice, thinDrawer->whiteWallShader->uniformBuffers[0]->memory,
        offset, sizeof(s_wallUbo_w), 0, (void**)&pData);
    memcpy(pData, &wallUbo, sizeof(s_wallUbo_w));
    vkUnmapMemory(thinDrawer->logicalDevice, thinDrawer->whiteWallShader->uniformBuffers[0]->memory);

    const s_levelGridData& currentGridData = getSquare(x, y);

    lastOffsetPos_W[pos.y][pos.x].push_back({x, y});
    s_gridData uData; uData.offset = offset;
    setSquare(x, y, { (levelObject)(WHITE_WALL | currentGridData.objectType), uData });
}

void Leveler::addGridSquare_B(short x, short y)
{
    s_gridPositionShader pos = thinDrawer->blackWallShader->getGridSquare(x + 0.5f, y + 0.5f);
    s_shaderGridData* gridData = &thinDrawer->blackWallShader->renderGridUp[1][pos.y][pos.x];
    gridData->numElements++;

    float rndVal = ((double)rand() / RAND_MAX + 2.0) / 3.0;
    s_wallUbo_b* wallUbo;
    int offset = gridData->offsetEnd * sizeof(s_wallUbo_b);
    gridData->offsetEnd++;
    vkMapMemory(thinDrawer->logicalDevice, thinDrawer->blackWallShader->uniformBuffers[0]->memory,
        offset, sizeof(s_wallUbo_b), 0, (void**)&wallUbo);
    wallUbo->posSize = glm::vec4(x + 0.5f, y + 0.5f, 1.0f, 1.0f);
    wallUbo->colour = glm::vec4(0.15, 0.15, 0.15, 1.0) * rndVal;
    wallUbo->data = glm::vec4(rand() > RAND_MAX / 2, 0.06, 1.0, 0.0);
    vkUnmapMemory(thinDrawer->logicalDevice, thinDrawer->blackWallShader->uniformBuffers[0]->memory);

    const s_levelGridData& currentGridData = getSquare(x, y);

    lastOffsetPos_B[pos.y][pos.x].push_back({ x, y });
    s_gridData uData; uData.offset = offset;
    setSquare(x, y, { (levelObject)(BLACK_WALL | currentGridData.objectType), uData });
}

void Leveler::addGridSquare_G(short x, short y)
{
    s_gridPositionShader pos = thinDrawer->diagonalLiner->getGridSquare(x + 0.5f, y + 0.5f);
    s_shaderGridData* gridData = &thinDrawer->diagonalLiner->renderGridUp[1][pos.y][pos.x];
    gridData->numElements++;

    s_wallUbo_l* wallUbo;
    int offset = gridData->offsetEnd * sizeof(s_wallUbo_l);
    gridData->offsetEnd++;
    vkMapMemory(thinDrawer->logicalDevice, thinDrawer->diagonalLiner->uniformBuffers[0]->memory,
        offset, sizeof(s_wallUbo_l), 0, (void**)&wallUbo);
    wallUbo->posSize = glm::vec4(x + 0.5f, y + 0.5f, 1.0f, 1.0f);
    wallUbo->colour = glm::vec4(0.0, 0.0, 0.0, 1.0);
    wallUbo->bgColour = glm::vec4(0.02, 0.02, 0.02, 0.7);
    wallUbo->data = glm::vec4(2.0, 0.2, 1.0, 1.0);
    vkUnmapMemory(thinDrawer->logicalDevice, thinDrawer->diagonalLiner->uniformBuffers[0]->memory);

    const s_levelGridData& currentGridData = getSquare(x, y);

    lastOffsetPos_G[pos.y][pos.x].push_back({ x, y });
    s_gridData uData; uData.offset = offset;
    setSquare(x, y, { (levelObject)(GLASS_WALL | currentGridData.objectType), uData });
}

void Leveler::addGridSquare_F(short x, short y)
{
    s_gridPositionShader pos = thinDrawer->diagonalLiner->getGridSquare(x + 0.5f, y + 0.5f);
    s_shaderGridData* gridData = &thinDrawer->diagonalLiner->renderGridUp[1][pos.y][pos.x];
    gridData->numElements++;

    s_wallUbo_l* wallUbo;
    int offset = gridData->offsetEnd * sizeof(s_wallUbo_l);
    gridData->offsetEnd++;
    vkMapMemory(thinDrawer->logicalDevice, thinDrawer->diagonalLiner->uniformBuffers[0]->memory,
        offset, sizeof(s_wallUbo_l), 0, (void**)&wallUbo);
    wallUbo->posSize = glm::vec4(x + 0.5f, y + 0.5f, 1.0f, 1.0f);
    wallUbo->colour = glm::vec4(0.1, 0.1, 0.1, 1.0);
    wallUbo->bgColour = glm::vec4(0.0, 0.0, 0.0, 0.0);
    wallUbo->data = glm::vec4(2.0, 0.4, 1.0, 0.5);
    vkUnmapMemory(thinDrawer->logicalDevice, thinDrawer->diagonalLiner->uniformBuffers[0]->memory);

    const s_levelGridData& currentGridData = getSquare(x, y);

    lastOffsetPos_G[pos.y][pos.x].push_back({ x, y });
    s_gridData uData; uData.offset = offset;
    setSquare(x, y, { (levelObject)(FENCE_WALL | currentGridData.objectType), uData });
}

void Leveler::addGridSquare_E(short x, short y)
{

}

void Leveler::removeGridSquare(short x, short y)
{
    s_levelGridData gData = getSquare(x, y);
    if (gData.objectType == NONE_LO) return;

    setSquare(x, y, { (levelObject)(gData.objectType & (GRID_DRAWS ^ -1ll)), 0 });

    s_gridPositionShader pos;
    std::vector<s_gridSquare>* lastOffsetPosVec = nullptr;
    WallBase* shader = nullptr;
    int dataSize;

    switch (gData.objectType & GRID_DRAWS)
    {
    case WHITE_WALL:
    {
        shader = thinDrawer->whiteWallShader;
        pos = shader->getGridSquare((float)x, (float)y);
        lastOffsetPosVec = &lastOffsetPos_W[pos.y][pos.x];
        dataSize = sizeof(s_wallUbo_w);
    }
    break;

    case BLACK_WALL:
    {
        shader = thinDrawer->blackWallShader;
        pos = shader->getGridSquare((float)x, (float)y);
        lastOffsetPosVec = &lastOffsetPos_B[pos.y][pos.x];
        dataSize = sizeof(s_wallUbo_b);
    }
    break;

    case GLASS_WALL:
    case FENCE_WALL:
    {
        shader = thinDrawer->diagonalLiner;
        pos = shader->getGridSquare((float)x, (float)y);
        lastOffsetPosVec = &lastOffsetPos_G[pos.y][pos.x];
        dataSize = sizeof(s_wallUbo_l);
    }
    break;

    default:
        break;
    }

    if (!shader) return;

    s_gridSquare lastOffsetPos = lastOffsetPosVec->back();
    s_shaderGridData* gridData = &shader->renderGridUp[1][pos.y][pos.x];

    gridData->numElements--;
    gridData->offsetEnd--;

    if (lastOffsetPos == s_gridSquare{ x, y })
    {
        lastOffsetPosVec->pop_back();
        return;
    }

    int offsetPosIndex = gData.data.offset / dataSize - gridData->offsetStart;

    (*lastOffsetPosVec)[offsetPosIndex] = lastOffsetPos;
    lastOffsetPosVec->pop_back();

    s_levelGridData gridWithLastOffset = getSquare(lastOffsetPos.x, lastOffsetPos.y);

    moveVkMem(shader, gridWithLastOffset.data.offset, gData.data.offset, dataSize);

    s_gridData uData; uData.offset = gData.data.offset;
    setSquare(lastOffsetPos.x, lastOffsetPos.y, { gridWithLastOffset.objectType, uData });
}

void Leveler::moveVkMem(ShaderBase* shader, int srcOffset, int destOffset, int size)
{
    static unsigned char wUboStager[512];
    void* wUbo;

    vkMapMemory(thinDrawer->logicalDevice, shader->uniformBuffers[0]->memory,
        srcOffset, size, 0, (void**)&wUbo);
    memcpy((void*)&wUboStager, wUbo, size);
    vkUnmapMemory(thinDrawer->logicalDevice, shader->uniformBuffers[0]->memory);

    vkMapMemory(thinDrawer->logicalDevice, shader->uniformBuffers[0]->memory,
        destOffset, size, 0, (void**)&wUbo);
    memcpy(wUbo, (void*)&wUboStager, size);
    vkUnmapMemory(thinDrawer->logicalDevice, shader->uniformBuffers[0]->memory);
}