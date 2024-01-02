#include <ThinDrawer.h>
#include <SwapChain.h>
#include <IOHelper.h>
#include <Camera.h>

#define PREFER_DISCRETE 0

void ThinDrawer::createWindow()
{
    wh = new WindowHandler(NULL);
    Camera* cam = new Camera(glm::vec2(0.0f, 0.0f), wh);
    wh->cam = cam;
}

void ThinDrawer::initBase()
{
    IOHelper::thinDrawer = this;

    vulkanInfo = new s_vulkanInfo;

    swapChain = new SwapChain(this);

    createWindow();

    createInstance();

    CHECK_RESULT_VK(glfwCreateWindowSurface(instance, wh->window, NULL, &swapChain->surface));

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

    uint32_t glfwExtensionCount;
    uint32_t extensionCount;
    const char** glfwExtensions;
    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

#ifdef __APPLE__
    instanceInfo.flags = VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
    extensionCount = glfwExtensionCount + 2;
    const char** requiredExtensions = (const char**)malloc(sizeof(const char*) * extensionCount);
    requiredExtensions[glfwExtensionCount] = VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME;
    requiredExtensions[glfwExtensionCount + 1] = VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME;
#else
    extensionCount = glfwExtensionCount;
    const char** requiredExtensions = (const char**)malloc(sizeof(char*) * extensionCount);
#endif

    memcpy(requiredExtensions, glfwExtensions, sizeof(const char*) * glfwExtensionCount);

    instanceInfo.enabledExtensionCount = extensionCount;
    instanceInfo.ppEnabledExtensionNames = requiredExtensions;

    CHECK_RESULT_VK(vkCreateInstance(&instanceInfo, nullptr, &instance));

    free(requiredExtensions);
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
    VkPhysicalDevice* devices = (VkPhysicalDevice*)malloc(sizeof(VkPhysicalDevice) * deviceCount);

    vkEnumeratePhysicalDevices(instance, &deviceCount, devices);

    physicalDevice = VK_NULL_HANDLE;

    bool gpuFound = false;

    for (uint32_t i = 0; i < deviceCount; i++)
    {
        VkPhysicalDeviceProperties deviceProperties;
        vkGetPhysicalDeviceProperties(devices[i], &deviceProperties);
        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(devices[i], &queueFamilyCount, nullptr);
        VkQueueFamilyProperties* queueFamilies = (VkQueueFamilyProperties*)malloc(sizeof(VkQueueFamilyProperties) * queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(devices[i], &queueFamilyCount, queueFamilies);

        if (isDeviceSuitable(devices[i], swapChain->surface))
        {
            for (uint32_t j = 0; j < queueFamilyCount; j++)
            {
                VkBool32 presentSupported;
                vkGetPhysicalDeviceSurfaceSupportKHR(devices[i], j, swapChain->surface, &presentSupported);
                if (queueFamilies[j].queueFlags & VK_QUEUE_GRAPHICS_BIT && presentSupported)
                {
                    vkGetPhysicalDeviceProperties(devices[i], &vulkanInfo->deviceProperties);
                    if (vulkanInfo->deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_CPU)
                    {
                        continue;
                    }
                    vkGetPhysicalDeviceMemoryProperties(devices[i], &vulkanInfo->deviceMemoryProperties);
#if PRINT_INFO_MESSAGES
                    printf("Selected GPU : %s\n", vulkanInfo->deviceProperties.deviceName);
#endif
                    physicalDevice = devices[i];
                    queues.graphicsQueueFamilyIndex = j;
                    gpuFound = true;

                    if (!PREFER_DISCRETE)
                    {
                        i = deviceCount;
                        break;
                    }

                    if (vulkanInfo->deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU && PREFER_DISCRETE)
                    {
                        i = deviceCount;
                        break;
                    }
                }
            }
        }
        free(queueFamilies);
    }
    free(devices);

    if (!gpuFound)
    {
        printf("failed to find a suitable GPU!");
        exit(1);
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

    vkGetPhysicalDeviceFeatures(physicalDevice, &vulkanInfo->features);

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
    deviceCreateInfo.pEnabledFeatures = &vulkanInfo->features;
    deviceCreateInfo.enabledExtensionCount = extensionCount;
    deviceCreateInfo.ppEnabledExtensionNames = enabledExtensions;

    CHECK_RESULT_VK(vkCreateDevice(physicalDevice, &deviceCreateInfo, NULL, &logicalDevice));

    vkGetDeviceQueue(logicalDevice, queues.graphicsQueueFamilyIndex, 0, &queues.graphicsQueue);

    free(enabledExtensions);
}

void ThinDrawer::setSamples()
{

    VkSampleCountFlags counts = vulkanInfo->deviceProperties.limits.framebufferColorSampleCounts &
                                vulkanInfo->deviceProperties.limits.framebufferDepthSampleCounts;

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
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
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

    ImGui_ImplGlfw_InitForVulkan(wh->window, true);
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
    ImGui_ImplVulkan_Init(&initInfo, renderPass);

    VkCommandBuffer commandBuffer = getCommandBuffer(true);
    ImGui_ImplVulkan_CreateFontsTexture(commandBuffer);
    flushCommandBuffer(commandBuffer);

    ImGui_ImplVulkan_DestroyFontUploadObjects();

    ImGuiStyle* style = &ImGui::GetStyle();
    style->Colors[ImGuiCol_WindowBg] = ImVec4(0, 0, 0, 0.95);
}