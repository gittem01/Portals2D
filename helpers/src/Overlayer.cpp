#include <Overlayer.h>
#include <Game.h>
#include <ThinDrawer.h>
#include <Sounder.h>
#include <OptionShader.h>

// funnel thingy
//if (wh->keyData[GLFW_KEY_SPACE] == 2)
//{
//    s_funnelUbo* funnelUbo;
//    vkMapMemory(thinDrawer->logicalDevice, thinDrawer->funnelShader->uniformBuffers[0]->memory, 0, sizeof(s_funnelUbo), 0, (void**)&funnelUbo);
//    float colorSplitPos = std::max(std::min(funnelUbo->data2.x + funnelUbo->data.w * ((float)game->gameTime - funnelUbo->data2.w) * -200.0f, 70.0f), -10.0f);
//    funnelUbo->data.y += (game->gameTime - funnelUbo->data2.w) * funnelUbo->data.w;
//    funnelUbo->data.w *= -1.0f;
//    funnelUbo->data2.x = colorSplitPos;
//    funnelUbo->data2.w = game->gameTime;
//    vkUnmapMemory(thinDrawer->logicalDevice, thinDrawer->funnelShader->uniformBuffers[0]->memory);
//
//    vkMapMemory(thinDrawer->logicalDevice, thinDrawer->funnelShader->uniformBuffers[0]->memory, sizeof(s_funnelUbo), sizeof(s_funnelUbo), 0, (void**)&funnelUbo);
//    funnelUbo->data.y += (game->gameTime - funnelUbo->data2.w) * funnelUbo->data.w;
//    funnelUbo->data.w *= -1.0f;
//    funnelUbo->data2.x = colorSplitPos;
//    funnelUbo->data2.w = game->gameTime;
//    vkUnmapMemory(thinDrawer->logicalDevice, thinDrawer->funnelShader->uniformBuffers[0]->memory);
//}

// hazard thingy -same for blue thingy-
//if (wh->keyData[GLFW_KEY_SPACE] == 2)
//{
//    s_hazardUbo* hazardUbo;
//    vkMapMemory(thinDrawer->logicalDevice, thinDrawer->hazardShader->uniformBuffers[0]->memory, 0, sizeof(s_funnelUbo), 0, (void**)&hazardUbo);
//
//    float dist = glm::length(glm::vec2(hazardUbo->pointts.z, hazardUbo->pointts.w) - glm::vec2(hazardUbo->pointts.x, hazardUbo->pointts.y));
//    const float speed = std::min(200.0f, dist * 5.0f);
//    const float smoothSize = std::min(dist * 0.1, 5.0);
//    float timeDiff = (game->gameTime - hazardUbo->data.w) * speed;
//    float mm0 = std::min(-smoothSize + timeDiff, dist * 0.5f + smoothSize);
//    float mm1 = dist * 0.5 + smoothSize;
//    float d = mm1 - mm0;
//
//    hazardUbo->data.z = hazardUbo->data.z < 0.5 ? 1.0 : 0.0;
//    hazardUbo->data.w = game->gameTime - d / speed;
//    vkUnmapMemory(thinDrawer->logicalDevice, thinDrawer->hazardShader->uniformBuffers[0]->memory);
//}

// blue shoot
//if (wh->keyData[GLFW_KEY_SPACE] == 2)
//{
//    s_blueFieldUbo* blueUbo;
//    vkMapMemory(thinDrawer->logicalDevice, thinDrawer->blueField->uniformBuffers[0]->memory, 0, sizeof(s_blueFieldUbo), 0, (void**)&blueUbo);
//    blueUbo->shootData = glm::vec4(0.0, 2.9, game->gameTime, 0.0);
//    vkUnmapMemory(thinDrawer->logicalDevice, thinDrawer->blueField->uniformBuffers[0]->memory);
//}

Overlayer::Overlayer(Game* game)
{
	this->game = game;
	this->thinDrawer = game->thinDrawer;
	this->wh = thinDrawer->wh;
    this->sounder = Sounder::object;
	this->as = thinDrawer->arrowShader;
    this->il = thinDrawer->itemList;
    this->mt = thinDrawer->manyTexture;

    this->os = new OptionShader(thinDrawer);
    thinDrawer->shaders.push_back(os);
    os->init();

    cam.zoom = 1.0;
    cam.baseX = abs(leftX) * 2.0;
    cam.defaultXSides = glm::vec2(leftX, -leftX);
    cam.defaultYSides = cam.defaultXSides * (9.0f / 16.0f);
    cam.init(glm::vec2(0.0, 0.0), wh);

    switchSound = sounder->loadSound("switch.wav");
    clickSound = sounder->loadSound("click.wav");
    bopSound = sounder->loadSound("boc.wav");
    removeSound = sounder->loadSound("remove.wav");

    thinDrawer->memMapFuncs.push_back({ maper, (void*)this });
	thinDrawer->lateDrawFuncs.push_back({ draw, (void*)this });

    this->cd = new CharDrawer(thinDrawer, "NotoSans-Medium.ttf");

    startAlloc = mt->lastAvailablePos;
    mt->lastAvailablePos += numX * numY * 4;

    addItem("PortallableWall", "ww.png", WHITE_WALL);
    addItem("N_PotallableWall", "bb.png", BLACK_WALL);
    addItem("GlassWall", "glassWall.png", GLASS_WALL);
    addItem("FenceWall", "fenceWall.png", FENCE_WALL);
    addItem("DefaultCube", "cube.png", DEFAULT_CUBE, { 0.5, 0.1 }, 1, { 1.0, 1.0 }, 0.8, false, 0);
    addItem("DownButton", "downButton.png", DOWN_BUTTON, { 1.0, +0.415 }, 1, { 1, 1 }, 1.0, true, 0);
    addItem("Laserer", "laserer.png", LASERER, { 0.5, +0.28 }, 1, { 1.0, 1.0 }, 0.5, true, 0);
    addItem("Funnel", "funnel.png", FUNNEL, { 0.5, +0.39 }, 1, { 2, 1 }, 2.0, true, 0);
    addItem("HazardLaser", "hazard.png", HAZARDER, { 0.5, +0.475 }, 2, { 1, 1 }, 0.8, true, 0);
    addItem("BlueField", "blueFielder.png", FORCE_FIELD, { 0.5, +0.475 }, 2, { 1, 1 }, 0.8, true, 0);
    addItem("Catapult", "plate.png", CATAPULT, { 0.5, +0.475 }, 1, { 1, 1 }, 0.8, true, 0);
    addItem("LightBridge", "lightBridge.png", LIGHT_BRIDGE, { 0.5, +0.402 }, 1, { 1, 1 }, 0.8, true, 0);
    addItem("Platform", "platform.png", PLATFORM, { 0.5, +0.0 }, 1, { 1, 1 }, 1.0, true, 0);
    addItem("Remove", "x.png", ERASER);

    addPop("button", { "Delete Item", "Connect to...", "Item Type", "Remove Connections" }, { "", "", "cube_select", "connection_select"}, 0x100c0000);
    addPop("connection_select", {"Default Cube #1", "Blue Gel #1"}, {"", ""}, 0x00000000);
    addPop("cube_select", {"Default Cube", "Companion Cube", "Laser Cube", "Sphere"}, {"", "", "", ""}, 0x000000ff);
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
            vkMapMemory(ol->mt->logicalDevice, ol->mt->uniformBuffers[0]->memory, (i + td->lastSwapChainImageIndex * ol->totalNum + ol->startAlloc) * sizeof(s_manyTextureData),
                sizeof(s_manyTextureData), 0, (void**)&mtd);
            mtd->posSize = glm::vec4(ol->leftX - ol->xSize + ol->itemRatio * ol->xSize + x * 9.0 + 5.0f, 31.5 - y * 9.0,
                6.0 + item.extraScale, 6.0 + item.extraScale);

            mtd->data.x = 0.0;
            vkUnmapMemory(ol->mt->logicalDevice, ol->mt->uniformBuffers[0]->memory);

            i++;
        }
    }
}

void Overlayer::draw(VkCommandBuffer cmdBuff, void* ptr)
{
    Overlayer* ol = (Overlayer*)ptr;
    ThinDrawer* td = ol->thinDrawer;
    OptionShader* os = ol->os;

    const VkDeviceSize offsets = 0;
    const uint32_t camOffset = ShaderBase::alignedSharedSize * (td->lastSwapChainImageIndex + 8);
    
    int i = 0;
    for (auto& rp : ol->renderPops)
    {
        s_pop& pop = ol->pops[rp.popName];
        glm::vec2 pos = rp.pos;

        s_optionData optData;
        optData.posSize = glm::vec4(pos.x, pos.y, pop.size.x, pop.size.y);
        optData.data.x = pop.mask;
        optData.data.y = 1;

        glm::vec2 cp = ol->cam.getMouseCoords();

        if (ol->popHoverIndices[i] >= 0)
        {
            optData.data.x |= (ol->popHoverIndices[i] & 0xff) << 24;
        }
        else
        {
            optData.data.x |= 0xF << 24;
        }

        vkCmdBindPipeline(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS, os->pipeline);

        vkCmdBindVertexBuffers(cmdBuff, 0, 1, &td->wallLight->buffers[0]->buffer, &offsets);

        vkCmdBindDescriptorSets(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS,
            os->pipelineLayout, 0, 1, &ShaderBase::sharedSet, 1, &camOffset);

        vkCmdPushConstants(cmdBuff, os->pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(s_optionData), (void*)&optData);

        vkCmdDraw(cmdBuff, td->wallLight->buffers[0]->count, 1, 0, 0);

        int j = 0;
        for (const auto& rd : pop.texts)
        {
            ol->cd->renderDatas.push_back({ rd, { optData.posSize.x + 4.5, optData.posSize.y - 2.0 - j++ * 4.0, 3.0, 0.0 }, { 0.0, 0.0, 0.0, 1.0 } });
        }

        i++;
    }

    // arrow
    if (ol->itemRatio > 0.0)
    {
        char hoverGridX = ol->selectedItemIndex % ol->numX;
        char hoverGridY = ol->selectedItemIndex / ol->numX;

        s_itemData iData;
        iData.data[0] = hoverGridX;
        iData.data[1] = hoverGridY;
        iData.posSize = glm::vec4(ol->leftX - ol->xSize * 0.5 + ol->itemRatio * ol->xSize, 0.0, ol->xSize, ol->ySize + 1.0);

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

            vkCmdDraw(cmdBuff, td->wallLight->buffers[0]->count, 1, 0, (i + td->lastSwapChainImageIndex * ol->totalNum + ol->startAlloc));

            i++;
        }
    }

    // arrow
    s_arrowData aData;
    aData.posSize = glm::vec4(ol->leftX + 1.5f + ol->itemRatio * ol->xSize, 0.0, 3.0, 3.0);
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

int Overlayer::addPop(std::string popName, std::vector<std::string> texts, std::vector<std::string> secondaryPops, int mask)
{
    assert(texts.size() == secondaryPops.size());

    s_pop pop;
    pop.mask = mask;
    pop.secondaryPops = secondaryPops;

    float maxSize = 1.0;
    for (std::string& text : texts)
    {
        float x;
        std::vector<s_charRenderData> rd = cd->getRenderData(text, 100, x);
        
        maxSize = std::max(maxSize, x);

        pop.texts.push_back(rd);
	}

    pop.size.x = maxSize * 3.0 + 6.5;
    pop.size.y = texts.size() * 4.0;

    pops[popName] = pop;

    return pops.size() - 1;
}

void Overlayer::addItem(char* name, char* fileName, levelObject objectType, glm::vec2 gridRenderTranslation, int numTextureObjects,
    glm::ivec2 gridScale, float gridRenderScale, bool canRotate, int requiredSideFill)
{
    s_gridItemData itemData;
    itemData.objectType = objectType;
    itemData.name = name;
    itemData.gridRenderTranslation = gridRenderTranslation;
    itemData.gridScale = gridScale;
    itemData.gridRenderScale = gridRenderScale;
    itemData.canRotate = canRotate;
    itemData.requiredSideFill = requiredSideFill;
    itemData.numTextureObjects = numTextureObjects;

    s_texture* tex = IOHelper::loadTexture(fileName, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);
    itemData.texSet = tex->set;
    itemData.extraScale = 0.0;

    items.push_back(itemData);
}

void Overlayer::popUpdate()
{
    renderPops.clear();

    glm::vec2 cp = cam.getMouseCoords();

    if (thinDrawer->wh->mouseData[3] == 2)
    {
        popHoverIndices[0] = -1;
        popHoverIndices[1] = -1;
        popShowName = "button";
        s_pop& pop = pops[popShowName];
        popShowPos = cp;
        popShowPos = glm::vec2(std::min(cam.xSides.y - pop.size.x, popShowPos.x), std::max(cam.ySides.x + pop.size.y, popShowPos.y));
    }

    if (popShowName.length() > 0)
    {
        s_pop& pop = pops[popShowName];

        if (cp.x > popShowPos.x && cp.x < popShowPos.x + pop.size.x &&
            cp.y < popShowPos.y && cp.y > popShowPos.y - pop.size.y)
        {
            int intPosY = (int)((-cp.y + popShowPos.y) / 4.0);
            popHoverIndices[0] = intPosY;
            popHoverIndices[1] = -1;

            if (thinDrawer->wh->mouseData[2] == 2 && !(1 << popHoverIndices[0] & pop.mask >> 16))
            {
                popShowName = "";
                goto ifEnd;
            }
        }
        else
        {
            if (thinDrawer->wh->mouseData[2] == 2)
            {
                popShowName = "";
                goto ifEnd;
            }

            if (!(1 << popHoverIndices[0] & pop.mask >> 16))
            {
                popHoverIndices[0] = -1;
                popHoverIndices[1] = -1;
            }
        }

        renderPops.push_back({ popShowName, popShowPos });

        if (popHoverIndices[0] >= 0 && (1 << popHoverIndices[0] & pop.mask >> 16))
        {
            std::string index2 = pop.secondaryPops[std::min((int)pop.secondaryPops.size() - 1, popHoverIndices[0])];
            if (index2.length() > 0 && pops.find(index2) != pops.end())
            {
                s_pop& pop2 = pops[index2];
                glm::vec2 pos2 = { popShowPos.x + pop.size.x, popShowPos.y - popHoverIndices[0] * 4.0f };
                pos2.y = std::max(cam.ySides.x + pop2.size.y, pos2.y);

                if (pos2.x + pop2.size.x > cam.xSides.y)
                {
                    pos2.x -= pop.size.x + pop2.size.x;
                }

                if (cp.x > pos2.x && cp.x < pos2.x + pop2.size.x &&
                    cp.y < pos2.y && cp.y > pos2.y - pop2.size.y)
                {
                    int intPosY = (int)((-cp.y + pos2.y) / 4.0);
                    popHoverIndices[1] = intPosY;
                    if (thinDrawer->wh->mouseData[2] == 2 && !(1 << popHoverIndices[1] & pop.mask >> 16))
                    {
                        popShowName = "";
                        goto ifEnd;
                    }
                }
                else if (!(1 << popHoverIndices[1] & pop2.mask >> 16))
                {
                    popHoverIndices[1] = -1;
                }

                renderPops.push_back({ index2, pos2 });
            }
        }
    }
    ifEnd:
}

void Overlayer::update()
{
    glm::vec2 p = cam.getMouseCoords();

    popUpdate();

    const float mult = 6.0;

    if (leftX + itemRatio * xSize + 1.5 < p.x || game->leveler->clickStart[0] != -100000 || game->leveler->consqClickVec.size() > 0)
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
        char leftX = this->leftX - xSize + itemRatio * xSize;
        char upY = ySize * 0.5f;
        
        char hoverGridX = std::floor((p.x - leftX - 0.5f) / 9.0f);
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
                        if (selectedItemIndex != hoverIndex)
                        {
                            game->leveler->selectedObjectRotation = 0;
                            game->leveler->consqClickVec.clear();
                        }

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