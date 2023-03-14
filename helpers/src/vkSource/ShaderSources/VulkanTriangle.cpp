#include <ShaderHeaders/VulkanTriangle.h>
#include <SwapChain.h>
#include <ThinDrawer.h>
#include <Camera.h>
#include <Shader.h>
#include <definitions.h>
#include <vkInit.h>

void VulkanTriangle::prepareVertexData()
{
    const int bufferSize = 2;
    buffers.resize(bufferSize);

    for (int i = 0; i < bufferSize; i++)
    {
        buffers[i] = (s_buffers*)malloc(sizeof(s_buffers));
    }
    s_buffers* vertices = buffers[0];
    s_buffers* indices = buffers[1];

    std::vector<s_vertex> vertexBuffer =
    {
        { {+1.0f, +1.0f}, {+1.0f, +0.0f} },
        { {-1.0f, +1.0f}, {+0.0f, +0.0f} },
        { {-1.0f, -1.0f}, {+0.0f, +1.0f} },
        { {+1.0f, -1.0f}, {+1.0f, +1.0f} },
    };
    uint32_t vertexBufferSize = static_cast<uint32_t>(vertexBuffer.size()) * sizeof(s_vertex);

    std::vector<uint32_t> indexBuffer = { 0, 1, 2, 2, 3, 0 };
    indices->count = static_cast<uint32_t>(indexBuffer.size());
    uint32_t indexBufferSize = indices->count * sizeof(uint32_t);

    void* data;

    thinDrawer->bufferStage(vertexBuffer.data(), vertexBufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, vertices);
    thinDrawer->bufferStage(indexBuffer.data(), indexBufferSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, indices);
}

void VulkanTriangle::prepareUniforms()
{
    const int bufferSize = 3;
    int imCount = thinDrawer->swapChain->imageCount;
    uniformBuffers.resize(imCount);

    for (int i = 0; i < imCount; i++)
    {
        uniformBuffers[i].resize(bufferSize);

        for (int j = 0; j < bufferSize; j++)
        {
            uniformBuffers[i][j] = (s_uniformBuffer*)malloc(sizeof(s_uniformBuffer));
        }

        s_uniformBuffer* uniformBufferVS = uniformBuffers[i][0];
        s_uniformBuffer* uniformBufferFS1 = uniformBuffers[i][1];
        s_uniformBuffer* uniformBufferFS2 = uniformBuffers[i][2];

        //thinDrawer->uniformHelper(sizeof(s_uboVS), uniformBufferVS);

        //s_uboVS uboVS;
        //uboVS.modelMatrix = glm::mat4(1.0f);
        //uboVS.modelMatrix = glm::rotate(uboVS.modelMatrix, glm::pi<float>() * 0.25f, glm::vec3(0, 0, 1));
        //uboVS.modelMatrix = glm::scale(uboVS.modelMatrix, glm::vec3(2.5f, 1.0f, 1.0f));

        uint8_t* pData;
        //CHECK_RESULT_VK(vkMapMemory(logicalDevice, uniformBufferVS->memory, 0, sizeof(s_uboVS), 0, (void**)&pData));
        //memcpy(pData, &uboVS, sizeof(s_uboVS));
        //vkUnmapMemory(logicalDevice, uniformBufferVS->memory);

        glm::vec4 vec[2] = { glm::vec4(-1.0f, 1.0f, 1.0f, 1.0f), glm::vec4(1.0f, -1.0f, -1.0f, -1.0f) };
        thinDrawer->uniformHelper(sizeof(vec), uniformBufferFS2);
        CHECK_RESULT_VK(vkMapMemory(logicalDevice, uniformBufferFS2->memory, 0, sizeof(vec), 0, (void**)&pData));
        memcpy(pData, &vec, sizeof(vec));
        vkUnmapMemory(logicalDevice, uniformBufferFS2->memory);

        thinDrawer->uniformHelper(sizeof(int), uniformBufferFS1);
        CHECK_RESULT_VK(vkMapMemory(logicalDevice, uniformBufferFS1->memory, 0, sizeof(int), 0, (void**)&pData));
        int val = sizeof(vec) / sizeof(glm::vec4);
        memcpy(pData, &val, sizeof(int));
        vkUnmapMemory(logicalDevice, uniformBufferFS1->memory);
    }
}

void VulkanTriangle::setupDescriptorSetLayout()
{
    const int descriptorSetLayoutCount = 2;

    descriptorSetLayouts.resize(descriptorSetLayoutCount);

    VkDescriptorSetLayoutBinding layoutBinding0 =
            vkinit::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 1, 0);

    VkDescriptorSetLayoutBinding layoutBinding1 =
        vkinit::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 1, 1);

    VkDescriptorSetLayoutBinding layoutBinding2 =
        vkinit::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT, 1, 2);

    VkDescriptorSetLayoutBinding layoutBinding3 =
        vkinit::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT, 1, 3);

    VkDescriptorSetLayoutBinding bindings[] = { layoutBinding0, layoutBinding1, layoutBinding2, layoutBinding3 };

    VkDescriptorSetLayoutCreateInfo descriptorLayout = vkinit::descriptorSetLayoutCreateInfo(bindings, sizeof(bindings) / sizeof(bindings[0]));
    CHECK_RESULT_VK(vkCreateDescriptorSetLayout(logicalDevice, &descriptorLayout, VK_NULL_HANDLE, &descriptorSetLayouts[0]));

    VkDescriptorSetLayoutBinding textureBind = vkinit::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1, 0);

    VkDescriptorSetLayoutCreateInfo setInfo = vkinit::descriptorSetLayoutCreateInfo(&textureBind, 1);
    CHECK_RESULT_VK(vkCreateDescriptorSetLayout(logicalDevice, &setInfo, VK_NULL_HANDLE, &descriptorSetLayouts[1]));

    VkPipelineLayoutCreateInfo pPipelineLayoutCreateInfo = vkinit::pipelineLayoutCreateInfo(
        descriptorSetLayouts.data(), descriptorSetLayouts.size());
    CHECK_RESULT_VK(vkCreatePipelineLayout(logicalDevice, &pPipelineLayoutCreateInfo, VK_NULL_HANDLE, &pipelineLayout));
}

void VulkanTriangle::preparePipeline()
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

void VulkanTriangle::setupDescriptorSet()
{
    ShaderBase::setupDescriptorSet();

    int imCount = thinDrawer->swapChain->imageCount;
    descriptorSets.resize(imCount);

    for (int i = 0; i < imCount; i++)
    {
        for (int j = 0; j < textureData.size(); j++)
        {
            thinDrawer->updateImageDescriptors(textureData[j], descriptorSetLayouts[1]);
        }
    }
}