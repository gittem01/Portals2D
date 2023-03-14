#include <ShaderHeaders/DebugLine.h>
#include <SwapChain.h>
#include <ThinDrawer.h>
#include <Camera.h>
#include <Shader.h>
#include <definitions.h>
#include <vkInit.h>

void DebugLine::prepareVertexData()
{
    const int bufferSize = 0;
    buffers.resize(bufferSize);
}

void DebugLine::prepareUniforms()
{
    int imCount = thinDrawer->swapChain->imageCount;
    int alignedSizeFS = thinDrawer->pad_storage_buffer_size(sizeof(s_uboAll));

    for (int i = 0; i < imCount; i++)
    {
        s_uboAll uboFSCircle;
        uboFSCircle.modelMatrix = glm::mat4(1.0f);
        uboFSCircle.color = glm::vec4(0.0f, 1.0f, 1.0f, 1.0f);
        uboFSCircle.data.y = 0.02f;
        uboFSCircle.data.z = 0.0f;

        uboFSCircle.portals[0] = glm::vec4(+2.0f, -1.0f, -1.0f, +1.0f);

        void* pData;
        CHECK_RESULT_VK(vkMapMemory(logicalDevice, commonStorageBuffers[i][0]->memory, 76 * alignedSizeFS, alignedSizeFS, 0, (void**)&pData));
        memcpy(pData, &uboFSCircle, alignedSizeFS);
        vkUnmapMemory(logicalDevice, commonStorageBuffers[i][0]->memory);
    }
}

void DebugLine::setupDescriptorSetLayout()
{
    const int descriptorSetLayoutCount = 1;

    descriptorSetLayouts.resize(descriptorSetLayoutCount);

    VkDescriptorSetLayoutBinding layoutBinding0 = vkinit::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 1, 0);

    VkDescriptorSetLayoutBinding bindings1[] = { layoutBinding0 };

    VkDescriptorSetLayoutCreateInfo descriptorLayout = vkinit::descriptorSetLayoutCreateInfo(bindings1, sizeof(bindings1) / sizeof(bindings1[0]));
    CHECK_RESULT_VK(vkCreateDescriptorSetLayout(logicalDevice, &descriptorLayout, VK_NULL_HANDLE, &descriptorSetLayouts[0]));

    VkPipelineLayoutCreateInfo pPipelineLayoutCreateInfo = vkinit::pipelineLayoutCreateInfo(descriptorSetLayouts.data(), descriptorSetLayouts.size());

    VkPushConstantRange pushConstant;
    pushConstant.offset = 0;
    pushConstant.size = sizeof(s_pushConstantLine);
    pushConstant.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    pPipelineLayoutCreateInfo.pushConstantRangeCount = 1;
    pPipelineLayoutCreateInfo.pPushConstantRanges = &pushConstant;

    CHECK_RESULT_VK(vkCreatePipelineLayout(logicalDevice, &pPipelineLayoutCreateInfo, VK_NULL_HANDLE, &pipelineLayout));
}

void DebugLine::preparePipeline()
{
    VkGraphicsPipelineCreateInfo* pipelineCreateInfo = thinDrawer->getPipelineInfoBase();
    pipelineCreateInfo->layout = pipelineLayout;

    VkVertexInputBindingDescription vertexInputBinding = { };
    vertexInputBinding.binding = 0;
    vertexInputBinding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    vertexInputBinding.stride = sizeof(s_basicVertex);

    VkVertexInputAttributeDescription vertexInputAttributeFor0;
    vertexInputAttributeFor0.binding = 0;
    vertexInputAttributeFor0.location = 0;
    vertexInputAttributeFor0.format = VK_FORMAT_R32G32_SFLOAT;
    vertexInputAttributeFor0.offset = 0;

    VkPipelineVertexInputStateCreateInfo vertexInputState = { };
    vertexInputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputState.vertexBindingDescriptionCount = 1;
    vertexInputState.vertexAttributeDescriptionCount = 1;
    vertexInputState.pVertexAttributeDescriptions = &vertexInputAttributeFor0;
    vertexInputState.pVertexBindingDescriptions = &vertexInputBinding;

    std::vector<std::string> fileNames = {
            std::string("shaders/DebugLine/vertex_shader.vert.spv"),
            std::string("shaders/DebugLine/fragment_shader.frag.spv")
    };

    Shader shader = Shader(logicalDevice, fileNames);

    pipelineCreateInfo->stageCount = shader.shaderStages.size();
    pipelineCreateInfo->pStages = shader.shaderStages.data();
    pipelineCreateInfo->pVertexInputState = &vertexInputState;

    ((VkPipelineDepthStencilStateCreateInfo*)pipelineCreateInfo->pDepthStencilState)->depthTestEnable = VK_FALSE;

    CHECK_RESULT_VK(vkCreateGraphicsPipelines(logicalDevice, VK_NULL_HANDLE, 1, pipelineCreateInfo, VK_NULL_HANDLE, &pipeline));

    ThinDrawer::freePipelineData(pipelineCreateInfo);
}

void DebugLine::setupDescriptorSet()
{
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
    }
}