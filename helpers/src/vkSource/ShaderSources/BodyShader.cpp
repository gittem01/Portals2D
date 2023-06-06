#include <ShaderHeaders/BodyShader.h>
#include <ThinDrawer.h>
#include <Camera.h>
#include <Shader.h>
#include <definitions.h>
#include <vkInit.h>

void BodyShader::prepareVertexData()
{
    const int bufferSize = 1;
    buffers.resize(bufferSize);

    for (int i = 0; i < bufferSize; i++)
    {
        buffers[i] = (s_buffers*)malloc(sizeof(s_buffers));
    }
    s_buffers* vertices = buffers[0];

    std::vector<s_vertex> vertexBuffer =
    {
        { {+1.0f, +1.0f}, {+1.0f, +0.0f} },
        { {-1.0f, +1.0f}, {+0.0f, +0.0f} },
        { {-1.0f, -1.0f}, {+0.0f, +1.0f} },
        { {-1.0f, -1.0f}, {+0.0f, +1.0f} },
        { {+1.0f, -1.0f}, {+1.0f, +1.0f} },
        { {+1.0f, +1.0f}, {+1.0f, +0.0f} },
    };
    uint32_t vertexBufferSize = static_cast<uint32_t>(vertexBuffer.size()) * sizeof(s_vertex);

    vertices->count = vertexBuffer.size();

    thinDrawer->bufferStage(vertexBuffer.data(), vertexBufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, vertices);
}

void BodyShader::prepareUniforms()
{
    const int bufferSize = 2;

    alignedSize_body = thinDrawer->pad_uniform_buffer_size(sizeof(s_renderBodyUbo));
    alignedSize_fixture = thinDrawer->pad_uniform_buffer_size(sizeof(s_renderFixtureUbo));

    lastAllocationPos_body = 0;
    lastAllocationPos_fixture = 0;

    for (int i = 0; i < UNIFORM_ALLOC_COUNT_BODY; i++)
    {
        availableOffsets_body[i] = i * alignedSize_body;
    }
    for (int i = 0; i < UNIFORM_ALLOC_COUNT_FIXTURE; i++)
    {
        availableOffsets_fixture[i] = i * alignedSize_fixture;
    }

    uniformBuffers.resize(bufferSize);

    for (int j = 0; j < bufferSize; j++)
    {
        uniformBuffers[j] = (s_uniformBuffer*)malloc(sizeof(s_uniformBuffer));
    }

    s_uniformBuffer* uniformBufferVS = uniformBuffers[0];
    s_uniformBuffer* uniformBufferFS1 = uniformBuffers[1];

    thinDrawer->uniformHelper(alignedSize_body * UNIFORM_ALLOC_COUNT_BODY, uniformBuffers[0], VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, UNIFORM_ALLOC_COUNT_BODY);
    thinDrawer->uniformHelper(alignedSize_fixture * UNIFORM_ALLOC_COUNT_FIXTURE, uniformBuffers[1], VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, UNIFORM_ALLOC_COUNT_FIXTURE);
}

void BodyShader::setupDescriptorSetLayout()
{
    const int descriptorSetLayoutCount = 3;

    descriptorSetLayouts.resize(descriptorSetLayoutCount);
    descriptorSetLayouts[0] = ShaderBase::sharedLayout;

    VkDescriptorSetLayoutBinding layoutBinding1 =
        vkinit::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, VK_SHADER_STAGE_VERTEX_BIT, 1, 0);

    VkDescriptorSetLayoutBinding layoutBinding2 =
        vkinit::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, VK_SHADER_STAGE_FRAGMENT_BIT, 1, 1);

    VkDescriptorSetLayoutBinding bindings2[] = { layoutBinding1, layoutBinding2 };

    VkDescriptorSetLayoutBinding textureBind = vkinit::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1, 0);

    VkDescriptorSetLayoutCreateInfo setInfo = vkinit::descriptorSetLayoutCreateInfo(&textureBind, 1);
    CHECK_RESULT_VK(vkCreateDescriptorSetLayout(logicalDevice, &setInfo, VK_NULL_HANDLE, &descriptorSetLayouts[1]));

    VkDescriptorSetLayoutCreateInfo setInfo2 = vkinit::descriptorSetLayoutCreateInfo(bindings2, 2);
    CHECK_RESULT_VK(vkCreateDescriptorSetLayout(logicalDevice, &setInfo2, VK_NULL_HANDLE, &descriptorSetLayouts[2]));

    VkPipelineLayoutCreateInfo pPipelineLayoutCreateInfo = vkinit::pipelineLayoutCreateInfo(
        descriptorSetLayouts.data(), descriptorSetLayouts.size());
    CHECK_RESULT_VK(vkCreatePipelineLayout(logicalDevice, &pPipelineLayoutCreateInfo, VK_NULL_HANDLE, &pipelineLayout));
}

void BodyShader::preparePipeline()
{
    VkGraphicsPipelineCreateInfo* pipelineCreateInfo = thinDrawer->getPipelineInfoBase();

    pipelineCreateInfo->layout = pipelineLayout;

    VkVertexInputBindingDescription vertexInputBinding = { };
    vertexInputBinding.binding = 0;
    vertexInputBinding.stride = sizeof(s_vertex);
    vertexInputBinding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    VkVertexInputAttributeDescription vertexInputAttributeFor0;
    vertexInputAttributeFor0.binding = 0;
    vertexInputAttributeFor0.location = 0;
    vertexInputAttributeFor0.format = VK_FORMAT_R32G32_SFLOAT;
    vertexInputAttributeFor0.offset = 0;

    VkVertexInputAttributeDescription vertexInputAttributeFor1;
    vertexInputAttributeFor1.binding = 0;
    vertexInputAttributeFor1.location = 1;
    vertexInputAttributeFor1.format = VK_FORMAT_R32G32_SFLOAT;
    vertexInputAttributeFor1.offset = offsetof(s_vertex, uv);

    VkVertexInputAttributeDescription attributes[] = { vertexInputAttributeFor0, vertexInputAttributeFor1 };

    VkPipelineVertexInputStateCreateInfo vertexInputState = { };
    vertexInputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputState.vertexBindingDescriptionCount = 1;
    vertexInputState.pVertexBindingDescriptions = &vertexInputBinding;
    vertexInputState.vertexAttributeDescriptionCount = sizeof(attributes) / sizeof(attributes[0]);
    vertexInputState.pVertexAttributeDescriptions = attributes;

    std::vector<std::string> fileNames = {
            std::string("shaders/Textured/vertex_shader.vert.spv"),
            std::string("shaders/Textured/fragment_shader.frag.spv")
    };

    Shader shader = Shader(logicalDevice, fileNames);

    pipelineCreateInfo->stageCount = shader.shaderStages.size();
    pipelineCreateInfo->pStages = shader.shaderStages.data();

    pipelineCreateInfo->pVertexInputState = &vertexInputState;

    ((VkPipelineDepthStencilStateCreateInfo*)pipelineCreateInfo->pDepthStencilState)->depthTestEnable = VK_FALSE;

    CHECK_RESULT_VK(vkCreateGraphicsPipelines(logicalDevice, VK_NULL_HANDLE, 1, pipelineCreateInfo, VK_NULL_HANDLE, &pipeline));

    ThinDrawer::freePipelineData(pipelineCreateInfo);
}

void BodyShader::setupDescriptorSet()
{
    descriptorSets.resize(2);

    VkDescriptorSetAllocateInfo allocInfo;

    VkWriteDescriptorSet writeDescriptorSet = { };
    writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet.descriptorCount = 1;
    writeDescriptorSet.dstBinding = 0;

    allocInfo = vkinit::descriptorSetAllocateInfo(thinDrawer->descriptorPool, &descriptorSetLayouts[2], 1);
    CHECK_RESULT_VK(vkAllocateDescriptorSets(logicalDevice, &allocInfo, &descriptorSets[1]));

    writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;

    for (int j = 0; j < uniformBuffers.size(); j++)
    {
        writeDescriptorSet.dstSet = descriptorSets[1];
        writeDescriptorSet.pBufferInfo = &uniformBuffers[j]->descriptor;
        writeDescriptorSet.dstBinding = j;

        vkUpdateDescriptorSets(logicalDevice, 1, &writeDescriptorSet, 0, VK_NULL_HANDLE);
    }
}