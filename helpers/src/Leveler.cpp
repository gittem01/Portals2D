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

    operationList = new s_gridOperationList();
    currentOperation = operationList;

    thinDrawer->whiteWallShader->recreateUniforms(numWhiteWall * 2);
    thinDrawer->blackWallShader->recreateUniforms(numBlackWall * 2);
    thinDrawer->wallLight->recreateUniforms(numWhiteWall * 2);
    thinDrawer->diagonalLiner->recreateUniforms(numWhiteWall * 2);

    //thinDrawer->earlyDrawFuncs.push_back({ renderParticles, (void*)this });
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

    const float sz = 2.5f;
    for (int i = -200; i < 200; i++)
    {
        // break for now
        //break;
        for (int j = -200; j < 200; j++)
        {
            float rndVal = ((double)rand() / RAND_MAX + 2.0) / 3.0;
            if (rndVal > 0.9f)
            {
                float randVal11 = (((float)rand() / RAND_MAX) - 0.5f) * 0.01f;
                float randVal22 = (((float)rand() / RAND_MAX)) * 0.0f;
                s_wallUbo_w wallUbo;
                wallUbo.posSize = glm::vec4(i * sz, j * sz, sz, sz);
                wallUbo.bgColour = glm::vec4(0.05f + randVal11, 0.05f + randVal11, 0.05f + randVal11, 1.0f);
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
            wallUbo->colour = glm::vec4(0.01, 0.01, 0.01, 1.0) * rndVal;
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

void Leveler::renderGrid(VkCommandBuffer cmdBuff, void* ptr)
{
    Leveler* leveler = (Leveler*)ptr;
    
    if (leveler->ol->selectedItemIndex == -1) return;

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

            printf("Fill Time: %f\n", glfwGetTime() - st);
        }

        clickStart[0] = -100000;
    }
    else if (wh->mouseData[2] == 2 && !ol->itemDir && ol->selectedItemIndex >= 0)
    {
        glm::ivec2 mousePos = getMouseGrid();
        clickStart[0] = mousePos.x;
        clickStart[1] = mousePos.y;
    }

    if (wh->keyData[wh->pIndLeftCommand] && wh->keyData[GLFW_KEY_Z] == 2)
    {
        double st = glfwGetTime();

        takeBack(wh->keyData[GLFW_KEY_LEFT_SHIFT] != 0);

        printf("Rev Time: %f\n", glfwGetTime() - st);
    }
}

void Leveler::takeBack(bool shiftPressed)
{
    if (shiftPressed)
    {
        if (currentOperation->next)
        {
            currentOperation = currentOperation->next;

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

            sideUpdate(currentOperation->region.x, currentOperation->region.y, currentOperation->region.z, currentOperation->region.w);
        }
    }
    else if (currentOperation->prev)
    {
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

        sideUpdate(currentOperation->region.x, currentOperation->region.y, currentOperation->region.z, currentOperation->region.w);

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
        gridSquareAdder = &Leveler::addGridSquare_W;
        break;
    }

    //vkDeviceWaitIdle(thinDrawer->logicalDevice);

    for (short i = minY; i <= maxY; i++)
    {
        for (short j = minX; j <= maxX; j++)
        {
            s_levelGridData lgData = getSquare(j, i);

            if (lgData.objectType == fillType)
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

            if (squareData.objectType == WHITE_WALL)
            {
                s_wallUbo_w* wData;
                vkMapMemory(thinDrawer->logicalDevice, thinDrawer->whiteWallShader->uniformBuffers[0]->memory,
                    squareData.data.offset, sizeof(s_wallUbo_w), 0, (void**)&wData);
                wData->sideData.x = sideData;
                vkUnmapMemory(thinDrawer->logicalDevice, thinDrawer->whiteWallShader->uniformBuffers[0]->memory);
            }
            else if (squareData.objectType == BLACK_WALL)
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
    switch (objectType)
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

    lastOffsetPos_W[pos.y][pos.x].push_back({x, y});
    setSquare(x, y, { WHITE_WALL, offset });
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
    wallUbo->colour = glm::vec4(0.07, 0.07, 0.07, 1.0) * rndVal;
    wallUbo->data = glm::vec4(rand() > RAND_MAX / 2, 0.06, 1.0, 0.0);
    vkUnmapMemory(thinDrawer->logicalDevice, thinDrawer->blackWallShader->uniformBuffers[0]->memory);

    lastOffsetPos_B[pos.y][pos.x].push_back({ x, y });
    setSquare(x, y, { BLACK_WALL, offset });
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
    wallUbo->bgColour = glm::vec4(0.02, 0.02, 0.02, 0.5);
    wallUbo->data = glm::vec4(2.0, 0.2, 1.0, 1.0);
    vkUnmapMemory(thinDrawer->logicalDevice, thinDrawer->diagonalLiner->uniformBuffers[0]->memory);

    lastOffsetPos_G[pos.y][pos.x].push_back({ x, y });
    setSquare(x, y, { GLASS_WALL, offset });
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
    wallUbo->colour = glm::vec4(0.05, 0.03, 0.0, 1.0);
    wallUbo->bgColour = glm::vec4(0.0, 0.0, 0.0, 0.0);
    wallUbo->data = glm::vec4(2.0, 0.4, 1.0, 0.5);
    vkUnmapMemory(thinDrawer->logicalDevice, thinDrawer->diagonalLiner->uniformBuffers[0]->memory);

    lastOffsetPos_G[pos.y][pos.x].push_back({ x, y });
    setSquare(x, y, { FENCE_WALL, offset });
}

void Leveler::addGridSquare_E(short x, short y)
{

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
        s_gridPositionShader pos = thinDrawer->whiteWallShader->getGridSquare((float)x, (float)y);
        std::vector<s_gridSquare>& lastOffsetPosVec = lastOffsetPos_W[pos.y][pos.x];
        s_gridSquare lastOffsetPos = lastOffsetPosVec.back();

        s_shaderGridData* gridData = &thinDrawer->whiteWallShader->renderGridUp[1][pos.y][pos.x];

        if (lastOffsetPos == s_gridSquare{ x, y })
        {
            lastOffsetPosVec.pop_back();
            gridData->numElements--;
            gridData->offsetEnd--;
            break;
		}

        int offsetPosIndex = gData.data.offset / sizeof(s_wallUbo_w) - gridData->offsetStart;

        lastOffsetPosVec[offsetPosIndex] = lastOffsetPos;
        lastOffsetPosVec.pop_back();

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

        gridData->numElements--;
        gridData->offsetEnd--;

        setSquare(lastOffsetPos.x, lastOffsetPos.y, { WHITE_WALL, gData.data.offset });
    }
    break;

    case BLACK_WALL:
    {
        s_gridPositionShader pos = thinDrawer->blackWallShader->getGridSquare(x, y);
        std::vector<s_gridSquare>& lastOffsetPosVec = lastOffsetPos_B[pos.y][pos.x];
        
        s_gridSquare lastOffsetPos = lastOffsetPosVec.back();
        s_shaderGridData* gridData = &thinDrawer->blackWallShader->renderGridUp[1][pos.y][pos.x];
        if (lastOffsetPos == s_gridSquare{ x, y })
        {
            lastOffsetPosVec.pop_back();
            gridData->numElements--;
            gridData->offsetEnd--;
            break;
        }

        int offsetPosIndex = gData.data.offset / sizeof(s_wallUbo_b) - gridData->offsetStart;

        lastOffsetPosVec[offsetPosIndex] = lastOffsetPos;
        lastOffsetPosVec.pop_back();

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

        gridData->numElements--;
        gridData->offsetEnd--;

        setSquare(lastOffsetPos.x, lastOffsetPos.y, { BLACK_WALL, gData.data.offset });
    }
    break;

    case GLASS_WALL:
    case FENCE_WALL:
    {
        s_gridPositionShader pos = thinDrawer->diagonalLiner->getGridSquare(x, y);
        std::vector<s_gridSquare>& lastOffsetPosVec = lastOffsetPos_G[pos.y][pos.x];

        s_gridSquare lastOffsetPos = lastOffsetPosVec.back();
        s_shaderGridData* gridData = &thinDrawer->diagonalLiner->renderGridUp[1][pos.y][pos.x];
        if (lastOffsetPos == s_gridSquare{ x, y })
        {
            lastOffsetPosVec.pop_back();
            gridData->numElements--;
            gridData->offsetEnd--;
            break;
        }

        int offsetPosIndex = gData.data.offset / sizeof(s_wallUbo_l) - gridData->offsetStart;

        lastOffsetPosVec[offsetPosIndex] = lastOffsetPos;
        lastOffsetPosVec.pop_back();

        s_levelGridData gridWithLastOffset = getSquare(lastOffsetPos.x, lastOffsetPos.y);

        s_wallUbo_l* wUbo, wUboStager;
        vkMapMemory(thinDrawer->logicalDevice, thinDrawer->diagonalLiner->uniformBuffers[0]->memory,
            gridWithLastOffset.data.offset, sizeof(s_wallUbo_l), 0, (void**)&wUbo);
        memcpy((void*)&wUboStager, (void*)wUbo, sizeof(s_wallUbo_l));
        vkUnmapMemory(thinDrawer->logicalDevice, thinDrawer->diagonalLiner->uniformBuffers[0]->memory);

        vkMapMemory(thinDrawer->logicalDevice, thinDrawer->diagonalLiner->uniformBuffers[0]->memory,
            gData.data.offset, sizeof(s_wallUbo_l), 0, (void**)&wUbo);
        memcpy((void*)wUbo, (void*)&wUboStager, sizeof(s_wallUbo_l));
        vkUnmapMemory(thinDrawer->logicalDevice, thinDrawer->diagonalLiner->uniformBuffers[0]->memory);

        gridData->numElements--;
        gridData->offsetEnd--;

        setSquare(lastOffsetPos.x, lastOffsetPos.y, { gData.objectType, gData.data.offset });
    }
    break;

    default:
        break;
    }
}