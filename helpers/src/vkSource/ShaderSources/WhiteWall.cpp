#include <ShaderHeaders/WhiteWall.h>
#include <SwapChain.h>
#include <ThinDrawer.h>
#include <Camera.h>
#include <Shader.h>
#include <definitions.h>
#include <vkInit.h>

void WhiteWall::prepareVertexData()
{
    const int bufferSize = 1;
    buffers.resize(bufferSize);

    for (int i = 0; i < bufferSize; i++)
    {
        buffers[i] = (s_buffers*)malloc(sizeof(s_buffers));
    }
    s_buffers* vertices = buffers[0];

    std::vector<s_averageVertex> vertexBuffer =
    {
            { {-0.5f, +0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f} },
            { {+0.5f, +0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f} },
            { {+0.5f, -0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f} },
            { {+0.5f, -0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f} },
            { {-0.5f, -0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f} },
            { {-0.5f, +0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f} },
    };

    uint32_t vertexBufferSize = static_cast<uint32_t>(vertexBuffer.size()) * sizeof(s_averageVertex);
    vertices->count = vertexBuffer.size();

    thinDrawer->bufferStage(vertexBuffer.data(), vertexBufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, vertices);
}

void WhiteWall::prepareUniforms()
{
    int imCount = thinDrawer->swapChain->imageCount;
    // offset is not used no need for alignment
    int alignedSizeFS = sizeof(s_wallUbo);
    uniformBuffers.resize(imCount);

    int x = 25;
    int y = 40;
    const int wallCountInMind = 10000;
    wallCount = 0;
    const glm::vec2 size = glm::vec2(1.25f, 2.5f);

    for (int i = 0; i < imCount; i++)
    {
        uniformBuffers[i].resize(1);

        uniformBuffers[i][0] = (s_uniformBuffer*)malloc(sizeof(s_uniformBuffer));

        thinDrawer->uniformHelper(alignedSizeFS * wallCountInMind, uniformBuffers[i][0], VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
    }

    for (int j = 0; j < x; j++)
    {
        for (int k = 0; k < y; k++)
        {
            float randVal11 = (((float)rand() / RAND_MAX) - 0.5f) * 0.4f;
            float randVal22 = (((float)rand() / RAND_MAX)) * 0.1f;
            for (int i = 0; i < imCount; i++)
            {
                s_wallUbo wallUbo;
                wallUbo.transformMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(j * size.x - 40 - 1.25*0.5, k * size.y - (y * size.y + size.y) * 0.5, -size.x));
                wallUbo.rotationMatrix = glm::rotate(glm::mat4(1.0f), glm::radians(0.0f), glm::vec3(0.0f, 1.0f, 0.0));
                wallUbo.bgColour = glm::vec4(0.7f + randVal11, 0.7f + randVal11, 0.7f + randVal11, 1.0f);
                wallUbo.edgeColour = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
                wallUbo.dotColour = glm::vec4(0.2f + randVal11, 0.2f + randVal11, 0.2f + randVal11, 1.0f);
                wallUbo.leftUp = wallUbo.transformMatrix * glm::vec4(-0.5f * size.x, -0.5f * size.y, 0.0f, 1.0f);
                wallUbo.data = glm::vec4(0.1 + randVal22, 0.06f, 2.0f, 0.3f);
                wallUbo.data2 = glm::vec4(size.x, size.y, 0.0f, 0.0f);

                void* pData;
                CHECK_RESULT_VK(vkMapMemory(logicalDevice, uniformBuffers[i][0]->memory, wallCount * alignedSizeFS, alignedSizeFS, 0, (void**)&pData));
                memcpy(pData, &wallUbo, alignedSizeFS);
                vkUnmapMemory(logicalDevice, uniformBuffers[i][0]->memory);
            }
            wallCount++;
        }
    }

    for (int j = 0; j < x; j++)
    {
        for (int k = 0; k < y; k++)
        {
            float randVal11 = (((float)rand() / RAND_MAX) - 0.5f) * 0.4f;
            float randVal22 = (((float)rand() / RAND_MAX)) * 0.1f;
            for (int i = 0; i < imCount; i++)
            {
                s_wallUbo wallUbo;
                wallUbo.transformMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(j * size.x + 41.25 - size.x * 0.5, k * size.y - (y * size.y + size.y) * 0.5, -size.x));
                wallUbo.rotationMatrix = glm::rotate(glm::mat4(1.0f), glm::radians(0.0f), glm::vec3(0.0f, 1.0f, 0.0));
                wallUbo.bgColour = glm::vec4(0.7f + randVal11, 0.7f + randVal11, 0.7f + randVal11, 1.0f);
                wallUbo.edgeColour = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
                wallUbo.dotColour = glm::vec4(0.2f + randVal11, 0.2f + randVal11, 0.2f + randVal11, 1.0f);
                wallUbo.leftUp = wallUbo.transformMatrix * glm::vec4(-0.5f * size.x, -0.5f * size.y, 0.0f, 1.0f);
                wallUbo.data = glm::vec4(0.1f + randVal22, 0.06f, 2.0f, 1.0f);
                wallUbo.data2 = glm::vec4(size.x, size.y, 0.0f, 0.0f);

                void* pData;
                CHECK_RESULT_VK(vkMapMemory(logicalDevice, uniformBuffers[i][0]->memory, wallCount * alignedSizeFS, alignedSizeFS, 0, (void**)&pData));
                memcpy(pData, &wallUbo, sizeof(s_wallUbo));
                vkUnmapMemory(logicalDevice, uniformBuffers[i][0]->memory);
            }
            wallCount++;
        }
    }
        
    x = 50 / 1.25;
    y = (y - 8) / 2;
    for (int j = 0; j < x; j++)
    {
        for (int k = 0; k < y; k++)
        {
            float randVal11 = (((float)rand() / RAND_MAX) - 0.5f) * 0.4f;
            float randVal22 = (((float)rand() / RAND_MAX)) * 0.1f;
            for (int i = 0; i < imCount; i++)
            {
                s_wallUbo wallUbo;
                wallUbo.transformMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(j * size.x - 8.75 - 1.25*0.5, k * size.y + 10 + size.y * 0.5, -size.x));
                wallUbo.rotationMatrix = glm::rotate(glm::mat4(1.0f), glm::radians(0.0f), glm::vec3(0.0f, 1.0f, 0.0));
                wallUbo.bgColour = glm::vec4(0.7f + randVal11, 0.7f + randVal11, 0.7f + randVal11, 1.0f);
                wallUbo.edgeColour = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
                wallUbo.dotColour = glm::vec4(0.2f + randVal11, 0.2f + randVal11, 0.2f + randVal11, 1.0f);
                wallUbo.leftUp = wallUbo.transformMatrix * glm::vec4(-0.5f * size.x, -0.5f * size.y, 0.0f, 1.0f);
                wallUbo.data = glm::vec4(0.1f + randVal22, 0.06f, 2.0f, 1.0f);
                wallUbo.data2 = glm::vec4(size.x, size.y, 0.0f, 0.0f);

                void* pData;
                CHECK_RESULT_VK(vkMapMemory(logicalDevice, uniformBuffers[i][0]->memory, wallCount * alignedSizeFS, alignedSizeFS, 0, (void**)&pData));
                memcpy(pData, &wallUbo, sizeof(s_wallUbo));
                vkUnmapMemory(logicalDevice, uniformBuffers[i][0]->memory);
            }
            wallCount++;
        }
    }

    for (int j = 0; j < x; j++)
    {
        for (int k = 0; k < y; k++)
        {
            float randVal11 = (((float)rand() / RAND_MAX) - 0.5f) * 0.4f;
            float randVal22 = (((float)rand() / RAND_MAX)) * 0.1f;
            for (int i = 0; i < imCount; i++)
            {
                s_wallUbo wallUbo;
                wallUbo.transformMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(j * size.x - 8.75 - 1.25 * 0.5, k * size.y - size.y * y - 8.75, -size.x));
                wallUbo.rotationMatrix = glm::rotate(glm::mat4(1.0f), glm::radians(0.0f), glm::vec3(0.0f, 1.0f, 0.0));
                wallUbo.bgColour = glm::vec4(0.7f + randVal11, 0.7f + randVal11, 0.7f + randVal11, 1.0f);
                wallUbo.edgeColour = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
                wallUbo.dotColour = glm::vec4(0.2f + randVal11, 0.2f + randVal11, 0.2f + randVal11, 1.0f);
                wallUbo.leftUp = wallUbo.transformMatrix * glm::vec4(-0.5f * size.x, -0.5f * size.y, 0.0f, 1.0f);
                wallUbo.data = glm::vec4(0.1f + randVal22, 0.06f, 2.0f, 1.0f);
                wallUbo.data2 = glm::vec4(size.x, size.y, 0.0f, 0.0f);

                void* pData;
                CHECK_RESULT_VK(vkMapMemory(logicalDevice, uniformBuffers[i][0]->memory, wallCount * alignedSizeFS, alignedSizeFS, 0, (void**)&pData));
                memcpy(pData, &wallUbo, sizeof(s_wallUbo));
                vkUnmapMemory(logicalDevice, uniformBuffers[i][0]->memory);
            }
            wallCount++;
        }
    }
}

void WhiteWall::setupDescriptorSetLayout()
{
    const int descriptorSetLayoutCount = 3;

    descriptorSetLayouts.resize(descriptorSetLayoutCount);

    VkDescriptorSetLayoutBinding layoutBinding0 = vkinit::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 1, 0);
    VkDescriptorSetLayoutBinding layoutBinding10 = vkinit::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
        VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 1, 0);

    VkDescriptorSetLayoutBinding bindings1[] = { layoutBinding0 };
    VkDescriptorSetLayoutBinding bindings2[] = { layoutBinding10 };

    VkDescriptorSetLayoutCreateInfo descriptorLayout = vkinit::descriptorSetLayoutCreateInfo(bindings1, sizeof(bindings1) / sizeof(bindings1[0]));
    CHECK_RESULT_VK(vkCreateDescriptorSetLayout(logicalDevice, &descriptorLayout, VK_NULL_HANDLE, &descriptorSetLayouts[0]));

    VkDescriptorSetLayoutCreateInfo descriptorLayout2 = vkinit::descriptorSetLayoutCreateInfo(bindings2, sizeof(bindings2) / sizeof(bindings2[0]));
    CHECK_RESULT_VK(vkCreateDescriptorSetLayout(logicalDevice, &descriptorLayout2, VK_NULL_HANDLE, &descriptorSetLayouts[1]));

    VkDescriptorSetLayoutBinding textureBind = vkinit::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1, 0);
    VkDescriptorSetLayoutBinding textureBind2 = vkinit::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1, 1);

    VkDescriptorSetLayoutBinding texBindings[2] = { textureBind, textureBind2 };

    VkDescriptorSetLayoutCreateInfo setInfo = vkinit::descriptorSetLayoutCreateInfo(texBindings, std::size(texBindings));
    CHECK_RESULT_VK(vkCreateDescriptorSetLayout(logicalDevice, &setInfo, VK_NULL_HANDLE, &descriptorSetLayouts[2]));

    VkPipelineLayoutCreateInfo pPipelineLayoutCreateInfo = vkinit::pipelineLayoutCreateInfo(descriptorSetLayouts.data(), descriptorSetLayouts.size());
    CHECK_RESULT_VK(vkCreatePipelineLayout(logicalDevice, &pPipelineLayoutCreateInfo, VK_NULL_HANDLE, &pipelineLayout));
}

void WhiteWall::preparePipeline()
{
    VkGraphicsPipelineCreateInfo* pipelineCreateInfo = thinDrawer->getPipelineInfoBase();
    pipelineCreateInfo->layout = pipelineLayout;

    VkVertexInputBindingDescription vertexInputBinding = { };
    vertexInputBinding.binding = 0;
    vertexInputBinding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    vertexInputBinding.stride = sizeof(s_averageVertex);

    VkVertexInputAttributeDescription attributeDescriptions[3];
    s_averageVertex::getVertexAttributeDescriptions(attributeDescriptions);

    VkPipelineVertexInputStateCreateInfo vertexInputState = { };
    vertexInputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputState.vertexBindingDescriptionCount = 1;
    vertexInputState.vertexAttributeDescriptionCount = std::size(attributeDescriptions);
    vertexInputState.pVertexAttributeDescriptions = attributeDescriptions;
    vertexInputState.pVertexBindingDescriptions = &vertexInputBinding;

    std::vector<std::string> fileNames =
    {
            std::string("shaders/Wall/vertex_shader.vert.spv"),
            std::string("shaders/Wall/fragment_shader.frag.spv")
    };

    Shader shader2 = Shader(logicalDevice, fileNames);

    pipelineCreateInfo->stageCount = shader2.shaderStages.size();
    pipelineCreateInfo->pStages = shader2.shaderStages.data();
    pipelineCreateInfo->pVertexInputState = &vertexInputState;

    CHECK_RESULT_VK(vkCreateGraphicsPipelines(logicalDevice, VK_NULL_HANDLE, 1, pipelineCreateInfo, VK_NULL_HANDLE, &pipeline));

    ThinDrawer::freePipelineData(pipelineCreateInfo);
}

void WhiteWall::setupDescriptorSet()
{
    s_texture* tex = (s_texture*)malloc(sizeof(s_texture));
    thinDrawer->loadTexture((char*)"textures/dots.png", tex);
    textureData.push_back(tex);

    s_texture* tex2 = (s_texture*)malloc(sizeof(s_texture));
    thinDrawer->loadTexture((char*)"textures/grad2.png", tex2);
    textureData.push_back(tex2);
 
    int bindingCount = 2;

    int imCount = thinDrawer->swapChain->imageCount;

    descriptorSets.resize(descriptorSetLayouts.size());
    for (int i = 0; i < descriptorSets.size(); i++)
    {
        descriptorSets[i].resize(imCount);
    }

    for (int i = 0; i < imCount; i++)
    {
        VkDescriptorSetAllocateInfo allocInfo = vkinit::descriptorSetAllocateInfo(thinDrawer->descriptorPool, &descriptorSetLayouts[0], 1);
        CHECK_RESULT_VK(vkAllocateDescriptorSets(logicalDevice, &allocInfo, &descriptorSets[0][i]));

        VkWriteDescriptorSet writeDescriptorSet = { };

        writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writeDescriptorSet.dstSet = descriptorSets[0][i];
        writeDescriptorSet.descriptorCount = 1;
        writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;

        writeDescriptorSet.pBufferInfo = &thinDrawer->sharedOrthoUniform[i]->descriptor;
        writeDescriptorSet.dstBinding = 0;
        vkUpdateDescriptorSets(logicalDevice, 1, &writeDescriptorSet, 0, VK_NULL_HANDLE);

        allocInfo = vkinit::descriptorSetAllocateInfo(thinDrawer->descriptorPool, &descriptorSetLayouts[1], 1);
        CHECK_RESULT_VK(vkAllocateDescriptorSets(logicalDevice, &allocInfo, &descriptorSets[1][i]));
        writeDescriptorSet.dstSet = descriptorSets[1][i];
        writeDescriptorSet.pBufferInfo = &uniformBuffers[i][0]->descriptor;
        writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;

        vkUpdateDescriptorSets(logicalDevice, 1, &writeDescriptorSet, 0, VK_NULL_HANDLE);
    }

    for (int j = 0; j < textureData.size(); j++)
    {
        VkDescriptorSetAllocateInfo allocInfo = vkinit::descriptorSetAllocateInfo(thinDrawer->descriptorPool, &descriptorSetLayouts[2], 1);
        vkAllocateDescriptorSets(logicalDevice, &allocInfo, &textureData[j]->set);

        VkDescriptorImageInfo imageBufferInfo;
        imageBufferInfo.imageView = textureData[j]->view;
        imageBufferInfo.sampler = textureData[j]->sampler;
        imageBufferInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        VkWriteDescriptorSet textureWrite = { };
        textureWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        textureWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        textureWrite.dstSet = textureData[0]->set;
        textureWrite.descriptorCount = 1;
        textureWrite.pImageInfo = &imageBufferInfo;
        textureWrite.dstBinding = j;

        vkUpdateDescriptorSets(logicalDevice, 1, &textureWrite, 0, VK_NULL_HANDLE);
    }
}