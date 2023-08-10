#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <tiny_gltf.h>

#include <ThinDrawer.h>
#include <SwapChain.h>
#include <vkInit.h>
#include <IOHelper.h>

bool ThinDrawer::isDeviceSuitable(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface)
{
    VkSurfaceCapabilitiesKHR surfaceCapabilities;
    VkSurfaceFormatKHR* surfaceFormats;
    VkPresentModeKHR* presentModes;

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &surfaceCapabilities);

    int formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, (uint32_t*)&formatCount, VK_NULL_HANDLE);
    if (formatCount != 0)
    {
        surfaceFormats = (VkSurfaceFormatKHR*)malloc(sizeof(VkSurfaceFormatKHR) * formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, (uint32_t*)&formatCount, surfaceFormats);
    }

    int presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, (uint32_t*)&presentModeCount, VK_NULL_HANDLE);
    if (presentModeCount != 0)
    {
        presentModes = (VkPresentModeKHR*)malloc(sizeof(VkPresentModeKHR) * presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, (uint32_t*)&presentModeCount, presentModes);
    }
    if (formatCount != 0)
    {
        free(surfaceFormats);
    }
    if (presentModeCount != 0)
    {
        free(presentModes);
    }

    return presentModeCount != 0 && formatCount != 0;
}

VkCommandBuffer ThinDrawer::getCommandBuffer(bool begin)
{
    VkCommandBuffer cmdBuffer;

    VkCommandBufferAllocateInfo cmdBufAllocateInfo = { };
    cmdBufAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cmdBufAllocateInfo.commandPool = uploadPool;
    cmdBufAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cmdBufAllocateInfo.commandBufferCount = 1;

    CHECK_RESULT_VK(vkAllocateCommandBuffers(logicalDevice, &cmdBufAllocateInfo, &cmdBuffer));

    if (begin)
    {
        VkCommandBufferBeginInfo cmdBufInfo = vkinit::commandBufferBeginInfo();
        CHECK_RESULT_VK(vkBeginCommandBuffer(cmdBuffer, &cmdBufInfo));
    }

    return cmdBuffer;
}

void ThinDrawer::flushCommandBuffer(VkCommandBuffer commandBuffer)
{
    CHECK_RESULT_VK(vkEndCommandBuffer(commandBuffer));

    VkSubmitInfo submitInfo = { };
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    VkFenceCreateInfo fenceCreateInfo = vkinit::fenceCreateInfo();
    VkFence fence;
    CHECK_RESULT_VK(vkCreateFence(logicalDevice, &fenceCreateInfo, VK_NULL_HANDLE, &fence));

    CHECK_RESULT_VK(vkQueueSubmit(queues.graphicsQueue, 1, &submitInfo, fence));
    CHECK_RESULT_VK(vkWaitForFences(logicalDevice, 1, &fence, VK_TRUE, UINT64_MAX));

    vkDestroyFence(logicalDevice, fence, VK_NULL_HANDLE);
    vkFreeCommandBuffers(logicalDevice, uploadPool, 1, &commandBuffer);
}

uint32_t ThinDrawer::getMemoryTypeIndex(uint32_t typeBits, VkMemoryPropertyFlags properties)
{
    for (uint32_t i = 0; i < vulkanInfo->deviceMemoryProperties.memoryTypeCount; i++)
    {
        if ((typeBits & 1) == 1)
        {
            if ((vulkanInfo->deviceMemoryProperties.memoryTypes[i].propertyFlags & properties) == properties)
            {
                return i;
            }
        }
        typeBits >>= 1;
    }

    printf("Could not find a suitable memory type!");
    exit(-1);
}

void ThinDrawer::createImage(uint32_t p_width, uint32_t p_height, uint32_t mipLevels,
                             VkSampleCountFlagBits numSamples, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage,
                             VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory)
{
    VkImageCreateInfo imageInfo = { };
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = p_width;
    imageInfo.extent.height = p_height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = mipLevels;
    imageInfo.arrayLayers = 1;
    imageInfo.format = format;
    imageInfo.tiling = tiling;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = usage;
    imageInfo.samples = numSamples;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    CHECK_RESULT_VK(vkCreateImage(logicalDevice, &imageInfo, VK_NULL_HANDLE, &image));

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(logicalDevice, image, &memRequirements);

    VkMemoryAllocateInfo allocInfo = { };
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = getMemoryTypeIndex(memRequirements.memoryTypeBits, properties);

    CHECK_RESULT_VK(vkAllocateMemory(logicalDevice, &allocInfo, VK_NULL_HANDLE, &imageMemory));

    CHECK_RESULT_VK(vkBindImageMemory(logicalDevice, image, imageMemory, 0));
}

void ThinDrawer::bufferStage(void* bufferData, uint32_t dataSize, VkBufferUsageFlags flags, s_buffers* fillBuffer)
{
    VkMemoryAllocateInfo memAlloc = { };
    memAlloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    VkMemoryRequirements memReqs;

    VkBufferCreateInfo vertexBufferInfo = vkinit::bufferCreateInfo(dataSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT);

    s_stagingBuffer stagingBuffer;

    CHECK_RESULT_VK(vkCreateBuffer(logicalDevice, &vertexBufferInfo, VK_NULL_HANDLE, &stagingBuffer.buffer));
    vkGetBufferMemoryRequirements(logicalDevice, stagingBuffer.buffer, &memReqs);
    memAlloc.allocationSize = memReqs.size;
    memAlloc.memoryTypeIndex = getMemoryTypeIndex(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
        VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    CHECK_RESULT_VK(vkAllocateMemory(logicalDevice, &memAlloc, VK_NULL_HANDLE, &stagingBuffer.memory));

    void* data;
    CHECK_RESULT_VK(vkMapMemory(logicalDevice, stagingBuffer.memory, 0, memAlloc.allocationSize, 0, &data));
    memcpy(data, bufferData, dataSize);
    vkUnmapMemory(logicalDevice, stagingBuffer.memory);
    CHECK_RESULT_VK(vkBindBufferMemory(logicalDevice, stagingBuffer.buffer, stagingBuffer.memory, 0));

    vertexBufferInfo.usage = flags;
    CHECK_RESULT_VK(vkCreateBuffer(logicalDevice, &vertexBufferInfo, VK_NULL_HANDLE, &fillBuffer->buffer));
    vkGetBufferMemoryRequirements(logicalDevice, fillBuffer->buffer, &memReqs);
    memAlloc.allocationSize = memReqs.size;
    memAlloc.memoryTypeIndex = getMemoryTypeIndex(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    CHECK_RESULT_VK(vkAllocateMemory(logicalDevice, &memAlloc, VK_NULL_HANDLE, &fillBuffer->memory));
    CHECK_RESULT_VK(vkBindBufferMemory(logicalDevice, fillBuffer->buffer, fillBuffer->memory, 0));

    VkCommandBuffer copyCmd = getCommandBuffer(true);
    VkBufferCopy copyRegion = { };
    copyRegion.size = dataSize;
    vkCmdCopyBuffer(copyCmd, stagingBuffer.buffer, fillBuffer->buffer, 1, &copyRegion);
    flushCommandBuffer(copyCmd);

    vkDestroyBuffer(logicalDevice, stagingBuffer.buffer, VK_NULL_HANDLE);
    vkFreeMemory(logicalDevice, stagingBuffer.memory, VK_NULL_HANDLE);
};

VkCommandBuffer ThinDrawer::createCommandBuffer(VkCommandBufferLevel level, VkCommandPool pool, const bool begin)
{
    VkCommandBufferAllocateInfo cmdBufAllocateInfo = { };
    cmdBufAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cmdBufAllocateInfo.commandPool = pool;
    cmdBufAllocateInfo.level = level;
    cmdBufAllocateInfo.commandBufferCount = 1;

    VkCommandBuffer cmdBuffer;
    CHECK_RESULT_VK(vkAllocateCommandBuffers(logicalDevice, &cmdBufAllocateInfo, &cmdBuffer));
    if (begin)
    {
        VkCommandBufferBeginInfo cmdBufInfo = vkinit::commandBufferBeginInfo();
        CHECK_RESULT_VK(vkBeginCommandBuffer(cmdBuffer, &cmdBufInfo));
    }
    return cmdBuffer;
}

void ThinDrawer::generateMipmaps(VkImage image, VkFormat imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels)
{
    VkFormatProperties formatProperties;
    vkGetPhysicalDeviceFormatProperties(physicalDevice, imageFormat, &formatProperties);

    if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT))
    {
        printf("linear blitting is not supported");
        exit(-1);
    }

    VkCommandBuffer commandBuffer = getCommandBuffer(true);

    VkImageMemoryBarrier barrier = { };
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.image = image;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    barrier.subresourceRange.levelCount = 1;

    int32_t mipWidth = texWidth;
    int32_t mipHeight = texHeight;

    for (uint32_t i = 1; i < mipLevels; i++)
    {
        barrier.subresourceRange.baseMipLevel = i - 1;
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

        vkCmdPipelineBarrier(commandBuffer,
            VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
            0, nullptr,
            0, nullptr,
            1, &barrier);

        VkImageBlit blit{};
        blit.srcOffsets[0] = { 0, 0, 0 };
        blit.srcOffsets[1] = { mipWidth, mipHeight, 1 };
        blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.srcSubresource.mipLevel = i - 1;
        blit.srcSubresource.baseArrayLayer = 0;
        blit.srcSubresource.layerCount = 1;
        blit.dstOffsets[0] = { 0, 0, 0 };
        blit.dstOffsets[1] = { mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1 };
        blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.dstSubresource.mipLevel = i;
        blit.dstSubresource.baseArrayLayer = 0;
        blit.dstSubresource.layerCount = 1;

        vkCmdBlitImage(commandBuffer,
            image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1, &blit,
            VK_FILTER_LINEAR);

        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        vkCmdPipelineBarrier(commandBuffer,
            VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
            0, nullptr,
            0, nullptr,
            1, &barrier);

        if (mipWidth > 1) mipWidth /= 2;
        if (mipHeight > 1) mipHeight /= 2;
    }

    barrier.subresourceRange.baseMipLevel = mipLevels - 1;
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    vkCmdPipelineBarrier(commandBuffer,
        VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
        0, nullptr,
        0, nullptr,
        1, &barrier);

    flushCommandBuffer(commandBuffer);
}

void ThinDrawer::loadTexture(char* fileName, s_texture* texture, bool enableAnisotropy, VkSamplerAddressMode addressMode, VkFilter filter, float lodBias)
{
    std::string finalPath = IOHelper::assetsFolder + fileName;

    int width, height, nChannels;
    unsigned char* pixels = (unsigned char*)stbi_load(finalPath.c_str(), &width, &height, &nChannels, STBI_default);

    if (!pixels)
    {
        printf("Failed to load texture file: %s\n", finalPath.c_str());
        exit(-1);
    }

    s_textureData tData;
    tData.data = pixels;
    tData.width = width;
    tData.height = height;
    tData.numChannels = nChannels;
    loadTextureFromMemory(&tData, texture, enableAnisotropy, addressMode, filter, lodBias);

    stbi_image_free(pixels);
}

void ThinDrawer::loadTextureFromMemory(s_textureData* texData, s_texture* texture, bool enableAnisotropy, VkSamplerAddressMode addressMode, VkFilter filter, float lodBias)
{
    VkFormat format;
    if (texData->numChannels == 4)
    {
        format = VK_FORMAT_R8G8B8A8_SRGB;
    }
    else
    {
        format = (VkFormat)(VK_FORMAT_R8_UNORM + (texData->numChannels - 1) * 7);
    }

    VkFormatProperties formatProperties;
    vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &formatProperties);

    if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT))
    {
        format = VK_FORMAT_R8G8B8A8_UNORM;
    }

    VkDeviceSize imageSize = texData->width * texData->height * texData->numChannels;

    texture->width = texData->width;
    texture->height = texData->height;
    texture->mipLevels = (uint32_t)std::floor(std::log2(std::max(texData->width, texData->height))) + 1;

    VkMemoryAllocateInfo memAllocInfo = { };
    memAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;

    VkMemoryRequirements memReqs = { };

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingMemory;

    VkBufferCreateInfo bufferCreateInfo = vkinit::bufferCreateInfo(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
    CHECK_RESULT_VK(vkCreateBuffer(logicalDevice, &bufferCreateInfo, VK_NULL_HANDLE, &stagingBuffer));

    vkGetBufferMemoryRequirements(logicalDevice, stagingBuffer, &memReqs);
    memAllocInfo.allocationSize = memReqs.size;
    memAllocInfo.memoryTypeIndex = getMemoryTypeIndex(memReqs.memoryTypeBits,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    CHECK_RESULT_VK(vkAllocateMemory(logicalDevice, &memAllocInfo, VK_NULL_HANDLE, &stagingMemory));
    CHECK_RESULT_VK(vkBindBufferMemory(logicalDevice, stagingBuffer, stagingMemory, 0));

    uint8_t* data;
    CHECK_RESULT_VK(vkMapMemory(logicalDevice, stagingMemory, 0, memReqs.size, 0, (void**)&data));
    memcpy(data, texData->data, imageSize);
    vkUnmapMemory(logicalDevice, stagingMemory);

    VkExtent3D imageExtent;
    imageExtent.width = static_cast<uint32_t>(texData->width);
    imageExtent.height = static_cast<uint32_t>(texData->height);
    imageExtent.depth = 1;

    VkBufferImageCopy copyRegion = { };
    copyRegion.bufferOffset = 0;
    copyRegion.bufferRowLength = 0;
    copyRegion.bufferImageHeight = 0;

    copyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    copyRegion.imageSubresource.mipLevel = 0;
    copyRegion.imageSubresource.baseArrayLayer = 0;
    copyRegion.imageSubresource.layerCount = 1;
    copyRegion.imageExtent = imageExtent;

    createImage(texture->width, texture->height, texture->mipLevels, VK_SAMPLE_COUNT_1_BIT, format, VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        texture->image, texture->deviceMemory);

    VkCommandBuffer copyCmd = createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, uploadPool, true);

    VkImageSubresourceRange subresourceRange = { };
    subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    subresourceRange.baseMipLevel = 0;
    subresourceRange.levelCount = texture->mipLevels;
    subresourceRange.layerCount = 1;

    VkImageMemoryBarrier imageMemoryBarrier = { };
    imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    imageMemoryBarrier.image = texture->image;
    imageMemoryBarrier.subresourceRange = subresourceRange;
    imageMemoryBarrier.srcAccessMask = 0;
    imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;

    vkCmdPipelineBarrier(
        copyCmd, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
        0, 0, VK_NULL_HANDLE, 0, VK_NULL_HANDLE, 1, &imageMemoryBarrier);

    vkCmdCopyBufferToImage(copyCmd, stagingBuffer, texture->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);

    flushCommandBuffer(copyCmd);

    vkFreeMemory(logicalDevice, stagingMemory, VK_NULL_HANDLE);
    vkDestroyBuffer(logicalDevice, stagingBuffer, VK_NULL_HANDLE);

    generateMipmaps(texture->image, format, texData->width, texData->height, texture->mipLevels);

    texture->imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    VkSamplerCreateInfo sampler = { };
    sampler.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    sampler.magFilter = filter;
    sampler.minFilter = VK_FILTER_LINEAR;
    sampler.mipmapMode = (VkSamplerMipmapMode)filter;
    sampler.addressModeU = addressMode;
    sampler.addressModeV = addressMode;
    sampler.addressModeW = addressMode;
    sampler.mipLodBias = lodBias;
    sampler.compareOp = VK_COMPARE_OP_NEVER;
    sampler.minLod = 0.0f;
    sampler.maxLod = VK_LOD_CLAMP_NONE;
    sampler.maxAnisotropy = 16.0;
    sampler.anisotropyEnable = (VkBool32)enableAnisotropy;
    sampler.borderColor = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;

    CHECK_RESULT_VK(vkCreateSampler(logicalDevice, &sampler, VK_NULL_HANDLE, &texture->sampler));

    VkImageViewCreateInfo view = { };
    view.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    view.viewType = VK_IMAGE_VIEW_TYPE_2D;
    view.format = format;
    view.components = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };

    view.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    view.subresourceRange.baseMipLevel = 0;
    view.subresourceRange.baseArrayLayer = 0;
    view.subresourceRange.layerCount = 1;
    view.subresourceRange.levelCount = texture->mipLevels;
    view.image = texture->image;

    CHECK_RESULT_VK(vkCreateImageView(logicalDevice, &view, VK_NULL_HANDLE, &texture->view));
}

VkGraphicsPipelineCreateInfo* ThinDrawer::getPipelineInfoBase()
{
    VkGraphicsPipelineCreateInfo* pipelineCreateInfo = (VkGraphicsPipelineCreateInfo*)calloc(1, sizeof(VkGraphicsPipelineCreateInfo));
    pipelineCreateInfo->sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineCreateInfo->renderPass = renderPass;

    VkPipelineInputAssemblyStateCreateInfo* inputAssemblyState = (VkPipelineInputAssemblyStateCreateInfo*)calloc(1, sizeof(VkPipelineInputAssemblyStateCreateInfo));
    inputAssemblyState->sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssemblyState->topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    VkPipelineRasterizationStateCreateInfo* rasterizationStateCreateInfo = (VkPipelineRasterizationStateCreateInfo*)calloc(1, sizeof(VkPipelineRasterizationStateCreateInfo));
    rasterizationStateCreateInfo->sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizationStateCreateInfo->polygonMode = VK_POLYGON_MODE_FILL;
    rasterizationStateCreateInfo->cullMode = VK_CULL_MODE_NONE;
    rasterizationStateCreateInfo->frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizationStateCreateInfo->depthClampEnable = VK_FALSE;
    rasterizationStateCreateInfo->rasterizerDiscardEnable = VK_FALSE;
    rasterizationStateCreateInfo->depthBiasEnable = VK_FALSE;
    rasterizationStateCreateInfo->lineWidth = 1.0f;

    VkPipelineColorBlendAttachmentState* colorBlendAttachment = (VkPipelineColorBlendAttachmentState*)calloc(1, sizeof(VkPipelineColorBlendAttachmentState));
    colorBlendAttachment->colorWriteMask = 0xF;
    colorBlendAttachment->blendEnable = VK_TRUE;
    colorBlendAttachment->alphaBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment->colorBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment->srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    colorBlendAttachment->dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorBlendAttachment->srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment->dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;

    VkPipelineColorBlendStateCreateInfo* colorBlendState = (VkPipelineColorBlendStateCreateInfo*)calloc(1, sizeof(VkPipelineColorBlendStateCreateInfo));
    colorBlendState->sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlendState->attachmentCount = 1;
    colorBlendState->pAttachments = colorBlendAttachment;
    colorBlendState->blendConstants[0] = 1.0f;
    colorBlendState->blendConstants[1] = 1.0f;
    colorBlendState->blendConstants[2] = 1.0f;
    colorBlendState->blendConstants[3] = 1.0f;

    VkPipelineViewportStateCreateInfo* viewportState = (VkPipelineViewportStateCreateInfo*)calloc(1, sizeof(VkPipelineViewportStateCreateInfo));
    viewportState->sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;

    VkViewport* viewport = (VkViewport*)calloc(1, sizeof(VkViewport));
    viewport->width = swapChain->extent.width;
    viewport->height = swapChain->extent.height;
    viewport->minDepth = 0.0f;
    viewport->maxDepth = +1.0f;

    viewportState->viewportCount = 1;
    viewportState->pViewports = viewport;

    VkRect2D* scissor = (VkRect2D*)calloc(1, sizeof(VkRect2D));
    scissor->offset = { 0, 0 };
    scissor->extent = swapChain->extent;

    viewportState->scissorCount = 1;
    viewportState->pScissors = scissor;

    VkPipelineMultisampleStateCreateInfo* multisampleState = (VkPipelineMultisampleStateCreateInfo*)calloc(1, sizeof(VkPipelineMultisampleStateCreateInfo));
    multisampleState->sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampleState->rasterizationSamples = samples;
    // avoid it, instead use manual sampling as much as you can
    multisampleState->sampleShadingEnable = VK_FALSE;
    multisampleState->minSampleShading = 1.0f;

    VkPipelineDepthStencilStateCreateInfo* pipelineDepthStencilCreateInfo = (VkPipelineDepthStencilStateCreateInfo*)calloc(1, sizeof(VkPipelineDepthStencilStateCreateInfo));
    pipelineDepthStencilCreateInfo->sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    pipelineDepthStencilCreateInfo->pNext = NULL;

    pipelineDepthStencilCreateInfo->depthTestEnable = VK_TRUE;
    pipelineDepthStencilCreateInfo->depthWriteEnable = VK_TRUE;
    pipelineDepthStencilCreateInfo->depthCompareOp = VK_COMPARE_OP_LESS;
    pipelineDepthStencilCreateInfo->depthBoundsTestEnable = VK_FALSE;
    pipelineDepthStencilCreateInfo->minDepthBounds = 0.0f; // Optional
    pipelineDepthStencilCreateInfo->maxDepthBounds = 1.0f; // Optional
    pipelineDepthStencilCreateInfo->stencilTestEnable = VK_FALSE;

    int numDynamicStates = 0;
    VkDynamicState* dynamicStates = (VkDynamicState*)calloc(numDynamicStates, sizeof(VkDynamicState));

    VkPipelineDynamicStateCreateInfo* dynamicStateCreateInfo = (VkPipelineDynamicStateCreateInfo*)calloc(1, sizeof(VkPipelineDynamicStateCreateInfo));
    dynamicStateCreateInfo->sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicStateCreateInfo->dynamicStateCount = numDynamicStates;
    dynamicStateCreateInfo->pDynamicStates = dynamicStates;

    pipelineCreateInfo->pInputAssemblyState = inputAssemblyState;
    pipelineCreateInfo->pRasterizationState = rasterizationStateCreateInfo;
    pipelineCreateInfo->pColorBlendState = colorBlendState;
    pipelineCreateInfo->pMultisampleState = multisampleState;
    pipelineCreateInfo->pViewportState = viewportState;
    pipelineCreateInfo->pDepthStencilState = pipelineDepthStencilCreateInfo;
    pipelineCreateInfo->pDynamicState = dynamicStateCreateInfo;

    return pipelineCreateInfo;
}

void ThinDrawer::freePipelineData(VkGraphicsPipelineCreateInfo* pipelineCreateInfo)
{
    free((void*)pipelineCreateInfo->pColorBlendState->pAttachments);
    free((void*)pipelineCreateInfo->pColorBlendState);
    free((void*)pipelineCreateInfo->pViewportState->pViewports);
    free((void*)pipelineCreateInfo->pViewportState->pScissors);
    free((void*)pipelineCreateInfo->pViewportState);
    free((void*)pipelineCreateInfo->pInputAssemblyState);
    free((void*)pipelineCreateInfo->pRasterizationState);
    free((void*)pipelineCreateInfo->pMultisampleState);
    free((void*)pipelineCreateInfo->pDepthStencilState);
    free((void*)pipelineCreateInfo->pDynamicState->pDynamicStates);
    free((void*)pipelineCreateInfo->pDynamicState);
    free((void*)pipelineCreateInfo);
}

void ThinDrawer::uniformHelper(int size, s_uniformBuffer* uniformBuffer, VkBufferUsageFlagBits usage, int numInstances)
{
    VkBufferCreateInfo bufferInfo = vkinit::bufferCreateInfo(size, usage);
    CHECK_RESULT_VK(vkCreateBuffer(logicalDevice, &bufferInfo, VK_NULL_HANDLE, &uniformBuffer->buffer));

    VkMemoryRequirements memReqs;
    VkMemoryAllocateInfo allocInfo = vkinit::memoryAllocateInfo();
    vkGetBufferMemoryRequirements(logicalDevice, uniformBuffer->buffer, &memReqs);
    allocInfo.allocationSize = memReqs.size;
    allocInfo.memoryTypeIndex = getMemoryTypeIndex(memReqs.memoryTypeBits,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    CHECK_RESULT_VK(vkAllocateMemory(logicalDevice, &allocInfo, VK_NULL_HANDLE, &uniformBuffer->memory));
    CHECK_RESULT_VK(vkBindBufferMemory(logicalDevice, uniformBuffer->buffer, uniformBuffer->memory, 0));

    uniformBuffer->descriptor.buffer = uniformBuffer->buffer;
    uniformBuffer->descriptor.offset = 0;
    uniformBuffer->descriptor.range = size / numInstances;
}

void ThinDrawer::updateImageDescriptors(s_texture* tex, VkDescriptorSetLayout& setLayout)
{
    VkDescriptorSetAllocateInfo allocInfo = vkinit::descriptorSetAllocateInfo(descriptorPool, &setLayout, 1);

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

int ThinDrawer::pad_uniform_buffer_size(int originalSize)
{
    // calculate required alignment based on minimum device offset alignment
    int minUboAlignment = vulkanInfo->deviceProperties.limits.minUniformBufferOffsetAlignment;
    int alignedSize_body = originalSize;
    if (minUboAlignment > 0)
    {
        alignedSize_body = (alignedSize_body + minUboAlignment - 1) & ~(minUboAlignment - 1);
    }
    return alignedSize_body;
}

int ThinDrawer::pad_storage_buffer_size(int originalSize)
{
    // calculate required alignment based on minimum device offset alignment
    int minStorageAlignment = vulkanInfo->deviceProperties.limits.minStorageBufferOffsetAlignment;
    int alignedSize_body = originalSize;
    if (minStorageAlignment > 0)
    {
        alignedSize_body = (alignedSize_body + minStorageAlignment - 1) & ~(minStorageAlignment - 1);
    }
    return alignedSize_body;
}