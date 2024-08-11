#include <ShaderHeaders/WallGel.h>
#include <ThinDrawer.h>
#include <Camera.h>
#include <Shader.h>
#include <definitions.h>
#include <vkInit.h>
#include "ShaderHeaders/PlatformRectangle.h"
#include "SwapChain.h"

void WallGel::prepareVertexData()
{

}

void WallGel::prepareUniforms()
{
    uniformBuffers.resize(2);

    thinDrawer->uniformHelper(sizeof(s_wallGelData) * 16000 + 4 * MAX_PLATFORMS * sizeof(s_wallGelData) * thinDrawer->swapChain->imageCount,
        &uniformBuffers[0], VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
    thinDrawer->uniformHelper(sizeof(s_wallPerGelData) * 160000 + 200 * MAX_PLATFORMS * sizeof(s_wallPerGelData),
        &uniformBuffers[1], VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
}

void WallGel::setupDescriptorSetLayout()
{
    const int descriptorSetLayoutCount = 2;

    descriptorSetLayouts.resize(descriptorSetLayoutCount);
    descriptorSetLayouts[0] = ShaderBase::sharedLayout;

    VkDescriptorSetLayoutBinding layoutBinding10 = vkinit::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
        VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 1, 0);

    VkDescriptorSetLayoutBinding layoutBinding11 = vkinit::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
        VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 1, 1);

    VkDescriptorSetLayoutBinding bindings2[] = { layoutBinding10, layoutBinding11 };

    VkDescriptorSetLayoutCreateInfo descriptorLayout2 = vkinit::descriptorSetLayoutCreateInfo(bindings2, sizeof(bindings2) / sizeof(bindings2[0]));
    CHECK_RESULT_VK(vkCreateDescriptorSetLayout(logicalDevice, &descriptorLayout2, VK_NULL_HANDLE, &descriptorSetLayouts[1]));

    VkPipelineLayoutCreateInfo pPipelineLayoutCreateInfo = vkinit::pipelineLayoutCreateInfo(descriptorSetLayouts.data(), descriptorSetLayouts.size());

    CHECK_RESULT_VK(vkCreatePipelineLayout(logicalDevice, &pPipelineLayoutCreateInfo, VK_NULL_HANDLE, &pipelineLayout));
}

void WallGel::preparePipeline()
{
    VkGraphicsPipelineCreateInfo* pipelineCreateInfo = thinDrawer->getPipelineInfoBase();
    pipelineCreateInfo->layout = pipelineLayout;

    VkVertexInputBindingDescription vertexInputBinding = { };
    vertexInputBinding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    VkPipelineVertexInputStateCreateInfo vertexInputState = { };
    vertexInputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

    std::vector<std::string> fileNames =
    {
            std::string("shaders/WallGel/vertex_shader.vert.spv"),
            std::string("shaders/WallGel/fragment_shader.frag.spv")
    };

    Shader shader2 = Shader(logicalDevice, fileNames);

    pipelineCreateInfo->stageCount = shader2.shaderStages.size();
    pipelineCreateInfo->pStages = shader2.shaderStages.data();
    pipelineCreateInfo->pVertexInputState = &vertexInputState;

    CHECK_RESULT_VK(vkCreateGraphicsPipelines(logicalDevice, VK_NULL_HANDLE, 1, pipelineCreateInfo, VK_NULL_HANDLE, &pipeline));

    ThinDrawer::freePipelineData(pipelineCreateInfo);
}

void WallGel::setupDescriptorSet()
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

    writeDescriptorSet.pBufferInfo = &uniformBuffers[1].descriptor;
    writeDescriptorSet.dstBinding = 1;
    vkUpdateDescriptorSets(logicalDevice, 1, &writeDescriptorSet, 0, VK_NULL_HANDLE);
}