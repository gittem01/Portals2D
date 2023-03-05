#include <ShaderHeaders/WhiteWall.h>
#include <SwapChain.h>
#include <ThinDrawer.h>
#include <Camera.h>
#include <Shader.h>
#include <definitions.h>
#include <vkInit.h>

void WhiteWall::prepareVertexData()
{
    const int bufferSize = 1;
    buffers.resize(bufferSize);

    for (int i = 0; i < bufferSize; i++)
    {
        buffers[i] = (s_buffers*)malloc(sizeof(s_buffers));
    }
    s_buffers* vertices = buffers[0];

    std::vector<s_averageVertex> vertexBuffer =
    {
            { {-0.5f, +0.5f, 0.0f}, {0.0f, 0.0f, 1.0f} },
            { {+0.5f, +0.5f, 0.0f}, {0.0f, 0.0f, 1.0f} },
            { {+0.5f, -0.5f, 0.0f}, {0.0f, 0.0f, 1.0f} },
            { {+0.5f, -0.5f, 0.0f}, {0.0f, 0.0f, 1.0f} },
            { {-0.5f, -0.5f, 0.0f}, {0.0f, 0.0f, 1.0f} },
            { {-0.5f, +0.5f, 0.0f}, {0.0f, 0.0f, 1.0f} },
    };

    uint32_t vertexBufferSize = static_cast<uint32_t>(vertexBuffer.size()) * sizeof(s_averageVertex);
    vertices->count = vertexBuffer.size();

    thinDrawer->bufferStage(vertexBuffer.data(), vertexBufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, vertices);
}

void WhiteWall::prepareUniforms()
{
    int imCount = thinDrawer->swapChain->imageCount;
    int alignedSizeFS = thinDrawer->pad_storage_buffer_size(sizeof(s_wallUbo));

    uniformBuffers.resize(imCount);

    for (int i = 0; i < imCount; i++)
    {
        uniformBuffers[i].resize(1);

        uniformBuffers[i][0] = (s_uniformBuffer*)malloc(sizeof(s_wallUbo));

        thinDrawer->uniformHelper(alignedSizeFS * 4, uniformBuffers[i][0], VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, 4);

        glm::vec2 size = glm::vec2(10.0f, 10.0f);
        glm::vec2 positions[4] = { {-5.0f, -5.0f}, {5.0f, -5.0f}, {5.0f, 5.0f}, {-5.0f, 5.0f} };

        for (int j = 0; j < 4; j++)
        {
            s_wallUbo wallUbo;
            wallUbo.transformMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(positions[j].x, positions[j].y, 0.0f));
            wallUbo.rotationMatrix = glm::rotate(glm::mat4(1.0f), glm::radians(0.0f), glm::vec3(0.0f, 1.0f, 0.0));
            wallUbo.bgColour = glm::vec4(0.95f, 0.95f, 0.95f, 1.0f);
            wallUbo.edgeColour = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
            wallUbo.dotColour = glm::vec4(1.0 - j * 0.25, j * 0.33, 1.0f, 1.0f);
            wallUbo.leftUp = wallUbo.transformMatrix * glm::vec4(-0.5f * size.x, -0.5f * size.y, 0.0f, 1.0f);
            wallUbo.data = glm::vec4(0.06f * (j + 4), 0.1f, 4.0f, 0.3f);
            wallUbo.data2 = glm::vec4(size.x, size.y, 0.0f, 0.0f);

            void* pData;
            CHECK_RESULT_VK(vkMapMemory(logicalDevice, uniformBuffers[i][0]->memory, j * alignedSizeFS, alignedSizeFS, 0, (void**)&pData));
            memcpy(pData, &wallUbo, alignedSizeFS);
            vkUnmapMemory(logicalDevice, uniformBuffers[i][0]->memory);
        }
    }
}

void WhiteWall::setupDescriptorSetLayout()
{
    const int descriptorSetLayoutCount = 2;

    descriptorSetLayouts.resize(descriptorSetLayoutCount);

    VkDescriptorSetLayoutBinding layoutBinding0 = vkinit::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 1, 0);
    VkDescriptorSetLayoutBinding layoutBinding10 = vkinit::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC,
        VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 1, 0);

    VkDescriptorSetLayoutBinding bindings1[] = { layoutBinding0 };
    VkDescriptorSetLayoutBinding bindings2[] = { layoutBinding10 };

    VkDescriptorSetLayoutCreateInfo descriptorLayout = vkinit::descriptorSetLayoutCreateInfo(bindings1, sizeof(bindings1) / sizeof(bindings1[0]));
    CHECK_RESULT_VK(vkCreateDescriptorSetLayout(logicalDevice, &descriptorLayout, VK_NULL_HANDLE, &descriptorSetLayouts[0]));

    VkDescriptorSetLayoutCreateInfo descriptorLayout2 = vkinit::descriptorSetLayoutCreateInfo(bindings2, sizeof(bindings2) / sizeof(bindings2[0]));
    CHECK_RESULT_VK(vkCreateDescriptorSetLayout(logicalDevice, &descriptorLayout2, VK_NULL_HANDLE, &descriptorSetLayouts[1]));

    VkPipelineLayoutCreateInfo pPipelineLayoutCreateInfo = vkinit::pipelineLayoutCreateInfo(descriptorSetLayouts.data(), descriptorSetLayouts.size());
    CHECK_RESULT_VK(vkCreatePipelineLayout(logicalDevice, &pPipelineLayoutCreateInfo, VK_NULL_HANDLE, &pipelineLayout));
}

void WhiteWall::preparePipeline()
{
    VkGraphicsPipelineCreateInfo* pipelineCreateInfo = thinDrawer->getPipelineInfoBase();
    pipelineCreateInfo->layout = pipelineLayout;

    VkVertexInputBindingDescription vertexInputBinding = { };
    vertexInputBinding.binding = 0;
    vertexInputBinding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    vertexInputBinding.stride = sizeof(s_averageVertex);

    VkVertexInputAttributeDescription attributeDescriptions[2];
    attributeDescriptions[0].binding = 0;
    attributeDescriptions[0].location = 0;
    attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[0].offset = offsetof(s_averageVertex, pos);

    attributeDescriptions[1].binding = 0;
    attributeDescriptions[1].location = 1;
    attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[1].offset = offsetof(s_averageVertex, normal);

    VkPipelineVertexInputStateCreateInfo vertexInputState = { };
    vertexInputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputState.vertexBindingDescriptionCount = 1;
    vertexInputState.vertexAttributeDescriptionCount = std::size(attributeDescriptions);
    vertexInputState.pVertexAttributeDescriptions = attributeDescriptions;
    vertexInputState.pVertexBindingDescriptions = &vertexInputBinding;

    std::vector<std::string> fileNames =
    {
            std::string("shaders/Wall/vertex_shader.vert.spv"),
            std::string("shaders/Wall/fragment_shader.frag.spv")
    };

    Shader shader2 = Shader(logicalDevice, fileNames);

    pipelineCreateInfo->stageCount = shader2.shaderStages.size();
    pipelineCreateInfo->pStages = shader2.shaderStages.data();
    pipelineCreateInfo->pVertexInputState = &vertexInputState;

    CHECK_RESULT_VK(vkCreateGraphicsPipelines(logicalDevice, VK_NULL_HANDLE, 1, pipelineCreateInfo, VK_NULL_HANDLE, &pipeline));
}

void WhiteWall::setupDescriptorSet()
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

        allocInfo = vkinit::descriptorSetAllocateInfo(thinDrawer->descriptorPool, &descriptorSetLayouts[1], 1);
        CHECK_RESULT_VK(vkAllocateDescriptorSets(logicalDevice, &allocInfo, &descriptorSets[1][i]));
        writeDescriptorSet.dstSet = descriptorSets[1][i];
        writeDescriptorSet.pBufferInfo = &uniformBuffers[i][0]->descriptor;
        writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;

        vkUpdateDescriptorSets(logicalDevice, 1, &writeDescriptorSet, 0, VK_NULL_HANDLE);
    }
}