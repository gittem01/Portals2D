#include <ShaderHeaders/BlackWall.h>
#include <ThinDrawer.h>
#include <Camera.h>
#include <Shader.h>
#include <definitions.h>
#include <vkInit.h>

void BlackWall::prepareUniforms()
{
    // offset is not used no need for alignment
    int alignedSizeFS = sizeof(s_wallUbo_b);
    uniformBuffers.resize(1);

    uniformBuffers.resize(1);

    uniformBuffers[0] = (s_uniformBuffer*)malloc(sizeof(s_uniformBuffer));

    thinDrawer->uniformHelper(alignedSizeFS * 1, uniformBuffers[0], VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
}

void BlackWall::testFill()
{
    renderGridUp.resize(2);

    int offs = 0;
    for (int l = 0; l < renderGridUp.size(); l++)
    {
        renderGridUp[l].resize(gridCount);
        for (int i = 0; i < gridCount; i++)
        {
            renderGridUp[l][i].resize(gridCount);
            for (int j = 0; j < gridCount; j++)
            {
                renderGridUp[l][i][j].numElements = 0;
                renderGridUp[l][i][j].offsetStart = offs;
                renderGridUp[l][i][j].offsetEnd = offs;
                offs += gridDivider * gridDivider;
            }
        }
    }

    recreateUniforms(2000000);

    int alignedSizeFS = sizeof(s_wallUbo_b);

    const float sz = 1.5f;
    for (int i = -60; i < 60; i++)
    {
        for (int j = -60; j < 60; j++)
        {
            s_gridPositionShader gridPos = getGridSquare(i * sz, j * sz);
            s_shaderGridData* gData = &renderGridUp[0][gridPos.y][gridPos.x];
            gData->numElements++;

            float rndVal = ((double)rand() / RAND_MAX + 2.0) / 3.0;
            s_wallUbo_b* wallUbo;
            vkMapMemory(logicalDevice, uniformBuffers[0]->memory, gData->offsetEnd * alignedSizeFS, alignedSizeFS, 0, (void**)&wallUbo);
            wallUbo->posSize = glm::vec4(i * sz, j * sz, sz, sz);
            wallUbo->colour = glm::vec4(0.03, 0.03, 0.03, 1.0) * rndVal;
            wallUbo->data = glm::vec4(rand() > RAND_MAX / 2, 0.04, 0.8, 0.0);
            vkUnmapMemory(logicalDevice, uniformBuffers[0]->memory);
            gData->offsetEnd++;
        }
    }

    const float sz2 = 2.0f;
    for (int i = 0; i < 5; i++)
    {
        for (int j = 0; j < 2; j++)
        {
            s_gridPositionShader gridPos = getGridSquare(i * sz2 + -4.0f, j * sz2 + 4.0f);
            s_shaderGridData* gData = &renderGridUp[1][gridPos.y][gridPos.x];
            gData->numElements++;

            float rndVal = ((double)rand() / RAND_MAX + 2.0) / 3.0;
            s_wallUbo_b* wallUbo;
            vkMapMemory(logicalDevice, uniformBuffers[0]->memory, gData->offsetEnd * alignedSizeFS, alignedSizeFS, 0, (void**)&wallUbo);
            wallUbo->posSize = glm::vec4(i * sz2 + -4.0, j * sz2 + 4.0, sz2, sz2);
            wallUbo->colour = glm::vec4(0.2, 0.2, 0.2, 1.0) * rndVal;
            wallUbo->data = glm::vec4(rand() > RAND_MAX / 2, 0.04, 1.0, 0.0);
            vkUnmapMemory(logicalDevice, uniformBuffers[0]->memory);
            gData->offsetEnd++;
        }
    }
}

void BlackWall::setupDescriptorSetLayout()
{
    const int descriptorSetLayoutCount = 2;

    descriptorSetLayouts.resize(descriptorSetLayoutCount);
    descriptorSetLayouts[0] = ShaderBase::sharedLayout;

    VkDescriptorSetLayoutBinding layoutBinding10 = vkinit::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
        VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 1, 0);

    VkDescriptorSetLayoutBinding bindings2[] = { layoutBinding10 };

    VkDescriptorSetLayoutCreateInfo descriptorLayout2 = vkinit::descriptorSetLayoutCreateInfo(bindings2, sizeof(bindings2) / sizeof(bindings2[0]));
    CHECK_RESULT_VK(vkCreateDescriptorSetLayout(logicalDevice, &descriptorLayout2, VK_NULL_HANDLE, &descriptorSetLayouts[1]));

    VkPipelineLayoutCreateInfo pPipelineLayoutCreateInfo = vkinit::pipelineLayoutCreateInfo(descriptorSetLayouts.data(), descriptorSetLayouts.size());
    CHECK_RESULT_VK(vkCreatePipelineLayout(logicalDevice, &pPipelineLayoutCreateInfo, VK_NULL_HANDLE, &pipelineLayout));
}

void BlackWall::preparePipeline()
{
    VkGraphicsPipelineCreateInfo* pipelineCreateInfo = thinDrawer->getPipelineInfoBase();
    pipelineCreateInfo->layout = pipelineLayout;

    VkVertexInputBindingDescription vertexInputBinding = { };
    vertexInputBinding.binding = 0;
    vertexInputBinding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    vertexInputBinding.stride = sizeof(s_basicVertex);

    VkVertexInputAttributeDescription attributeDescriptions[1];
    attributeDescriptions[0].binding = 0;
    attributeDescriptions[0].location = 0;
    attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
    attributeDescriptions[0].offset = 0;

    VkPipelineVertexInputStateCreateInfo vertexInputState = { };
    vertexInputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputState.vertexBindingDescriptionCount = 1;
    vertexInputState.vertexAttributeDescriptionCount = std::size(attributeDescriptions);
    vertexInputState.pVertexAttributeDescriptions = attributeDescriptions;
    vertexInputState.pVertexBindingDescriptions = &vertexInputBinding;

    std::vector<std::string> fileNames =
    {
            std::string("shaders/BlackWall/vertex_shader.vert.spv"),
            std::string("shaders/BlackWall/fragment_shader.frag.spv")
    };

    Shader shader2 = Shader(logicalDevice, fileNames);

    pipelineCreateInfo->stageCount = shader2.shaderStages.size();
    pipelineCreateInfo->pStages = shader2.shaderStages.data();
    pipelineCreateInfo->pVertexInputState = &vertexInputState;
    ((VkPipelineColorBlendAttachmentState*)pipelineCreateInfo->pColorBlendState->pAttachments)->blendEnable = false;

    CHECK_RESULT_VK(vkCreateGraphicsPipelines(logicalDevice, VK_NULL_HANDLE, 1, pipelineCreateInfo, VK_NULL_HANDLE, &pipeline));

    ThinDrawer::freePipelineData(pipelineCreateInfo);
}

void BlackWall::setupDescriptorSet()
{
    int bindingCount = 2;

    descriptorSets.resize(descriptorSetLayouts.size());

    VkDescriptorSetAllocateInfo allocInfo;

    VkWriteDescriptorSet writeDescriptorSet = { };
    writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet.descriptorCount = 1;
    writeDescriptorSet.dstBinding = 0;

    allocInfo = vkinit::descriptorSetAllocateInfo(thinDrawer->descriptorPool, &descriptorSetLayouts[1], 1);
    CHECK_RESULT_VK(vkAllocateDescriptorSets(logicalDevice, &allocInfo, &descriptorSets[1]));
    writeDescriptorSet.dstSet = descriptorSets[1];
    writeDescriptorSet.pBufferInfo = &uniformBuffers[0]->descriptor;
    writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;

    vkUpdateDescriptorSets(logicalDevice, 1, &writeDescriptorSet, 0, VK_NULL_HANDLE);
}

void BlackWall::recreateUniforms(int size)
{
    vkDeviceWaitIdle(logicalDevice);

    vkDestroyBuffer(logicalDevice, uniformBuffers[0]->buffer, VK_NULL_HANDLE);
    vkFreeMemory(logicalDevice, uniformBuffers[0]->memory, VK_NULL_HANDLE);

    thinDrawer->uniformHelper(sizeof(s_wallUbo_w) * size, uniformBuffers[0], VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);

    VkWriteDescriptorSet writeDescriptorSet = { };
    writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet.descriptorCount = 1;
    writeDescriptorSet.dstBinding = 0;
    writeDescriptorSet.dstSet = descriptorSets[1];
    writeDescriptorSet.pBufferInfo = &uniformBuffers[0]->descriptor;
    writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;

    vkUpdateDescriptorSets(logicalDevice, 1, &writeDescriptorSet, 0, VK_NULL_HANDLE);
}