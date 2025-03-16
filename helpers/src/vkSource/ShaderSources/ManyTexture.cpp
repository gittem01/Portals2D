#include <ShaderHeaders/ManyTexture.h>
#include <ThinDrawer.h>
#include <Camera.h>
#include <Shader.h>
#include <definitions.h>
#include <vkInit.h>
#include <SwapChain.h>

void ManyTexture::prepareVertexData()
{

}

void ManyTexture::prepareUniforms()
{
    uniformBuffers.resize(1);

    thinDrawer->uniformHelper(sizeof(s_manyTextureData) * 16384 * thinDrawer->swapChain->imageCount, &uniformBuffers[0], VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
}

void ManyTexture::setupDescriptorSetLayout()
{
    const int descriptorSetLayoutCount = 3;

    descriptorSetLayouts.resize(descriptorSetLayoutCount);
    descriptorSetLayouts[0] = ShaderBase::sharedLayout;

    VkDescriptorSetLayoutBinding layoutBinding10 = vkinit::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
        VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 1, 0);

    VkDescriptorSetLayoutBinding textureBind = vkinit::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1, 0);

    VkDescriptorSetLayoutBinding bindings2[] = { layoutBinding10 };
    VkDescriptorSetLayoutBinding texBindings[] = { textureBind };

    VkDescriptorSetLayoutCreateInfo descriptorLayout2 = vkinit::descriptorSetLayoutCreateInfo(bindings2, sizeof(bindings2) / sizeof(bindings2[0]));
    CHECK_RESULT_VK(vkCreateDescriptorSetLayout(logicalDevice, &descriptorLayout2, VK_NULL_HANDLE, &descriptorSetLayouts[1]));

    VkDescriptorSetLayoutCreateInfo setInfo = vkinit::descriptorSetLayoutCreateInfo(texBindings, std::size(texBindings));
    CHECK_RESULT_VK(vkCreateDescriptorSetLayout(logicalDevice, &setInfo, VK_NULL_HANDLE, &descriptorSetLayouts[2]));

    VkPipelineLayoutCreateInfo pPipelineLayoutCreateInfo = vkinit::pipelineLayoutCreateInfo(descriptorSetLayouts.data(), descriptorSetLayouts.size());
    CHECK_RESULT_VK(vkCreatePipelineLayout(logicalDevice, &pPipelineLayoutCreateInfo, VK_NULL_HANDLE, &pipelineLayout));
}

void ManyTexture::preparePipeline()
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
            std::string("shaders/ManyTexture/vertex_shader.vert.spv"),
            std::string("shaders/ManyTexture/fragment_shader.frag.spv")
    };

    Shader shader2 = Shader(logicalDevice, fileNames);

    pipelineCreateInfo->stageCount = shader2.shaderStages.size();
    pipelineCreateInfo->pStages = shader2.shaderStages.data();
    pipelineCreateInfo->pVertexInputState = &vertexInputState;

    CHECK_RESULT_VK(vkCreateGraphicsPipelines(logicalDevice, VK_NULL_HANDLE, 1, pipelineCreateInfo, VK_NULL_HANDLE, &pipeline));

    ThinDrawer::freePipelineData(pipelineCreateInfo);
}

void ManyTexture::setupDescriptorSet()
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