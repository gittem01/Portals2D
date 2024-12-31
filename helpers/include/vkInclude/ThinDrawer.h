#pragma once

#include <ShaderHeaders/BodyShader.h>
#include <ShaderHeaders/DebugCircle.h>
#include <ShaderHeaders/DebugLine.h>
#include <ShaderHeaders/DebugPolygon.h>
#include <ShaderHeaders/WhiteWall.h>
#include <ShaderHeaders/BlackWall.h>
#include <ShaderHeaders/Body.h>
#include <ShaderHeaders/PortalShader.h>
#include <ShaderHeaders/PortalParticleShader.h>
#include <ShaderHeaders/DualParticleShader.h>
#include <ShaderHeaders/PortalShader3D.h>
#include <ShaderHeaders/WallLight.h>
#include <ShaderHeaders/GunEffect.h>
#include <ShaderHeaders/LaserShader.h>
#include <ShaderHeaders/LaserPshShader.h>
#include <ShaderHeaders/LineZ.h>
#include <ShaderHeaders/LightBridge.h>
#include <ShaderHeaders/FunnelShader.h>
#include <ShaderHeaders/HazardShader.h>
#include <ShaderHeaders/BlueField.h>
#include <ShaderHeaders/PlayerShader.h>
#include <ShaderHeaders/EndlessGrid.h>
#include <ShaderHeaders/GridSquare.h>
#include <ShaderHeaders/ArrowShader.h>
#include <ShaderHeaders/ItemList.h>
#include <ShaderHeaders/ManyTexture.h>
#include <ShaderHeaders/DiagonalLiner.h>
#include <ShaderHeaders/PlatformRectangle.h>
#include <ShaderHeaders/PolynomialShader.h>
#include <ShaderHeaders/Gel.h>
#include <ShaderHeaders/WallGel.h>
#include <ShaderHeaders/SplashShader.h>
#include <ShaderHeaders/PushLine.h>
#include <ShaderHeaders/MenuButton.h>
#include "CharDrawer.h"

#include <WindowHandler.h>
#include <definitions.h>
#include <RenderExtra.h>
#include <SDL3/SDL.h>
#include <imgui.h>
#include <backends/imgui_impl_sdl3.h>
#include <backends/imgui_impl_vulkan.h>
#include <vulkan/vulkan.h>
#include <mutex>

#include <IOHelper.h>

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

typedef struct TextureData
{
    unsigned char* data;
    int width;
    int height;
    int numChannels;
} s_textureData;

class SwapChain;
class PortalBody;
class Portal;
class PortalWorld;

class ThinDrawer
{
public:
    static ThinDrawer* object;

    PortalWorld* pWorld;
    VkDescriptorPool imguiPool;

    ThinDrawer(PortalWorld* pWorld);

    std::mutex queueMutex;

    bool drawReleases = false;
    bool drawDebugBodies = false;
    bool drawDebugEdges = false;
    bool drawDebugPortals = false;

    glm::vec4 adjustableValues = { 0.0f, 0.0f, 0.0f, 0.0f };

    void initImgui();
    ImDrawData* drawData = nullptr;
    // Base var start

    uint32_t camOffset;
    uint32_t frameNumber = 0;
    uint32_t frameIndex = 0;
    uint32_t swapchainImageIndex = 0;
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

    std::vector<std::pair<void (*) (void*), void*>> memMapFuncs;
    
    std::vector<std::pair<void (*) (VkCommandBuffer, void*), void*>> earlyDrawFuncs;
    std::vector<std::pair<void (*) (VkCommandBuffer, void*), void*>> midDrawFuncs;
    std::vector<std::pair<void (*) (VkCommandBuffer, void*), void*>> drawFuncs;
    std::vector<std::pair<void (*) (VkCommandBuffer, void*), void*>> lateDrawFuncs;

    // Diff var start

    std::vector<s_debug_renderGroup*> debug_renderGroups;
    std::vector<s_renderGroup*> renderGroups;
    std::map<Portal*, s_portalRender> renderPortalData;
    std::vector<s_renderGroup3D*> renderGroups3D;
    std::map<Portal*, s_portalRender3D> portalsToRender3D;

    std::vector<s_debugLineUbo> debugLineRegistry;
    std::vector<s_debugLineUbo> oneTimeRenderLines;

    s_lineRenderData popLrd;
    double popTextTime = 0.0;

    AnimGroup* animations = nullptr;
    double gameTime = 0.0;
    double deltaTime = 0.0;
    double frameDT = 0.0;
    double upUpAnim = 0.0;
    double upDownAnim = 0.0;

    DebugCircle* debugCircleShader;
    DebugLine* debugLineShader;
    DebugPolygon* debugPolygonShader;
    WhiteWall* whiteWallShader;
    BlackWall* blackWallShader;;
    PortalShader* portalShader;
    PortalParticleShader* portalParticleShader;
    DualParticleShader* dualParticleShader;
    Body* bodyShader3D;
    PortalShader3D* portalShader3D;
    BodyShader* bodyShader;
    WallLight* wallLight;
    LaserShader* laserShader;
    LaserPshShader* laserPshShader;
    LineZ* lineZ;
    LightBridgeShader* lightBridge;
    FunnelShader* funnelShader;
    HazardShader* hazardShader;
    BlueField* blueField;
    GunEffect* gunEffect;
    PlayerShader* playerShader;
    EndlessGrid* endlessGrid;
    GridSquare* gridSquare;
    ArrowShader* arrowShader;
    ItemList* itemList;
    ManyTexture* manyTexture;
    DiagonalLiner* diagonalLiner;
    PlatformRectangle* platformRectangle;
    PolynomialShader* polynomShader;;
    GelShader* gelShader;
    WallGel* wallGel;
    SplashShader* splash;
	PushLine* pushLine;
    MenuButton* menuButton;

    std::vector<ShaderBase*> shaders;

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
    void preparePipelines();
    void setupDescriptorPool();

    void renderCommandBeginner(VkCommandBuffer& cmdBuff);
    void debug_renderPortalBodies(VkCommandBuffer& cmdBuff);
    void debug_renderPortals(VkCommandBuffer& cmdBuff);
    void debug_renderEdges(VkCommandBuffer& cmdBuff);
    void renderPortalBodies(VkCommandBuffer& cmdBuff);
    void renderPortals(VkCommandBuffer& cmdBuff);
    void renderPortalBodies3D(VkCommandBuffer& cmdBuff);
    void renderWalls(VkCommandBuffer& cmdBuff);
    void renderPortals3D(VkCommandBuffer& cmdBuff);
    void buildCommandBuffers(VkCommandBuffer cmdBuff);

    void beforeCreate();
    void afterCreate();
    
    void addPopText(std::string text, glm::vec2 pos, glm::vec3 colour);

    void uniformFiller();
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

    void generateMipmaps(VkImage image, VkFormat imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels);
    void loadTexture(char* fileName, s_texture* texture, bool enableAnisotropy = true,
        VkSamplerAddressMode addressMode = VK_SAMPLER_ADDRESS_MODE_REPEAT, float lodBias=0.0);
    void loadTextureFromMemory(s_textureData* texData, s_texture* texture, bool enableAnisotropy = true,
        VkSamplerAddressMode addressMode = VK_SAMPLER_ADDRESS_MODE_REPEAT, int numMipLevels=0, VkFilter filter=VK_FILTER_MAX_ENUM);
    void updateImageDescriptors(s_texture* tex, VkDescriptorSetLayout& setLayout);
    void createImage(uint32_t width, uint32_t p_height, uint32_t mipLevels,
                     VkSampleCountFlagBits numSamples, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage,
                     VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory, uint32_t depth=1, uint32_t arrayLayers=1);

    void registerDebugLine(const glm::vec4 positions, const glm::vec4 color, const float thickness, const float smoothness);
    void oneTimeRenderLine(const glm::vec4 positions, const glm::vec4 color, const float thickness, const float smoothness);

    // mesh names are hold per fixture
    void addPortalBody_3d(PortalBody* portalBody, std::vector<std::pair<std::string, std::string>> meshNames);
    void debug_addPortalBody(PortalBody* portalBody);
    s_renderGroup* addPortalBody(PortalBody* portalBody, std::vector<std::string> textures);
    void releaseRenderResources();
    void eraseFromMaperFunctions(std::vector<std::pair<void (*) (void*), void*>>& vec, void* ptr);
    void eraseFromRenderFunctions(std::vector<std::pair<void (*) (VkCommandBuffer, void*), void*>>& vec, void* ptr);

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