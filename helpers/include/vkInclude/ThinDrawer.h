#pragma once

#include <ShaderHeaders/VulkanTriangle.h>
#include <ShaderHeaders/DebugCircle.h>
#include <ShaderHeaders/DebugLine.h>
#include <ShaderHeaders/DebugPolygon.h>
#include <WindowHandler.h>
#include <definitions.h>
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <vector>
#include <set>
#include <box2d/box2d.h>

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

class SwapChain;
class PortalBody;
class Portal;
class PortalWorld;

// per fixture
typedef struct
{
    std::vector<uint32_t> dynamicOffsets;
    s_buffers* vertexBuffer;
    struct fixtureData* fixData;
    b2Shape::Type renderType;
} s_renderStruct;

// per fixture
typedef struct
{
    std::vector<s_renderStruct*> renderFixtures;
    PortalBody* portalBody;
} s_renderBody;

// per worldIndex
typedef struct
{
    // ordered consistently with the fixture list
    std::vector<s_buffers*> vertexBuffers;
    std::vector<s_buffers*> vertexBuffersMirrored;
    // available only for polygon fixtures
    // data in between polygons are irrelevant
    std::vector<int> vertexStorageOffsets;
    std::vector<int> vertexStorageOffsetsMirrored;

    std::vector<b2Vec2> circlePositionings;
    std::vector<b2Vec2> circlePositioningsMirrored;

    std::set<s_renderBody*> renderBodies;
} s_renderGroup;

class ThinDrawer
{
public:

    PortalWorld* pWorld;

    ThinDrawer(PortalWorld* pWorld);

    // Base var start

    uint32_t frameNumber = 0;
    uint32_t lastSwapChainImageIndex = 0;
    VkSampleCountFlagBits samples;
    VkSampleCountFlagBits desiredSamples = VK_SAMPLE_COUNT_1_BIT;

    s_vulkanInfo* vulkanInfo;
    VkInstance instance;
    VkPhysicalDevice physicalDevice;
    VkDevice logicalDevice;
    s_queueData queues;
    SwapChain* swapChain;
    VkRenderPass renderPass;
    VkCommandPool uploadPool;
    VkFence uploadFence;

    WindowHandler* wh;

    std::vector<s_frameData> frames;

    VkDescriptorPool descriptorPool;

    // Diff var start

    std::vector<s_renderGroup*> renderGroups;

    std::vector<s_uniformBuffer*> sharedOrthoUniform;
    ShaderBase* debugCircleShader;
    ShaderBase* debugLineShader;
    ShaderBase* debugPolygonShader;

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
    void buildCommandBuffers(int frameNumber);

    void uniformFiller(int frameNum);
    void renderLoop();

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
    void loadTexture(char* fileName, s_texture* texture);
    void updateImageDescriptors(s_texture* tex, VkDescriptorSetLayout& setLayout);
    void createImage(uint32_t width, uint32_t p_height, uint32_t mipLevels,
                     VkSampleCountFlagBits numSamples, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage,
                     VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory);

    void addPortalBody(PortalBody* portalBody);

    static bool isDeviceSuitable(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface);

    static void portalBodyCreateCallback(PortalBody* pb, void* data);
    static void portalBodyDestroyCallback(PortalBody* pb, void* data);
};