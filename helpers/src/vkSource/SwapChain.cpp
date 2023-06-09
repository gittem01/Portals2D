#include <SwapChain.h>
#include <ThinDrawer.h>
#include <algorithm>

#define MIN(a, b) a > (b) ? (a) : b
#define MAX(a, b) a < (b) ? (a) : b

SwapChain::SwapChain(ThinDrawer* td)
{
    this->thinDrawer = td;
}

void SwapChain::destroy()
{
    for (int i = 0; i < imageCount; i++) {
        vkDestroyFramebuffer(thinDrawer->logicalDevice, frameBuffers[i], VK_NULL_HANDLE);
        vkDestroyImageView(thinDrawer->logicalDevice, imageViews[i], VK_NULL_HANDLE);
    }

    free(frameBuffers);
    free(imageViews);
    free(images);

    vkDestroySwapchainKHR(thinDrawer->logicalDevice, swapChain, VK_NULL_HANDLE);

    if (thinDrawer->samples > VK_SAMPLE_COUNT_1_BIT) {
        vkDestroyImageView(thinDrawer->logicalDevice, colorImageView, VK_NULL_HANDLE);
        vkFreeMemory(thinDrawer->logicalDevice, colorImageMemory, VK_NULL_HANDLE);
        vkDestroyImage(thinDrawer->logicalDevice, colorImage, VK_NULL_HANDLE);
    }
}

void SwapChain::creationLoop()
{
    for (;;)
    {
        glfwPollEvents();
        querySwapChainSupport();
        extent = chooseSwapExtent();
        if (extent.width > 0 && extent.height > 0)
        {
            createSwapChain();
            return;
        }
    }
}

VkExtent2D SwapChain::chooseSwapExtent() {
    if (surfaceCapabilities.currentExtent.width != UINT32_MAX)
    {
        return surfaceCapabilities.currentExtent;
    }
    else
    {
        int width, height;
        glfwGetFramebufferSize(thinDrawer->wh->window, &width, &height);

        VkExtent2D actualExtent =
        {
            static_cast<uint32_t>(width),
            static_cast<uint32_t>(height)
        };

        actualExtent.width = std::clamp(actualExtent.width, surfaceCapabilities.minImageExtent.width, surfaceCapabilities.maxImageExtent.width);
        actualExtent.height = std::clamp(actualExtent.height, surfaceCapabilities.minImageExtent.height, surfaceCapabilities.maxImageExtent.height);

        return actualExtent;
    }
}

void SwapChain::querySwapChainSupport()
{
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(thinDrawer->physicalDevice, surface, &surfaceCapabilities);

    int surfaceFormatCount;
    int presentModeCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(thinDrawer->physicalDevice, surface, (uint32_t*)&surfaceFormatCount, NULL);
    if (surfaceFormatCount != 0)
    {
        surfaceFormats.resize(surfaceFormatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(thinDrawer->physicalDevice, surface, (uint32_t*)&surfaceFormatCount, surfaceFormats.data());
    }

    vkGetPhysicalDeviceSurfacePresentModesKHR(thinDrawer->physicalDevice, surface, (uint32_t*)&presentModeCount, NULL);
    if (presentModeCount != 0)
    {
        presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(thinDrawer->physicalDevice, surface, (uint32_t*)&presentModeCount, presentModes.data());
    }
}

void SwapChain::createSwapChain()
{
    querySwapChainSupport();

    if (surfaceFormats.size() == 0 || presentModes.size() == 0)
    {
        printf("surfaceFormatCount == 0 || presentModeCount == 0\n");
        exit(1);
    }

    int i;
    for (i = 0; i < surfaceFormats.size(); i++)
    {
        if (surfaceFormats[i].format == VK_FORMAT_B8G8R8A8_SRGB && surfaceFormats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
        {
            surfaceFormat = surfaceFormats[i];
            break;
        }
    }
    if (i == surfaceFormats.size())
    {
        surfaceFormat = surfaceFormats[0];
    }

    VkPresentModeKHR presentMode;
    int mask = 0;
    for (i = 0; i < presentModes.size(); i++)
    {
        mask |= (int)presentModes[i];
    }

    if (mask & VK_PRESENT_MODE_FIFO_KHR)
    {
        presentMode = VK_PRESENT_MODE_FIFO_KHR;
    }
    else if (mask & VK_PRESENT_MODE_MAILBOX_KHR)
    {
        presentMode = VK_PRESENT_MODE_MAILBOX_KHR;
    }
    else
    {
        presentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
    }

    presentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;

    extent = chooseSwapExtent();

    imageCount = (int)surfaceCapabilities.minImageCount + 1;
    if (surfaceCapabilities.maxImageCount > 0 && imageCount > surfaceCapabilities.maxImageCount)
    {
        imageCount = (int)surfaceCapabilities.maxImageCount;
    }
    else
    {
        imageCount = (int)surfaceCapabilities.minImageCount + 1;
    }

    VkSwapchainCreateInfoKHR swapChainCreateInfo = { };
    swapChainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapChainCreateInfo.surface = surface;
    swapChainCreateInfo.minImageCount = imageCount;
    swapChainCreateInfo.imageExtent = extent;
    swapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    swapChainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    swapChainCreateInfo.imageArrayLayers = 1;
    swapChainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapChainCreateInfo.preTransform = surfaceCapabilities.currentTransform;
    swapChainCreateInfo.imageFormat = surfaceFormat.format;
    swapChainCreateInfo.imageColorSpace = surfaceFormat.colorSpace;
    swapChainCreateInfo.presentMode = presentMode;
    swapChainCreateInfo.clipped = VK_TRUE;

    CHECK_RESULT_VK(vkCreateSwapchainKHR(thinDrawer->logicalDevice, &swapChainCreateInfo, NULL, &swapChain));

    vkGetSwapchainImagesKHR(thinDrawer->logicalDevice, swapChain,
        (uint32_t*)&imageCount, NULL);
    images = (VkImage*)malloc(sizeof(VkImage) * imageCount);
    imageViews = (VkImageView*)malloc(sizeof(VkImageView) * imageCount);
    vkGetSwapchainImagesKHR(thinDrawer->logicalDevice, swapChain,
        (uint32_t*)&imageCount, images);

    for (i = 0; i < imageCount; i++)
    {
        VkImageViewCreateInfo createInfo = { };
        createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.image = images[i];
        createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        createInfo.format = surfaceFormat.format;
        createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        createInfo.subresourceRange.baseMipLevel = 0;
        createInfo.subresourceRange.levelCount = 1;
        createInfo.subresourceRange.baseArrayLayer = 0;
        createInfo.subresourceRange.layerCount = 1;

        CHECK_RESULT_VK(vkCreateImageView(thinDrawer->logicalDevice, &createInfo, NULL, &imageViews[i]));
    }

    if (thinDrawer->samples > VK_SAMPLE_COUNT_1_BIT)
    {
        createColorResources();
    }
}

void SwapChain::createFrameBuffers()
{
    VkFramebufferCreateInfo frameBufferInfo = { };
	frameBufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;

	frameBufferInfo.renderPass = thinDrawer->renderPass;
	frameBufferInfo.width = extent.width;
	frameBufferInfo.height = extent.height;
	frameBufferInfo.layers = 1;

	frameBuffers = (VkFramebuffer*)malloc(sizeof(VkFramebuffer) * imageCount);

	for (int i = 0; i < imageCount; i++)
    {
        std::vector<VkImageView> attachments;
        if (thinDrawer->samples > VK_SAMPLE_COUNT_1_BIT) attachments = { colorImageView, imageViews[i] };
        else attachments = { imageViews[i] };

		frameBufferInfo.attachmentCount = attachments.size();
		frameBufferInfo.pAttachments = attachments.data();

        CHECK_RESULT_VK(vkCreateFramebuffer(thinDrawer->logicalDevice, &frameBufferInfo, NULL, frameBuffers + i));
	}
}

void SwapChain::createColorResources()
{
    VkExtent3D colorImageExtent =
    {
            extent.width,
            extent.height,
            1
    };

    thinDrawer->createImage(extent.width, extent.height, 1, thinDrawer->samples, surfaceFormat.format,
        VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, colorImage, colorImageMemory);

    VkImageViewCreateInfo imageViewCreateInfo = { };
    imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    imageViewCreateInfo.format = surfaceFormat.format;
    imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
    imageViewCreateInfo.subresourceRange.levelCount = 1;
    imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
    imageViewCreateInfo.subresourceRange.layerCount = 1;
    imageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    imageViewCreateInfo.image = colorImage;

    CHECK_RESULT_VK(vkCreateImageView(thinDrawer->logicalDevice, &imageViewCreateInfo, NULL, &colorImageView));
}