#pragma once

#include <definitions.h>
#include <SDL3/SDL.h>
#include <vulkan/vulkan.h>
#include <vector>

class ThinDrawer;

class SwapChain
{
public:
    static bool enableVsync;

    ThinDrawer* thinDrawer;

    VkSwapchainKHR swapChain;
    VkSurfaceKHR surface;
    VkSurfaceFormatKHR surfaceFormat;
    VkExtent2D extent;

    VkSurfaceCapabilitiesKHR surfaceCapabilities;
    std::vector<VkSurfaceFormatKHR> surfaceFormats;
    std::vector<VkPresentModeKHR> presentModes;

    std::vector<VkImage> images;
    std::vector<VkImageView> imageViews;
    
    VkImage colorImage;
    VkImageView colorImageView;
    VkDeviceMemory colorImageMemory;

    std::vector<VkFramebuffer> frameBuffers;
    int imageCount;

    SwapChain(ThinDrawer* td);

    void destroy();

    VkExtent2D chooseSwapExtent();
    void querySwapChainSupport();
    void createSwapChain();
    void createFrameBuffers();
    void createColorResources();

    void creationLoop();
};