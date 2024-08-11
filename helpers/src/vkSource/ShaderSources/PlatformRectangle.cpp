#include <ShaderHeaders/PlatformRectangle.h>
#include <ThinDrawer.h>
#include <Camera.h>
#include <Shader.h>
#include <definitions.h>
#include <vkInit.h>
#include "SwapChain.h"

void PlatformRectangle::prepareVertexData()
{

}

void PlatformRectangle::prepareUniforms()
{
    uniformBuffers.resize(1);

    thinDrawer->uniformHelper(sizeof(s_platformRectUbo) * MAX_PLATFORMS * thinDrawer->swapChain->imageCount, &uniformBuffers[0], VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);

    s_platformRectUbo* platformUbo;
    vkMapMemory(logicalDevice, uniformBuffers[0].memory, 0, sizeof(s_platformRectUbo), 0, (void**)&platformUbo);
    platformUbo->posSize = glm::vec4(0.0, 0.0, 2.0, 0.5);
    platformUbo->inColour = glm::vec4(0.0, 0.0, 0.0, 0.8);
    platformUbo->outColour = glm::vec4(1.0, 1.0, 1.0, 1.0);
    platformUbo->data = glm::vec4(0.15, 0.15, 1.0, 1.0);
    vkUnmapMemory(logicalDevice, uniformBuffers[0].memory);
}

void PlatformRectangle::setupDescriptorSetLayout()
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

void PlatformRectangle::preparePipeline()
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
            std::string("shaders/PlatformRectangle/vertex_shader.vert.spv"),
            std::string("shaders/PlatformRectangle/fragment_shader.frag.spv")
    };

    Shader shader2 = Shader(logicalDevice, fileNames);

    pipelineCreateInfo->stageCount = shader2.shaderStages.size();
    pipelineCreateInfo->pStages = shader2.shaderStages.data();
    pipelineCreateInfo->pVertexInputState = &vertexInputState;

    CHECK_RESULT_VK(vkCreateGraphicsPipelines(logicalDevice, VK_NULL_HANDLE, 1, pipelineCreateInfo, VK_NULL_HANDLE, &pipeline));

    ThinDrawer::freePipelineData(pipelineCreateInfo);
}

void PlatformRectangle::setupDescriptorSet()
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