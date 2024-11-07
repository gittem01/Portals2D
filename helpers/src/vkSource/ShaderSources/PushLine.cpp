#include <ShaderHeaders/PushLine.h>
#include <ThinDrawer.h>
#include <Camera.h>
#include <Shader.h>
#include <definitions.h>
#include <vkInit.h>

void PushLine::prepareVertexData()
{
    const int bufferSize = 0;
    buffers.resize(bufferSize);
}

void PushLine::prepareUniforms()
{
    uniformBuffers.resize(0);
}

void PushLine::setupDescriptorSetLayout()
{
    const int descriptorSetLayoutCount = 1;

    descriptorSetLayouts.resize(descriptorSetLayoutCount);
    descriptorSetLayouts[0] = ShaderBase::sharedLayout;

    VkPipelineLayoutCreateInfo pPipelineLayoutCreateInfo = vkinit::pipelineLayoutCreateInfo(descriptorSetLayouts.data(), descriptorSetLayouts.size());
    
    VkPushConstantRange range = vkinit::initPushConstantRange(sizeof(s_debugLineUbo), VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);
    pPipelineLayoutCreateInfo.pushConstantRangeCount = 1;
    pPipelineLayoutCreateInfo.pPushConstantRanges = &range;
    
    CHECK_RESULT_VK(vkCreatePipelineLayout(logicalDevice, &pPipelineLayoutCreateInfo, VK_NULL_HANDLE, &pipelineLayout));
}

void PushLine::preparePipeline()
{
    VkGraphicsPipelineCreateInfo* pipelineCreateInfo = thinDrawer->getPipelineInfoBase();
    pipelineCreateInfo->layout = pipelineLayout;

    VkPipelineVertexInputStateCreateInfo vertexInputState = { };
    vertexInputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputState.vertexBindingDescriptionCount = 0;
    vertexInputState.vertexAttributeDescriptionCount = 0;
    vertexInputState.pVertexAttributeDescriptions = nullptr;
    vertexInputState.pVertexBindingDescriptions = nullptr;

    std::vector<std::string> fileNames = {
            std::string("shaders/PushLine/vertex_shader.vert.spv"),
            std::string("shaders/PushLine/fragment_shader.frag.spv")
    };

    Shader shader = Shader(logicalDevice, fileNames);

    pipelineCreateInfo->stageCount = shader.shaderStages.size();
    pipelineCreateInfo->pStages = shader.shaderStages.data();
    pipelineCreateInfo->pVertexInputState = &vertexInputState;

    CHECK_RESULT_VK(vkCreateGraphicsPipelines(logicalDevice, VK_NULL_HANDLE, 1, pipelineCreateInfo, VK_NULL_HANDLE, &pipeline));

    ThinDrawer::freePipelineData(pipelineCreateInfo);
}

void PushLine::setupDescriptorSet()
{
    descriptorSets.resize(descriptorSetLayouts.size());
}