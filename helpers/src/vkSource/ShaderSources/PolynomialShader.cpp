#include <ShaderHeaders/PolynomialShader.h>
#include <ThinDrawer.h>
#include <Camera.h>
#include <Shader.h>
#include <definitions.h>
#include <vkInit.h>

void PolynomialShader::prepareVertexData()
{
    const int bufferSize = 1;
    buffers.resize(bufferSize);

    for (int i = 0; i < bufferSize; i++)
    {
        buffers[i] = (s_buffers*)malloc(sizeof(s_buffers));
    }
    s_buffers* vertices = buffers[0];

    std::vector<s_basicVertex> vertexBuffer;

    const int numTriangles = 1800;
    assert(numTriangles % 6 == 0);
    vertexBuffer.resize(numTriangles);
    for (int i = 0; i < numTriangles / 6; i += 1)
    {
        int index = i * 6;
        vertexBuffer[index + 0].pos = glm::vec2(index / (float)numTriangles, -0.5);
        vertexBuffer[index + 1].pos = glm::vec2(index / (float)numTriangles, +0.5);
        vertexBuffer[index + 2].pos = glm::vec2((index + 6) / (float)numTriangles, -0.5);

        vertexBuffer[index + 3].pos = glm::vec2((index + 6) / (float)numTriangles, -0.5);
        vertexBuffer[index + 4].pos = glm::vec2(index / (float)numTriangles, +0.5);
        vertexBuffer[index + 5].pos = glm::vec2((index + 6) / (float)numTriangles, +0.5);
	}

    uint32_t vertexBufferSize = static_cast<uint32_t>(vertexBuffer.size()) * sizeof(s_basicVertex);
    vertices->count = vertexBuffer.size();

    thinDrawer->bufferStage(vertexBuffer.data(), vertexBufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, vertices);
}

void PolynomialShader::prepareUniforms()
{
    s_polynomUbo polynomUbo;
    polynomUbo.equation = glm::vec4(-50.0, 0.0, 0.0, 0.07);
    polynomUbo.data = glm::vec4(0.5, -0.5, 2.0, 0.0);
    //datas.push_back({ polynomUbo, true });
}

void PolynomialShader::setupDescriptorSetLayout()
{
    const int descriptorSetLayoutCount = 1;

    descriptorSetLayouts.resize(descriptorSetLayoutCount);
    descriptorSetLayouts[0] = ShaderBase::sharedLayout;

    VkPipelineLayoutCreateInfo pPipelineLayoutCreateInfo = vkinit::pipelineLayoutCreateInfo(descriptorSetLayouts.data(), descriptorSetLayouts.size());
    
    VkPushConstantRange pushConstant;
    pushConstant.offset = 0;
    pushConstant.size = sizeof(s_polynomUbo);
    pushConstant.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    pPipelineLayoutCreateInfo.pushConstantRangeCount = 1;
    pPipelineLayoutCreateInfo.pPushConstantRanges = &pushConstant;
    
    CHECK_RESULT_VK(vkCreatePipelineLayout(logicalDevice, &pPipelineLayoutCreateInfo, VK_NULL_HANDLE, &pipelineLayout));
}

void PolynomialShader::preparePipeline()
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
            std::string("shaders/PolynomialShader/vertex_shader.vert.spv"),
            std::string("shaders/PolynomialShader/fragment_shader.frag.spv")
    };

    Shader shader2 = Shader(logicalDevice, fileNames);

    pipelineCreateInfo->stageCount = shader2.shaderStages.size();
    pipelineCreateInfo->pStages = shader2.shaderStages.data();
    pipelineCreateInfo->pVertexInputState = &vertexInputState;

    CHECK_RESULT_VK(vkCreateGraphicsPipelines(logicalDevice, VK_NULL_HANDLE, 1, pipelineCreateInfo, VK_NULL_HANDLE, &pipeline));

    ThinDrawer::freePipelineData(pipelineCreateInfo);
}

void PolynomialShader::setupDescriptorSet()
{
    descriptorSets.resize(descriptorSetLayouts.size());
}

void PolynomialShader::render(VkCommandBuffer cmdBuff, class Camera* cam)
{
    const VkDeviceSize offsets = 0;
    const uint32_t camOffset = ShaderBase::alignedSharedSize * thinDrawer->lastSwapChainImageIndex;

    vkCmdBindPipeline(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

    vkCmdBindVertexBuffers(cmdBuff, 0, 1, &buffers[0]->buffer, &offsets);

    vkCmdBindDescriptorSets(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS,
        pipelineLayout, 0, 1, &ShaderBase::sharedSet, 1, &camOffset);


    for (s_polynomRenderData& renderData : datas)
    {
        if (renderData.shouldRender)
        {
            vkCmdPushConstants(cmdBuff, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(s_polynomUbo), &renderData.data);

            vkCmdDraw(cmdBuff, buffers[0]->count, 1, 0, 0);
        }
    }
}