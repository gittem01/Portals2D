#include <Overlayer.h>
#include <Game.h>
#include <ThinDrawer.h>
#include <Sounder.h>
#include <OptionShader.h>
#include "Menu.h"
#include "PopUp.h"

// funnel thingy
//if (wh->keyData[SDL_SCANCODE_SPACE] == 2)
//{
//    s_funnelUbo* funnelUbo;
//    vkMapMemory(thinDrawer->logicalDevice, thinDrawer->funnelShader->uniformBuffers[0].memory, 0, sizeof(s_funnelUbo), 0, (void**)&funnelUbo);
//    float colorSplitPos = std::max(std::min(funnelUbo->data2.x + funnelUbo->data.w * ((float)game->gameTime - funnelUbo->data2.w) * -200.0f, 70.0f), -10.0f);
//    funnelUbo->data.y += (game->gameTime - funnelUbo->data2.w) * funnelUbo->data.w;
//    funnelUbo->data.w *= -1.0f;
//    funnelUbo->data2.x = colorSplitPos;
//    funnelUbo->data2.w = game->gameTime;
//    vkUnmapMemory(thinDrawer->logicalDevice, thinDrawer->funnelShader->uniformBuffers[0].memory);
//
//    vkMapMemory(thinDrawer->logicalDevice, thinDrawer->funnelShader->uniformBuffers[0].memory, sizeof(s_funnelUbo), sizeof(s_funnelUbo), 0, (void**)&funnelUbo);
//    funnelUbo->data.y += (game->gameTime - funnelUbo->data2.w) * funnelUbo->data.w;
//    funnelUbo->data.w *= -1.0f;
//    funnelUbo->data2.x = colorSplitPos;
//    funnelUbo->data2.w = game->gameTime;
//    vkUnmapMemory(thinDrawer->logicalDevice, thinDrawer->funnelShader->uniformBuffers[0].memory);
//}

// hazard thingy -same for blue thingy-
//if (wh->keyData[SDL_SCANCODE_SPACE] == 2)
//{
//    s_hazardUbo* hazardUbo;
//    vkMapMemory(thinDrawer->logicalDevice, thinDrawer->hazardShader->uniformBuffers[0].memory, 0, sizeof(s_funnelUbo), 0, (void**)&hazardUbo);
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
//    vkUnmapMemory(thinDrawer->logicalDevice, thinDrawer->hazardShader->uniformBuffers[0].memory);
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

    rightClickItem.ptr = nullptr;

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
    undoSound = sounder->loadSound("undo.wav");
    redoSound = sounder->loadSound("redo.wav");

    this->cd = new CharDrawer(thinDrawer);

    pushRenderFunctions();

    items.resize(getLevelObjectIndex(PLATFORM) + 1);

    addItem("PortallableWall", "ww.png", WHITE_WALL);
    addItem("N_PotallableWall", "bb.png", BLACK_WALL);
    addItem("GlassWall", "glassWall.png", GLASS_WALL);
    addItem("FenceWall", "fenceWall.png", FENCE_WALL);

    addItem("", "cube.png", DEFAULT_CUBE, "remove", { 0.5, 0.1 }, 1, { 1.0, 1.0 }, 0.8, false, 0, true);
    addItem("", "downButton.png", DOWN_BUTTON, "base_connector", { 1.0, +0.35 }, 1, { 2, 1 }, 1.6, true, 0);
    addItem("", "laserer.png", LASERER, "activated_connector", { 0.5, +0.29 }, 1, { 1.0, 1.0 }, 0.6, true, 0);
    addItem("", "funnel.png", FUNNEL, "activated_connector_funnel", { 0.5, +0.23 }, 1, { 2, 1 }, 2.0, true, 0);
    addItem("", "hazard.png", HAZARDER, "activated_connector", { 0.5, +0.44 }, 2, { 1, 1 }, 0.8, true, 0);
    addItem("", "blueFielder.png", FORCE_FIELD, "activated_connector", { 0.5, +0.43 }, 2, { 1, 1 }, 0.8, true, 0);
    addItem("", "plate.png", CATAPULT, "remove", { 0.5, +0.345 }, 1, { 1, 1 }, 1.0, true, 0);
    addItem("", "lightBridge.png", LIGHT_BRIDGE, "activated_connector", { 0.5, +0.32 }, 1, { 1, 1 }, 0.8, true, 0);

    addItem("", "laserCube.png", LASER_CUBE, "remove", { 0.5, 0.1 }, 1, { 1.0, 1.0 }, 0.8, false, 0, true);
    addItem("", "laserReceiverGround.png", LASER_REC_GND, "base_connector", { 1.0, +0.235 }, 1, { 1, 1 }, 0.6, true, 0);
    addItem("", "laserReceiverWall.png", LASER_REC_WALL, "base_connector", { 0.5, +0.285 }, 1, { 1.0, 1.0 }, 0.8, true, 0);
    addItem("", "sphere.png", SPHERE_CUBE, "remove", { 0.5, +0.01 }, 1, { 1, 1 }, 1.0, false, 0, true);
    addItem("", "turret.png", TURRET, "remove", { 0.5, +0.23 }, 1, { 1, 2 }, 1.6, false, 0, true);
    addItem("", "clickButton.png", CLICK_BUTTON, "push_button", { 0.5, +0.32 }, 2, { 1, 2 }, 1.4, true, 0, true);
    addItem("", "cubeButton.png", CUBE_BUTTON, "base_connector", { 0.5, +0.35 }, 1, { 2, 1 }, 1.6, true, 0);
    addItem("", "circleButton.png", SPHERE_BUTTON, "base_connector", { 0.5, +0.37 }, 1, { 2, 1 }, 1.6, true, 0);

    addItem("", "blueGeller.png", BLUE_GELLER, "activated_connector", { 0.5, -0.02 }, 1, { 2, 2 }, 2.0, true, 2);
    addItem("", "orangeGeller.png", ORANGE_GELLER, "activated_connector", { 0.5, -0.02 }, 1, { 2, 2 }, 2.0, true, 2);
    addItem("", "whiteGeller.png", WHITE_GELLER, "activated_connector", { 0.5, -0.02 }, 1, { 2, 2 }, 2.0, true, 2);
    addItem("", "waterGeller.png", WATER_GELLER, "activated_connector", { 0.5, -0.02 }, 1, { 2, 2 }, 2.0, true, 2);

    addItem("", "platform.png", PLATFORM, "platform", { 0.5, +0.0 }, 1, { 1, 1 }, 1.0, true, 0);

    addItem("", "spawner.png", CUBE_SPAWNER, "spawner", { 0.5, 0.0 }, 1, { 2, 2 }, 2.0, false, 2);

    addItem("", "x.png", ERASER);

    getPops();

    for (int i = 0; i < items.size(); i++)
    {
        std::string s = "<" + std::to_string(i) + ">";
        items[i].name = Leveler::object->itemNameMap[s][cd->selectedLanguageIndex];
    }

    auto polarizations = IOHelper::loadLines("pops/polarization_language.txt");
    auto polarizationMap = IOHelper::mapVector(polarizations);

    for (auto& p : polarizationMap)
    {
        if (p.first.size() < 2 || p.first[1] < '0' || p.first[1] > '9') continue;
        std::string subString = p.first.substr(1, p.first.size() - 2);
        polarizationString[std::stoi(subString)] = p.second[0];
    }

    s_pop& levelerOptions = pops["leveler_options"];
    const float by = 2.0f;

	topLeftButtons[0].colour = glm::vec4(1.0, 1.0, 1.0, 1.0);
	topLeftButtons[0].posSize = glm::vec4(0.0f, 0.0f, xSize * 0.45f, by);
    topLeftButtons[0].data = glm::vec4(xSize * 0.03f, xSize * 0.04f, 1.0f, 0.5f);
    topLeftHoverTimes[0] = 0.0;

    topLeftButtons[1].colour = glm::vec4(1.0, 1.0, 1.0, 1.0);
    topLeftButtons[1].posSize = glm::vec4(0.0f, 0.0f, xSize * 0.45f, by);
    topLeftButtons[1].data = glm::vec4(xSize * 0.03f, xSize * 0.04f, 1.0f, 0.5f);
    topLeftHoverTimes[1] = 0.0;
}

void Overlayer::pushRenderFunctions()
{
    thinDrawer->memMapFuncs.push_back({ maper, (void*)this });
    thinDrawer->lateDrawFuncs.push_back({ draw, (void*)this });
    thinDrawer->lateDrawFuncs.push_back({ CharDrawer::draw, cd });
}

void Overlayer::eraseRenderFunctions()
{
    thinDrawer->eraseFromMaperFunctions(thinDrawer->memMapFuncs, (void*)this);
    thinDrawer->eraseFromRenderFunctions(thinDrawer->lateDrawFuncs, (void*)this);
}

void Overlayer::camMap(void* ptr)
{
    Overlayer* ol = Leveler::object->ol;
    ThinDrawer* td = ThinDrawer::object;

    s_sharedUniformData* sharedData;
    CHECK_RESULT_VK(vkMapMemory(td->logicalDevice, ShaderBase::sharedOrthoUniform->memory,
        ShaderBase::alignedSharedSize * (td->frameIndex + 8),
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

void Overlayer::maper(void* ptr)
{
    Overlayer* ol = Leveler::object->ol;
    ThinDrawer* td = ThinDrawer::object;
    WindowHandler* wh = ol->wh;

    camMap(ptr);

    if (ol->itemRatio > 0.0)
    {
        int i = 0;
        for (s_gridItemData& item : ol->items)
        {
            char x = i % ol->numX;
            char y = i / ol->numX;

            s_manyTextureData* mtd;
            vkMapMemory(ol->mt->logicalDevice, ol->mt->uniformBuffers[0].memory, (i + td->frameIndex * ol->totalNum + ol->startAlloc) * sizeof(s_manyTextureData),
                sizeof(s_manyTextureData), 0, (void**)&mtd);
            mtd->posSize = glm::vec4(ol->leftX - ol->xSize + ol->itemRatio * ol->xSize + x * 9.0 + 5.0f, 31.5 - y * 9.0,
                (6.0 + item.extraScale) * item.scaleRatio.x, (6.0 + item.extraScale) * item.scaleRatio.y);

            mtd->data.x = 0.0;
            vkUnmapMemory(ol->mt->logicalDevice, ol->mt->uniformBuffers[0].memory);

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
    const uint32_t camOffset = ShaderBase::alignedSharedSize * (td->frameIndex + 8);
    
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

        vkCmdBindVertexBuffers(cmdBuff, 0, 1, &td->wallLight->buffers[0].buffer, &offsets);

        vkCmdBindDescriptorSets(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS,
            os->pipelineLayout, 0, 1, &ShaderBase::sharedSet, 1, &camOffset);

        vkCmdPushConstants(cmdBuff, os->pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(s_optionData), (void*)&optData);

        vkCmdDraw(cmdBuff, td->wallLight->buffers[0].count, 1, 0, 0);

        int j = 0;
        for (const auto& rd : pop.texts)
        {
            ol->cd->renderDatas.push_back({ rd, { optData.posSize.x + 3.5, optData.posSize.y - 1.5 - j++ * 3.0, 2.5, 0.0 }, { 0.0, 0.0, 0.0, 1.0 } });
        }

        i++;
    }

    if (ol->itemRatio > 0.0)
    {
        char hoverGridX = ol->selectedItemIndex % ol->numX;
        char hoverGridY = ol->selectedItemIndex / ol->numX;

        s_itemData iData;
        iData.data[0] = hoverGridX;
        iData.data[1] = hoverGridY;
        iData.posSize = glm::vec4(ol->leftX - ol->xSize * 0.5 + ol->itemRatio * ol->xSize, 0.0, ol->xSize, ol->ySize + 1.0);

        vkCmdBindPipeline(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS, ol->il->pipeline);

        vkCmdBindVertexBuffers(cmdBuff, 0, 1, &td->wallLight->buffers[0].buffer, &offsets);

        vkCmdBindDescriptorSets(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS,
            ol->il->pipelineLayout, 0, 1, &ShaderBase::sharedSet, 1, &camOffset);

        vkCmdPushConstants(cmdBuff, ol->il->pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
            0,sizeof(s_itemData), (void*)&iData);

        vkCmdDraw(cmdBuff, td->wallLight->buffers[0].count, 1, 0, 0);

        // items tex
        vkCmdBindPipeline(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS, ol->mt->pipeline);

        vkCmdBindVertexBuffers(cmdBuff, 0, 1, &td->wallLight->buffers[0].buffer, &offsets);

        vkCmdBindDescriptorSets(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS,
            ol->mt->pipelineLayout, 0, 1, &ShaderBase::sharedSet, 1, &camOffset);

        vkCmdBindDescriptorSets(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS,
            ol->mt->pipelineLayout, 1, 1, &ol->mt->descriptorSets[1], 0, VK_NULL_HANDLE);

        int i = 0;
        for (s_gridItemData& item : ol->items)
        {
            vkCmdBindDescriptorSets(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS,
                ol->mt->pipelineLayout, 2, 1, &item.tex->set, 0, VK_NULL_HANDLE);

            vkCmdDraw(cmdBuff, td->wallLight->buffers[0].count, 1, 0, (i + td->frameIndex * ol->totalNum + ol->startAlloc));

            i++;
        }

        vkCmdBindPipeline(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS, td->menuButton->pipeline);

        vkCmdBindDescriptorSets(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS,
            td->menuButton->pipelineLayout, 0, 1, &ShaderBase::sharedSet, 1, &camOffset);
        for (int i = 0; i < 2; i++)
        {
            if (ol->overlayOverlay - LOAD_LEVEL == i)
            {
                continue;
            }

            vkCmdPushConstants(cmdBuff, td->menuButton->pipelineLayout,
                VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(s_menuButton), &ol->topLeftButtons[i]);

            vkCmdDraw(cmdBuff, 6, 1, 0, 0);
        }
    }

    // arrow
    s_arrowData aData;
    aData.posSize = glm::vec4(ol->leftX + 1.5f + ol->itemRatio * ol->xSize, 0.0, 3.0, 3.0);
    aData.color = glm::vec4(0.5f, 1.0f, 1.0f, 1.0f - ol->itemRatio * 0.75);
    aData.data = glm::vec4(-ol->itemRatio * 0.3, 0.5 - ol->itemRatio * 0.2, -0.5 + ol->itemRatio * 0.5, 0.5);
    aData.data2 = glm::vec4(0.0, 0.3 - ol->itemRatio * 0.2, 0.5 - ol->itemRatio * 0.3, 0.1);

    vkCmdBindPipeline(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS, ol->as->pipeline);

    vkCmdBindVertexBuffers(cmdBuff, 0, 1, &td->wallLight->buffers[0].buffer, &offsets);

    vkCmdBindDescriptorSets(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS,
        ol->as->pipelineLayout, 0, 1, &ShaderBase::sharedSet, 1, &camOffset);

    vkCmdPushConstants(cmdBuff, ol->as->pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(s_arrowData), (void*)&aData);

    vkCmdDraw(cmdBuff, td->wallLight->buffers[0].count, 1, 0, 0);

    if (ol->selectedItemIndex == -1)
    {
        static float addition = 0.0f;

        if (ol->hoveringPlay)
        {
            addition += ol->game->deltaTime * 10.0;
        }
        else
        {
            addition -= ol->game->deltaTime * 10.0;
        }

        addition = std::max(std::min(addition, 1.0f), 0.0f);

        // topArrow
        aData.posSize.x = 0.0;
        aData.posSize.y = ol->cam.ySides.y - 2.0;
        aData.color = glm::vec4(1.0f, 1.0f, 0.0f, 0.5 + addition * 0.5);
        aData.data = { -0.2f - 0.3 * addition, 0.5f, -0.5f, 0.5f };
        aData.data2 = { 0.0, 0.3 - addition * 0.15, 0.3 - addition * 0.15, 0.3 };

        vkCmdPushConstants(cmdBuff, ol->as->pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(s_arrowData), (void*)&aData);

        vkCmdDraw(cmdBuff, td->wallLight->buffers[0].count, 1, 0, 0);
    }
}

void Overlayer::getPops()
{
    float camY = cam.ySides.y - cam.ySides.x;
    float camRatio = 2.5f / camY;
    popHeight = cd->creationHeight * camRatio * 1.5f;

    auto popNames = IOHelper::loadLines("popNames.txt");
    for (auto& name : popNames)
    {
        std::string fileName = "pops/" + name + ".txt";
        auto popLines = IOHelper::loadLines(fileName);
        auto popMap = IOHelper::mapVector(popLines);
        std::string langString = "<" + std::to_string(cd->selectedLanguageIndex) + ">";

        auto popStrings = popMap[langString];
        auto links = popMap["<linked_options>"];
        auto signal_options = popMap["<signal_options>"];

        int mask = 0;
        for (int i = 0; i < signal_options.size(); i++)
        {
            int option = std::stoi(signal_options[i]);
            mask |= (option << i);
        }

        for (int i = 0; i < links.size(); i++)
        {
            auto link = links[i];
            if (!(link.size() == 1 && link[0] == '-'))
            {
                mask |= ((1 << 16) << i);
            }
        }

        addPop(name, popStrings, links, mask);
    }
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
        std::vector<int> rd = cd->getRenderData(text, popHeight, x);
        
        maxSize = std::max(maxSize, x);

        pop.texts.push_back(rd);
	}

    pop.size.x = maxSize * 2.5 + 6.5;
    pop.size.y = texts.size() * 3.0;

    pops[popName] = pop;

    return pops.size() - 1;
}

void Overlayer::addItem(const char* name, const char* fileName, levelObject objectType,
    std::string popName,glm::vec2 gridRenderTranslation, int numTextureObjects, glm::ivec2 gridScale,
    float gridRenderScale, bool canRotate, int requiredSideFill, bool canMirror, float extraScale)
{
    s_gridItemData itemData;
    itemData.objectType = objectType;
    itemData.name = name;
    itemData.popName = popName;
    itemData.gridRenderTranslation = gridRenderTranslation;
    itemData.gridScale = gridScale;
    itemData.gridRenderScale = gridRenderScale;
    itemData.canRotate = canRotate;
    itemData.requiredSideFill = requiredSideFill;
    itemData.numTextureObjects = numTextureObjects;
    itemData.canMirror = canMirror;

    itemData.extraScale = extraScale;

    s_texture* tex = IOHelper::loadTexture(fileName, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);
    itemData.tex = tex;
    itemData.scaleRatio = glm::vec2(tex->width / (float)tex->height, 1.0f);
    if (itemData.scaleRatio.x > 1.0)
    {
		itemData.scaleRatio = glm::vec2(1.0f, 1.0f / itemData.scaleRatio.x);
	}

    if (objectType & (ERASER))
    {
        items.push_back(itemData);
    }
    else
    {
        int index = getLevelObjectIndex(objectType);
        items[index] = itemData;
    }
}

void Overlayer::popUpdate()
{
    renderPops.clear();

    glm::vec2 cp = cam.getMouseCoords();

    if (thinDrawer->wh->mouseData[4] == 2 && !itemDir && selectedItemIndex == -1)
    {
        levelerRCPos = game->leveler->getMouseGrid();
        const s_levelGridData& lgd = game->leveler->getSquare(levelerRCPos.x, levelerRCPos.y);
        if (lgd.objectType & (SINGLE_DRAWS | PLATFORM))
        {
            rightClickItem = lgd.data;
            s_objectDataGrid* objectData = (s_objectDataGrid*)rightClickItem.ptr;

            s_pop* ac = &pops["activated_connector"];
            ac->mask &= 0xF0 ^ -1ll;

            std::vector<std::string> connectionNames;
            std::vector<std::string> empties;
            for (s_connection* conn : objectData->connections)
            {
                int itemIndex;
                if (conn->transmitter == objectData)
                {
                    itemIndex = getLevelObjectIndex(conn->receiver->objectType);
                }
                else
                {
                    itemIndex = getLevelObjectIndex(conn->transmitter->objectType);
                }

                const s_gridItemData& gid = items[itemIndex];
                connectionNames.push_back(gid.name + (conn->changePolarization ? (" - " + polarizationString[cd->selectedLanguageIndex]) : ""));
                empties.push_back("");
            }

            addPop("connection_select", connectionNames, empties, 0);
        }

        int itemIndex = getLevelObjectIndex(lgd.objectType);
        if (itemIndex < 0)
        {
            popShowName = "none";
        }
        else
        {
            const s_gridItemData& gid = items[itemIndex];
            popShowName = gid.popName;
        }

        if (popShowName == "none")
        {
            rightClickItem.ptr = nullptr;
        }
        
        lastPopName = popShowName;

        if (pops.find(popShowName) != pops.end())
        {
            popHoverIndices[0] = -1;
            popHoverIndices[1] = -1;
            s_pop& pop = pops[popShowName];
            popShowPos = cp;
            popShowPos = glm::vec2(std::min(cam.xSides.y - pop.size.x, popShowPos.x), std::max(cam.ySides.x + pop.size.y, popShowPos.y));
        }
    }

    if (popShowName.length() > 0)
    {
        s_pop& pop = pops[popShowName];

        if (rightClickItem.ptr)
        {
            s_objectDataGrid* objectData = (s_objectDataGrid*)rightClickItem.ptr;
            if (objectData->objectType & RECEIVERS)
            {
                int shift = objectData->objectType & PLATFORM ? 12 : 11;
                pop.mask &= (3 << shift) ^ -1ll;
                pop.mask |= objectData->startActive << shift;
                pop.mask |= (objectData->objectType & PLATFORM ? objectData->loop : (
                    objectData->objectType & (FUNNEL | CUBE_SPAWNER) ? objectData->startPolarized : objectData->speed)) << (shift + 1);
            }
        }

        if (cp.x > popShowPos.x && cp.x < popShowPos.x + pop.size.x && cp.y < popShowPos.y && cp.y > popShowPos.y - pop.size.y)
        {
            int intPosY = (int)((-cp.y + popShowPos.y) / 3.0);
            popHoverIndices[0] = intPosY;
            popHoverIndices[1] = -1;

            if (thinDrawer->wh->mouseData[2] == 2 && !(1 << popHoverIndices[0] & pop.mask >> 16) && !(1 << popHoverIndices[0] & pop.mask))
            {
                popShowName = "";
                return;
            }
        }
        else
        {
            if (!(1 << popHoverIndices[0] & pop.mask >> 16))
            {
                popHoverIndices[0] = -1;
                popHoverIndices[1] = -1;
            }

            if (thinDrawer->wh->mouseData[2] == 2 && popHoverIndices[1] == -1)
            {
                popShowName = "";
                return;
            }
        }

        renderPops.push_back({ popShowName, popShowPos });

        if (popHoverIndices[0] >= 0 && (1 << popHoverIndices[0] & pop.mask >> 16))
        {
            std::string index2 = pop.secondaryPops[std::min((int)pop.secondaryPops.size() - 1, popHoverIndices[0])];
            if (index2.length() > 0 && pops.find(index2) != pops.end())
            {
                s_pop& pop2 = pops[index2];

                if (rightClickItem.ptr)
                {
                    s_objectDataGrid* objectData = (s_objectDataGrid*)rightClickItem.ptr;
                    if (objectData->objectType & PLATFORM)
                    {
                        if (popHoverIndices[0] == 3)
                        {
                            pop2.mask &= (0xF << 8) ^ -1ll;
                            pop2.mask |= 1 << (objectData->platformType + 8);
                        }
						else if (popHoverIndices[0] == 6)
						{
							pop2.mask &= (0xF << 8) ^ -1ll;
							pop2.mask |= 1 << (objectData->speed + 8);
						}
                    }
                    else if (objectData->objectType & CLICK_BUTTON && popHoverIndices[0] == 3)
                    {
                        pop2.mask &= (0x1F << 8) ^ -1ll;
                        pop2.mask |= 1 << (objectData->speed + 8);
                    }
                    else if (objectData->objectType & CUBE_SPAWNER && popHoverIndices[0] == 5)
                    {
                        pop2.mask &= (0x1F << 8) ^ -1ll;
                        pop2.mask |= 1 << (objectData->speed + 8);
                    }
                }

                glm::vec2 pos2 = { popShowPos.x + pop.size.x, popShowPos.y - popHoverIndices[0] * 3.0f };
                pos2.y = std::max(cam.ySides.x + pop2.size.y, pos2.y);

                if (pos2.x + pop2.size.x > cam.xSides.y)
                {
                    pos2.x -= pop.size.x + pop2.size.x;
                }

                if (cp.x > pos2.x && cp.x < pos2.x + pop2.size.x &&
                    cp.y < pos2.y && cp.y > pos2.y - pop2.size.y)
                {
                    int intPosY = (int)((-cp.y + pos2.y) / 3.0);
                    popHoverIndices[1] = intPosY;
                    if (thinDrawer->wh->mouseData[2] == 2 && !(1 << popHoverIndices[1] & pop2.mask >> 16) && !(1 << popHoverIndices[1] & pop2.mask))
                    {
                        popShowName = "";
                        return;
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
    else
    {
        popHoverIndices[0] = -1;
        popHoverIndices[1] = -1;
        rightClickItem.ptr = nullptr;
    }
}

void Overlayer::handleHoverPlay()
{
    glm::vec2 playPos = { 0.0f, cam.ySides.y - 2.0 };
    glm::vec2 p = cam.getMouseCoords();
    clickedPlay = false;

    if (glm::length(p - playPos) < 1.5)
    {
        hoveringPlay = true;
    }
    else
    {
        hoveringPlay = false;
    }

    if (wh->mouseData[2] == 2)
    {
        if (hoveringPlay)
        {
            clickStartOnPlay = true;
        }
    }

    if (wh->mouseData[2] == 0)
    {
        if (clickStartOnPlay && hoveringPlay && selectedItemIndex == -1)
        {
            clickedPlay = true;
        }

        clickStartOnPlay = false;
    }
}

const float mult = 6.0;

void Overlayer::topLeftUpdate()
{
    const glm::vec4 buttonClr = glm::vec4(0.03f, 0.00f, 0.10f, 1.0f);

    if (wh->keyData[SDL_SCANCODE_ESCAPE] == 2)
    {
        selectedItemIndex = -1;
        popShowName = "";
        renderPops.clear();

        if (overlayOverlay == LEVELER)
        {
            overlayOverlay = LEVELER_MENU;

            game->menu->initDefaults();

            game->menu->initWithName("back", glm::vec4(1.0, 0.5, 0.0, 0.5f));
            game->menu->initSettings(glm::vec4(1.0, 1.0, 1.0, 0.4));
            game->menu->initWithName("to_menu", glm::vec4(1.0f, 1.0f, 1.0f, 0.4f));

            game->menu->renderDestroy();

			thinDrawer->lateDrawFuncs.push_back({ Menu::draw, game->menu });
            
            game->menu->setNewState(true);
        }
        else if (!(overlayOverlay == LEVELER_MENU && game->pop.active))
        {
			overlayOverlay = LEVELER;

            game->menu->setNewState(false);
        }
    }

    if (overlayOverlay == LEVELER_MENU)
    {
		if (game->menu->activeButton)
		{
            if (game->menu->activeButton->strText == "back")
            {
			    overlayOverlay = LEVELER;

                game->menu->setNewState(false);
            }
            else if (game->menu->activeButton->strText == "to_menu")
            {
                s_menuButton initialButton = {
                    glm::vec4(0.0f, 0.0f, 300.0f, 300.0f), // posSize
                    glm::vec4(0.0f, 0.0f, 0.0f, 0.0), // clr
                    glm::vec4(5.0f, 5.5f, 1.0f, 0.1f) }; // data
                game->pop.init(initialButton, "darkness", 0.5);

                game->pop.endFunction = [this]
                {
                    game->gameStater = &Game::MenuState;
                    game->shouldQuit = true;
                };
            }
            else
            {
                game->menu->update();
            }
		}
        else
        {
            game->menu->update();
        }
    }

    s_pop& levelerOptions = pops["leveler_options"];
    float xAdd = -xSize * (1.0f - itemRatio) + 0.3f;

    int hoverIndex = -1;
    for (int i = 0; i < 2; i++)
    {
        float texSize = cd->calcSize(levelerOptions.texts[i]);

        topLeftButtons[i].posSize.x = leftX + xSize * (0.24f + i * 0.50f) + xAdd;
        topLeftButtons[i].posSize.y = cam.defaultYSides.y - topLeftButtons[i].posSize.w * 0.5f - 0.9f;

        if (topLeftButtons[i].hoverCheck(cam.getMouseCoords()) && hoverIndex < 0 && overlayOverlay == LEVELER)
        {
            topLeftButtons[i].colour = buttonClr * 0.75f;
            topLeftHoverTimes[i] = thinDrawer->gameTime;
            hoverIndex = i;
        }
        else
        {
            topLeftButtons[i].colour = buttonClr;
        }

        topLeftButtons[i].data.y = std::max(1.0f - (float)(thinDrawer->gameTime - topLeftHoverTimes[i]) * 4.0f, xSize * 0.05f);

        if (overlayOverlay - LOAD_LEVEL == i)
            continue;

        cd->renderDatas.push_back({ levelerOptions.texts[i],
            { topLeftButtons[i].posSize.x - texSize * 1.5f, topLeftButtons[i].posSize.y, 3.0, 0.0 }, { 1.0, 1.0, 1.0, 1.0 } });
    }

    if (hoverIndex >= 0 && wh->mouseData[2] == 2 && !game->pop.active)
    {
        overlayOverlay = (overlayType)(LOAD_LEVEL + hoverIndex);

        s_menuButton finalButton;
		finalButton.colour = glm::vec4(0.05, 0.05, 0.05, 1.0);
		finalButton.posSize = glm::vec4(0.0f, 0.0f, 80.0f, 10.0f);
		finalButton.data = glm::vec4(5.0f, 5.25f, 1.1f, 1.0f);

        const char* tlNames[2] = { "loadRender", "saveRender" };

        selectedItemIndex = -1;

        Game::object->pop.init(topLeftButtons[hoverIndex], tlNames[hoverIndex], 0.5);

        if (overlayOverlay == SAVE_LEVEL)
        {
            Game::object->pop.renderTexts[0] = Leveler::object->openLevelName;
            Game::object->pop.textSizes.assign(Leveler::object->textSizes.begin(), Leveler::object->textSizes.end());
        }
    }
}

void Overlayer::update()
{
    glm::vec2 p = cam.getMouseCoords();

    topLeftUpdate();

    if (overlayOverlay != LEVELER)
    {
		itemRatio -= game->deltaTime * mult;
		itemRatio = std::clamp(itemRatio, 0.0f, 1.0f);
        
        return;
    }

    handleHoverPlay();

    popUpdate();

    if (wh->mouseData[4] == 2 || wh->keyData[SDL_SCANCODE_ESCAPE] == 2)
    {
        selectedItemIndex = -1;
    }

    if ((leftX + itemRatio * xSize + 1.5 < p.x && !wh->keyData[SDL_SCANCODE_TAB]) || game->leveler->clickStart[0] != -100000 ||
        game->leveler->consqClickVec.size() > 0 || popShowName != "" || wh->mouseData[3] != 0.0)
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

    itemRatio = std::clamp(itemRatio, 0.0f, 1.0f);

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
                            game->leveler->selectedObjectMirror = 0;
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