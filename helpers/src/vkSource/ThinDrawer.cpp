#include <ThinDrawer.h>
#include <SwapChain.h>
#include <Camera.h>
#include <Shader.h>
#include <vkInit.h>
#include <vector>
#include <array>
#include <Portal.h>
#include <PortalBody.h>

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

    prepareVertices();
    prepareUniformBuffers();
    setupDescriptorSetLayout();
    preparePipelines();
    setupDescriptorPool();
    setupDescriptorSet();
}

void ThinDrawer::uniformFiller(int frameNum)
{
    s_sharedUniformData* sharedData;
    CHECK_RESULT_VK(vkMapMemory(logicalDevice, sharedOrthoUniform[frameNum]->memory, 0, sizeof(s_sharedUniformData), 0, (void**)&sharedData));
    sharedData->orthoMatrix = wh->cam->ortho;
    sharedData->data.x = wh->cam->zoom;
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

    if (result == VK_ERROR_OUT_OF_DATE_KHR)
    {
        surfaceRecreate();
        return;
    }

    vkResetCommandBuffer(frames[lastSwapChainImageIndex].commandBuffer, 0);
    buildCommandBuffers(lastSwapChainImageIndex);

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
}

void ThinDrawer::setupDescriptorSetLayout()
{
    debugCircleShader->setupDescriptorSetLayout();
    debugLineShader->setupDescriptorSetLayout();
    debugPolygonShader->setupDescriptorSetLayout();
}

void ThinDrawer::preparePipelines()
{
    debugCircleShader->preparePipeline();
    debugLineShader->preparePipeline();
    debugPolygonShader->preparePipeline();
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
}

void ThinDrawer::buildCommandBuffers(int frameNumber)
{
    VkCommandBufferBeginInfo cmdBufInfo = vkinit::commandBufferBeginInfo();

    VkClearValue clearValue;
    clearValue.color = { 0.0f, 0.0f, 0.0f, 0.0f };
    VkRenderPassBeginInfo renderPassBeginInfo = vkinit::renderPassBeginInfo(renderPass, swapChain->extent.width, swapChain->extent.height, &clearValue);

    renderPassBeginInfo.framebuffer = swapChain->frameBuffers[frameNumber];
    VkCommandBuffer cmdBuff = frames[frameNumber].commandBuffer;

    CHECK_RESULT_VK(vkBeginCommandBuffer(cmdBuff, &cmdBufInfo));

    vkCmdBeginRenderPass(cmdBuff, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

    VkViewport viewport = { };
    viewport.height = (float)swapChain->extent.height;
    viewport.width = (float)swapChain->extent.width;
    viewport.minDepth = (float)0.0f;
    viewport.maxDepth = (float)1.0f;
    vkCmdSetViewport(cmdBuff, 0, 1, &viewport);

    VkDeviceSize offsets = 0;

    vkCmdBindDescriptorSets(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS,
        debugCircleShader->pipelineLayout, 0, 1, &debugCircleShader->descriptorSets[0][frameNumber], 0, VK_NULL_HANDLE);

    vkCmdBindDescriptorSets(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS,
        debugLineShader->pipelineLayout, 0, 1, &debugLineShader->descriptorSets[0][frameNumber], 0, VK_NULL_HANDLE);

    vkCmdBindDescriptorSets(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS,
        debugPolygonShader->pipelineLayout, 0, 1, &debugPolygonShader->descriptorSets[0][frameNumber], 0, VK_NULL_HANDLE);

    // for debug line

    for (s_renderGroup* rg : renderGroups)
    {
        for (s_renderBody* renderBody : rg->renderBodies)
        {
            b2Vec2 bodyPos = renderBody->portalBody->body->GetPosition();
            float bodyAngle = renderBody->portalBody->body->GetAngle() + renderBody->portalBody->offsetAngle;

            glm::mat4 modelMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(bodyPos.x, bodyPos.y, 0.0f));
            modelMatrix = glm::rotate(modelMatrix, bodyAngle, glm::vec3(0.0f, 0.0f, 1.0f));

            for (s_renderStruct* renderStruct : renderBody->renderFixtures)
            {
                s_uboAll* allData;
                vkMapMemory(logicalDevice, ShaderBase::commonStorageBuffers[frameNumber][0]->memory,
                    renderStruct->dynamicOffsets[frameNumber][0], ShaderBase::alignedSizes[0], 0, (void**)&allData);
                allData->modelMatrix = modelMatrix;

                allData->data.x = renderStruct->fixData->pCollisions.size();

                bool fullOut = false;
                for (int i = 0; i < renderStruct->fixData->pCollisions.size(); i++)
                {
                    portalCollision* pc = renderStruct->fixData->pCollisions[i];
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
                        vkUnmapMemory(logicalDevice, ShaderBase::commonStorageBuffers[frameNumber][0]->memory);
                        continue;
                    }
                }
                else
                {
                    allData->extraData.z = drawReleases ? -1.0f : 0.0f;
                }

                vkUnmapMemory(logicalDevice, ShaderBase::commonStorageBuffers[frameNumber][0]->memory);

                switch (renderStruct->renderType)
                {
                    case b2Shape::e_circle:
                    {
                        vkCmdBindPipeline(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS, debugCircleShader->pipeline);

                        vkCmdBindDescriptorSets(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS,
                            debugCircleShader->pipelineLayout, 1, 1, &debugCircleShader->descriptorSets[1][frameNumber],
                            renderStruct->dynamicOffsets[frameNumber].size(), renderStruct->dynamicOffsets[frameNumber].data());

                        vkCmdBindVertexBuffers(cmdBuff, 0, 1, &renderStruct->vertexBuffer->buffer, &offsets);
                        vkCmdDraw(cmdBuff, renderStruct->vertexBuffer->count, 1, 0, 0);
                    }
                    break;

                    case b2Shape::e_polygon:
                    {
                        vkCmdBindPipeline(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS, debugPolygonShader->pipeline);

                        vkCmdBindVertexBuffers(cmdBuff, 0, 1, &renderStruct->vertexBuffer->buffer, &offsets);

                        vkCmdBindDescriptorSets(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS,
                            debugPolygonShader->pipelineLayout, 1, 1, &debugPolygonShader->descriptorSets[1][frameNumber],
                            renderStruct->dynamicOffsets[frameNumber].size(), renderStruct->dynamicOffsets[frameNumber].data());

                        vkCmdDraw(cmdBuff, renderStruct->vertexBuffer->count, 1, 0, 0);
                    }
                    break;
                default:
                    break;
                }
            }
        }
    }

    vkCmdBindDescriptorSets(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS,
        debugLineShader->pipelineLayout, 0, 1, &debugLineShader->descriptorSets[0][frameNumber], 0, VK_NULL_HANDLE);
    vkCmdBindPipeline(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS, debugLineShader->pipeline);
    vkCmdBindVertexBuffers(cmdBuff, 0, 1, &debugCircleShader->buffers[0]->buffer, &offsets);

    s_pushConstantLine pushData;
    //pushData.positions = glm::vec4(1.0f, 1.0f, -1.0f, -1.0f);
    pushData.colour = glm::vec4(1.0f, 1.0f, 0.0f, 1.0f);
    pushData.data.y = 0.02f;
    pushData.data.z = 0.0f;

    for (b2Body* body = pWorld->GetBodyList(); body; body = body->GetNext())
    {
        if (body->GetType() == b2_staticBody)
        {
            for (b2Fixture* fix = body->GetFixtureList(); fix; fix = fix->GetNext())
            {
                if (fix->GetType() == b2Shape::e_edge)
                {
                    b2EdgeShape* edgeShape = (b2EdgeShape*)fix->GetShape();
                    
                    b2Vec2 v1 = body->GetWorldPoint(edgeShape->m_vertex1);
                    b2Vec2 v2 = body->GetWorldPoint(edgeShape->m_vertex2);

                    pushData.positions = glm::vec4(v1.x, v1.y, v2.x, v2.y);

                    vkCmdPushConstants(cmdBuff, debugLineShader->pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                        0, sizeof(s_pushConstantLine), (void*)&pushData);
                    vkCmdDraw(cmdBuff, debugCircleShader->buffers[0]->count, 1, 0, 0);
                }
            }
        }
    }

    pushData.data.y = 0.03f;
    pushData.colour = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
    for (Portal* p : pWorld->portals)
    {
        pushData.positions = glm::vec4(p->points[0].x, p->points[0].y, p->points[1].x, p->points[1].y);

        vkCmdPushConstants(cmdBuff, debugLineShader->pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
            0, sizeof(s_pushConstantLine), (void*)&pushData);
        vkCmdDraw(cmdBuff, debugCircleShader->buffers[0]->count, 1, 0, 0);
    }

    // end

    vkCmdEndRenderPass(cmdBuff);
    CHECK_RESULT_VK(vkEndCommandBuffer(cmdBuff));
}

void ThinDrawer::addPortalBody(PortalBody* portalBody)
{
    s_renderGroup* renderGroup = new s_renderGroup;
    s_renderBody* renderBody = new s_renderBody;
    renderBody->portalBody = portalBody;

    renderGroup->vertexStorageOffsets.resize(portalBody->fixtures.size());
    renderGroup->vertexStorageOffsetsMirrored.resize(portalBody->fixtures.size());
    renderGroup->circlePositionings.resize(portalBody->fixtures.size());
    renderGroup->circlePositioningsMirrored.resize(portalBody->fixtures.size());

    for (int i = 0; i < portalBody->fixtures.size(); i++)
    {
        b2Fixture* fix = portalBody->fixtures.at(i);

        s_renderStruct* renderStruct = new s_renderStruct;
        renderBody->renderFixtures.push_back(renderStruct);
        renderStruct->renderType = fix->GetType();
        renderStruct->fixData = (fixtureData*)fix->GetUserData().pointer;
        renderStruct->dynamicOffsets.resize(swapChain->imageCount);
        renderStruct->vertexBuffer = (s_buffers*)malloc(sizeof(s_buffers));
        s_buffers* mirroredVertices = (s_buffers*)malloc(sizeof(s_buffers));

        renderGroup->vertexBuffers.push_back(renderStruct->vertexBuffer);
        renderGroup->vertexBuffersMirrored.push_back(mirroredVertices);

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
                allData->data.z = 0.0f;
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
                allData->data.z = 0.0f;
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
    renderGroups.push_back(renderGroup);

    portalBody->worldIndex->createCallbacks.push_back(std::make_pair(portalBodyCreateCallback, (void*)renderGroup));
    portalBody->worldIndex->destroyCallbacks.push_back(std::make_pair(portalBodyDestroyCallback, (void*)renderGroup));
}

void ThinDrawer::portalBodyCreateCallback(PortalBody* portalBody, void* data)
{
    extern ThinDrawer* thinDrawer;
    s_renderGroup* renderGroup = (s_renderGroup*)data;
    
    s_renderBody* renderBody = new s_renderBody;
    renderBody->portalBody = portalBody;
    renderGroup->renderBodies.insert(renderBody);
    for (int i = 0; i < portalBody->fixtures.size(); i++)
    {
        b2Fixture* fix = portalBody->fixtures.at(i);

        s_renderStruct* renderStruct = new s_renderStruct;
        renderBody->renderFixtures.push_back(renderStruct);
        renderStruct->dynamicOffsets.resize(thinDrawer->swapChain->imageCount);
        renderStruct->renderType = fix->GetType();
        renderStruct->fixData = (fixtureData*)fix->GetUserData().pointer;
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
                allData->data.z = 0.0f;
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
                allData->data.z = 0.0f;
                allData->extraData.z = -1.0f;
                allData->outerColor = glm::vec4(0.0f, 1.0f, 1.0f, 1.0f);
                allData->modelMatrix = glm::mat4(1.0f);
                vkUnmapMemory(thinDrawer->logicalDevice, ShaderBase::commonStorageBuffers[j][0]->memory);
            }
        }
    }
}

void ThinDrawer::portalBodyDestroyCallback(PortalBody* portalBody, void* data)
{
    extern ThinDrawer* thinDrawer;
    s_renderGroup* renderGroup = (s_renderGroup*)data;

    for (auto iter = renderGroup->renderBodies.begin(); iter != renderGroup->renderBodies.end(); iter++)
    {
        s_renderBody* renderBody = *iter;
        if ((*iter)->portalBody == portalBody)
        {
            renderGroup->renderBodies.erase(iter);

            for (s_renderStruct* renderStruct : renderBody->renderFixtures)
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