#include <ShaderHeaders/Gel.h>
#include <ThinDrawer.h>
#include <Camera.h>
#include <Shader.h>
#include <definitions.h>
#include <vkInit.h>
#include <SwapChain.h>

void Gel::prepareVertexData()
{

}

void Gel::prepareUniforms()
{
    uniformBuffers.resize(2);

    uniformBuffers[0] = (s_uniformBuffer*)malloc(sizeof(s_uniformBuffer));
    uniformBuffers[1] = (s_uniformBuffer*)malloc(sizeof(s_uniformBuffer));

    Camera* cam = thinDrawer->wh->cam;
    numSquares = (int)(cam->baseX / (cam->zoomLimits.x * GEL_SQUARE_SIZE)) + 1; numSquares *= numSquares;

    spatialDataSize = sizeof(s_gelSpatialData) * numSquares * thinDrawer->swapChain->imageCount * NUM_GEL_TYPE;
    spatialDataLayerSize = sizeof(s_gelSpatialData) * numSquares * NUM_GEL_TYPE;

    thinDrawer->uniformHelper(sizeof(s_perGelData) * 1024 * thinDrawer->swapChain->imageCount * NUM_GEL_TYPE,
                                uniformBuffers[0], VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
    thinDrawer->uniformHelper(spatialDataSize, uniformBuffers[1], VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);

    // fill with random data
    basePositions.resize(1024);
    for (int i = 0; i < basePositions.size(); i++)
    {
        basePositions[i].posR = glm::vec4(((float)rand() / RAND_MAX) * 20.0, ((float)rand() / RAND_MAX) * 20.0,
            ((float)rand() / RAND_MAX) * 0.8 + 0.2, 0.0);
    }

    s_perGelData* mapMem;
    vkMapMemory(logicalDevice, uniformBuffers[0]->memory, 0, sizeof(s_perGelData) * basePositions.size(), 0, (void**)&mapMem);
    memcpy(mapMem, basePositions.data(), sizeof(s_perGelData) * 1024);
    vkUnmapMemory(logicalDevice, uniformBuffers[0]->memory);
}

void Gel::setupDescriptorSetLayout()
{
    const int descriptorSetLayoutCount = 2;

    descriptorSetLayouts.resize(descriptorSetLayoutCount);
    descriptorSetLayouts[0] = ShaderBase::sharedLayout;

    VkDescriptorSetLayoutBinding layoutBinding10 = vkinit::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
        VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 1, 0);

    VkDescriptorSetLayoutBinding layoutBinding11 = vkinit::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
        VK_SHADER_STAGE_VERTEX_BIT, 1, 1);

    VkDescriptorSetLayoutBinding bindings2[] = { layoutBinding10, layoutBinding11 };

    VkDescriptorSetLayoutCreateInfo descriptorLayout2 = vkinit::descriptorSetLayoutCreateInfo(bindings2, sizeof(bindings2) / sizeof(bindings2[0]));
    CHECK_RESULT_VK(vkCreateDescriptorSetLayout(logicalDevice, &descriptorLayout2, VK_NULL_HANDLE, &descriptorSetLayouts[1]));

    VkPipelineLayoutCreateInfo pPipelineLayoutCreateInfo = vkinit::pipelineLayoutCreateInfo(descriptorSetLayouts.data(), descriptorSetLayouts.size());

    VkPushConstantRange pushConstant;
    pushConstant.offset = 0;
    pushConstant.size = sizeof(s_gelData);
    pushConstant.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    pPipelineLayoutCreateInfo.pushConstantRangeCount = 1;
    pPipelineLayoutCreateInfo.pPushConstantRanges = &pushConstant;

    CHECK_RESULT_VK(vkCreatePipelineLayout(logicalDevice, &pPipelineLayoutCreateInfo, VK_NULL_HANDLE, &pipelineLayout));
}

void Gel::preparePipeline()
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
            std::string("shaders/Gel/vertex_shader.vert.spv"),
            std::string("shaders/Gel/fragment_shader.frag.spv")
    };

    Shader shader2 = Shader(logicalDevice, fileNames);

    pipelineCreateInfo->stageCount = shader2.shaderStages.size();
    pipelineCreateInfo->pStages = shader2.shaderStages.data();
    pipelineCreateInfo->pVertexInputState = &vertexInputState;

    CHECK_RESULT_VK(vkCreateGraphicsPipelines(logicalDevice, VK_NULL_HANDLE, 1, pipelineCreateInfo, VK_NULL_HANDLE, &pipeline));

    ThinDrawer::freePipelineData(pipelineCreateInfo);
}

void Gel::setupDescriptorSet()
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
    writeDescriptorSet.pBufferInfo = &uniformBuffers[0]->descriptor;
    writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;

    vkUpdateDescriptorSets(logicalDevice, 1, &writeDescriptorSet, 0, VK_NULL_HANDLE);

    writeDescriptorSet.pBufferInfo = &uniformBuffers[1]->descriptor;
    writeDescriptorSet.dstBinding = 1;
    vkUpdateDescriptorSets(logicalDevice, 1, &writeDescriptorSet, 0, VK_NULL_HANDLE);
}