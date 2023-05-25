#include <ShaderHeaders/PortalShader3D.h>
#include <SwapChain.h>
#include <ThinDrawer.h>
#include <Camera.h>
#include <Shader.h>
#include <definitions.h>
#include <vkInit.h>

void PortalShader3D::prepareVertexData()
{
    const int bufferSize = 1;
    buffers.resize(bufferSize);

    for (int i = 0; i < bufferSize; i++)
    {
        buffers[i] = (s_buffers*)malloc(sizeof(s_buffers));
    }
    s_buffers* vertices = buffers[0];

    std::vector<s_basicVertex> vertexBuffer =
    {
        { { -0.5f, +0.5f } },
        { { +0.5f, +0.5f } },
        { { +0.5f, -0.5f } },
        { { +0.5f, -0.5f } },
        { { -0.5f, -0.5f } },
        { { -0.5f, +0.5f } },
    };

    uint32_t vertexBufferSize = static_cast<uint32_t>(vertexBuffer.size()) * sizeof(s_basicVertex);
    vertices->count = vertexBuffer.size();

    thinDrawer->bufferStage(vertexBuffer.data(), vertexBufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, vertices);
}

glm::vec2 PortalShader3D::getEllipsePosOnAngle(float a, float b, float angle)
{
    glm::vec2 retVec = glm::vec2(   (a * b) / sqrt(b * b + a * a * tan(angle) * tan(angle)),
                                    (a * b * tan(angle)) / sqrt(b * b + a * a * tan(angle) * tan(angle)));

    return retVec;
}

void PortalShader3D::prepareUniforms()
{
    int imCount = thinDrawer->swapChain->imageCount;
    alignedSize = thinDrawer->pad_uniform_buffer_size(sizeof(s_portalUbo3D));
    uniformBuffers.resize(imCount);

    lastAllocationPos = 0;

    for (int i = 0; i < UNIFORM_ALLOC_COUNT; i++)
    {
        availableOffsets[i] = i * alignedSize;
    }

    const glm::vec2 size = glm::vec2(2.5f, 5.0f);

    for (int i = 0; i < imCount; i++)
    {
        uniformBuffers[i].resize(1);

        uniformBuffers[i][0] = (s_uniformBuffer*)malloc(sizeof(s_uniformBuffer));

        thinDrawer->uniformHelper(alignedSize * UNIFORM_ALLOC_COUNT, uniformBuffers[i][0], VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, UNIFORM_ALLOC_COUNT);
    }
}

void PortalShader3D::setupDescriptorSetLayout()
{
    const int descriptorSetLayoutCount = 3;

    descriptorSetLayouts.resize(descriptorSetLayoutCount);

    VkDescriptorSetLayoutBinding layoutBinding0 = vkinit::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 1, 0);
    VkDescriptorSetLayoutBinding layoutBinding10 = vkinit::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
        VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 1, 0);

    VkDescriptorSetLayoutBinding textureBind = vkinit::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1, 0);

    VkDescriptorSetLayoutBinding bindings1[] = { layoutBinding0 };
    VkDescriptorSetLayoutBinding bindings2[] = { layoutBinding10 };
    VkDescriptorSetLayoutBinding texBindings[] = { textureBind };

    VkDescriptorSetLayoutCreateInfo descriptorLayout = vkinit::descriptorSetLayoutCreateInfo(bindings1, sizeof(bindings1) / sizeof(bindings1[0]));
    CHECK_RESULT_VK(vkCreateDescriptorSetLayout(logicalDevice, &descriptorLayout, VK_NULL_HANDLE, &descriptorSetLayouts[0]));

    VkDescriptorSetLayoutCreateInfo descriptorLayout2 = vkinit::descriptorSetLayoutCreateInfo(bindings2, sizeof(bindings2) / sizeof(bindings2[0]));
    CHECK_RESULT_VK(vkCreateDescriptorSetLayout(logicalDevice, &descriptorLayout2, VK_NULL_HANDLE, &descriptorSetLayouts[1]));

    VkDescriptorSetLayoutCreateInfo setInfo = vkinit::descriptorSetLayoutCreateInfo(texBindings, std::size(texBindings));
    CHECK_RESULT_VK(vkCreateDescriptorSetLayout(logicalDevice, &setInfo, VK_NULL_HANDLE, &descriptorSetLayouts[2]));

    VkPipelineLayoutCreateInfo pPipelineLayoutCreateInfo = vkinit::pipelineLayoutCreateInfo(descriptorSetLayouts.data(), descriptorSetLayouts.size());
    CHECK_RESULT_VK(vkCreatePipelineLayout(logicalDevice, &pPipelineLayoutCreateInfo, VK_NULL_HANDLE, &pipelineLayout));
}

void PortalShader3D::preparePipeline()
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
            std::string("shaders/Portal3D/vertex_shader.vert.spv"),
            std::string("shaders/Portal3D/fragment_shader.frag.spv")
    };

    Shader shader2 = Shader(logicalDevice, fileNames);

    pipelineCreateInfo->stageCount = shader2.shaderStages.size();
    pipelineCreateInfo->pStages = shader2.shaderStages.data();
    pipelineCreateInfo->pVertexInputState = &vertexInputState;

    ((VkPipelineDepthStencilStateCreateInfo*)pipelineCreateInfo->pDepthStencilState)->depthCompareOp = VK_COMPARE_OP_ALWAYS;
    ((VkPipelineRasterizationStateCreateInfo*)pipelineCreateInfo->pRasterizationState)->cullMode = VK_CULL_MODE_BACK_BIT;
    ((VkPipelineMultisampleStateCreateInfo*)pipelineCreateInfo->pMultisampleState)->sampleShadingEnable = VK_TRUE;

    CHECK_RESULT_VK(vkCreateGraphicsPipelines(logicalDevice, VK_NULL_HANDLE, 1, pipelineCreateInfo, VK_NULL_HANDLE, &pipeline));

    ThinDrawer::freePipelineData(pipelineCreateInfo);
}

void PortalShader3D::setupDescriptorSet()
{
    s_texture* tex = (s_texture*)malloc(sizeof(s_texture));
    thinDrawer->loadTexture((char*)"textures/grad2.png", tex);
    textureData.push_back(tex);

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

        allocInfo = vkinit::descriptorSetAllocateInfo(thinDrawer->descriptorPool, &descriptorSetLayouts[1], 1);
        CHECK_RESULT_VK(vkAllocateDescriptorSets(logicalDevice, &allocInfo, &descriptorSets[1][i]));
        writeDescriptorSet.dstSet = descriptorSets[1][i];
        writeDescriptorSet.pBufferInfo = &uniformBuffers[i][0]->descriptor;
        writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;

        vkUpdateDescriptorSets(logicalDevice, 1, &writeDescriptorSet, 0, VK_NULL_HANDLE);
    }

    VkDescriptorSetAllocateInfo allocInfo = vkinit::descriptorSetAllocateInfo(thinDrawer->descriptorPool, &descriptorSetLayouts[2], 1);
    vkAllocateDescriptorSets(logicalDevice, &allocInfo, &tex->set);

    VkDescriptorImageInfo imageBufferInfo;
    imageBufferInfo.imageView = tex->view;
    imageBufferInfo.sampler = tex->sampler;
    imageBufferInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    VkWriteDescriptorSet textureWrite = { };
    textureWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    textureWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    textureWrite.dstSet = tex->set;
    textureWrite.descriptorCount = 1;
    textureWrite.pImageInfo = &imageBufferInfo;
    textureWrite.dstBinding = 0;

    vkUpdateDescriptorSets(logicalDevice, 1, &textureWrite, 0, VK_NULL_HANDLE);
}