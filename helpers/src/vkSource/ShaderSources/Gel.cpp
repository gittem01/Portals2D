#include <ShaderHeaders/Gel.h>
#include <ThinDrawer.h>
#include <Camera.h>
#include <Shader.h>
#include <definitions.h>
#include <vkInit.h>
#include <SwapChain.h>

void GelShader::preCalculate()
{
    xSquareCount = (thinDrawer->wh->cam->xSides.y - thinDrawer->wh->cam->xSides.x) / GEL_SQUARE_SIZE + 4;
    ySquareCount = (thinDrawer->wh->cam->ySides.y - thinDrawer->wh->cam->ySides.x) / GEL_SQUARE_SIZE + 4;
    xSquareCount = std::max(xSquareCount, 4);
    ySquareCount = std::max(ySquareCount, 4);
    requiredSquareSize = xSquareCount * ySquareCount;

    for (int i = 0; i < requiredSquareSize; i++)
    {
		preFillSpatialData[i].gels[0][0] = 0;
    }
    
    lastIndex = 0;
}

void GelShader::putGel(int gelType, glm::vec2 pos, float radius)
{
    int i = lastIndex++;
    
    preFillGelData[i].posR.x = pos.x;
    preFillGelData[i].posR.y = pos.y;
    preFillGelData[i].posR.z = radius;
    preFillGelData[i].color = gelColors[gelType];

    glm::vec2 posDiff = pos + glm::vec2(xSquareCount, ySquareCount) * (GEL_SQUARE_SIZE / 2.0f) - thinDrawer->wh->cam->pos * glm::vec2(1.0, -1.0);

    if (posDiff.x < 0 || posDiff.y < 0 || posDiff.x >= xSquareCount * GEL_SQUARE_SIZE || posDiff.y >= ySquareCount * GEL_SQUARE_SIZE)
    {
        return;
    }

    int ind = (int)(posDiff.x / GEL_SQUARE_SIZE) + (int)(posDiff.y / GEL_SQUARE_SIZE) * xSquareCount;
    if (ind < 0 || ind >= requiredSquareSize)
    {
        assert(false);
    }
    
    glm::ivec4* spatialData = preFillSpatialData[ind].gels;
    int sInd = ++spatialData[0][0];
    spatialData[sInd / 4][sInd % 4] = i;
}

void GelShader::render(VkCommandBuffer cmdBuff, int swapChainImage)
{
    int memStart1 = sizeof(s_perGelData) * MAX_GEL_COUNT * swapChainImage;
    int memSize1 = sizeof(s_perGelData) * lastIndex;

    int memStart2 = sizeof(s_gelSpatialData) * numSquares * swapChainImage;
    int memSize2 = sizeof(s_gelSpatialData) * requiredSquareSize;

    int numRender = (int)(thinDrawer->wh->cam->xSides.y - thinDrawer->wh->cam->xSides.x + 2) *
                    (int)(thinDrawer->wh->cam->ySides.y - thinDrawer->wh->cam->ySides.x + 2) * (1 << 2);

    s_perGelData* pGelData;
    s_gelSpatialData* pGelSpatialData;
    vkMapMemory(logicalDevice, uniformBuffers[0].memory, memStart1, memSize1, 0, (void**)&pGelData);
    vkMapMemory(logicalDevice, uniformBuffers[1].memory, memStart2, memSize2, 0, (void**)&pGelSpatialData);

	memcpy(pGelData, preFillGelData, memSize1);
	memcpy(pGelSpatialData, preFillSpatialData, memSize2);

    vkUnmapMemory(logicalDevice, uniformBuffers[0].memory);
    vkUnmapMemory(logicalDevice, uniformBuffers[1].memory);

    VkDeviceSize offsets = 0;

    s_gelData gelData;
    gelData.iData = glm::ivec4(swapChainImage, 0, numSquares, memStart2 / sizeof(s_gelSpatialData));
    gelData.iData2 = glm::ivec4(xSquareCount, ySquareCount, 0, memStart1 / sizeof(s_perGelData));
    vkCmdPushConstants(cmdBuff, pipelineLayout,
        VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(s_gelData), &gelData);

    vkCmdBindPipeline(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

    vkCmdBindVertexBuffers(cmdBuff, 0, 1, &thinDrawer->wallLight->buffers[0].buffer, &offsets);

    vkCmdBindDescriptorSets(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS,
        pipelineLayout, 0, 1, &ShaderBase::sharedSet, 1, &thinDrawer->camOffset);

    vkCmdBindDescriptorSets(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS,
        pipelineLayout, 1, 1, &descriptorSets[1], 0, VK_NULL_HANDLE);

    vkCmdDraw(cmdBuff, 6, numRender, 0, 0);
}

void GelShader::prepareVertexData()
{

}

void GelShader::prepareUniforms()
{
    uniformBuffers.resize(2);

    Camera* cam = thinDrawer->wh->cam;
    numSquares = (int)(cam->baseX / (cam->zoomLimits.x * cam->zoomLimits.x * GEL_SQUARE_SIZE)) + 6; numSquares *= numSquares;
    spatialDataSize = sizeof(s_gelSpatialData) * numSquares * thinDrawer->swapChain->imageCount;
    spatialDataLayerSize = sizeof(s_gelSpatialData) * numSquares;

    thinDrawer->uniformHelper(sizeof(s_perGelData) * MAX_GEL_COUNT * thinDrawer->swapChain->imageCount,
                                &uniformBuffers[0], VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
    thinDrawer->uniformHelper(spatialDataSize, &uniformBuffers[1], VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);

    // fill with random data
    basePositions.resize(MAX_GEL_COUNT);
    for (int i = 0; i < basePositions.size(); i++)
    {
        basePositions[i].posR = glm::vec4(((float)rand() / RAND_MAX) * 20.0 - 10, ((float)rand() / RAND_MAX) * 20.0 - 10,
            ((float)rand() / RAND_MAX) * 0.7 + 0.4, 0.0);
		
        preFillGelData[i] = basePositions[i];
    }

    s_perGelData* mapMem;
    vkMapMemory(logicalDevice, uniformBuffers[0].memory, 0, sizeof(s_perGelData) * basePositions.size(), 0, (void**)&mapMem);
    memcpy(mapMem, basePositions.data(), sizeof(s_perGelData) * MAX_GEL_COUNT);
    vkUnmapMemory(logicalDevice, uniformBuffers[0].memory);
}

void GelShader::setupDescriptorSetLayout()
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

void GelShader::preparePipeline()
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

void GelShader::setupDescriptorSet()
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
    writeDescriptorSet.pBufferInfo = &uniformBuffers[0].descriptor;
    writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;

    vkUpdateDescriptorSets(logicalDevice, 1, &writeDescriptorSet, 0, VK_NULL_HANDLE);

    writeDescriptorSet.pBufferInfo = &uniformBuffers[1].descriptor;
    writeDescriptorSet.dstBinding = 1;
    vkUpdateDescriptorSets(logicalDevice, 1, &writeDescriptorSet, 0, VK_NULL_HANDLE);

    preCalculate();
}