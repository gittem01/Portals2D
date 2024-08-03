#include <ShaderHeaders/WallLight.h>
#include <ThinDrawer.h>
#include <Camera.h>
#include <Shader.h>
#include <definitions.h>
#include <vkInit.h>

void WallLight::prepareUniforms()
{
    alignedSize = sizeof(s_walLightlUbo);

    uniformBuffers.resize(1);

    thinDrawer->uniformHelper(alignedSize * 10000, &uniformBuffers[0], VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
}

void WallLight::testFill()
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

    int num = 40;
    float v = 0.0;
    for (int j = 0; j < num; j++)
    {
        float ang = 0.5 * b2_pi;
        glm::vec2 pos = glm::vec2(j * 2.5f - 71.25 * 0.5, 15.0);

        s_gridPositionShader gridPos = getGridSquare(pos.x, pos.y);
        s_shaderGridData* gData = &renderGridUp[1][gridPos.y][gridPos.x];
        gData->numElements++;

        s_walLightlUbo* pData;
        vkMapMemory(logicalDevice, uniformBuffers[0].memory, gData->offsetEnd * alignedSize, alignedSize, 0, (void**)&pData);
        pData->color = glm::vec4(1.0, 1.0, 1.0, 1.0);
        pData->posSize = glm::vec4(pos.x, pos.y, 5.0, 1.25);
        pData->data = glm::vec4(4.0, 0.8, 0.3, -2.0);
        pData->data2 = glm::vec4(ang, 0.0, 1.0, 1.0);
        vkUnmapMemory(logicalDevice, uniformBuffers[0].memory);
        gData->offsetEnd++;
    }


    s_gridPositionShader gridPos = getGridSquare(0.0f, 30.0f);
    s_shaderGridData* gData = &renderGridUp[1][gridPos.y][gridPos.x];
    gData->numElements++;

    s_walLightlUbo* pData;
    vkMapMemory(logicalDevice, uniformBuffers[0].memory, gData->offsetEnd * alignedSize, alignedSize, 0, (void**)&pData);
    pData->color = glm::vec4(1.0, 1.0, 5.0, 1.0);
    pData->posSize = glm::vec4(0.0, 30.0, 20.0, 20.0);
    pData->data = glm::vec4(20.0, 20.0, 1.5, -10.0);
    pData->data2 = glm::vec4(0.0, 1.0, 1.0, 5.0);
    vkUnmapMemory(logicalDevice, uniformBuffers[0].memory);
    gData->offsetEnd++;

    gridPos = getGridSquare(9.75f, -3.75f);
    gData = &renderGridUp[1][gridPos.y][gridPos.x];
    gData->numElements++;

    vkMapMemory(logicalDevice, uniformBuffers[0].memory, gData->offsetEnd * alignedSize, alignedSize, 0, (void**)&pData);
    pData->color = glm::vec4(1.0, 2.0, 1.0, 1.0);
    pData->posSize = glm::vec4(9.75, -3.75, 12.0, 3.0);
    pData->data = glm::vec4(11.0, 1.5, 0.4, -5.0);
    pData->data2 = glm::vec4(0.0, 0.0, 0.8, 1.0);
    vkUnmapMemory(logicalDevice, uniformBuffers[0].memory);
    gData->offsetEnd++;
}

void WallLight::setupDescriptorSetLayout()
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

void WallLight::preparePipeline()
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
            std::string("shaders/WallLight/vertex_shader.vert.spv"),
            std::string("shaders/WallLight/fragment_shader.frag.spv")
    };

    Shader shader2 = Shader(logicalDevice, fileNames);

    pipelineCreateInfo->stageCount = shader2.shaderStages.size();
    pipelineCreateInfo->pStages = shader2.shaderStages.data();
    pipelineCreateInfo->pVertexInputState = &vertexInputState;
    ((VkPipelineColorBlendAttachmentState*)pipelineCreateInfo->pColorBlendState->pAttachments)->blendEnable = false;

    CHECK_RESULT_VK(vkCreateGraphicsPipelines(logicalDevice, VK_NULL_HANDLE, 1, pipelineCreateInfo, VK_NULL_HANDLE, &pipeline));

    ThinDrawer::freePipelineData(pipelineCreateInfo);
}

void WallLight::setupDescriptorSet()
{
    descriptorSets.resize(descriptorSetLayouts.size());

    VkDescriptorSetAllocateInfo allocInfo;

    VkWriteDescriptorSet writeDescriptorSet = { };

    writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet.descriptorCount = 1;
    writeDescriptorSet.dstBinding = 0;

    allocInfo = vkinit::descriptorSetAllocateInfo(thinDrawer->descriptorPool, &descriptorSetLayouts[1], 1);
    CHECK_RESULT_VK(vkAllocateDescriptorSets(logicalDevice, &allocInfo, &descriptorSets[1]));
    writeDescriptorSet.dstSet = descriptorSets[1];
    writeDescriptorSet.pBufferInfo = &uniformBuffers[0].descriptor;
    writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;

    vkUpdateDescriptorSets(logicalDevice, 1, &writeDescriptorSet, 0, VK_NULL_HANDLE);
}

void WallLight::recreateUniforms(int size)
{
    vkDeviceWaitIdle(logicalDevice);

    vkDestroyBuffer(logicalDevice, uniformBuffers[0].buffer, VK_NULL_HANDLE);
    vkFreeMemory(logicalDevice, uniformBuffers[0].memory, VK_NULL_HANDLE);

    thinDrawer->uniformHelper(sizeof(s_walLightlUbo) * size, &uniformBuffers[0], VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);

    VkWriteDescriptorSet writeDescriptorSet = { };
    writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet.descriptorCount = 1;
    writeDescriptorSet.dstBinding = 0;
    writeDescriptorSet.dstSet = descriptorSets[1];
    writeDescriptorSet.pBufferInfo = &uniformBuffers[0].descriptor;
    writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;

    vkUpdateDescriptorSets(logicalDevice, 1, &writeDescriptorSet, 0, VK_NULL_HANDLE);
}