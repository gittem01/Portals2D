#include <ThinDrawer.h>
#include <SwapChain.h>
#include <Camera.h>
#include <Shader.h>
#include <vkInit.h>
#include <vector>
#include <array>
#include <Portal.h>
#include <PortalBody.h>
#include <GameBase.h>
#include <IOHelper.h>
#include <Game.h>
#include <cmath>

ThinDrawer* ThinDrawer::object = nullptr;

ThinDrawer::ThinDrawer(PortalWorld* pWorld)
{
    ThinDrawer::object = this;

    this->pWorld = pWorld;

    initBase();
    initExtra();
}

void ThinDrawer::surfaceRecreate()
{
    vkDeviceWaitIdle(logicalDevice);

    for (ShaderBase* shader : shaders)
    {
        vkDestroyPipeline(logicalDevice, shader->pipeline, VK_NULL_HANDLE);
    }

    swapChain->destroy();
    
    swapChain->creationLoop();
    createRenderPass();
    swapChain->createFrameBuffers();
    preparePipelines();

   for (int i = 0; i < frames.size(); i++)
   {
       vkResetCommandBuffer(frames[i].commandBuffer, 0);
   }

    vkDeviceWaitIdle(logicalDevice);
}

void ThinDrawer::initExtra()
{
    debugCircleShader = new DebugCircle(this); shaders.push_back(debugCircleShader);
    debugLineShader = new DebugLine(this); shaders.push_back(debugLineShader);
    debugPolygonShader = new DebugPolygon(this); shaders.push_back(debugPolygonShader);
    whiteWallShader = new WhiteWall(this); shaders.push_back(whiteWallShader);
    blackWallShader = new BlackWall(this); shaders.push_back(blackWallShader);
    portalShader = new PortalShader(this); shaders.push_back(portalShader);
    portalParticleShader = new PortalParticleShader(this); shaders.push_back(portalParticleShader);
    dualParticleShader = new DualParticleShader(this); shaders.push_back(dualParticleShader);
    bodyShader3D = new Body(this); shaders.push_back(bodyShader3D);
    portalShader3D = new PortalShader3D(this); shaders.push_back(portalShader3D);
    bodyShader = new BodyShader(this); shaders.push_back(bodyShader);
    wallLight = new WallLight(this); shaders.push_back(wallLight);
    laserShader = new LaserShader(this); shaders.push_back(laserShader);
    laserPshShader = new LaserPshShader(this); shaders.push_back(laserPshShader);
    lineZ = new LineZ(this); shaders.push_back(lineZ);
    lightBridge = new LightBridgeShader(this); shaders.push_back(lightBridge);
    funnelShader = new FunnelShader(this); shaders.push_back(funnelShader);
    hazardShader = new HazardShader(this); shaders.push_back(hazardShader);
    blueField = new BlueField(this); shaders.push_back(blueField);
    gunEffect = new GunEffect(this); shaders.push_back(gunEffect);
    playerShader = new PlayerShader(this); shaders.push_back(playerShader);
    endlessGrid = new EndlessGrid(this); shaders.push_back(endlessGrid);
    gridSquare = new GridSquare(this); shaders.push_back(gridSquare);
    arrowShader = new ArrowShader(this); shaders.push_back(arrowShader);
    itemList = new ItemList(this); shaders.push_back(itemList);
    manyTexture = new ManyTexture(this); shaders.push_back(manyTexture);
    diagonalLiner = new DiagonalLiner(this); shaders.push_back(diagonalLiner);
    platformRectangle = new PlatformRectangle(this); shaders.push_back(platformRectangle);
    polynomShader = new PolynomialShader(this); shaders.push_back(polynomShader);
    gelShader = new GelShader(this); shaders.push_back(gelShader);
    wallGel = new WallGel(this); shaders.push_back(wallGel);
    splash = new SplashShader(this); shaders.push_back(splash);

    setupDescriptorPool();

    ShaderBase::init(this);
    ShaderBase::setSharedLayout(this);
    ShaderBase::writeSharedDescriptors(this);

    for (ShaderBase* shader : shaders)
    {
        shader->init();
    }

    IOHelper::loadTexture("null.png");

    initImgui();
}

void ThinDrawer::uniformFiller()
{
    s_sharedUniformData* sharedData;
    CHECK_RESULT_VK(vkMapMemory(logicalDevice, ShaderBase::sharedOrthoUniform->memory, ShaderBase::alignedSharedSize * lastSwapChainImageIndex,
        ShaderBase::alignedSharedSize, 0, (void**)&sharedData));

    sharedData->orthoMatrix = wh->cam->camMatrix;
    sharedData->cameraPos = glm::vec4(
        wh->cam->pos.x, wh->cam->pos.y,
        wh->cam->xSides.y - wh->cam->xSides.x, wh->cam->ySides.y - wh->cam->ySides.x);
    sharedData->data.x = wh->cam->zoom;
    sharedData->data.y = gameTime;

    glm::vec2 worldMouse = wh->cam->getMouseCoords();
    sharedData->data.z = worldMouse.x;
    sharedData->data.w = worldMouse.y;

    vkUnmapMemory(logicalDevice, ShaderBase::sharedOrthoUniform->memory);
}

void ThinDrawer::renderLoop()
{
    int imNum = frameNumber % swapChain->imageCount;

    s_frameData currentFrame = frames[imNum];

    CHECK_RESULT_VK(vkWaitForFences(logicalDevice, 1, &currentFrame.renderFence, true, UINT64_MAX));

    VkResult result = vkAcquireNextImageKHR(logicalDevice, swapChain->swapChain, UINT64_MAX,
                          currentFrame.presentSemaphore, VK_NULL_HANDLE, &lastSwapChainImageIndex);

    CHECK_RESULT_VK(vkResetFences(logicalDevice, 1, &currentFrame.renderFence));

    animate();

    for (auto& f : memMapFuncs)
    {
        f.first(f.second);
    }

    uniformFiller();

    vkResetCommandBuffer(frames[lastSwapChainImageIndex].commandBuffer, 0);

    buildCommandBuffers();

    VkSubmitInfo submit = { };
    submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    submit.pWaitDstStageMask = &waitStage;

    submit.waitSemaphoreCount = 1;
    submit.pWaitSemaphores = &currentFrame.presentSemaphore;

    submit.signalSemaphoreCount = 1;
    submit.pSignalSemaphores = &currentFrame.renderSemaphore;

    VkCommandBuffer cmdBuffers[] = { frames[lastSwapChainImageIndex].commandBuffer };
    submit.commandBufferCount = std::size(cmdBuffers);
    submit.pCommandBuffers = cmdBuffers;

    CHECK_RESULT_VK(vkQueueSubmit(queues.graphicsQueue, 1, &submit, currentFrame.renderFence));
    VkPresentInfoKHR presentInfo = { };
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.pSwapchains = &swapChain->swapChain;
    presentInfo.swapchainCount = 1;

    presentInfo.pWaitSemaphores = &currentFrame.renderSemaphore;
    presentInfo.waitSemaphoreCount = 1;

    presentInfo.pImageIndices = &lastSwapChainImageIndex;

    result = vkQueuePresentKHR(queues.graphicsQueue, &presentInfo);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
    {
        surfaceRecreate();
    }

    frameNumber++;
}

void ThinDrawer::buildCommandBuffers()
{
    VkCommandBuffer cmdBuff = frames[lastSwapChainImageIndex].commandBuffer;

    renderCommandBeginner(cmdBuff);

    VkDeviceSize offsets = 0;
    camOffset = ShaderBase::alignedSharedSize * lastSwapChainImageIndex;

    for (auto& f : earlyDrawFuncs)
    {
        f.first(cmdBuff, f.second);
    }

    renderWalls(cmdBuff);

    renderPortalBodies(cmdBuff);

    for (auto& f : drawFuncs)
    {
        f.first(cmdBuff, f.second);
    }

    renderPortals(cmdBuff);

    if (pWorld)
    {
        if (drawDebugBodies)
        {
            debug_renderPortalBodies(cmdBuff);
        }
        if (drawDebugEdges)
        {
            debug_renderEdges(cmdBuff);
        }
        if (drawDebugPortals)
        {
            debug_renderPortals(cmdBuff);
        }
    }

    for (auto& f : lateDrawFuncs)
    {
        f.first(cmdBuff, f.second);
    }

    if (drawData)
    {
        ImGui_ImplVulkan_RenderDrawData(drawData, cmdBuff);
    }

    vkCmdEndRenderPass(cmdBuff);
    CHECK_RESULT_VK(vkEndCommandBuffer(cmdBuff));
}

void ThinDrawer::preparePipelines()
{
    for (ShaderBase* shader : shaders)
    {
        shader->preparePipeline();
    }
}

void ThinDrawer::setupDescriptorPool()
{
    std::vector<VkDescriptorPoolSize> sizes =
    {
            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 100 },
            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 100 },
            { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 100 },
            { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 100 },
            { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
    };

    VkDescriptorPoolCreateInfo descriptorPoolInfo = { };
    descriptorPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    descriptorPoolInfo.poolSizeCount = sizes.size();
    descriptorPoolInfo.pPoolSizes = sizes.data();
    descriptorPoolInfo.maxSets = 10000;

    CHECK_RESULT_VK(vkCreateDescriptorPool(logicalDevice, &descriptorPoolInfo, VK_NULL_HANDLE, &descriptorPool));
}

void ThinDrawer::renderCommandBeginner(VkCommandBuffer& cmdBuff)
{
    VkCommandBufferBeginInfo cmdBufInfo = vkinit::commandBufferBeginInfo();

    VkClearValue clrVal = {};
    clrVal.color = { 0.0f, 0.0f, 0.0f, 1.0f };

    VkRenderPassBeginInfo renderPassBeginInfo = vkinit::renderPassBeginInfo(renderPass, swapChain->extent.width, swapChain->extent.height, &clrVal);

    renderPassBeginInfo.framebuffer = swapChain->frameBuffers[lastSwapChainImageIndex];

    CHECK_RESULT_VK(vkBeginCommandBuffer(cmdBuff, &cmdBufInfo));

    vkCmdBeginRenderPass(cmdBuff, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

    VkViewport viewport = { };
    viewport.height = (float)swapChain->extent.height;
    viewport.width = (float)swapChain->extent.width;
    viewport.minDepth = (float)0.0f;
    viewport.maxDepth = (float)1.0f;
    vkCmdSetViewport(cmdBuff, 0, 1, &viewport);
}

void ThinDrawer::debug_renderPortalBodies(VkCommandBuffer& cmdBuff)
{
    const VkDeviceSize offsets = 0;

    for (s_debug_renderGroup* rg : debug_renderGroups)
    {
        for (s_debug_renderBody* renderBody : rg->renderBodies)
        {
            b2Vec2 bodyPos = renderBody->portalBody->body->GetPosition();
            float bodyAngle = renderBody->portalBody->body->GetAngle();

            glm::mat4 modelMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(bodyPos.x, bodyPos.y, 0.0f));
            modelMatrix = glm::rotate(modelMatrix, bodyAngle, glm::vec3(0.0f, 0.0f, 1.0f));

            for (s_debug_renderStruct* renderStruct : renderBody->renderFixtures)
            {
                s_uboAll* allData;
                vkMapMemory(logicalDevice, ShaderBase::commonStorageBuffers[0]->memory,
                    renderStruct->dynamicOffsets[lastSwapChainImageIndex][0], ShaderBase::alignedSizes[0], 0, (void**)&allData);
                allData->modelMatrix = modelMatrix;

                allData->data.x = renderStruct->pbfData->pCollisions.size();

                bool fullOut = false;
                for (int i = 0; i < renderStruct->pbfData->pCollisions.size(); i++)
                {
                    portalCollision* pc = renderStruct->pbfData->pCollisions[i];
                    if (pc->status < 0)
                    {
                        fullOut = true;
                        allData->extraData.z = -2.0f;
                        break;
                    }
                    allData->portals[i] = glm::vec4(pc->portal->points[1 ^ pc->side].x, pc->portal->points[1 ^ pc->side].y,
                        pc->portal->points[pc->side].x, pc->portal->points[pc->side].y);
                }

                if (fullOut)
                {
                    if (!drawReleases)
                    {
                        vkUnmapMemory(logicalDevice, ShaderBase::commonStorageBuffers[0]->memory);
                        continue;
                    }
                }
                else
                {
                    allData->extraData.z = drawReleases ? -1.0f : 0.0f;
                }

                vkUnmapMemory(logicalDevice, ShaderBase::commonStorageBuffers[0]->memory);

                switch (renderStruct->renderType)
                {
                case b2Shape::e_circle:
                {
                    vkCmdBindPipeline(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS, debugCircleShader->pipeline);

                    vkCmdBindDescriptorSets(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS,
                        debugCircleShader->pipelineLayout, 0, 1, &ShaderBase::sharedSet, 1, &camOffset);

                    vkCmdBindDescriptorSets(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS,
                        debugCircleShader->pipelineLayout, 1, 1, &debugCircleShader->descriptorSets[1],
                        renderStruct->dynamicOffsets[lastSwapChainImageIndex].size(), renderStruct->dynamicOffsets[lastSwapChainImageIndex].data());

                    vkCmdBindVertexBuffers(cmdBuff, 0, 1, &renderStruct->vertexBuffer->buffer, &offsets);
                    vkCmdDraw(cmdBuff, renderStruct->vertexBuffer->count, 1, 0, 0);
                }
                break;

                case b2Shape::e_polygon:
                {
                    vkCmdBindPipeline(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS, debugPolygonShader->pipeline);

                    vkCmdBindVertexBuffers(cmdBuff, 0, 1, &renderStruct->vertexBuffer->buffer, &offsets);

                    vkCmdBindDescriptorSets(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS,
                        debugPolygonShader->pipelineLayout, 0, 1, &ShaderBase::sharedSet, 1, &camOffset);

                    vkCmdBindDescriptorSets(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS,
                        debugPolygonShader->pipelineLayout, 1, 1, &debugPolygonShader->descriptorSets[1],
                        renderStruct->dynamicOffsets[lastSwapChainImageIndex].size(), renderStruct->dynamicOffsets[lastSwapChainImageIndex].data());

                    vkCmdDraw(cmdBuff, renderStruct->vertexBuffer->count, 1, 0, 0);
                }
                break;
                default:
                    break;
                }
            }
        }
    }
}

void ThinDrawer::debug_renderPortals(VkCommandBuffer& cmdBuff)
{
    for (Portal* p : pWorld->portals)
    {
        addDebugLine(glm::vec4(p->points[0].x, p->points[0].y, p->points[1].x, p->points[1].y), glm::vec4(1.0, 1.0, 1.0, 1.0), 0.04f, 0.5f);
    }
}

void ThinDrawer::debug_renderEdges(VkCommandBuffer& cmdBuff)
{
    for (b2Body* body = pWorld->GetBodyList(); body; body = body->GetNext())
    {
        for (b2Fixture* fix = body->GetFixtureList(); fix; fix = fix->GetNext())
        {
            if (fix->GetType() == b2Shape::e_edge)
            {
                b2EdgeShape* edgeShape = (b2EdgeShape*)fix->GetShape();

                glm::vec4 colorToRender = glm::vec4(1.0f, 1.0f, 0.0f, 1.0f);
                fixtureData* fixData = (fixtureData*)fix->GetUserData().pointer;
                if (fixData)
                {
                    switch (fixData->type)
                    {
                    case F_EDGE:
                        colorToRender = glm::vec4(0.0f, 1.0f, 0.0f, 1.0f);
                        break;
                    case F_GEDGE:
                        colorToRender = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);
                        break;
                    case F_FEDGE:
                        colorToRender = glm::vec4(0.0f, 0.0f, 1.0f, 1.0f);
                        break;
                    default:
                        break;
                    }
                }

                glm::vec4 gelColor = colorToRender;

                b2Vec2 v1 = body->GetWorldPoint(edgeShape->m_vertex1);
                b2Vec2 v2 = body->GetWorldPoint(edgeShape->m_vertex2);

                addDebugLine(glm::vec4(v1.x, v1.y, v2.x, v2.y), gelColor, 0.03f, 0.5f);
            }
        }
    }

    if (debugLineRegistry.size() > 0)
    {
        s_debugLineUbo* lineUbo;
        vkMapMemory(logicalDevice, debugLineShader->uniformBuffers[0].memory, debugLineShader->numLines * sizeof(s_debugLineUbo),
            sizeof(s_debugLineUbo) * debugLineRegistry.size(), 0, (void**)&lineUbo);
        memcpy(lineUbo, debugLineRegistry.data(), sizeof(s_debugLineUbo) * debugLineRegistry.size());
        vkUnmapMemory(logicalDevice, debugLineShader->uniformBuffers[0].memory);
    }
    
    const VkDeviceSize offsets = 0;

    vkCmdBindPipeline(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS, debugLineShader->pipeline);
    vkCmdBindVertexBuffers(cmdBuff, 0, 1, &debugCircleShader->buffers[0].buffer, &offsets);

    vkCmdBindDescriptorSets(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS,
        debugLineShader->pipelineLayout, 0, 1, &ShaderBase::sharedSet, 1, &camOffset);

    vkCmdBindDescriptorSets(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS,
        debugLineShader->pipelineLayout, 1, 1, &debugLineShader->descriptorSets[1], 0, VK_NULL_HANDLE);

    vkCmdDraw(cmdBuff, debugCircleShader->buffers[0].count, debugLineShader->numLines + debugLineRegistry.size(), 0, 0);

    debugLineShader->numLines = 0;
}

void ThinDrawer::renderWalls(VkCommandBuffer& cmdBuff)
{
    const VkDeviceSize offsets = 0;
    
    glm::vec2 camPos = wh->cam->pos;

    blackWallShader->render(cmdBuff, 0, camPos.x * WallBase::layerMult, -camPos.y * WallBase::layerMult);
    whiteWallShader->render(cmdBuff, 0, camPos.x * WallBase::layerMult, -camPos.y * WallBase::layerMult);
    wallLight->render(cmdBuff, 0, camPos.x * WallBase::layerMult, -camPos.y * WallBase::layerMult);
    diagonalLiner->render(cmdBuff, 0, camPos.x * WallBase::layerMult, -camPos.y * WallBase::layerMult);

    /*gelShader->preCalculate();

    int gelType = 1;

    for (int i = 0; i < 600; i++)
    {
        glm::vec2 p = glm::vec2(gelShader->basePositions[i].posR.x + sin(gameTime + i) * 0.7f,
            gelShader->basePositions[i].posR.y + cos(gameTime + i) * 0.7f);
        gelType = i % 4;
        gelShader->putGel(gelType, p, gelShader->basePositions[i].posR.z);
    }

    for (int i = 600; i < 1024; i++)
    {
        glm::vec2 p = glm::vec2((i - 1000) * 0.3 + gameTime, sin(i * 0.3 + gameTime) * 2.0);
        gelType = i % 4;
        gelShader->putGel(gelType, p, gelShader->basePositions[i].posR.z);
    }*/

    vkCmdBindPipeline(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS, hazardShader->pipeline);

    vkCmdBindVertexBuffers(cmdBuff, 0, 1, &wallLight->buffers[0].buffer, &offsets);

    vkCmdBindDescriptorSets(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS,
        hazardShader->pipelineLayout, 0, 1, &ShaderBase::sharedSet, 1, &camOffset);

    vkCmdBindDescriptorSets(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS,
        hazardShader->pipelineLayout, 1, 1, &hazardShader->descriptorSets[1], 0, VK_NULL_HANDLE);

    vkCmdDraw(cmdBuff, wallLight->buffers[0].count, hazardShader->numHazards, 0, 0);


    vkCmdBindPipeline(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS, blueField->pipeline);

    vkCmdBindVertexBuffers(cmdBuff, 0, 1, &wallLight->buffers[0].buffer, &offsets);

    vkCmdBindDescriptorSets(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS,
        blueField->pipelineLayout, 0, 1, &ShaderBase::sharedSet, 1, &camOffset);

    vkCmdBindDescriptorSets(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS,
        blueField->pipelineLayout, 1, 1, &blueField->descriptorSets[1], 0, VK_NULL_HANDLE);

    vkCmdDraw(cmdBuff, wallLight->buffers[0].count, blueField->numFields, 0, 0);


    vkCmdBindPipeline(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS, lineZ->pipeline);

    vkCmdBindVertexBuffers(cmdBuff, 0, 1, &wallLight->buffers[0].buffer, &offsets);

    vkCmdBindDescriptorSets(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS,
        lineZ->pipelineLayout, 0, 1, &ShaderBase::sharedSet, 1, &camOffset);

    vkCmdBindDescriptorSets(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS,
        lineZ->pipelineLayout, 1, 1, &lineZ->descriptorSets[1], 0, VK_NULL_HANDLE);

    vkCmdDraw(cmdBuff, wallLight->buffers[0].count, lineZ->numZs, 0, 0);

    for (auto& f : midDrawFuncs)
    {
        f.first(cmdBuff, f.second);
    }

    blackWallShader->render(cmdBuff, 1, camPos.x, -camPos.y);
    whiteWallShader->render(cmdBuff, 1, camPos.x, -camPos.y);
    wallLight->render(cmdBuff, 1, camPos.x, -camPos.y);
    diagonalLiner->render(cmdBuff, 1, camPos.x, -camPos.y);

    polynomShader->render(cmdBuff, wh->cam);

    vkCmdBindPipeline(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS, wallGel->pipeline);

    vkCmdBindDescriptorSets(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS,
        wallGel->pipelineLayout, 0, 1, &ShaderBase::sharedSet, 1, &camOffset);

    vkCmdBindDescriptorSets(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS,
        wallGel->pipelineLayout, 1, 1, &wallGel->descriptorSets[1], 0, nullptr);

    vkCmdDraw(cmdBuff, 6, wallGel->numWallLines, 0, 0);
}

void ThinDrawer::renderPortalBodies(VkCommandBuffer& cmdBuff)
{
    const VkDeviceSize offsets = 0;

    vkCmdBindPipeline(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS, bodyShader->pipeline);

    vkCmdBindDescriptorSets(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS,
        bodyShader->pipelineLayout, 0, 1, &ShaderBase::sharedSet, 1, &camOffset);

    for (s_renderGroup* renderGroup : renderGroups)
    {
        double animVal = upUpAnim * renderGroup->imageSets.size();
        vkCmdBindDescriptorSets(frames[lastSwapChainImageIndex].commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
            bodyShader->pipelineLayout, 1, 1, renderGroup->imageSets[renderGroup->currentImgIndex % renderGroup->imageSets.size()], 0, VK_NULL_HANDLE);
        for (s_renderBody* renderBody : renderGroup->renderBodies)
        {
            int i = 0;
            s_renderBodyPsh pshConst;
            pshConst.posRot = glm::vec4(renderBody->portalBody->body->GetPosition().x, renderBody->portalBody->body->GetPosition().y,
                renderBody->portalBody->body->GetAngle(), renderBody->portalBody->isMirrored * -1);
            pshConst.movScale = glm::vec4(0.0f, 0.0f, 1.0f, 1.0f);
            pshConst.data.y = gameTime - adjustableValues.x;
            pshConst.data.y = renderBody->portalBody->worldIndex->destrTime;

            for (PBFixtureData* pFixData : renderBody->renderFixtures)
            {
                bool renderDisabled = false;
                if (pFixData->pCollisions.size() > 0)
                {
                    for (int i = 0; i < pFixData->pCollisions.size(); i++)
                    {
                        portalCollision* pc = pFixData->pCollisions[i];
                        if (pc->status < 0)
                        {
                            renderDisabled = true; i++;
                            break;
                        }

                        pshConst.portals[i] = glm::vec4(
                            pc->portal->points[pc->side ^ 1].x, pc->portal->points[pc->side ^ 1].y,
                            pc->portal->points[pc->side].x, pc->portal->points[pc->side].y);
                    }
                    pshConst.data.x = pFixData->pCollisions.size();
                }
                else
                {
                    pshConst.data.x = 0;
                }

                if (!renderDisabled)
                {
                    vkCmdBindVertexBuffers(frames[lastSwapChainImageIndex].commandBuffer, 0, 1, &renderGroup->vertexBuffers[i]->buffer, &offsets);

                    vkCmdPushConstants(frames[lastSwapChainImageIndex].commandBuffer, bodyShader->pipelineLayout,
                        						VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(s_renderBodyPsh), &pshConst);

                    vkCmdDraw(frames[lastSwapChainImageIndex].commandBuffer, renderGroup->vertexBuffers[i]->count, 1, 0, 0);
                }
                
                i++;
            }
        }
    }
}

void ThinDrawer::renderPortalBodies3D(VkCommandBuffer& cmdBuff)
{
    const VkDeviceSize offsets = 0;

    for (s_renderGroup3D* renderGroup : renderGroups3D)
    {
        for (s_renderBody3D* renderBody : renderGroup->renderBodies)
        {
            PortalBody* pb = renderBody->portalBody;
            s_bodyUbo* bodyUbo;
            
            CHECK_RESULT_VK(vkMapMemory(logicalDevice, bodyShader3D->uniformBuffers[0].memory,
                renderBody->dynamicOffsets[lastSwapChainImageIndex], bodyShader3D->alignedSize_body, 0, (void**)&bodyUbo));

            bodyUbo->modelMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(pb->body->GetPosition().x, pb->body->GetPosition().y, 0.0f));
            bodyUbo->rotationMatrix = glm::rotate(glm::mat4(1.0f), pb->body->GetAngle(), glm::vec3(0.0f, 0.0f, 1.0));
            bodyUbo->modelMatrix = bodyUbo->modelMatrix * bodyUbo->rotationMatrix * renderGroup->scaleMatrix;
            vkUnmapMemory(logicalDevice, bodyShader3D->uniformBuffers[0].memory);

            for (s_renderStruct3D* renderStruct : renderBody->renderFixtures)
            {
                s_fixUbo* fixUbo;
                CHECK_RESULT_VK(vkMapMemory(logicalDevice, bodyShader3D->uniformBuffers[1].memory,
                    renderStruct->dynamicOffsets[lastSwapChainImageIndex][1], bodyShader3D->alignedSize_fixture, 0, (void**)&fixUbo));

                int i = 0;
                bool fullOut = false;
                for (portalCollision* pc : renderStruct->pbfData->pCollisions)
                {
                    if (pc->status < 0)
                    {
                        fullOut = true;
                        break;
                    }
                    fixUbo->portals[i] = glm::vec4( pc->portal->points[pc->side ^ 1].x, pc->portal->points[pc->side ^ 1].y,
                                                    pc->portal->points[pc->side].x, pc->portal->points[pc->side].y);
                }

                if (fullOut)
                {
                    vkUnmapMemory(logicalDevice, bodyShader3D->uniformBuffers[1].memory);
                    continue;
                }

                fixUbo->data.x = (float)renderStruct->pbfData->pCollisions.size();

                vkUnmapMemory(logicalDevice, bodyShader3D->uniformBuffers[1].memory);

                vkCmdBindDescriptorSets(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS,
                    bodyShader3D->pipelineLayout, 1, 1, &bodyShader3D->descriptorSets[1],
                    std::size(renderStruct->dynamicOffsets[lastSwapChainImageIndex]), renderStruct->dynamicOffsets[lastSwapChainImageIndex].data());


                vkCmdBindDescriptorSets(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS,
                    bodyShader3D->pipelineLayout, 2, 1, &renderStruct->imageDescriptorSet, 0, VK_NULL_HANDLE);


                VkIndexType indexType = VK_INDEX_TYPE_UINT16;
                vkCmdBindVertexBuffers(cmdBuff, 0, 1, &renderStruct->vertexBuffer->buffer, &offsets);
                vkCmdBindIndexBuffer(cmdBuff, renderStruct->indexBuffer->buffer, 0, indexType);
                vkCmdDrawIndexed(cmdBuff, renderStruct->indexBuffer->count, 1, 0, 0, 0);
            }
        }
    }
}

void ThinDrawer::renderPortals3D(VkCommandBuffer& cmdBuff)
{
#if 0
    const VkDeviceSize offsets = 0;

    vkCmdBindPipeline(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS, portalShader->pipeline);

    vkCmdBindVertexBuffers(cmdBuff, 0, 1, &portalShader->buffers[0].buffer, &offsets);

    vkCmdBindDescriptorSets(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS,
        portalShader->pipelineLayout, 2, 1, &portalShader->textureData[0]->set, 0, VK_NULL_HANDLE);

    for (auto pair : portalsToRender3D)
    {
        s_portalUbo* pUbo;
        vkMapMemory(logicalDevice, portalShader->uniformBuffers[0].memory, pair.second.dynamicOffsets[lastSwapChainImageIndex], sizeof(s_portalUbo), 0, (void**)&pUbo);

        pUbo->data0 = glm::vec4(0.5, pair.second.portalSizeY, 0.5 / pair.second.portalSizeY, 0.1);
        pUbo->data1.x = pair.second.portalSizeUp;

        pUbo->data1.y = glm::length(PortalShader3D::getEllipsePosOnAngle(pUbo->data0.x, pUbo->data0.y, 0.0) -
            PortalShader3D::getEllipsePosOnAngle(pUbo->data0.x, pUbo->data0.y, 0.00001));

        pUbo->data1.z = glm::length(PortalShader3D::getEllipsePosOnAngle(pUbo->data0.x, pUbo->data0.y, b2_pi / 2.0) -
            PortalShader3D::getEllipsePosOnAngle(pUbo->data0.x, pUbo->data0.y, b2_pi / 2.0 + 0.00001));

        pUbo->data1.w = pair.second.noiseScale;

        pUbo->data2.z = SDL_GetTicksSec();

        pUbo->data3.z = pair.second.noiseMult;

        vkUnmapMemory(logicalDevice, portalShader->uniformBuffers[0].memory);

        vkCmdBindDescriptorSets(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS,
            portalShader->pipelineLayout, 1, 1, &portalShader->descriptorSets[1], 1, &pair.second.dynamicOffsets[lastSwapChainImageIndex]);

        vkCmdDraw(cmdBuff, portalShader->buffers[0].count, 1, 0, 0);
    }
#endif
}

void ThinDrawer::animate()
{
    upUpAnim = fmod(gameTime, 1.0);
    upDownAnim = fmod(gameTime, 2.0);
    if (upDownAnim > 1.0) upDownAnim = 2.0 - upDownAnim;

    double pDeltaTime = Game::object->deltaTime;

    for (AnimGroup* animGroup = animations; ;)
    {
        if (!animGroup)
        {
            break;
        }

        for (AnimStruct* animStruct = animGroup->animStructs; ;)
        {
            if (!animStruct)
            {
                break;
            }
            
            animStruct->xProg += pDeltaTime * animStruct->scale;

            if (animStruct->xProg >= 1.0f)
            {
                animStruct->registeredFunction(1.0, animStruct->magnitude, animStruct->from, animStruct->value);
                AnimStruct* curAnim = animStruct;

                animStruct = animGroup->removeStruct(curAnim);

                continue;
            }

            animStruct->registeredFunction(animStruct->xProg, animStruct->magnitude, animStruct->from, animStruct->value);

            animStruct = animStruct->next;
        }

        if (animGroup->animStructs == nullptr)
        {
            AnimGroup* nextAnimGroup = animGroup->next;
            if (animGroup->prev == nullptr)
            {
                animations = nextAnimGroup;
            }
            delete animGroup;
            animGroup = nextAnimGroup;
        }
        else
        {
            animGroup = animGroup->next;
        }
    }
}

void ThinDrawer::afterCreate()
{

}

void ThinDrawer::beforeCreate()
{
    pWorld->portalCreateCallbacks.push_back(portalCreateCallback);
    pWorld->portalDestroyCallbacks.push_back(portalDestroyCallback);
}

void ThinDrawer::renderPortals(VkCommandBuffer& cmdBuff)
{
    if (renderPortalData.size() == 0) return;

    VkDeviceSize offsets = 0;

    for (auto& pair : renderPortalData)
    {
        s_portalRender* pr = &pair.second;

        Portal* portal = pr->portal;

        pr->portalPsh.posSize.x = portal->pos.x; pr->portalPsh.posSize.y = portal->pos.y;
        pr->portalPsh.posSize.z = pr->scale.x;
        pr->portalPsh.posSize.w = pr->scale.y + (sin(gameTime * 3.0) + 1.0) * 0.03;
        pr->portalPsh.data0.z = pr->portalPsh.posSize.w * 0.4;
        pr->portalPsh.data0.x = portal->dirAngle;
        
        if (portal->connections[0].size() > 0)
        {
            b2Vec2 p0 = portal->pos + fmax(pr->scale.x * 0.5 - 2.0, 0.01) * portal->rotDir;
            b2Vec2 p1 = portal->pos - fmax(pr->scale.x * 0.5 - 2.0, 0.01) * portal->rotDir;
                     
            pr->particlePsh.portalPoints = glm::vec4(p0.x, p0.y, p1.x, p1.y);
            pr->particlePsh.data0 = glm::vec4(0.6f, 1.0f, pr->particlePsh.data0.z, 3.0f);
            pr->particlePsh.data1 = glm::vec4(0.2f, 0.2f, pr->particlePsh.data1.z, 100.0f);
        }
    }

    vkCmdBindPipeline(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS, portalParticleShader->pipeline);

    vkCmdBindVertexBuffers(cmdBuff, 0, 1, &portalParticleShader->buffers[0].buffer, &offsets);

    vkCmdBindDescriptorSets(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS,
        portalParticleShader->pipelineLayout, 0, 1, &ShaderBase::sharedSet, 1, &camOffset);

    for (auto& pair : renderPortalData)
    {
        if (pair.second.portal->connections[0].size() > 0)
        {
            vkCmdPushConstants(cmdBuff, portalParticleShader->pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                				0, sizeof(s_portalParticlePsh), &pair.second.particlePsh);

            vkCmdDraw(cmdBuff, portalParticleShader->buffers[0].count, 100, 0, 0);
        }
    }

    vkCmdBindPipeline(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS, portalShader->pipeline);

    vkCmdBindVertexBuffers(cmdBuff, 0, 1, &portalShader->buffers[0].buffer, &offsets);

    vkCmdBindDescriptorSets(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS,
        portalShader->pipelineLayout, 0, 1, &ShaderBase::sharedSet, 1, &camOffset);

    for (auto& pair : renderPortalData)
    {
        vkCmdPushConstants(cmdBuff, portalShader->pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
            							0, sizeof(s_portalPsh), &pair.second.portalPsh);
        vkCmdDraw(cmdBuff, portalShader->buffers[0].count, 1, 0, 0);
    }
}

void ThinDrawer::portalCreateCallback(Portal* portal)
{
    ThinDrawer* thinDrawer = ThinDrawer::object;

    s_portalRender* pr = &thinDrawer->renderPortalData[portal];
    pr->portal = portal;
    pr->portalPsh.data0 = glm::vec4(portal->dirAngle, 0.0001f, 0.06f, 0.0);
    pr->scale = glm::vec2(0.0f, 3.0f);

    AnimGroup* ag = new AnimGroup(3);

    pr->animGroup = ag;
    ag->toNull = &pr->animGroup;

    AnimStruct* animStruct = ag->createAnimation(&pr->scale.x, 4.0f, 0.0f, portal->size * 2.0f + 4.0);
    animStruct->registeredFunction = AnimStruct::linearFunc;

    AnimStruct* animStruct3 = ag->createChainedAnimation(animStruct, &pr->scale.y, 4.0f, 3.0f, 4.0f);
    animStruct3->registeredFunction = AnimStruct::linearFunc;

    animStruct = ag->createChainedAnimation(animStruct, &pr->portalPsh.data0.w, 4.0f, 0.0f, portal->size * 1.5);
    animStruct->registeredFunction = AnimStruct::linearFunc;

    ag->next = thinDrawer->animations;
    if (thinDrawer->animations) thinDrawer->animations->prev = ag;
    thinDrawer->animations = ag;

    pr->portalPsh.color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);

    pr->particlePsh.color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
    pr->particlePsh.portalPoints = glm::vec4(portal->points[0].x, portal->points[0].y, portal->points[1].x, portal->points[1].y);
    pr->particlePsh.data0 = glm::vec4(0.6f, 1.0f, thinDrawer->gameTime, 3.0f);
    pr->particlePsh.data1.z = thinDrawer->gameTime;
}

void ThinDrawer::portalDestroyCallback(Portal* portal)
{
    ThinDrawer* thinDrawer = ThinDrawer::object;

    auto iter = thinDrawer->renderPortalData.find(portal);
    s_portalRender* pr = &(*iter).second;

    AnimGroup* animGroup = pr->animGroup;
    if (animGroup)
    {
        if (!animGroup->prev)
        {
            thinDrawer->animations = animGroup->next;
        }
        delete animGroup;
    }

    thinDrawer->renderPortalData.erase(iter);
}

void ThinDrawer::portalCreateCallback_3d(Portal* portal)
{
    ThinDrawer* thinDrawer = ThinDrawer::object;

    s_portalRender3D* pr = &thinDrawer->portalsToRender3D[portal];

    pr->noiseScale = 5.0f;
    pr->noiseMult = 0.0f;
    pr->portalSizeUp = 1.0f;
    pr->portalSizeY = portal->size - 0.02;

    pr->dynamicOffsets.resize(thinDrawer->swapChain->imageCount);

    glm::mat4 modelMatrix = glm::translate(glm::mat4(1.0), glm::vec3(portal->pos.x, portal->pos.y, 0.0));
    modelMatrix = glm::rotate(modelMatrix, portal->dirAngle, glm::vec3(0.0, 0.0, 1.0));

    for (int i = 0; i < thinDrawer->swapChain->imageCount; i++)
    {
        uint32_t dynOffset = thinDrawer->portalShader3D->availableOffsets[thinDrawer->portalShader3D->lastAllocationPos++];

        pr->dynamicOffsets[i] = dynOffset;
        s_portalUbo3D* pUbo;
        vkMapMemory(thinDrawer->logicalDevice, thinDrawer->portalShader3D->uniformBuffers[0].memory, dynOffset, thinDrawer->portalShader3D->alignedSize, 0, (void**)&pUbo);

        pUbo->modelMatrix = modelMatrix;

        pUbo->data0 = glm::vec4(0.5, pr->portalSizeY, 0.5 / pr->portalSizeY, 0.1);

        pUbo->data1 = glm::vec4(pr->portalSizeUp * 0.8, 0.0, 0.0, pr->noiseScale);
        pUbo->data1.y = glm::length(PortalShader3D::getEllipsePosOnAngle(pUbo->data0.x, pUbo->data0.y, 0.0) -
            PortalShader3D::getEllipsePosOnAngle(pUbo->data0.x, pUbo->data0.y, 0.00001));

        pUbo->data1.z = glm::length(PortalShader3D::getEllipsePosOnAngle(pUbo->data0.x, pUbo->data0.y, b2_pi / 2.0) -
            PortalShader3D::getEllipsePosOnAngle(pUbo->data0.x, pUbo->data0.y, b2_pi / 2.0 + 0.00001));

        pUbo->data2 = glm::vec4(2.5, portal->size * 2.0 + 0.04, 0.0, 0.0);

        pUbo->data3 = glm::vec4(0.007415251, 0.0054131, pr->noiseMult, 0.0);

        pUbo->portalColour = glm::vec4(0.0, 0.2, 1.0, 0.0);
        pUbo->edgeColour = glm::vec4(0.0, 0.5, 1.0, 0.0);

        vkUnmapMemory(thinDrawer->logicalDevice, thinDrawer->portalShader3D->uniformBuffers[0].memory);
    }
}

void ThinDrawer::portalDestroyCallback_3d(Portal* portal)
{
    ThinDrawer* thinDrawer = ThinDrawer::object;

    auto iter = thinDrawer->portalsToRender3D.find(portal);

    if (iter == thinDrawer->portalsToRender3D.end())
    {
        return;
    }

    AnimGroup* animGroup = (*iter).second.animGroup;
    if (animGroup)
    {
        if (!animGroup->prev)
        {
            thinDrawer->animations = animGroup->next;
        }
        delete animGroup;
    }

    for (uint32_t offset : (*iter).second.dynamicOffsets)
    {
        thinDrawer->portalShader3D->availableOffsets[--thinDrawer->portalShader3D->lastAllocationPos] = offset;
    }

    thinDrawer->portalsToRender3D.erase(portal);
}

void ThinDrawer::addDebugLine(const glm::vec4 positions, const glm::vec4 color, const float thickness, const float smoothness)
{
    s_debugLineUbo* lineUbo;
    vkMapMemory(logicalDevice, debugLineShader->uniformBuffers[0].memory, debugLineShader->numLines * sizeof(s_debugLineUbo), sizeof(s_debugLineUbo), 0, (void**)&lineUbo);
    lineUbo->colour = color;
    lineUbo->positions = positions;
    lineUbo->data.x = 10000.0f;
    lineUbo->data.y = thickness; lineUbo->data.z = smoothness;
    vkUnmapMemory(logicalDevice, debugLineShader->uniformBuffers[0].memory);

    debugLineShader->numLines++;
}

void ThinDrawer::registerDebugLine(const glm::vec4 positions, const glm::vec4 color, const float thickness, const float smoothness)
{
    debugLineRegistry.push_back({positions, color, { 0.0, thickness, smoothness, 0.0 }});
}

void ThinDrawer::addPortalBody_3d(PortalBody* portalBody, std::vector<std::pair<std::string, std::string>> meshNames)
{
#if 1

#else
    s_renderGroup* renderGroup = new s_renderGroup;
    s_renderBody* renderBody = new s_renderBody;
    renderGroup->renderBodies.insert(renderBody);
    renderBody->portalBody = portalBody;

    renderBody->dynamicOffsets.resize(swapChain->imageCount);
    for (int i = 0; i < swapChain->imageCount; i++)
    {
        renderBody->dynamicOffsets[i] = bodyShader3D->availableOffsets_body[bodyShader3D->lastAllocationPos_body++];
    }

    renderGroups3D.push_back(renderGroup);

    s_meshData* meshData;

    int i;
    for (i = 0; i < portalBody->fixtures.size(); i++)
    {
        fixtureData* fixData = (fixtureData*)portalBody->fixtures[i]->GetUserData().pointer;

        s_renderStruct* renderStruct = new s_renderStruct;
        renderBody->renderFixtures.push_back(renderStruct);
        
        renderStruct->dynamicOffsets.resize(swapChain->imageCount);
        for (int j = 0; j < swapChain->imageCount; j++)
        {
            renderStruct->dynamicOffsets[j][0] = renderBody->dynamicOffsets[j];
            renderStruct->dynamicOffsets[j][1] = bodyShader3D->availableOffsets_fixture[bodyShader3D->lastAllocationPos_fixture++];
        }

        int meshIndex = std::min(i, (int)meshNames.size() - 1);

        if (meshNames.size() == 0) meshData = IOHelper::assetMap["default.default"];
        else meshData = IOHelper::assetMap[meshNames[meshIndex].first];
        if (!meshData) meshData = IOHelper::assetMap["default.default"];

        if (meshNames.size() == 0 || IOHelper::textureMap.find(meshNames[meshIndex].second) == IOHelper::textureMap.end())
        {
            renderStruct->imageDescriptorSet = IOHelper::textureMap["defaultTex.png"]->set;
        }
        else
        {
            renderStruct->imageDescriptorSet = IOHelper::textureMap[meshNames[meshIndex].second]->set;
        }

        renderGroup->meshDatas.push_back(meshData);

        renderStruct->vertexBuffer = meshData->vertexBuffers[0];
        renderStruct->indexBuffer = meshData->indexBuffers[0];
        renderStruct->pbfData = (PBFixtureData*)fixData->data;
    }

    if (meshNames.size() == 0) meshData = IOHelper::assetMap["default.default"];
    else meshData = IOHelper::assetMap[meshNames[0].first];
    if (!meshData) meshData = IOHelper::assetMap["default.default"];

    b2Fixture* firstFix = portalBody->fixtures[0];
    glm::vec2 xLimits;
    glm::vec2 yLimits;

    if (firstFix->GetType() == b2Shape::e_polygon)
    {
        b2PolygonShape* polyShape = (b2PolygonShape*)firstFix->GetShape();

        xLimits = glm::vec2(polyShape->m_vertices[0].x, polyShape->m_vertices[0].x);
        yLimits = glm::vec2(polyShape->m_vertices[0].y, polyShape->m_vertices[0].y);

        for (int i = 1; i < polyShape->m_count; i++)
        {
            b2Vec2 vertex = polyShape->m_vertices[i];
            if (vertex.x < xLimits[0]) xLimits[0] = vertex.x;
            else if (vertex.x > xLimits[1]) xLimits[1] = vertex.x;

            if (vertex.y < yLimits[0]) yLimits[0] = vertex.y;
            else if (vertex.y > yLimits[1]) yLimits[1] = vertex.y;
        }
    }
    else if (firstFix->GetType() == b2Shape::e_circle)
    {
        b2CircleShape* circleShape = (b2CircleShape*)firstFix->GetShape();

        b2Vec2 circlePos = circleShape->m_p;

        xLimits = glm::vec2(circlePos.x - circleShape->m_radius, circlePos.x + circleShape->m_radius);
        yLimits = glm::vec2(circlePos.y - circleShape->m_radius, circlePos.y + circleShape->m_radius);
    }

    float xScale = (xLimits[1] - xLimits[0]) / (meshData->minMaxValues[1][0] - meshData->minMaxValues[0][0]);
    float yScale = (yLimits[1] - yLimits[0]) / (meshData->minMaxValues[1][1] - meshData->minMaxValues[0][1]);

    renderGroup->scaleMatrix = glm::scale(glm::mat4(1.0f), glm::vec3(xScale, yScale, xScale));

    portalBody->worldIndex->createCallbacks.push_back(std::make_pair(portalBodyCreateCallback, (void*)renderGroup));
    portalBody->worldIndex->destroyCallbacks.push_back(std::make_pair(portalBodyDestroyCallback, (void*)renderGroup));
#endif
}

void ThinDrawer::portalBodyCreateCallback_3d(PortalBody* portalBody, void* data)
{
    ThinDrawer* thinDrawer = ThinDrawer::object;
    s_renderGroup3D* renderGroup = (s_renderGroup3D*)data;

    s_renderBody3D* firstRenderBody = *renderGroup->renderBodies.begin();

    s_renderBody3D* renderBody = new s_renderBody3D;
    renderGroup->renderBodies.insert(renderBody);

    renderBody->dynamicOffsets.resize(thinDrawer->swapChain->imageCount);
    for (int i = 0; i < thinDrawer->swapChain->imageCount; i++)
    {
        renderBody->dynamicOffsets[i] = thinDrawer->bodyShader3D->availableOffsets_body[thinDrawer->bodyShader3D->lastAllocationPos_body++];
    }
    renderBody->portalBody = portalBody;
    renderBody->renderFixtures.resize(portalBody->numFixtures);

    int bufferIndex = portalBody->isMirrored ? 1 : 0;

    for (int i = 0; i < portalBody->numFixtures; i++)
    {
        fixtureData* fixData = (fixtureData*)portalBody->fixtures[i]->GetUserData().pointer;

        s_renderStruct3D* renderStruct = new s_renderStruct3D;
        renderBody->renderFixtures[i] = renderStruct;

        renderStruct->dynamicOffsets.resize(thinDrawer->swapChain->imageCount);
        for (int j = 0; j < thinDrawer->swapChain->imageCount; j++)
        {
            renderStruct->dynamicOffsets[j][0] = renderBody->dynamicOffsets[j];
            renderStruct->dynamicOffsets[j][1] = thinDrawer->bodyShader3D->availableOffsets_fixture[thinDrawer->bodyShader3D->lastAllocationPos_fixture++];
        }

        renderStruct->imageDescriptorSet = firstRenderBody->renderFixtures[i]->imageDescriptorSet;

        renderStruct->pbfData = (PBFixtureData*)fixData->data;

        renderStruct->vertexBuffer = renderGroup->meshDatas[i]->vertexBuffers[bufferIndex];
        renderStruct->indexBuffer = renderGroup->meshDatas[i]->indexBuffers[bufferIndex];
    }
}

void ThinDrawer::portalBodyDestroyCallback_3d(PortalBody* portalBody, void* data)
{
    ThinDrawer* thinDrawer = ThinDrawer::object;
    s_renderGroup3D* renderGroup = (s_renderGroup3D*)data;
    
    for (auto iter = renderGroup->renderBodies.begin(); iter != renderGroup->renderBodies.end(); iter++)
    {
        s_renderBody3D* renderBody = *iter;
        if (renderBody->portalBody == portalBody)
        {
            for (int i = 0; i < thinDrawer->swapChain->imageCount; i++)
            {
                thinDrawer->bodyShader3D->lastAllocationPos_body--;
                thinDrawer->bodyShader3D->availableOffsets_body[thinDrawer->bodyShader3D->lastAllocationPos_body] = renderBody->dynamicOffsets[i];
            }

            for (s_renderStruct3D* rs : renderBody->renderFixtures)
            {
                for (int i = 0; i < thinDrawer->swapChain->imageCount; i++)
                {
                    thinDrawer->bodyShader3D->lastAllocationPos_fixture--;
                    thinDrawer->bodyShader3D->availableOffsets_fixture[thinDrawer->bodyShader3D->lastAllocationPos_fixture] = rs->dynamicOffsets[i][1];
                }
                
                delete rs;
            }

            renderGroup->renderBodies.erase(iter);
            delete renderBody;
            break;
        }
    }
}

s_renderGroup* ThinDrawer::addPortalBody(PortalBody* portalBody, std::vector<std::string> textures)
{
    s_renderGroup* renderGroup = new s_renderGroup;
    renderGroups.push_back(renderGroup);

    for (int i = 0; i < textures.size(); i++)
    {
        if (IOHelper::textureMap.find(textures[i]) == IOHelper::textureMap.end())
        {
            try
            {
                s_texture* tex = IOHelper::loadTexture(textures[i], VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER);
                renderGroup->imageSets.push_back(&tex->set);
            }
            catch (const std::exception& ex)
            {
                printf("failed to load texture %s. Using default texture\n", textures[i].data());
                renderGroup->imageSets.push_back(&(IOHelper::textureMap["null.png"]->set));
            }
        }
        else
        {
            renderGroup->imageSets.push_back(&(IOHelper::textureMap[textures[i]]->set));
        }
    }

    if (textures.size() == 0)
    {
        renderGroup->imageSets.push_back(&(IOHelper::textureMap["null.png"]->set));
    }

    float xm = portalBody->isMirrored ? -1.0f : 1.0f;

    s_renderBody* renderBody = new s_renderBody;
    
    int numFixtures = portalBody->fixtures.size();

    renderGroup->renderBodies.insert(renderBody);
    renderGroup->vertexBuffers.resize(numFixtures);
    renderBody->renderFixtures.resize(numFixtures);

    renderBody->portalBody = portalBody;
    
    glm::vec2 minPoint = glm::vec2(+7654443.3252f, +34566345.76543345f);
    glm::vec2 maxPoint = glm::vec2(-3743456.8765f, -45356465.23456764f);
    glm::vec2 size;

    for (int i = 0; i < numFixtures; i++)
    {
        b2Fixture* fix = portalBody->fixtures.at(i);

        if (fix->GetType() == b2Shape::e_circle)
        {
            b2CircleShape* circShape = (b2CircleShape*)fix->GetShape();
            b2Vec2 centerPos = circShape->m_p;
            centerPos.x *= xm;
            float r = circShape->m_radius;

            if (centerPos.x - r < minPoint.x)
            {
                minPoint.x = centerPos.x - r;
            }
            if (centerPos.x + r > maxPoint.x)
            {
                maxPoint.x = centerPos.x + r;
            }
            if (centerPos.y - r < minPoint.y)
            {
                minPoint.y = centerPos.y - r;
            }
            if (centerPos.y + r > maxPoint.y)
            {
                maxPoint.y = centerPos.y + r;
            }
        }
        else if (fix->GetType() == b2Shape::e_polygon)
        {
            b2PolygonShape* polyShape = (b2PolygonShape*)fix->GetShape();

            for (int j = 0; j < polyShape->m_count; j++)
            {
                b2Vec2 vertex = polyShape->m_vertices[j];
                vertex.x *= xm;
                if (vertex.x < minPoint.x)
                {
                    minPoint.x = vertex.x;
                }
                if (vertex.x > maxPoint.x)
                {
                    maxPoint.x = vertex.x;
                }
                if (vertex.y < minPoint.y)
                {
                    minPoint.y = vertex.y;
                }
                if (vertex.y > maxPoint.y)
                {
                    maxPoint.y = vertex.y;
                }
            }
        }
    }

    size = maxPoint - minPoint;

    for (int i = 0; i < numFixtures; i++)
    {
        b2Fixture* fix = portalBody->fixtures.at(i);

        fixtureData* fData = (fixtureData*)fix->GetUserData().pointer;
        PBFixtureData* pbfData = (PBFixtureData*)fData->data;
        renderBody->renderFixtures[i] = pbfData;

        std::vector<s_vertex> initialVertices;
        std::vector<s_vertex> finalVertices;
        if (fix->GetType() == b2Shape::e_circle)
        {
            const int vertexCount = 32;
            initialVertices.resize(vertexCount);
            finalVertices.resize((vertexCount - 2) * 3);

            b2CircleShape* circShape = (b2CircleShape*)fix->GetShape();
            b2Vec2 centerPos = circShape->m_p;
            float r = circShape->m_radius * 1.01;

            for (int j = 0; j < vertexCount; j++)
            {
				float angle = j * 2.0f * b2_pi / vertexCount;
				initialVertices[j].pos = glm::vec2(centerPos.x * xm + r * cos(angle), centerPos.y + r * sin(angle));
			}
        }
        else if (fix->GetType() == b2Shape::e_polygon)
        {
            b2PolygonShape* polyShape = (b2PolygonShape*)fix->GetShape();
            initialVertices.resize(polyShape->m_count);
            finalVertices.resize((polyShape->m_count - 2) * 3);

            for (int j = 0; j < polyShape->m_count; j++)
            {
                b2Vec2 vertex = polyShape->m_vertices[j];
                initialVertices[j].pos = glm::vec2(vertex.x * xm, vertex.y);
            }
        }

        for (int j = 0; j < initialVertices.size(); j++)
        {
            glm::vec2 p = initialVertices[j].pos;
            glm::vec2 minDiff = p - minPoint;
            glm::vec2 ratio = minDiff / size;
            ratio.y = 1.0f - ratio.y;
            initialVertices[j].uv = ratio;
        }

        finalVertices[0] = initialVertices[0];
        finalVertices[1] = initialVertices[1];
        finalVertices[2] = initialVertices[2];

        int k = 3;
        for (int j = 2; j < initialVertices.size() - 1; j++)
        {
            finalVertices[k + 0] = initialVertices[j];
            finalVertices[k + 1] = initialVertices[j + 1];
            finalVertices[k + 2] = initialVertices[0];

            k += 3;
        }

        s_buffers* vertexBuffer = (s_buffers*)malloc(sizeof(s_buffers));
        vertexBuffer->count = finalVertices.size();
        bufferStage(finalVertices.data(), finalVertices.size() * sizeof(s_vertex), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, vertexBuffer);
        renderGroup->vertexBuffers[i] = vertexBuffer;
    }

    portalBody->worldIndex->createCallbacks.push_back(std::make_pair(portalBodyCreateCallback, (void*)renderGroup));
    portalBody->worldIndex->destroyCallbacks.push_back(std::make_pair(portalBodyDestroyCallback, (void*)renderGroup));

    return renderGroup;
}

void ThinDrawer::debug_addPortalBody(PortalBody* portalBody)
{
    s_debug_renderGroup* renderGroup = new s_debug_renderGroup;
    s_debug_renderBody* renderBody = new s_debug_renderBody;
    renderBody->portalBody = portalBody;

    int numFixtures = portalBody->fixtures.size();

    renderGroup->vertexStorageOffsets.resize(numFixtures);
    renderGroup->vertexStorageOffsetsMirrored.resize(numFixtures);
    renderGroup->circlePositionings.resize(numFixtures);
    renderGroup->circlePositioningsMirrored.resize(numFixtures);
    renderGroup->vertexBuffers.resize(numFixtures);
    renderGroup->vertexBuffersMirrored.resize(numFixtures);

    for (int i = 0; i < numFixtures; i++)
    {
        b2Fixture* fix = portalBody->fixtures.at(i);

        s_debug_renderStruct* renderStruct = new s_debug_renderStruct;
        renderBody->renderFixtures.push_back(renderStruct);
        renderStruct->renderType = fix->GetType();
        fixtureData* fData = (fixtureData*)fix->GetUserData().pointer;
        PBFixtureData* pbfData = (PBFixtureData*)fData->data;
        renderStruct->pbfData = pbfData;
        renderStruct->dynamicOffsets.resize(swapChain->imageCount);
        renderStruct->vertexBuffer = (s_buffers*)malloc(sizeof(s_buffers));
        s_buffers* mirroredVertices = (s_buffers*)malloc(sizeof(s_buffers));

        renderGroup->vertexBuffers[i] = renderStruct->vertexBuffer;
        renderGroup->vertexBuffersMirrored[i] = mirroredVertices;

        std::vector<b2Vec2> vertexBuffer;
        std::vector<b2Vec2> vertexBufferMirrored;

        if (fix->GetType() == b2Shape::e_circle)
        {
            b2CircleShape* circShape = (b2CircleShape*)fix->GetShape();

            vertexBuffer =
            {
                {-circShape->m_radius + circShape->m_p.x, +circShape->m_radius + circShape->m_p.y},
                {+circShape->m_radius + circShape->m_p.x, +circShape->m_radius + circShape->m_p.y},
                {+circShape->m_radius + circShape->m_p.x, -circShape->m_radius + circShape->m_p.y},
                {+circShape->m_radius + circShape->m_p.x, -circShape->m_radius + circShape->m_p.y},
                {-circShape->m_radius + circShape->m_p.x, -circShape->m_radius + circShape->m_p.y},
                {-circShape->m_radius + circShape->m_p.x, +circShape->m_radius + circShape->m_p.y},
            };

            renderGroup->circlePositionings[i] = circShape->m_p;
            renderGroup->circlePositioningsMirrored[i] = PortalWorld::mirror(b2Vec2(0.0f, 1.0f), circShape->m_p);

            vertexBufferMirrored.resize(vertexBuffer.size());
            int j = 0;
            for (b2Vec2 vertex : vertexBuffer)
            {
                vertexBufferMirrored[j++] = PortalWorld::mirror(b2Vec2(0.0f, 1.0f), vertex);
            }

            renderGroup->vertexStorageOffsets[i] = -1;
            renderGroup->vertexStorageOffsetsMirrored[i] = -1;

            for (j = 0; j < swapChain->imageCount; j++)
            {
                int offsetIndex = ShaderBase::freeIndicesList[0][ShaderBase::avlAllocLastLoc[0]];
                ShaderBase::avlAllocLastLoc[0]--;
                renderStruct->dynamicOffsets[j] = { (uint32_t)(offsetIndex * ShaderBase::alignedSizes[0]) };

                s_uboAll* allData;
                vkMapMemory(logicalDevice, ShaderBase::commonStorageBuffers[0]->memory,
                            offsetIndex * ShaderBase::alignedSizes[0], ShaderBase::alignedSizes[0], 0, (void**)&allData);
                allData->color = glm::vec4(0.3f, 1.0f, 0.0f, 0.3f);
                allData->data.y = 0.03f;
                allData->data.z = 0.6f;
                allData->data.w = circShape->m_radius;
                allData->extraData.x = circShape->m_p.x;
                allData->extraData.y = circShape->m_p.y;
                allData->extraData.z = -1.0f;
                allData->outerColor = glm::vec4(0.0f, 1.0f, 0.0f, 1.0f);
                allData->modelMatrix = glm::mat4(1.0f);
                vkUnmapMemory(logicalDevice, ShaderBase::commonStorageBuffers[0]->memory);
            }
        }
        else if (fix->GetType() == b2Shape::e_polygon)
        {
            b2PolygonShape* polyShape = (b2PolygonShape*)fix->GetShape();
            b2Vec2 vertex0 = polyShape->m_vertices[0];
            b2Vec2 mirrored0 = PortalWorld::mirror(b2Vec2(0.0f, 1.0f), vertex0);
            for (int j = 1; j < polyShape->m_count - 1; j++)
            {
                for (int k = 0; k < 2; k++)
                {
                    b2Vec2 vertex = polyShape->m_vertices[j + k];
                    b2Vec2 mirroredVertex = PortalWorld::mirror(b2Vec2(0.0f, 1.0f), vertex);
                    vertexBuffer.push_back(vertex);
                    vertexBufferMirrored.push_back(mirroredVertex);
                }
                
                vertexBuffer.push_back(vertex0);
                vertexBufferMirrored.push_back(mirrored0);
            }


            int offsetIndex1 = ShaderBase::freeIndicesList[1][ShaderBase::avlAllocLastLoc[1]];
            int offsetIndex1Mirrored = ShaderBase::freeIndicesList[1][ShaderBase::avlAllocLastLoc[1] - 1];
            ShaderBase::avlAllocLastLoc[1] -= 2;

            renderGroup->vertexStorageOffsets[i] = offsetIndex1 * ShaderBase::alignedSizes[1];
            renderGroup->vertexStorageOffsetsMirrored[i] = offsetIndex1Mirrored * ShaderBase::alignedSizes[1];

            for (int j = 0; j < swapChain->imageCount; j++)
            {
                int offsetIndex0 = ShaderBase::freeIndicesList[0][ShaderBase::avlAllocLastLoc[0]];
                ShaderBase::avlAllocLastLoc[0]--;
                renderStruct->dynamicOffsets[j] = { (uint32_t)(offsetIndex0 * ShaderBase::alignedSizes[0]),
                                                    (uint32_t)(offsetIndex1 * ShaderBase::alignedSizes[1]) };

                s_uboAll* allData;
                vkMapMemory(logicalDevice, ShaderBase::commonStorageBuffers[0]->memory,
                    offsetIndex0 * ShaderBase::alignedSizes[0], ShaderBase::alignedSizes[0], 0, (void**)&allData);
                allData->color = glm::vec4(0.0f, 0.1f, 1.0f, 0.3f);
                allData->data.y = 0.03f;
                allData->data.z = 0.6f;
                allData->extraData.z = -1.0f;
                allData->outerColor = glm::vec4(0.0f, 1.0f, 1.0f, 1.0f);
                allData->modelMatrix = glm::mat4(1.0f);
                vkUnmapMemory(logicalDevice, ShaderBase::commonStorageBuffers[0]->memory);

                s_uboPolyData* data;
                vkMapMemory(logicalDevice, ShaderBase::commonStorageBuffers[1]->memory,
                    offsetIndex1 * ShaderBase::alignedSizes[1], ShaderBase::alignedSizes[1], 0, (void**)&data);
                data->numPolyPoints = polyShape->m_count;
                b2Vec2* hackedVector = (b2Vec2*)data->polyPoints;
                for (int k = 0; k < polyShape->m_count; k++)
                {
                    *(hackedVector + k) = polyShape->m_vertices[k];
                }
                vkUnmapMemory(logicalDevice, ShaderBase::commonStorageBuffers[1]->memory);

                vkMapMemory(logicalDevice, ShaderBase::commonStorageBuffers[1]->memory,
                    offsetIndex1Mirrored * ShaderBase::alignedSizes[1], ShaderBase::alignedSizes[1], 0, (void**)&data);
                data->numPolyPoints = polyShape->m_count;
                hackedVector = (b2Vec2*)data->polyPoints;
                for (int k = 0; k < polyShape->m_count; k++)
                {
                    *(hackedVector + k) = PortalWorld::mirror(b2Vec2(0.0f, 1.0f), polyShape->m_vertices[k]);
                }
                vkUnmapMemory(logicalDevice, ShaderBase::commonStorageBuffers[1]->memory);
            }
        }

        uint32_t vertexBufferSize = static_cast<uint32_t>(vertexBuffer.size()) * sizeof(b2Vec2);
        renderStruct->vertexBuffer->count = vertexBuffer.size();

        uint32_t vertexBufferSize2 = static_cast<uint32_t>(vertexBufferMirrored.size()) * sizeof(b2Vec2);
        mirroredVertices->count = vertexBufferMirrored.size();

        bufferStage(vertexBuffer.data(), vertexBufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, renderStruct->vertexBuffer);
        bufferStage(vertexBufferMirrored.data(), vertexBufferSize2, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, mirroredVertices);
    }

    renderGroup->renderBodies.insert(renderBody);
    debug_renderGroups.push_back(renderGroup);

    portalBody->worldIndex->createCallbacks.push_back(std::make_pair(debug_portalBodyCreateCallback, (void*)renderGroup));
    portalBody->worldIndex->destroyCallbacks.push_back(std::make_pair(debug_portalBodyDestroyCallback, (void*)renderGroup));

    if (portalBody->isMirrored)
    {
        renderGroup->swapMirroredBuffers();
    }
}

void ThinDrawer::debug_portalBodyCreateCallback(PortalBody* portalBody, void* data)
{
    ThinDrawer* thinDrawer = ThinDrawer::object;
    s_debug_renderGroup* renderGroup = (s_debug_renderGroup*)data;
    
    s_debug_renderBody* renderBody = new s_debug_renderBody;
    renderBody->portalBody = portalBody;
    renderGroup->renderBodies.insert(renderBody);
    for (int i = 0; i < portalBody->fixtures.size(); i++)
    {
        b2Fixture* fix = portalBody->fixtures.at(i);

        s_debug_renderStruct* renderStruct = new s_debug_renderStruct;
        renderBody->renderFixtures.push_back(renderStruct);
        renderStruct->dynamicOffsets.resize(thinDrawer->swapChain->imageCount);
        renderStruct->renderType = fix->GetType();
        fixtureData* fData = (fixtureData*)fix->GetUserData().pointer;
        PBFixtureData* pbfData = (PBFixtureData*)fData->data;
        renderStruct->pbfData = pbfData;
        if (!portalBody->isMirrored)
        {
            renderStruct->vertexBuffer = renderGroup->vertexBuffers[i];
        }
        else
        {
            renderStruct->vertexBuffer = renderGroup->vertexBuffersMirrored[i];
        }

        if (fix->GetType() == b2Shape::e_circle)
        {
            b2CircleShape* circShape = (b2CircleShape*)fix->GetShape();

            for (int j = 0; j < thinDrawer->swapChain->imageCount; j++)
            {
                int offsetIndex = ShaderBase::freeIndicesList[0][ShaderBase::avlAllocLastLoc[0]];
                ShaderBase::avlAllocLastLoc[0]--;
                renderStruct->dynamicOffsets[j] = { (uint32_t)(offsetIndex * ShaderBase::alignedSizes[0]) };

                s_uboAll* allData;
                vkMapMemory(thinDrawer->logicalDevice, ShaderBase::commonStorageBuffers[0]->memory,
                    offsetIndex * ShaderBase::alignedSizes[0], ShaderBase::alignedSizes[0], 0, (void**)&allData);
                allData->color = glm::vec4(0.3f, 1.0f, 0.0f, 0.3f);
                allData->data.y = 0.02f;
                allData->data.z = 0.6f;
                allData->data.w = circShape->m_radius;
                b2Vec2 circPos = portalBody->isMirrored ? renderGroup->circlePositioningsMirrored[i] : renderGroup->circlePositionings[i];
                allData->extraData.x = circPos.x;
                allData->extraData.y = circPos.y;
                allData->extraData.z = -1.0f;
                allData->outerColor = glm::vec4(0.0f, 1.0f, 0.0f, 1.0f);
                allData->modelMatrix = glm::mat4(1.0f);
                vkUnmapMemory(thinDrawer->logicalDevice, ShaderBase::commonStorageBuffers[0]->memory);
            }
        }
        else if (fix->GetType() == b2Shape::e_polygon)
        {
            b2PolygonShape* polyShape = (b2PolygonShape*)fix->GetShape();

            for (int j = 0; j < thinDrawer->swapChain->imageCount; j++)
            {
                int offsetIndex0 = ShaderBase::freeIndicesList[0][ShaderBase::avlAllocLastLoc[0]];
                ShaderBase::avlAllocLastLoc[0]--;
                renderStruct->dynamicOffsets[j] = { (uint32_t)(offsetIndex0 * ShaderBase::alignedSizes[0]),
                                                    (uint32_t)(portalBody->isMirrored ? renderGroup->vertexStorageOffsetsMirrored[i] : renderGroup->vertexStorageOffsets[i]) };

                s_uboAll* allData;
                vkMapMemory(thinDrawer->logicalDevice, ShaderBase::commonStorageBuffers[0]->memory,
                    offsetIndex0 * ShaderBase::alignedSizes[0], ShaderBase::alignedSizes[0], 0, (void**)&allData);
                allData->color = glm::vec4(0.0f, 0.1f, 1.0f, 0.3f);
                allData->data.y = 0.02f;
                allData->data.z = 0.6f;
                allData->extraData.z = -1.0f;
                allData->outerColor = glm::vec4(0.0f, 1.0f, 1.0f, 1.0f);
                allData->modelMatrix = glm::mat4(1.0f);
                vkUnmapMemory(thinDrawer->logicalDevice, ShaderBase::commonStorageBuffers[0]->memory);
            }
        }
    }
}

void ThinDrawer::portalBodyCreateCallback(PortalBody* portalBody, void* data)
{
    ThinDrawer* thinDrawer = ThinDrawer::object;
    s_renderGroup* renderGroup = (s_renderGroup*)data;

    s_renderBody* renderBody = new s_renderBody;
    renderGroup->renderBodies.insert(renderBody);
    renderBody->portalBody = portalBody;
    
    int numFixtures = portalBody->numFixtures;
    renderBody->renderFixtures.resize(numFixtures);
    for (int i = 0; i < numFixtures; i++)
    {
        b2Fixture* fix = portalBody->fixtures.at(i);
        fixtureData* fData = (fixtureData*)fix->GetUserData().pointer;
        PBFixtureData* pbfData = (PBFixtureData*)fData->data;

        renderBody->renderFixtures[i] = pbfData;
    }
}

void ThinDrawer::portalBodyDestroyCallback(PortalBody* portalBody, void* data)
{
    ThinDrawer* thinDrawer = ThinDrawer::object;
    s_renderGroup* renderGroup = (s_renderGroup*)data;

    for (auto iter = renderGroup->renderBodies.begin(); iter != renderGroup->renderBodies.end(); iter++)
    {
        s_renderBody* renderBody = *iter;
        if ((*iter)->portalBody == portalBody)
        {
            renderGroup->renderBodies.erase(iter);

            delete renderBody;

            break;
        }
    }
}

void ThinDrawer::debug_portalBodyDestroyCallback(PortalBody* portalBody, void* data)
{
    ThinDrawer* thinDrawer = ThinDrawer::object;
    s_debug_renderGroup* renderGroup = (s_debug_renderGroup*)data;

    for (auto iter = renderGroup->renderBodies.begin(); iter != renderGroup->renderBodies.end(); iter++)
    {
        s_debug_renderBody* renderBody = *iter;
        if ((*iter)->portalBody == portalBody)
        {
            renderGroup->renderBodies.erase(iter);

            for (s_debug_renderStruct* renderStruct : renderBody->renderFixtures)
            {
                for (int j = 0; j < thinDrawer->swapChain->imageCount; j++)
                {
                    ShaderBase::freeIndicesList[0][++ShaderBase::avlAllocLastLoc[0]] =
                            renderStruct->dynamicOffsets[j][0] / ShaderBase::alignedSizes[0];
                }
                delete renderStruct;
            }

            delete renderBody;

            break;
        }
    }
}

void ThinDrawer::eraseFromMaperFunctions(std::vector<std::pair<void (*) (void*), void*>>& vec, void* ptr)
{
    for (auto iter = vec.begin(); iter < vec.end(); )
    {
        if (iter->second == ptr)
        {
            iter = vec.erase(iter);
        }
        else
        {
            iter++;
        }
    }
}

void ThinDrawer::eraseFromRenderFunctions(std::vector<std::pair<void (*) (VkCommandBuffer, void*), void*>>& vec, void* ptr)
{
    for (auto iter = vec.begin(); iter < vec.end(); )
    {
        if (iter->second == ptr)
        {
            iter = vec.erase(iter);
        }
        else
        {
            iter++;
        }
    }
}

void ThinDrawer::releaseRenderResources()
{
    memMapFuncs.clear();

    earlyDrawFuncs.clear();
    midDrawFuncs.clear();
    drawFuncs.clear();
    lateDrawFuncs.clear();
    renderPortalData.clear();

    drawData = nullptr;

    vkDeviceWaitIdle(logicalDevice);

    for (s_debug_renderGroup* renderGroup : debug_renderGroups)
    {
        for (s_debug_renderBody* renderBody : renderGroup->renderBodies)
        {
            for (s_debug_renderStruct* renderStruct : renderBody->renderFixtures)
            {
                for (int j = 0; j < swapChain->imageCount; j++)
                {
                    ShaderBase::freeIndicesList[0][++ShaderBase::avlAllocLastLoc[0]] =
                        renderStruct->dynamicOffsets[j][0] / ShaderBase::alignedSizes[0];
                }
                delete renderStruct;
            }
            delete renderBody;
        }

        for (int i = 0; i < renderGroup->vertexBuffers.size(); i++)
        {
            s_buffers* buffer0 = renderGroup->vertexBuffers[i];
            s_buffers* buffer1 = renderGroup->vertexBuffersMirrored[i];
            vkDestroyBuffer(logicalDevice, buffer0->buffer, nullptr);
            vkDestroyBuffer(logicalDevice, buffer1->buffer, nullptr);
            vkFreeMemory(logicalDevice, buffer0->memory, nullptr);
            vkFreeMemory(logicalDevice, buffer1->memory, nullptr);
            free(buffer0);
            free(buffer1);

            if (renderGroup->vertexStorageOffsets[i] != -1)
            {
                ShaderBase::freeIndicesList[1][++ShaderBase::avlAllocLastLoc[1]] =
                    renderGroup->vertexStorageOffsets[i] / ShaderBase::alignedSizes[1];
                
                ShaderBase::freeIndicesList[1][++ShaderBase::avlAllocLastLoc[1]] =
                    renderGroup->vertexStorageOffsetsMirrored[i] / ShaderBase::alignedSizes[1];
            }
        }

        delete renderGroup;
    }

    debug_renderGroups.clear();

    for (s_renderGroup* renderGroup : renderGroups)
    {
        for (int i = 0; renderGroup->renderBodies.size() > 0 && i < (*renderGroup->renderBodies.begin())->renderFixtures.size(); i++)
        {
            s_buffers* buffer0 = renderGroup->vertexBuffers[i];
            vkDestroyBuffer(logicalDevice, buffer0->buffer, nullptr);
            vkFreeMemory(logicalDevice, buffer0->memory, nullptr);
            free(buffer0);
        }

        for (s_renderBody* renderBody : renderGroup->renderBodies)
        {
            delete renderBody;
        }

        delete renderGroup;
    }

    renderGroups.clear();

    for (s_renderGroup3D* renderGroup : renderGroups3D)
    {
        for (s_renderBody3D* renderBody : renderGroup->renderBodies)
        {
            for (int j = 0; j < swapChain->imageCount; j++)
            {
                bodyShader3D->lastAllocationPos_body--;
                bodyShader3D->availableOffsets_body[bodyShader3D->lastAllocationPos_body] = renderBody->dynamicOffsets[j];
            }
            for (s_renderStruct3D* renderStruct : renderBody->renderFixtures)
            {
                for (int j = 0; j < swapChain->imageCount; j++)
                {
                    bodyShader3D->lastAllocationPos_fixture--;
                    bodyShader3D->availableOffsets_fixture[bodyShader3D->lastAllocationPos_fixture] = renderStruct->dynamicOffsets[j][1];
                }
                delete renderStruct;
            }
            delete renderBody;
        }
        delete renderGroup;
    }

    renderGroups3D.clear();
}