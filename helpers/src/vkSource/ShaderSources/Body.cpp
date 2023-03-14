#include <ShaderHeaders/Body.h>
#include <SwapChain.h>
#include <ThinDrawer.h>
#include <Camera.h>
#include <Shader.h>
#include <definitions.h>
#include <vkInit.h>

#include <IOHelper.h>

void Body::prepareVertexData()
{
    IOHelper::loadGlTF("default");
    IOHelper::loadGlTF("pCube");
    IOHelper::loadGlTF("sphere");
}

void Body::prepareUniforms()
{
    int imCount = thinDrawer->swapChain->imageCount;
    
    alignedSize_body = thinDrawer->pad_uniform_buffer_size(sizeof(s_bodyUbo));
    alignedSize_fixture = thinDrawer->pad_uniform_buffer_size(sizeof(s_fixUbo));
    
    uniformBuffers.resize(imCount);

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
        uniformBuffers[i].resize(2);
        for (int j = 0; j < uniformBuffers[i].size(); j++)
        {
            uniformBuffers[i][j] = (s_uniformBuffer*)malloc(sizeof(s_uniformBuffer));
        }

        thinDrawer->uniformHelper(alignedSize_body * UNIFORM_ALLOC_COUNT_BODY, uniformBuffers[i][0], VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, UNIFORM_ALLOC_COUNT_BODY);
        thinDrawer->uniformHelper(alignedSize_fixture * UNIFORM_ALLOC_COUNT_FIXTURE, uniformBuffers[i][1], VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, UNIFORM_ALLOC_COUNT_FIXTURE);
    }
}

void Body::setupDescriptorSetLayout()
{
    const int descriptorSetLayoutCount = 3;

    descriptorSetLayouts.resize(descriptorSetLayoutCount);

    VkDescriptorSetLayoutBinding layoutBinding0 = vkinit::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 1, 0);

    VkDescriptorSetLayoutBinding layoutBinding10 = vkinit::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
        VK_SHADER_STAGE_VERTEX_BIT, 1, 0);

    VkDescriptorSetLayoutBinding layoutBinding11 = vkinit::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
        VK_SHADER_STAGE_FRAGMENT_BIT, 1, 1);

    VkDescriptorSetLayoutBinding textureBind = vkinit::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1, 0);

    VkDescriptorSetLayoutBinding bindings[] = { layoutBinding0 };
    VkDescriptorSetLayoutBinding bindings2[] = { layoutBinding10, layoutBinding11 };
    VkDescriptorSetLayoutBinding texBindings[] = { textureBind };

    VkDescriptorSetLayoutCreateInfo descriptorLayout = vkinit::descriptorSetLayoutCreateInfo(bindings, sizeof(bindings) / sizeof(bindings[0]));
    CHECK_RESULT_VK(vkCreateDescriptorSetLayout(logicalDevice, &descriptorLayout, VK_NULL_HANDLE, &descriptorSetLayouts[0]));

    VkDescriptorSetLayoutCreateInfo descriptorLayout2 = vkinit::descriptorSetLayoutCreateInfo(bindings2, sizeof(bindings2) / sizeof(bindings2[0]));
    CHECK_RESULT_VK(vkCreateDescriptorSetLayout(logicalDevice, &descriptorLayout2, VK_NULL_HANDLE, &descriptorSetLayouts[1]));

    VkDescriptorSetLayoutCreateInfo setInfo = vkinit::descriptorSetLayoutCreateInfo(texBindings, std::size(texBindings));
    CHECK_RESULT_VK(vkCreateDescriptorSetLayout(logicalDevice, &setInfo, VK_NULL_HANDLE, &descriptorSetLayouts[2]));

    VkPipelineLayoutCreateInfo pPipelineLayoutCreateInfo = vkinit::pipelineLayoutCreateInfo(
        descriptorSetLayouts.data(), descriptorSetLayouts.size());
    CHECK_RESULT_VK(vkCreatePipelineLayout(logicalDevice, &pPipelineLayoutCreateInfo, VK_NULL_HANDLE, &pipelineLayout));
}

void Body::preparePipeline()
{
    VkGraphicsPipelineCreateInfo* pipelineCreateInfo = thinDrawer->getPipelineInfoBase();

    pipelineCreateInfo->layout = pipelineLayout;

    VkVertexInputBindingDescription vertexInputBinding = { };
    vertexInputBinding.binding = 0;
    vertexInputBinding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    vertexInputBinding.stride = sizeof(s_averageVertex);

    VkVertexInputAttributeDescription attributeDescriptions[3];
    s_averageVertex::getVertexAttributeDescriptions(attributeDescriptions);

    VkPipelineVertexInputStateCreateInfo vertexInputState = { };
    vertexInputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputState.vertexBindingDescriptionCount = 1;
    vertexInputState.vertexAttributeDescriptionCount = std::size(attributeDescriptions);
    vertexInputState.pVertexAttributeDescriptions = attributeDescriptions;
    vertexInputState.pVertexBindingDescriptions = &vertexInputBinding;

    std::vector<std::string> fileNames =
    {
            std::string("shaders/Body/vertex_shader.vert.spv"),
            std::string("shaders/Body/fragment_shader.frag.spv")
    };

    Shader shader = Shader(logicalDevice, fileNames);

    pipelineCreateInfo->stageCount = shader.shaderStages.size();
    pipelineCreateInfo->pStages = shader.shaderStages.data();

    pipelineCreateInfo->pVertexInputState = &vertexInputState;

    ((VkPipelineRasterizationStateCreateInfo*)pipelineCreateInfo->pRasterizationState)->cullMode = VK_CULL_MODE_BACK_BIT;

    CHECK_RESULT_VK(vkCreateGraphicsPipelines(logicalDevice, VK_NULL_HANDLE, 1, pipelineCreateInfo, VK_NULL_HANDLE, &pipeline));

    ThinDrawer::freePipelineData(pipelineCreateInfo);
}

void Body::setupDescriptorSet()
{
    int bindingCount = 1;

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

        writeDescriptorSet.dstBinding = 1;
        writeDescriptorSet.pBufferInfo = &uniformBuffers[i][1]->descriptor;
        vkUpdateDescriptorSets(logicalDevice, 1, &writeDescriptorSet, 0, VK_NULL_HANDLE);
    }
}