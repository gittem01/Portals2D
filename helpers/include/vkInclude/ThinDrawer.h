#pragma once

#include <ShaderHeaders/VulkanTriangle.h>
#include <ShaderHeaders/DebugCircle.h>
#include <ShaderHeaders/DebugLine.h>
#include <ShaderHeaders/DebugPolygon.h>
#include <ShaderHeaders/WhiteWall.h>
#include <ShaderHeaders/Body.h>
#include <ShaderHeaders/PortalShader.h>
#include <WindowHandler.h>
#include <definitions.h>
#include <RenderExtra.h>
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>
#include <vulkan/vulkan.h>

#include <IOHelper.h>

#define PRINT_INFO_MESSAGES 1

static const char* ENABLE_EXTENSIONS[] =
{
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
};

static const int NUM_ENABLE_EXTENSIONS = sizeof(ENABLE_EXTENSIONS) / sizeof(ENABLE_EXTENSIONS[0]);

typedef struct
{
    VkCommandPool commandPool;
    VkCommandBuffer commandBuffer;
    VkFence renderFence;
    VkSemaphore presentSemaphore;
    VkSemaphore renderSemaphore;
} s_frameData;

typedef struct
{
    VkQueue graphicsQueue;
    uint32_t graphicsQueueFamilyIndex;
} s_queueData;

typedef struct
{
    VkPhysicalDeviceProperties deviceProperties;
    VkPhysicalDeviceFeatures features;
    VkPhysicalDeviceMemoryProperties deviceMemoryProperties;
} s_vulkanInfo;

typedef struct
{
    VkDeviceMemory memory;
    VkBuffer buffer;
} s_stagingBuffer;

typedef struct
{
    s_stagingBuffer vertices;
    s_stagingBuffer indices;
} s_stagingBuffers;

typedef struct
{
    glm::mat4 orthoMatrix;
    glm::vec4 cameraPos;
    glm::vec4 data;
} s_sharedUniformData;

typedef struct
{
    glm::mat4 modelMatrix;
    glm::vec4 portals[4];
    glm::vec4 color;
    glm::vec4 outerColor;
    glm::vec4 data;
    glm::vec4 extraData;
} s_uboAll;

typedef struct
{
    glm::mat4 transformMatrix;
    glm::mat4 rotationMatrix;
    glm::vec4 bgColour;
    glm::vec4 dotColour;
    glm::vec4 edgeColour;
    glm::vec4 leftUp;
    glm::vec4 data; // dotSize, sideMargin, sideMult, smoothRatio
    glm::vec4 data2; // sizeX, sizeY, unused, unused
} s_wallUbo;

typedef struct s_portalUbo
{
    glm::mat4 modelMatrix;
    glm::vec4 data0; // a, b, abRatio, eThick
    glm::vec4 data1; // lineLength, maxAngleInc, minAngleInc, noiseScale
    glm::vec4 data2; // sizex, sizey, iTime, 0
    glm::vec4 data3; // funcMult0, funcMult1, noiseMult, 0
    glm::vec4 portalColour;
    glm::vec4 edgeColour;
}s_portalUbo;

typedef struct
{
    glm::mat4 modelMatrix;
    glm::mat4 rotationMatrix;
}s_bodyUbo;

typedef struct
{
    glm::vec4 portals[4];
    glm::vec4 data; // numPortals, unused, unused, unused
} s_fixUbo;

typedef struct
{
    glm::vec4 polyPoints[4];
    int numPolyPoints;
} s_uboPolyData;

typedef struct
{
    glm::vec4 positions;
    glm::vec4 colour;
    glm::vec4 data; // unused, lineThickness, outerSmoothThicknessRatio, unused
} s_pushConstantLine;

typedef struct
{
    glm::vec2 pos;
    glm::vec2 uv;
} s_vertex;

typedef struct
{
    glm::vec2 pos;
} s_basicVertex;

typedef struct AverageVertex
{
    glm::vec3 pos;
    glm::vec3 normal;
    glm::vec2 tc;

    static void getVertexAttributeDescriptions(VkVertexInputAttributeDescription* descriptions)
    {
        descriptions[0].binding = 0;
        descriptions[0].location = 0;
        descriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        descriptions[0].offset = offsetof(AverageVertex, pos);
        
        descriptions[1].binding = 0;
        descriptions[1].location = 1;
        descriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        descriptions[1].offset = offsetof(AverageVertex, normal);

        descriptions[2].binding = 0;
        descriptions[2].location = 2;
        descriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
        descriptions[2].offset = offsetof(AverageVertex, tc);
    }
} s_averageVertex;

class SwapChain;
class PortalBody;
class Portal;
class PortalWorld;

class ThinDrawer
{
public:

    PortalWorld* pWorld;
    VkDescriptorPool imguiPool;

    double pDeltaTime = 0.0;

    ThinDrawer(PortalWorld* pWorld);

    bool drawReleases;

    void initImgui();
    ImDrawData* drawData = nullptr;
    // Base var start

    uint32_t frameNumber = 0;
    uint32_t lastSwapChainImageIndex = 0;
    VkSampleCountFlagBits samples;
    VkSampleCountFlagBits desiredSamples = VK_SAMPLE_COUNT_4_BIT;

    s_vulkanInfo* vulkanInfo;
    VkInstance instance;
    VkPhysicalDevice physicalDevice;
    VkDevice logicalDevice;
    s_queueData queues;
    SwapChain* swapChain;
    VkRenderPass renderPass;
    VkCommandPool uploadPool;
    VkFence uploadFence;

    SwapChain* thickSwapChain;
    VkRenderPass* thickRenderPass;

    WindowHandler* wh;

    std::vector<s_frameData> frames;

    VkDescriptorPool descriptorPool;

    // Diff var start

    std::vector<s_debug_renderGroup*> debug_renderGroups;
    std::vector<s_renderGroup*> renderGroups;

    std::map<Portal*, s_portalRender> portalsToRender;

    AnimGroup* animations = nullptr;

    std::vector<s_uniformBuffer*> sharedOrthoUniform;
    ShaderBase* debugCircleShader;
    ShaderBase* debugLineShader;
    ShaderBase* debugPolygonShader;
    WhiteWall* whiteWallShader;
    Body* bodyShader;
    PortalShader* portalShader;

    // var end

    void surfaceRecreate();

    void createWindow();
    void initBase();
    void initExtra();

    void createInstance();
    void createCommands();
    void createSyncThings();
    void createRenderPass();
    void createLogicalDevice();
    void selectPhysicalDevice();
    void prepareVertices();
    void prepareUniformBuffers();
    void setupDescriptorSetLayout();
    void preparePipelines();
    void setupDescriptorPool();
    void setupDescriptorSet();

    void renderCommandBeginner(VkCommandBuffer& cmdBuff);
    void debug_renderPortalBodies(VkCommandBuffer& cmdBuff);
    void debug_renderPortals(VkCommandBuffer& cmdBuff);
    void debug_renderEdges(VkCommandBuffer& cmdBuff);
    void renderPortalBodies(VkCommandBuffer& cmdBuff);
    void renderWalls(VkCommandBuffer& cmdBuff);
    void renderPortals(VkCommandBuffer& cmdBuff);
    void buildCommandBuffers();

    void beforeCreate();
    void afterCreate();
    
    void uniformFiller(int frameNum);
    void renderLoop();
    void animate();

    void bufferStage(void* bufferData, uint32_t dataSize, VkBufferUsageFlags flags, s_buffers* fillBuffer);
    VkGraphicsPipelineCreateInfo* getPipelineInfoBase();
    static void freePipelineData(VkGraphicsPipelineCreateInfo* pipelineCreateInfo);
    void uniformHelper( int size, s_uniformBuffer* uniformBuffer,
                        VkBufferUsageFlagBits usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, int numInstances = 1);

    int pad_uniform_buffer_size(int originalSize);
    int pad_storage_buffer_size(int originalSize);

    VkCommandBuffer getCommandBuffer(bool begin);
    void flushCommandBuffer(VkCommandBuffer commandBuffer);
    uint32_t getMemoryTypeIndex(uint32_t typeBits, VkMemoryPropertyFlags properties);
    void setSamples();

    VkCommandBuffer createCommandBuffer(VkCommandBufferLevel level, VkCommandPool pool, bool begin);
    void generateMipmaps(VkImage image, VkFormat imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels);
    void loadTexture(char* fileName, s_texture* texture);
    void updateImageDescriptors(s_texture* tex, VkDescriptorSetLayout& setLayout);
    void createImage(uint32_t width, uint32_t p_height, uint32_t mipLevels,
                     VkSampleCountFlagBits numSamples, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage,
                     VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory);

    // mesh names are hold per fixture
    void addPortalBody(PortalBody* portalBody, std::vector<std::pair<std::string, std::string>> meshNames);
    void debug_addPortalBody(PortalBody* portalBody);
    void releaseRenderResources();

    static bool isDeviceSuitable(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface);

    static void portalCreateCallback(Portal* portal);
    static void portalDestroyCallback(Portal* portal);

    static void portalBodyCreateCallback(PortalBody* pb, void* data);
    static void portalBodyDestroyCallback(PortalBody* pb, void* data);

    static void debug_portalBodyCreateCallback(PortalBody* pb, void* data);
    static void debug_portalBodyDestroyCallback(PortalBody* pb, void* data);
};