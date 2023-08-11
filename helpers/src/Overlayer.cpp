#include <Overlayer.h>
#include <Game.h>
#include <ThinDrawer.h>
#include <Sounder.h>

Overlayer::Overlayer(Game* game)
{
	this->game = game;
	this->thinDrawer = game->thinDrawer;
	this->wh = thinDrawer->wh;
    this->sounder = Sounder::object;
	this->as = thinDrawer->arrowShader;
    this->il = thinDrawer->itemList;
    this->mt = thinDrawer->manyTexture;

    cam.zoom = 1.0;
    cam.baseX = 160.0;
    cam.defaultXSides = glm::vec2(-80.0, +80.0);
    cam.defaultYSides = glm::vec2(-45.0, +45.0);
    cam.init(glm::vec2(0.0, 0.0), wh);

    switchSound = sounder->loadSound("switch.wav");
    clickSound = sounder->loadSound("click.wav");

    thinDrawer->memMapFuncs.push_back({ maper, (void*)this });
	thinDrawer->lateDrawFuncs.push_back({ draw, (void*)this });

    startAlloc = mt->lastAvailablePos;
    mt->lastAvailablePos += numX * numY * 4;

    addItem("PortallableWall", "ww.png", WHITE_WALL);
    addItem("N_PotallableWall", "bb.png", BLACK_WALL);
    addItem("GlassWall", "glassWall.png", GLASS_WALL);
    addItem("FenceWall", "fenceWall.png", FENCE_WALL);
    addItem("DefaultCube", "cube.png", DEFAULT_CUBE);
    addItem("Remove", "x.png", ERASER);
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

    if (ol->itemRatio > 0.0)
    {
        int i = 0;
        for (s_gridItemData& item : ol->items)
        {
            char x = i % ol->numX;
            char y = i / ol->numX;

            s_manyTextureData* mtd;
            vkMapMemory(ol->mt->logicalDevice, ol->mt->uniformBuffers[0]->memory, (i + td->lastSwapChainImageIndex * ol->totalNum) * sizeof(s_manyTextureData),
                sizeof(s_manyTextureData), 0, (void**)&mtd);
            mtd->posSize = glm::vec4(-94.5 + ol->itemRatio * 20.0 + x * 9.0, 31.5 - y * 9.0, 6.0 + item.extraScale, 6.0 + item.extraScale);
            vkUnmapMemory(ol->mt->logicalDevice, ol->mt->uniformBuffers[0]->memory);

            i++;
        }
    }
}

void Overlayer::draw(VkCommandBuffer cmdBuff, void* ptr)
{
    Overlayer* ol = (Overlayer*)ptr;
    ThinDrawer* td = ol->thinDrawer;

    const VkDeviceSize offsets = 0;
    const uint32_t camOffset = ShaderBase::alignedSharedSize * (td->lastSwapChainImageIndex + 8);

    if (ol->itemRatio > 0.0)
    {
        char hoverGridX = ol->selectedItemIndex % ol->numX;
        char hoverGridY = ol->selectedItemIndex / ol->numX;

        s_itemData iData;
        iData.data[0] = hoverGridX;
        iData.data[1] = hoverGridY;
        iData.posSize = glm::vec4(-90.0 + ol->itemRatio * 20.0, 0.0, 19.0, 73.0);

        vkCmdBindPipeline(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS, ol->il->pipeline);

        vkCmdBindVertexBuffers(cmdBuff, 0, 1, &td->wallLight->buffers[0]->buffer, &offsets);

        vkCmdBindDescriptorSets(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS,
            ol->il->pipelineLayout, 0, 1, &ShaderBase::sharedSet, 1, &camOffset);

        vkCmdPushConstants(cmdBuff, ol->il->pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(s_itemData), (void*)&iData);

        vkCmdDraw(cmdBuff, td->wallLight->buffers[0]->count, 1, 0, 0);

        // items tex
        vkCmdBindPipeline(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS, ol->mt->pipeline);

        vkCmdBindVertexBuffers(cmdBuff, 0, 1, &td->wallLight->buffers[0]->buffer, &offsets);

        vkCmdBindDescriptorSets(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS,
            ol->mt->pipelineLayout, 0, 1, &ShaderBase::sharedSet, 1, &camOffset);

        vkCmdBindDescriptorSets(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS,
            ol->mt->pipelineLayout, 1, 1, &ol->mt->descriptorSets[1], 0, VK_NULL_HANDLE);

        int i = 0;
        for (s_gridItemData& item : ol->items)
        {
            vkCmdBindDescriptorSets(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS,
                ol->mt->pipelineLayout, 2, 1, &item.texSet, 0, VK_NULL_HANDLE);

            vkCmdDraw(cmdBuff, td->wallLight->buffers[0]->count, 1, 0, (i + td->lastSwapChainImageIndex * ol->totalNum));

            i++;
        }
    }

    // arrow
    s_arrowData aData;
    aData.posSize = glm::vec4(-78.5 + ol->itemRatio * 20.0, 0.0, 3.0, 3.0);
    aData.color = glm::vec4(0.5f, 1.0f, 1.0f, 1.0f - ol->itemRatio * 0.75);
    aData.data = glm::vec4(-ol->itemRatio * 0.3, 0.5 - ol->itemRatio * 0.2, -0.5 + ol->itemRatio * 0.5, 0.5);
    aData.data2 = glm::vec4(0.0, 0.3 - ol->itemRatio * 0.2, 0.5 - ol->itemRatio * 0.3, 0.1);

    vkCmdBindPipeline(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS, ol->as->pipeline);

    vkCmdBindVertexBuffers(cmdBuff, 0, 1, &td->wallLight->buffers[0]->buffer, &offsets);

    vkCmdBindDescriptorSets(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS,
        ol->as->pipelineLayout, 0, 1, &ShaderBase::sharedSet, 1, &camOffset);

    vkCmdPushConstants(cmdBuff, ol->as->pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(s_arrowData), (void*)&aData);

    vkCmdDraw(cmdBuff, td->wallLight->buffers[0]->count, 1, 0, 0);
}

void Overlayer::addItem(char* name, char* fileName, levelObject objectType)
{
    s_gridItemData itemData;
    itemData.objectType = objectType;
    itemData.name = name;
    
    s_texture* tex = IOHelper::loadTexture(fileName, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);
    itemData.texSet = tex->set;
    itemData.extraScale = 0.0;

    items.push_back(itemData);
}

void Overlayer::update()
{
    glm::vec2 p = cam.getMouseCoords();

    const float mult = 6.0;

    if (-78.0 + itemRatio * 17.0 < p.x || game->leveler->clickStart[0] != -100000)
    {
        if (itemDir == 1)
        {
            ALuint source = sounder->playSound(switchSound, 0.01);
            alSourcei(source, AL_SOURCE_RELATIVE, AL_TRUE);
        }

        itemDir = 0;

        itemRatio -= game->deltaTime * mult;
    }
    else
    {
        if (itemDir == 0)
        {
            ALuint source = sounder->playSound(switchSound, 0.01);
            alSourcei(source, AL_SOURCE_RELATIVE, AL_TRUE);
        }

        itemDir = 1;

        itemRatio += game->deltaTime * mult;
    }

    itemRatio = fmax(fmin(itemRatio, 1.0), 0.0);

    if (itemRatio > 0.0)
    {
        char leftX = -97.0 + itemRatio * 18.0;
        char upY = 36.0;
        
        char hoverGridX = std::floor((p.x - leftX) / 9.0f);
        char hoverGridY = std::floor((upY - p.y) / 9.0f);

        char hoverIndex = -1;
        if (hoverGridX >= 0 && hoverGridX < numX && hoverGridY >= 0 && hoverGridY < numY)
        {
			hoverIndex = hoverGridX + hoverGridY * numX;

            if (hoverIndex != lastHoverIndex && hoverIndex < items.size())
            {
                ALuint source = sounder->playSound(switchSound, 0.001);
                alSourcei(source, AL_SOURCE_RELATIVE, AL_TRUE);
            }

            lastHoverIndex = hoverIndex;
		}

        int i = 0;
        for (s_gridItemData& item : items)
        {
            if (i == hoverIndex)
            {
                if (wh->mouseData[2])
                {
                    item.extraScale = fmax(item.extraScale - game->deltaTime * 15.0 * item.extraScale, 0.0);
                    if (wh->mouseData[2] == 2)
                    {
                        selectedItemIndex = hoverIndex;

                        ALuint source = sounder->playSound(clickSound, 0.3);
                        alSourcei(source, AL_SOURCE_RELATIVE, AL_TRUE);
                    }
                }
                else
                {
                    item.extraScale = fmin(item.extraScale + game->deltaTime * 15.0, 1.0);
                }
            }
            else
            {
                item.extraScale = fmax(item.extraScale - game->deltaTime * 15.0, 0.0);
            }

            i++;
        }
    }
}