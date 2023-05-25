#include <ShaderHeaders/BodyShader.h>
#include <SwapChain.h>
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
    int imCount = thinDrawer->swapChain->imageCount;
    uniformBuffers.resize(imCount);

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

    for (int i = 0; i < imCount; i++)
    {
        uniformBuffers[i].resize(bufferSize);

        for (int j = 0; j < bufferSize; j++)
        {
            uniformBuffers[i][j] = (s_uniformBuffer*)malloc(sizeof(s_uniformBuffer));
        }

        s_uniformBuffer* uniformBufferVS = uniformBuffers[i][0];
        s_uniformBuffer* uniformBufferFS1 = uniformBuffers[i][1];

        thinDrawer->uniformHelper(alignedSize_body * UNIFORM_ALLOC_COUNT_BODY, uniformBuffers[i][0], VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, UNIFORM_ALLOC_COUNT_BODY);
        thinDrawer->uniformHelper(alignedSize_fixture * UNIFORM_ALLOC_COUNT_FIXTURE, uniformBuffers[i][1], VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, UNIFORM_ALLOC_COUNT_FIXTURE);
    }
}

void BodyShader::setupDescriptorSetLayout()
{
    const int descriptorSetLayoutCount = 3;

    descriptorSetLayouts.resize(descriptorSetLayoutCount);

    VkDescriptorSetLayoutBinding layoutBinding0 =
            vkinit::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 1, 0);

    VkDescriptorSetLayoutBinding layoutBinding1 =
        vkinit::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, VK_SHADER_STAGE_VERTEX_BIT, 1, 0);

    VkDescriptorSetLayoutBinding layoutBinding2 =
        vkinit::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, VK_SHADER_STAGE_FRAGMENT_BIT, 1, 1);

    VkDescriptorSetLayoutBinding bindings[] = { layoutBinding0 };
    VkDescriptorSetLayoutBinding bindings2[] = { layoutBinding1, layoutBinding2 };

    VkDescriptorSetLayoutCreateInfo descriptorLayout = vkinit::descriptorSetLayoutCreateInfo(bindings, sizeof(bindings) / sizeof(bindings[0]));
    CHECK_RESULT_VK(vkCreateDescriptorSetLayout(logicalDevice, &descriptorLayout, VK_NULL_HANDLE, &descriptorSetLayouts[0]));

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
    int imCount = thinDrawer->swapChain->imageCount;
    descriptorSets.resize(2);

    for (int i = 0; i < imCount; i++)
    {
        descriptorSets[0].resize(imCount);
        descriptorSets[1].resize(imCount);

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
        if (uniformBuffers.size() == 0) continue;

        allocInfo = vkinit::descriptorSetAllocateInfo(thinDrawer->descriptorPool, &descriptorSetLayouts[2], 1);
        CHECK_RESULT_VK(vkAllocateDescriptorSets(logicalDevice, &allocInfo, &descriptorSets[1][i]));

        writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;

        for (int j = 0; j < uniformBuffers[i].size(); j++)
        {
            writeDescriptorSet.dstSet = descriptorSets[1][i];
            writeDescriptorSet.pBufferInfo = &uniformBuffers[i][j]->descriptor;
            writeDescriptorSet.dstBinding = j;

            vkUpdateDescriptorSets(logicalDevice, 1, &writeDescriptorSet, 0, VK_NULL_HANDLE);
        }
    }
}