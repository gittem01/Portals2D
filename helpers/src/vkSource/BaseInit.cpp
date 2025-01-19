#include <ThinDrawer.h>
#include <SwapChain.h>
#include <IOHelper.h>
#include <Camera.h>
#include "SDL3/SDL_vulkan.h"

#define PREFER_DISCRETE 1

void ThinDrawer::createWindow()
{
    wh = new WindowHandler(NULL);
    Camera* cam = new Camera(glm::vec2(0.0f, 0.0f), wh);
    wh->cam = cam;
}

void ThinDrawer::initBase()
{
    IOHelper::thinDrawer = this;

    swapChain = new SwapChain(this);

    createWindow();

    createInstance();

    if (!SDL_Vulkan_CreateSurface(wh->window, instance, nullptr, &swapChain->surface))
    {
		assert(0);
    }

    selectPhysicalDevice();

    createLogicalDevice();

    setSamples();

    swapChain->createSwapChain();

    createRenderPass();

    swapChain->createFrameBuffers();

    frames.resize(swapChain->imageCount);

    createCommands();

    createSyncThings();
}

void ThinDrawer::createInstance()
{
    VkApplicationInfo appInfo = { };
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.apiVersion = VK_API_VERSION_1_3;
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pApplicationName = "Vulkan2D";

    VkInstanceCreateInfo instanceInfo = { };
    instanceInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instanceInfo.pApplicationInfo = &appInfo;

    uint32_t sdlExtensionCount;
    uint32_t extensionCount;
    const char* const* sdlExtensions;
    sdlExtensions = SDL_Vulkan_GetInstanceExtensions(&sdlExtensionCount);

#ifdef __APPLE__
    instanceInfo.flags = VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
    extensionCount = sdlExtensionCount + 2;
    const char** requiredExtensions = (const char**)malloc(sizeof(const char*) * extensionCount);
    requiredExtensions[sdlExtensionCount] = VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME;
    requiredExtensions[sdlExtensionCount + 1] = VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME;
#else
    extensionCount = sdlExtensionCount;
    const char** requiredExtensions = (const char**)malloc(sizeof(char*) * extensionCount);
#endif

    memcpy(requiredExtensions, sdlExtensions, sizeof(const char*) * sdlExtensionCount);

    instanceInfo.enabledExtensionCount = extensionCount;
    instanceInfo.ppEnabledExtensionNames = requiredExtensions;

    CHECK_RESULT_VK(vkCreateInstance(&instanceInfo, nullptr, &instance));

    free(requiredExtensions);
}

float getSimilarity(std::string deviceName, std::string s2)
{
    std::string smallString;
    std::string bigString;
    if (deviceName.size() < s2.size())
    {
        smallString = deviceName;
        bigString = s2;
    }
    else
    {
        smallString = s2;
        bigString = deviceName;
    }

    int maxHits = 0;

    for (int i = 0; i < bigString.size() - smallString.size() + 1; i++)
    {
        int numHits = 0;
        for (int j = 0; j < smallString.size(); j++)
        {
            if (smallString[j] == bigString[i + j])
            {
                numHits++;
            }
        }

        maxHits = std::max(maxHits, numHits);
    }

    return (float)maxHits / deviceName.size();
}

void ThinDrawer::selectPhysicalDevice()
{
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
    if (deviceCount == 0)
    {
        printf("failed to find GPUs with Vulkan support!");
        exit(1);
    }
    std::vector<VkPhysicalDevice> devices(deviceCount);

    vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

    physicalDevice = VK_NULL_HANDLE;

    bool gpuFound = false;

    deviceProperties.resize(deviceCount);

    std::ofstream oFile;
    oFile.open(IOHelper::assetsFolder + "gameData/availableCards.txt");

    for (uint32_t i = 0; i < deviceCount; i++)
    {
        vkGetPhysicalDeviceProperties(devices[i], &deviceProperties[i].prop);

        if (!isDeviceSuitable(devices[i], swapChain->surface) || deviceProperties[i].prop.deviceType == VK_PHYSICAL_DEVICE_TYPE_CPU)
        {
            deviceProperties[i].ignore = true;

            continue;
        }

        deviceProperties[i].ignore = false;

        std::string deviceName = deviceProperties[i].prop.deviceName;
        
        deviceName += "\n";
        oFile.write(deviceName.c_str(), deviceName.length());
    }

    oFile.close();

    bool forceSelect = selectedPhysicalDeviceName.size() > 1;
    float maxSimilarity = -1.0f;

    for (uint32_t i = 0; i < deviceCount; i++)
    {
        if (deviceProperties[i].ignore)
        {
            continue;
        }

        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(devices[i], &queueFamilyCount, nullptr);
        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(devices[i], &queueFamilyCount, queueFamilies.data());

        for (uint32_t j = 0; j < queueFamilyCount; j++)
        {
            VkBool32 presentSupported;
            vkGetPhysicalDeviceSurfaceSupportKHR(devices[i], j, swapChain->surface, &presentSupported);
            if (queueFamilies[j].queueFlags & VK_QUEUE_GRAPHICS_BIT && presentSupported)
            {
                if (forceSelect)
                {
                    float similarity = getSimilarity(deviceProperties[i].prop.deviceName, selectedPhysicalDeviceName);
                    if (similarity >= maxSimilarity)
                    {
                        physicalDevice = devices[i];
                        selectedPhysicalDeviceIndex = i;
                        queues.graphicsQueueFamilyIndex = j;
                        gpuFound = true;

                        maxSimilarity = similarity;

                        vkGetPhysicalDeviceMemoryProperties(devices[i], &vulkanInfo.deviceMemoryProperties);
                        vulkanInfo.deviceProperties = deviceProperties[i].prop;
                    }
                }
                else if (selectedVendorAndDeviceId != 0)
                {
                    uint32_t physicalDeviceId = selectedVendorAndDeviceId & 0xFFFFFFFF;
                    uint32_t vendorId = (selectedVendorAndDeviceId >> 32) & 0xFFFFFFFF;

                    if (!gpuFound || (physicalDeviceId == deviceProperties[i].prop.deviceID && vendorId == deviceProperties[i].prop.vendorID))
                    {
                        vkGetPhysicalDeviceMemoryProperties(devices[i], &vulkanInfo.deviceMemoryProperties);
                        vulkanInfo.deviceProperties = deviceProperties[i].prop;

                        physicalDevice = devices[i];
                        selectedPhysicalDeviceIndex = i;
                        queues.graphicsQueueFamilyIndex = j;

                        if (gpuFound)
                        {
                            i = deviceCount;
                            break;
                        }

                        gpuFound = true;                        
                    }
                }
                else
                {
                    vkGetPhysicalDeviceMemoryProperties(devices[i], &vulkanInfo.deviceMemoryProperties);
                    vulkanInfo.deviceProperties = deviceProperties[i].prop;

                    physicalDevice = devices[i];
                    selectedPhysicalDeviceIndex = i;
                    queues.graphicsQueueFamilyIndex = j;
                    gpuFound = true;

                    if (!PREFER_DISCRETE)
                    {
                        i = deviceCount;
                        break;
                    }

                    if (vulkanInfo.deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU && PREFER_DISCRETE)
                    {
                        i = deviceCount;
                        break;
                    }
                }
            }
        }
    }

    if (!gpuFound)
    {
        printf("failed to find a suitable GPU!");
        exit(1);
    }
    else
    {
        printf("Selected GPU : %s\n", vulkanInfo.deviceProperties.deviceName);
    }
}

void ThinDrawer::createLogicalDevice()
{
    VkDeviceQueueCreateInfo queueCreateInfo = { };
    queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo.queueFamilyIndex = queues.graphicsQueueFamilyIndex;
    queueCreateInfo.queueCount = 1;
    float queuePriority = 1.0f;
    queueCreateInfo.pQueuePriorities = &queuePriority;

    vkGetPhysicalDeviceFeatures(physicalDevice, &vulkanInfo.features);

    uint32_t extensionPropertyCount;
    vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionPropertyCount, nullptr);
    VkExtensionProperties* extensionProperties = (VkExtensionProperties*)malloc(sizeof(VkExtensionProperties) * extensionPropertyCount);
    vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionPropertyCount, extensionProperties);

    int found = 0;
    for (uint32_t i = 0; i < extensionPropertyCount; i++)
    {
        if (strcmp(extensionProperties[i].extensionName, "VK_KHR_portability_subset") == 0)
        {
            found = 1;
            break;
        }
    }

    int extensionCount;
    if (found)
    {
        extensionCount = NUM_ENABLE_EXTENSIONS + 1;
    }
    else
    {
        extensionCount = NUM_ENABLE_EXTENSIONS;
    }

    const char** enabledExtensions = (const char**)malloc(sizeof(const char*) * extensionCount);
    memcpy(enabledExtensions, ENABLE_EXTENSIONS, NUM_ENABLE_EXTENSIONS * sizeof(const char*));
    if (found)
    {
        enabledExtensions[NUM_ENABLE_EXTENSIONS] = "VK_KHR_portability_subset";
    }

    VkDeviceCreateInfo deviceCreateInfo = { };
    deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceCreateInfo.pQueueCreateInfos = &queueCreateInfo;
    deviceCreateInfo.queueCreateInfoCount = 1;
    deviceCreateInfo.pEnabledFeatures = &vulkanInfo.features;
    deviceCreateInfo.enabledExtensionCount = extensionCount;
    deviceCreateInfo.ppEnabledExtensionNames = enabledExtensions;

    CHECK_RESULT_VK(vkCreateDevice(physicalDevice, &deviceCreateInfo, NULL, &logicalDevice));

    vkGetDeviceQueue(logicalDevice, queues.graphicsQueueFamilyIndex, 0, &queues.graphicsQueue);

    free(enabledExtensions);
}

void ThinDrawer::setSamples()
{

    VkSampleCountFlags counts = vulkanInfo.deviceProperties.limits.framebufferColorSampleCounts &
                                vulkanInfo.deviceProperties.limits.framebufferDepthSampleCounts;

    for (uint32_t flags = desiredSamples; flags >= VK_SAMPLE_COUNT_1_BIT; flags >>= 1)
    {
        if (flags & counts)
        {
            samples = (VkSampleCountFlagBits)flags;
            break;
        }
    }
}

void ThinDrawer::createRenderPass()
{
    std::vector<VkAttachmentDescription> attachments;

    VkAttachmentDescription colorAttachment = { };

    colorAttachment.format = swapChain->surfaceFormat.format;
    colorAttachment.samples = samples;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    if (samples > VK_SAMPLE_COUNT_1_BIT)
    {
        colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    }
    else
    {
        colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    }

    attachments.push_back(colorAttachment);

    VkAttachmentReference colorReference = { };
    colorReference.attachment = 0;
    colorReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    if (samples > VK_SAMPLE_COUNT_1_BIT)
    {
        VkAttachmentDescription colorAttachmentResolve = { };

        colorAttachmentResolve.format = swapChain->surfaceFormat.format;
        colorAttachmentResolve.samples = VK_SAMPLE_COUNT_1_BIT;
        colorAttachmentResolve.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachmentResolve.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachmentResolve.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachmentResolve.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachmentResolve.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachmentResolve.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        attachments.push_back(colorAttachmentResolve);
    }

    VkAttachmentReference colorAttachmentResolveRef = { };
    colorAttachmentResolveRef.attachment = 1;
    colorAttachmentResolveRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpassDescription = { };
    subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpassDescription.colorAttachmentCount = 1;
    subpassDescription.pColorAttachments = &colorReference;

    if (samples > VK_SAMPLE_COUNT_1_BIT)
    {
        subpassDescription.pResolveAttachments = &colorAttachmentResolveRef;
    }

    VkRenderPassCreateInfo renderPassInfo = { };
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = attachments.size();
    renderPassInfo.pAttachments = attachments.data();
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpassDescription;
    renderPassInfo.dependencyCount = 0;

    CHECK_RESULT_VK(vkCreateRenderPass(logicalDevice, &renderPassInfo, nullptr, &renderPass));
}

void ThinDrawer::createCommands()
{
    VkCommandPoolCreateInfo commandCreateInfo = { };
    commandCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    commandCreateInfo.queueFamilyIndex = queues.graphicsQueueFamilyIndex;
    CHECK_RESULT_VK(vkCreateCommandPool(logicalDevice, &commandCreateInfo, nullptr, &uploadPool));
    
    commandCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

    for (int i = 0; i < frames.size(); i++)
    {
        CHECK_RESULT_VK(vkCreateCommandPool(logicalDevice, &commandCreateInfo, nullptr, &frames[i].commandPool));

        VkCommandBufferAllocateInfo cmdAllocInfo = { };
        cmdAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        cmdAllocInfo.commandPool = frames[i].commandPool;
        cmdAllocInfo.commandBufferCount = 1;
        cmdAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

        CHECK_RESULT_VK(vkAllocateCommandBuffers(logicalDevice, &cmdAllocInfo, &frames[i].commandBuffer));
    }
}

void ThinDrawer::createSyncThings()
{
    VkFenceCreateInfo fenceCreateInfo = { };
    fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    VkSemaphoreCreateInfo semaphoreCreateInfo = { };
    semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    semaphoreCreateInfo.flags = VK_SEMAPHORE_TYPE_BINARY;

    for (int i = 0; i < frames.size(); i++)
    {
        CHECK_RESULT_VK(vkCreateFence(logicalDevice, &fenceCreateInfo, VK_NULL_HANDLE, &frames[i].renderFence));

        CHECK_RESULT_VK(vkCreateSemaphore(logicalDevice, &semaphoreCreateInfo, VK_NULL_HANDLE, &frames[i].presentSemaphore));
        CHECK_RESULT_VK(vkCreateSemaphore(logicalDevice, &semaphoreCreateInfo, VK_NULL_HANDLE, &frames[i].renderSemaphore));
    }

    fenceCreateInfo.flags = 0;
    CHECK_RESULT_VK(vkCreateFence(logicalDevice, &fenceCreateInfo, VK_NULL_HANDLE, &uploadFence));
}

void ThinDrawer::initImgui()
{
    VkDescriptorPoolSize pool_sizes[] =
    {
        { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
        { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
        { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
    };

    VkDescriptorPoolCreateInfo pool_info = {};
    pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    pool_info.maxSets = 1000;
    pool_info.poolSizeCount = std::size(pool_sizes);
    pool_info.pPoolSizes = pool_sizes;

    CHECK_RESULT_VK(vkCreateDescriptorPool(logicalDevice, &pool_info, nullptr, &imguiPool));

    ImGui::CreateContext();
    ImGui::StyleColorsDark();

    ImGui_ImplSDL3_InitForVulkan(wh->window);
    ImGui_ImplVulkan_InitInfo initInfo = {};
    initInfo.Instance = instance;
    initInfo.PhysicalDevice = physicalDevice;
    initInfo.Device = logicalDevice;
    initInfo.Queue = queues.graphicsQueue;
    initInfo.QueueFamily = queues.graphicsQueueFamilyIndex;
    initInfo.DescriptorPool = imguiPool;
    initInfo.MinImageCount = swapChain->imageCount;
    initInfo.ImageCount = swapChain->imageCount;
    initInfo.MSAASamples = samples;
	initInfo.RenderPass = renderPass;
    ImGui_ImplVulkan_Init(&initInfo);

    ImGui_ImplVulkan_CreateFontsTexture();

    ImGuiStyle* style = &ImGui::GetStyle();
    style->Colors[ImGuiCol_WindowBg] = ImVec4(0, 0, 0, 0.95);
}