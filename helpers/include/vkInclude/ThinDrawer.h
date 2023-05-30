#pragma once

#include <ShaderHeaders/BodyShader.h>
#include <ShaderHeaders/DebugCircle.h>
#include <ShaderHeaders/DebugLine.h>
#include <ShaderHeaders/DebugPolygon.h>
#include <ShaderHeaders/WhiteWall.h>
#include <ShaderHeaders/Body.h>
#include <ShaderHeaders/PortalShader.h>
#include <ShaderHeaders/PortalParticleShader.h>
#include <ShaderHeaders/DualParticleShader.h>
#include <ShaderHeaders/PortalShader3D.h>
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

typedef struct s_portalUbo3D
{
    glm::mat4 modelMatrix;
    glm::vec4 data0; // a, b, abRatio, eThick
    glm::vec4 data1; // lineLength, maxAngleInc, minAngleInc, noiseScale
    glm::vec4 data2; // sizex, sizey, iTime, 0
    glm::vec4 data3; // funcMult0, funcMult1, noiseMult, 0
    glm::vec4 portalColour;
    glm::vec4 edgeColour;
}s_portalUbo3D;

typedef struct s_portalUbo
{
    glm::vec4 posSize; // pos.x, pos.y, size.x, size.y
    glm::vec4 color;
    glm::vec4 data0; // dirAngle, pixeler, midThick, xDropSize
}s_portalUbo;

typedef struct s_portalParticleUbo
{
    glm::vec4 portalPoints;
    glm::vec4 color;
    glm::vec4 data0; // dist, speed, time, upSize
    glm::vec4 data1; // sizex, sizey, randomization, instanceCount
}s_portalParticleUbo;

typedef struct s_dualParticleUbo
{
    glm::vec4 color0;
    glm::vec4 color1;
    glm::vec4 data0; // none, speed, time, midCircleRadius
    glm::vec4 data1; // sizex, sizey, randomization, none
}s_dualParticleUbo;

typedef struct
{
    glm::mat4 modelMatrix;
    int isMirrored;
    int isMirrored2;
}s_renderBodyUbo;

typedef struct
{
    glm::vec4 portals[4];
    int numPortals;
}s_renderFixtureUbo;

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

    bool drawReleases = false;
    bool drawDebugBodies = false;
    bool drawDebugEdges = false;
    bool drawDebugPortals = false;

    void initImgui();
    ImDrawData* drawData = nullptr;
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

    SwapChain* thickSwapChain;
    VkRenderPass* thickRenderPass;

    WindowHandler* wh;

    std::vector<s_frameData> frames;

    VkDescriptorPool descriptorPool;

    // Diff var start

    std::vector<s_debug_renderGroup*> debug_renderGroups;
    std::vector<s_renderGroup*> renderGroups;
    std::map<Portal*, s_portalRender> renderPortalData;
    std::vector<s_renderGroup3D*> renderGroups3D;

    std::map<Portal*, s_portalRender3D> portalsToRender3D;

    AnimGroup* animations = nullptr;
    double gameTime = 0.0;
    double upUpAnim = 0.0;
    double upDownAnim = 0.0;

    std::vector<s_uniformBuffer*> sharedOrthoUniform;
    ShaderBase* debugCircleShader;
    ShaderBase* debugLineShader;
    ShaderBase* debugPolygonShader;
    WhiteWall* whiteWallShader;
    PortalShader* portalShader;
    PortalParticleShader* portalParticleShader;
    DualParticleShader* dualParticleShader;
    Body* bodyShader3D;
    PortalShader3D* portalShader3D;
    BodyShader* bodyShader;

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
    void renderPortals(VkCommandBuffer& cmdBuff);
    void renderBackgroundParticles(VkCommandBuffer& cmdBuff);
    void renderPortalBodies3D(VkCommandBuffer& cmdBuff);
    void renderWalls(VkCommandBuffer& cmdBuff);
    void renderPortals3D(VkCommandBuffer& cmdBuff);
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
    void loadTexture(char* fileName, s_texture* texture, bool enableAnisotropy=true,
        VkSamplerAddressMode addressMode=VK_SAMPLER_ADDRESS_MODE_REPEAT, VkFilter filter= VK_FILTER_LINEAR);
    void updateImageDescriptors(s_texture* tex, VkDescriptorSetLayout& setLayout);
    void createImage(uint32_t width, uint32_t p_height, uint32_t mipLevels,
                     VkSampleCountFlagBits numSamples, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage,
                     VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory);

    // mesh names are hold per fixture
    void addPortalBody_3d(PortalBody* portalBody, std::vector<std::pair<std::string, std::string>> meshNames);
    void debug_addPortalBody(PortalBody* portalBody);
    void addPortalBody(PortalBody* portalBody, std::vector<std::string> textures);
    void releaseRenderResources();

    static bool isDeviceSuitable(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface);

    static void portalBodyCreateCallback(PortalBody* pb, void* data);
    static void portalBodyDestroyCallback(PortalBody* pb, void* data);

    static void portalCreateCallback(Portal* portal);
    static void portalDestroyCallback(Portal* portal);

    static void portalCreateCallback_3d(Portal* portal);
    static void portalDestroyCallback_3d(Portal* portal);

    static void portalBodyCreateCallback_3d(PortalBody* pb, void* data);
    static void portalBodyDestroyCallback_3d(PortalBody* pb, void* data);

    static void debug_portalBodyCreateCallback(PortalBody* pb, void* data);
    static void debug_portalBodyDestroyCallback(PortalBody* pb, void* data);
};