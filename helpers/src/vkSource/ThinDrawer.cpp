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

ThinDrawer::ThinDrawer(PortalWorld* pWorld)
{
    drawReleases = true;
    this->pWorld = pWorld;

    initBase();
    initExtra();
}

void ThinDrawer::surfaceRecreate()
{
    vkDeviceWaitIdle(logicalDevice);

    vkDestroyPipeline(logicalDevice, debugCircleShader->pipeline, VK_NULL_HANDLE);
    vkDestroyPipeline(logicalDevice, debugLineShader->pipeline, VK_NULL_HANDLE);
    vkDestroyPipeline(logicalDevice, debugPolygonShader->pipeline, VK_NULL_HANDLE);
    vkDestroyPipeline(logicalDevice, whiteWallShader->pipeline, VK_NULL_HANDLE);
    vkDestroyPipeline(logicalDevice, bodyShader->pipeline, VK_NULL_HANDLE);
    vkDestroyRenderPass(logicalDevice, renderPass, VK_NULL_HANDLE);
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
    debugCircleShader = new DebugCircle(this);
    debugLineShader = new DebugLine(this);
    debugPolygonShader = new DebugPolygon(this);
    whiteWallShader = new WhiteWall(this);
    bodyShader = new Body(this);

    prepareVertices();
    prepareUniformBuffers();
    setupDescriptorSetLayout();
    preparePipelines();
    setupDescriptorPool();
    setupDescriptorSet();

    initImgui();
}

void ThinDrawer::uniformFiller(int frameNum)
{
    s_sharedUniformData* sharedData;
    CHECK_RESULT_VK(vkMapMemory(logicalDevice, sharedOrthoUniform[frameNum]->memory, 0, sizeof(s_sharedUniformData), 0, (void**)&sharedData));
    sharedData->orthoMatrix = wh->cam->camMatrix;
    sharedData->cameraPos = glm::vec4(glm::vec3(wh->cam->pos), 1.0f);
    sharedData->data.x = sqrt(wh->cam->zoom * 0.25);
    vkUnmapMemory(logicalDevice, sharedOrthoUniform[frameNum]->memory);
}

void ThinDrawer::renderLoop()
{
    wh->looper();
    wh->cam->update();

    int imNum = frameNumber % swapChain->imageCount;

    s_frameData currentFrame = frames[imNum];

    CHECK_RESULT_VK(vkWaitForFences(logicalDevice, 1, &currentFrame.renderFence, true, UINT64_MAX));
    CHECK_RESULT_VK(vkResetFences(logicalDevice, 1, &currentFrame.renderFence));

    VkResult result = vkAcquireNextImageKHR(logicalDevice, swapChain->swapChain, UINT64_MAX,
                          currentFrame.presentSemaphore, VK_NULL_HANDLE, &lastSwapChainImageIndex);

    uniformFiller(lastSwapChainImageIndex);

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

    submit.commandBufferCount = 1;
    submit.pCommandBuffers = &frames[lastSwapChainImageIndex].commandBuffer;

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

void ThinDrawer::prepareVertices()
{
    debugCircleShader->prepareVertexData();
    debugLineShader->prepareVertexData();
    debugPolygonShader->prepareVertexData();
    whiteWallShader->prepareVertexData();
    bodyShader->prepareVertexData();
}

void ThinDrawer::prepareUniformBuffers()
{
    ShaderBase::createCommonStorageBuffers(this);
    sharedOrthoUniform.resize(swapChain->imageCount);
    for (int i = 0; i < swapChain->imageCount; i++)
    {
        sharedOrthoUniform[i] = (s_uniformBuffer*)malloc(sizeof(s_uniformBuffer));
        uniformHelper(sizeof(s_sharedUniformData), sharedOrthoUniform[i]);
    }

    debugCircleShader->prepareUniforms();
    debugLineShader->prepareUniforms();
    debugPolygonShader->prepareUniforms();
    whiteWallShader->prepareUniforms();
    bodyShader->prepareUniforms();
}

void ThinDrawer::setupDescriptorSetLayout()
{
    debugCircleShader->setupDescriptorSetLayout();
    debugLineShader->setupDescriptorSetLayout();
    debugPolygonShader->setupDescriptorSetLayout();
    whiteWallShader->setupDescriptorSetLayout();
    bodyShader->setupDescriptorSetLayout();
}

void ThinDrawer::preparePipelines()
{
    debugCircleShader->preparePipeline();
    debugLineShader->preparePipeline();
    debugPolygonShader->preparePipeline();
    whiteWallShader->preparePipeline();
    bodyShader->preparePipeline();
}

void ThinDrawer::setupDescriptorPool()
{
    std::vector<VkDescriptorPoolSize> sizes =
    {
            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 100 },
            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 100 },
            { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 100 },
            { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 100 },
            { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 100 },
    };

    VkDescriptorPoolCreateInfo descriptorPoolInfo = { };
    descriptorPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    descriptorPoolInfo.poolSizeCount = sizes.size();
    descriptorPoolInfo.pPoolSizes = sizes.data();
    descriptorPoolInfo.maxSets = 100;

    CHECK_RESULT_VK(vkCreateDescriptorPool(logicalDevice, &descriptorPoolInfo, VK_NULL_HANDLE, &descriptorPool));
}

void ThinDrawer::setupDescriptorSet()
{
    debugCircleShader->setupDescriptorSet();
    debugLineShader->setupDescriptorSet();
    debugPolygonShader->setupDescriptorSet();
    whiteWallShader->setupDescriptorSet();
    bodyShader->setupDescriptorSet();

    IOHelper::loadTexture("defaultTex.png");
    IOHelper::loadTexture("sphere.png");

    IOHelper::loadTexture("edge1.png");
    IOHelper::loadTexture("edge2.png");
    IOHelper::loadTexture("edge3.png");
    IOHelper::loadTexture("edge4.png");
    IOHelper::loadTexture("body.png");
}

void ThinDrawer::renderCommandBeginner(VkCommandBuffer& cmdBuff)
{
    VkCommandBufferBeginInfo cmdBufInfo = vkinit::commandBufferBeginInfo();

    VkClearValue clearValue;
    clearValue.color = { 0.0f, 0.0f, 0.0f, 0.0f };

    VkClearValue depthClear;
    depthClear.depthStencil.depth = 1.0f;

    VkClearValue clearValues[] = { clearValue, depthClear };

    VkRenderPassBeginInfo renderPassBeginInfo = vkinit::renderPassBeginInfo(renderPass, swapChain->extent.width, swapChain->extent.height, clearValues);
    renderPassBeginInfo.clearValueCount = std::size(clearValues);

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

    vkCmdBindDescriptorSets(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS,
        debugCircleShader->pipelineLayout, 0, 1, &debugCircleShader->descriptorSets[0][lastSwapChainImageIndex], 0, VK_NULL_HANDLE);

    vkCmdBindDescriptorSets(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS,
        debugLineShader->pipelineLayout, 0, 1, &debugLineShader->descriptorSets[0][lastSwapChainImageIndex], 0, VK_NULL_HANDLE);

    vkCmdBindDescriptorSets(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS,
        debugPolygonShader->pipelineLayout, 0, 1, &debugPolygonShader->descriptorSets[0][lastSwapChainImageIndex], 0, VK_NULL_HANDLE);

    for (s_debug_renderGroup* rg : debug_renderGroups)
    {
        for (s_debug_renderBody* renderBody : rg->renderBodies)
        {
            b2Vec2 bodyPos = renderBody->portalBody->body->GetPosition();
            float bodyAngle = renderBody->portalBody->body->GetAngle() + renderBody->portalBody->offsetAngle;

            glm::mat4 modelMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(bodyPos.x, bodyPos.y, 0.0f));
            modelMatrix = glm::rotate(modelMatrix, bodyAngle, glm::vec3(0.0f, 0.0f, 1.0f));

            for (s_debug_renderStruct* renderStruct : renderBody->renderFixtures)
            {
                s_uboAll* allData;
                vkMapMemory(logicalDevice, ShaderBase::commonStorageBuffers[lastSwapChainImageIndex][0]->memory,
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
                        vkUnmapMemory(logicalDevice, ShaderBase::commonStorageBuffers[lastSwapChainImageIndex][0]->memory);
                        continue;
                    }
                }
                else
                {
                    allData->extraData.z = drawReleases ? -1.0f : 0.0f;
                }

                vkUnmapMemory(logicalDevice, ShaderBase::commonStorageBuffers[lastSwapChainImageIndex][0]->memory);

                switch (renderStruct->renderType)
                {
                case b2Shape::e_circle:
                {
                    vkCmdBindPipeline(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS, debugCircleShader->pipeline);

                    vkCmdBindDescriptorSets(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS,
                        debugCircleShader->pipelineLayout, 1, 1, &debugCircleShader->descriptorSets[1][lastSwapChainImageIndex],
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
                        debugPolygonShader->pipelineLayout, 1, 1, &debugPolygonShader->descriptorSets[1][lastSwapChainImageIndex],
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
    const VkDeviceSize offsets = 0;
 
    vkCmdBindDescriptorSets(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS,
        debugLineShader->pipelineLayout, 0, 1, &debugLineShader->descriptorSets[0][lastSwapChainImageIndex], 0, VK_NULL_HANDLE);
    vkCmdBindPipeline(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS, debugLineShader->pipeline);
    vkCmdBindVertexBuffers(cmdBuff, 0, 1, &debugCircleShader->buffers[0]->buffer, &offsets);

    s_pushConstantLine pushData;
    pushData.colour = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
    pushData.data.z = 0.5f;
    pushData.data.y = 0.03f;
    pushData.colour = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
    for (Portal* p : pWorld->portals)
    {
        pushData.positions = glm::vec4(p->points[0].x, p->points[0].y, p->points[1].x, p->points[1].y);

        vkCmdPushConstants(cmdBuff, debugLineShader->pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
            0, sizeof(s_pushConstantLine), (void*)&pushData);
        vkCmdDraw(cmdBuff, debugCircleShader->buffers[0]->count, 1, 0, 0);
    }
}

void ThinDrawer::debug_renderEdges(VkCommandBuffer& cmdBuff)
{
    const VkDeviceSize offsets = 0;

    vkCmdBindDescriptorSets(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS,
        debugLineShader->pipelineLayout, 0, 1, &debugLineShader->descriptorSets[0][lastSwapChainImageIndex], 0, VK_NULL_HANDLE);
    vkCmdBindPipeline(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS, debugLineShader->pipeline);
    vkCmdBindVertexBuffers(cmdBuff, 0, 1, &debugCircleShader->buffers[0]->buffer, &offsets);

    s_pushConstantLine pushData;
    pushData.data.y = 0.02f;
    pushData.data.z = 0.5f;

    for (b2Body* body = pWorld->GetBodyList(); body; body = body->GetNext())
    {
        for (b2Fixture* fix = body->GetFixtureList(); fix; fix = fix->GetNext())
        {
            if (fix->GetType() == b2Shape::e_edge)
            {
                b2EdgeShape* edgeShape = (b2EdgeShape*)fix->GetShape();

                glm::vec4 colorToRender = glm::vec4(1.0f, 1.0f, 0.0f, 1.0f);
                glm::vec4 gelColor = colorToRender;
                fixtureData* fixData = (fixtureData*)fix->GetUserData().pointer;
                if (fixData)
                {
                    g_PhysicsData* pData = (g_PhysicsData*)fixData->extraData;
                    if (pData && pData->type == EDGE)
                    {
                        g_EdgeData* eData = (g_EdgeData*)pData->data;
                        switch (eData->wall)
                        {
                        case PORTALLABLE:
                            colorToRender = glm::vec4(0.0f, 1.0f, 0.0f, 1.0f);
                            break;
                        case NOT_PORTALLABLE:
                            colorToRender = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);
                            break;
                        default:
                            break;
                        }

                        switch (eData->gel)
                        {
                        case NONE:
                            gelColor = glm::vec4(1.0f, 0.0f, 1.0f, 1.0f);
                            break;
                        case BLUE:
                            gelColor = glm::vec4(0.0f, 0.0f, 1.0f, 1.0f);
                            break;
                        case ORANGE:
                            gelColor = glm::vec4(1.0f, 0.5f, 0.0f, 1.0f);
                            break;
                        case WHITE:
                            gelColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
                            break;
                        default:
                            break;
                        }
                    }
                }

                b2Vec2 v1 = body->GetWorldPoint(edgeShape->m_vertex1);
                b2Vec2 v2 = body->GetWorldPoint(edgeShape->m_vertex2);

                b2Vec2 dir = PortalWorld::rotateVec(v2 - v1, b2_pi * 0.25f);
                dir.Normalize();
                dir *= 0.1f;

                pushData.colour = gelColor;
                pushData.positions = glm::vec4(v1.x, v1.y, v2.x, v2.y);
                vkCmdPushConstants(cmdBuff, debugLineShader->pipelineLayout,
                    VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                    0, sizeof(s_pushConstantLine), (void*)&pushData);
                vkCmdDraw(cmdBuff, debugCircleShader->buffers[0]->count, 1, 0, 0);

                pushData.colour = colorToRender;
                pushData.positions = glm::vec4(v1.x, v1.y, v1.x + dir.x, v1.y + dir.y);
                vkCmdPushConstants(cmdBuff, debugLineShader->pipelineLayout,
                    VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                    0, sizeof(s_pushConstantLine), (void*)&pushData);
                vkCmdDraw(cmdBuff, debugCircleShader->buffers[0]->count, 1, 0, 0);

                dir = PortalWorld::rotateVec(dir, b2_pi * 0.5f);
                pushData.positions = glm::vec4(v2.x, v2.y, v2.x + dir.x, v2.y + dir.y);

                vkCmdPushConstants(cmdBuff, debugLineShader->pipelineLayout,
                    VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                    0, sizeof(s_pushConstantLine), (void*)&pushData);
                vkCmdDraw(cmdBuff, debugCircleShader->buffers[0]->count, 1, 0, 0);
            }
        }
    }
}

void ThinDrawer::renderWalls(VkCommandBuffer& cmdBuff)
{
    const VkDeviceSize offsets = 0;

    vkCmdBindPipeline(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS, whiteWallShader->pipeline);

    vkCmdBindVertexBuffers(cmdBuff, 0, 1, &whiteWallShader->buffers[0]->buffer, &offsets);

    vkCmdBindDescriptorSets(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS,
        whiteWallShader->pipelineLayout, 0, 1, &whiteWallShader->descriptorSets[0][lastSwapChainImageIndex], 0, VK_NULL_HANDLE);

    vkCmdBindDescriptorSets(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS,
        whiteWallShader->pipelineLayout, 1, 1, &whiteWallShader->descriptorSets[1][lastSwapChainImageIndex], 0, 0);

    vkCmdBindDescriptorSets(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS,
        whiteWallShader->pipelineLayout, 2, 1, &whiteWallShader->textureData[0]->set, 0, VK_NULL_HANDLE);

    vkCmdDraw(cmdBuff, whiteWallShader->buffers[0]->count, whiteWallShader->wallCount, 0, 0);
}

void ThinDrawer::renderPortalBodies(VkCommandBuffer& cmdBuff)
{
    const VkDeviceSize offsets = 0;

    uint32_t dynamicOffset = 0;
    vkCmdBindPipeline(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS, bodyShader->pipeline);

    vkCmdBindDescriptorSets(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS,
        bodyShader->pipelineLayout, 0, 1, &bodyShader->descriptorSets[0][lastSwapChainImageIndex], 0, VK_NULL_HANDLE);

    for (s_renderGroup* renderGroup : renderGroups)
    {
        for (s_renderBody* renderBody : renderGroup->renderBodies)
        {
            PortalBody* pb = renderBody->portalBody;
            s_bodyUbo* bodyUbo;
            
            CHECK_RESULT_VK(vkMapMemory(logicalDevice, bodyShader->uniformBuffers[lastSwapChainImageIndex][0]->memory,
                renderBody->dynamicOffsets[lastSwapChainImageIndex], bodyShader->alignedSize_body, 0, (void**)&bodyUbo));

            bodyUbo->modelMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(pb->body->GetPosition().x, pb->body->GetPosition().y, 0.0f));
            bodyUbo->rotationMatrix = glm::rotate(glm::mat4(1.0f), pb->body->GetAngle() + pb->offsetAngle, glm::vec3(0.0f, 0.0f, 1.0));
            bodyUbo->modelMatrix = bodyUbo->modelMatrix * bodyUbo->rotationMatrix * renderGroup->scaleMatrix;
            vkUnmapMemory(logicalDevice, bodyShader->uniformBuffers[lastSwapChainImageIndex][0]->memory);

            for (s_renderStruct* renderStruct : renderBody->renderFixtures)
            {
                s_fixUbo* fixUbo;
                CHECK_RESULT_VK(vkMapMemory(logicalDevice, bodyShader->uniformBuffers[lastSwapChainImageIndex][1]->memory,
                    renderStruct->dynamicOffsets[lastSwapChainImageIndex][1], bodyShader->alignedSize_fixture, 0, (void**)&fixUbo));

                fixUbo->data.x = (float)renderStruct->pbfData->pCollisions.size();
                int i = 0;
                for (portalCollision* pc : renderStruct->pbfData->pCollisions)
                {
                    fixUbo->portals[i] = glm::vec4( pc->portal->points[pc->side ^ 1].x, pc->portal->points[pc->side ^ 1].y,
                                                    pc->portal->points[pc->side].x, pc->portal->points[pc->side].y);
                }

                vkUnmapMemory(logicalDevice, bodyShader->uniformBuffers[lastSwapChainImageIndex][1]->memory);

                vkCmdBindDescriptorSets(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS,
                    bodyShader->pipelineLayout, 1, 1, &bodyShader->descriptorSets[1][lastSwapChainImageIndex],
                    std::size(renderStruct->dynamicOffsets[lastSwapChainImageIndex]), renderStruct->dynamicOffsets[lastSwapChainImageIndex].data());


                vkCmdBindDescriptorSets(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS,
                    bodyShader->pipelineLayout, 2, 1, &renderStruct->imageDescriptorSet, 0, VK_NULL_HANDLE);


                VkIndexType indexType = VK_INDEX_TYPE_UINT16;
                vkCmdBindVertexBuffers(cmdBuff, 0, 1, &renderStruct->vertexBuffer->buffer, &offsets);
                vkCmdBindIndexBuffer(cmdBuff, renderStruct->indexBuffer->buffer, 0, indexType);
                vkCmdDrawIndexed(cmdBuff, renderStruct->indexBuffer->count, 1, 0, 0, 0);
            }
        }
    }
}

void ThinDrawer::buildCommandBuffers()
{
    VkCommandBuffer cmdBuff = frames[lastSwapChainImageIndex].commandBuffer;

    renderCommandBeginner(cmdBuff);

    VkDeviceSize offsets = 0;

    renderWalls(cmdBuff);

    renderPortalBodies(cmdBuff);

    //debug_renderPortalBodies(cmdBuff);

    if (pWorld)
    {
        debug_renderEdges(cmdBuff);

        debug_renderPortals(cmdBuff);
    }

    if (drawData)
    {
        ImGui_ImplVulkan_RenderDrawData(drawData, cmdBuff);
    }

    vkCmdEndRenderPass(cmdBuff);
    CHECK_RESULT_VK(vkEndCommandBuffer(cmdBuff));
}

void ThinDrawer::addPortalBody(PortalBody* portalBody, std::vector<std::pair<std::string, std::string>> meshNames)
{
    s_renderGroup* renderGroup = new s_renderGroup;
    s_renderBody* renderBody = new s_renderBody;
    renderGroup->renderBodies.insert(renderBody);
    renderBody->portalBody = portalBody;

    renderBody->dynamicOffsets.resize(swapChain->imageCount);
    for (int i = 0; i < swapChain->imageCount; i++)
    {
        renderBody->dynamicOffsets[i] = bodyShader->availableOffsets_body[bodyShader->lastAllocationPos_body++];
    }

    renderGroups.push_back(renderGroup);

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
            renderStruct->dynamicOffsets[j][1] = bodyShader->availableOffsets_fixture[bodyShader->lastAllocationPos_fixture++];
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
}

void ThinDrawer::portalBodyCreateCallback(PortalBody* portalBody, void* data)
{
    extern ThinDrawer* thinDrawer;
    s_renderGroup* renderGroup = (s_renderGroup*)data;

    s_renderBody* firstRenderBody = *renderGroup->renderBodies.begin();

    s_renderBody* renderBody = new s_renderBody;
    renderGroup->renderBodies.insert(renderBody);

    renderBody->dynamicOffsets.resize(thinDrawer->swapChain->imageCount);
    for (int i = 0; i < thinDrawer->swapChain->imageCount; i++)
    {
        renderBody->dynamicOffsets[i] = thinDrawer->bodyShader->availableOffsets_body[thinDrawer->bodyShader->lastAllocationPos_body++];
    }
    renderBody->portalBody = portalBody;
    renderBody->renderFixtures.resize(portalBody->numFixtures);

    int bufferIndex = portalBody->isMirrored ? 1 : 0;

    for (int i = 0; i < portalBody->numFixtures; i++)
    {
        fixtureData* fixData = (fixtureData*)portalBody->fixtures[i]->GetUserData().pointer;

        s_renderStruct* renderStruct = new s_renderStruct;
        renderBody->renderFixtures[i] = renderStruct;

        renderStruct->dynamicOffsets.resize(thinDrawer->swapChain->imageCount);
        for (int j = 0; j < thinDrawer->swapChain->imageCount; j++)
        {
            renderStruct->dynamicOffsets[j][0] = renderBody->dynamicOffsets[j];
            renderStruct->dynamicOffsets[j][1] = thinDrawer->bodyShader->availableOffsets_fixture[thinDrawer->bodyShader->lastAllocationPos_fixture++];
        }

        renderStruct->imageDescriptorSet = firstRenderBody->renderFixtures[i]->imageDescriptorSet;

        renderStruct->pbfData = (PBFixtureData*)fixData->data;

        renderStruct->vertexBuffer = renderGroup->meshDatas[i]->vertexBuffers[bufferIndex];
        renderStruct->indexBuffer = renderGroup->meshDatas[i]->indexBuffers[bufferIndex];
    }
}

void ThinDrawer::portalBodyDestroyCallback(PortalBody* portalBody, void* data)
{
    extern ThinDrawer* thinDrawer;
    s_renderGroup* renderGroup = (s_renderGroup*)data;
    
    for (auto iter = renderGroup->renderBodies.begin(); iter != renderGroup->renderBodies.end(); iter++)
    {
        s_renderBody* renderBody = *iter;
        if (renderBody->portalBody == portalBody)
        {
            for (int i = 0; i < thinDrawer->swapChain->imageCount; i++)
            {
                thinDrawer->bodyShader->lastAllocationPos_body--;
                thinDrawer->bodyShader->availableOffsets_body[thinDrawer->bodyShader->lastAllocationPos_body] = renderBody->dynamicOffsets[i];
            }

            for (s_renderStruct* rs : renderBody->renderFixtures)
            {
                for (int i = 0; i < thinDrawer->swapChain->imageCount; i++)
                {
                    thinDrawer->bodyShader->lastAllocationPos_fixture--;
                    thinDrawer->bodyShader->availableOffsets_fixture[thinDrawer->bodyShader->lastAllocationPos_fixture] = rs->dynamicOffsets[i][1];
                }
                
                delete rs;
            }

            renderGroup->renderBodies.erase(iter);
            delete renderBody;
            break;
        }
    }
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
                vkMapMemory(logicalDevice, ShaderBase::commonStorageBuffers[j][0]->memory,
                            offsetIndex * ShaderBase::alignedSizes[0], ShaderBase::alignedSizes[0], 0, (void**)&allData);
                allData->color = glm::vec4(0.3f, 1.0f, 0.0f, 0.3f);
                allData->data.y = 0.02f;
                allData->data.z = 0.6f;
                allData->data.w = circShape->m_radius;
                allData->extraData.x = circShape->m_p.x;
                allData->extraData.y = circShape->m_p.y;
                allData->extraData.z = -1.0f;
                allData->outerColor = glm::vec4(0.0f, 1.0f, 0.0f, 1.0f);
                allData->modelMatrix = glm::mat4(1.0f);
                vkUnmapMemory(logicalDevice, ShaderBase::commonStorageBuffers[j][0]->memory);
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
                vkMapMemory(logicalDevice, ShaderBase::commonStorageBuffers[j][0]->memory,
                    offsetIndex0 * ShaderBase::alignedSizes[0], ShaderBase::alignedSizes[0], 0, (void**)&allData);
                allData->color = glm::vec4(0.0f, 0.1f, 1.0f, 0.3f);
                allData->data.y = 0.02f;
                allData->data.z = 0.6f;
                allData->extraData.z = -1.0f;
                allData->outerColor = glm::vec4(0.0f, 1.0f, 1.0f, 1.0f);
                allData->modelMatrix = glm::mat4(1.0f);
                vkUnmapMemory(logicalDevice, ShaderBase::commonStorageBuffers[j][0]->memory);

                s_uboPolyData* data;
                vkMapMemory(logicalDevice, ShaderBase::commonStorageBuffers[j][1]->memory,
                    offsetIndex1 * ShaderBase::alignedSizes[1], ShaderBase::alignedSizes[1], 0, (void**)&data);
                data->numPolyPoints = polyShape->m_count;
                b2Vec2* hackedVector = (b2Vec2*)data->polyPoints;
                for (int k = 0; k < polyShape->m_count; k++)
                {
                    *(hackedVector + k) = polyShape->m_vertices[k];
                }
                vkUnmapMemory(logicalDevice, ShaderBase::commonStorageBuffers[j][1]->memory);

                vkMapMemory(logicalDevice, ShaderBase::commonStorageBuffers[j][1]->memory,
                    offsetIndex1Mirrored * ShaderBase::alignedSizes[1], ShaderBase::alignedSizes[1], 0, (void**)&data);
                data->numPolyPoints = polyShape->m_count;
                hackedVector = (b2Vec2*)data->polyPoints;
                for (int k = 0; k < polyShape->m_count; k++)
                {
                    *(hackedVector + k) = PortalWorld::mirror(b2Vec2(0.0f, 1.0f), polyShape->m_vertices[k]);
                }
                vkUnmapMemory(logicalDevice, ShaderBase::commonStorageBuffers[j][1]->memory);
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
}

void ThinDrawer::debug_portalBodyCreateCallback(PortalBody* portalBody, void* data)
{
    extern ThinDrawer* thinDrawer;
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
                vkMapMemory(thinDrawer->logicalDevice, ShaderBase::commonStorageBuffers[j][0]->memory,
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
                vkUnmapMemory(thinDrawer->logicalDevice, ShaderBase::commonStorageBuffers[j][0]->memory);
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
                vkMapMemory(thinDrawer->logicalDevice, ShaderBase::commonStorageBuffers[j][0]->memory,
                    offsetIndex0 * ShaderBase::alignedSizes[0], ShaderBase::alignedSizes[0], 0, (void**)&allData);
                allData->color = glm::vec4(0.0f, 0.1f, 1.0f, 0.3f);
                allData->data.y = 0.02f;
                allData->data.z = 0.6f;
                allData->extraData.z = -1.0f;
                allData->outerColor = glm::vec4(0.0f, 1.0f, 1.0f, 1.0f);
                allData->modelMatrix = glm::mat4(1.0f);
                vkUnmapMemory(thinDrawer->logicalDevice, ShaderBase::commonStorageBuffers[j][0]->memory);
            }
        }
    }
}

void ThinDrawer::debug_portalBodyDestroyCallback(PortalBody* portalBody, void* data)
{
    extern ThinDrawer* thinDrawer;
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

void ThinDrawer::releaseRenderResources()
{
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
        for (s_renderBody* renderBody : renderGroup->renderBodies)
        {
            for (int j = 0; j < swapChain->imageCount; j++)
            {
                bodyShader->lastAllocationPos_body--;
                bodyShader->availableOffsets_body[bodyShader->lastAllocationPos_body] = renderBody->dynamicOffsets[j];
            }
            for (s_renderStruct* renderStruct : renderBody->renderFixtures)
            {
                for (int j = 0; j < swapChain->imageCount; j++)
                {
                    bodyShader->lastAllocationPos_fixture--;
                    bodyShader->availableOffsets_fixture[bodyShader->lastAllocationPos_fixture] = renderStruct->dynamicOffsets[j][1];
                }
                delete renderStruct;
            }
            delete renderBody;
        }
        delete renderGroup;
    }

    renderGroups.clear();
}