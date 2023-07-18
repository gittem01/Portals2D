#pragma once

#include <vulkan/vulkan.h>
#include <string>
#include <vector>
#include <glm/glm.hpp>

typedef struct s_buffers
{
    VkDeviceMemory memory;
    VkBuffer buffer;
    uint32_t count;
} s_buffers;

typedef struct
{
    VkDeviceMemory memory;
    VkBuffer buffer;
    VkDescriptorBufferInfo descriptor;
} s_uniformBuffer;

typedef struct s_texture
{
    VkSampler sampler;
    VkImage image;
    VkImageLayout imageLayout;
    VkDeviceMemory deviceMemory;
    VkDescriptorSet set;
    VkImageView view;
    uint32_t width, height;
    uint32_t mipLevels;
} s_texture;

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

typedef struct
{
    glm::vec2 pos;
    glm::vec2 uv;
} s_vertex;

typedef struct
{
    glm::vec2 pos;
} s_basicVertex;


class ThinDrawer;

class ShaderBase
{
public:
    std::vector<VkDescriptorSetLayout> descriptorSetLayouts;
    std::vector<VkDescriptorSet> descriptorSets;
    std::vector<s_texture*> textureData;
    VkPipelineLayout pipelineLayout;
    VkPipeline pipeline;

    std::vector<s_buffers*> buffers;
    std::vector<s_uniformBuffer*> uniformBuffers;
    
    static VkDescriptorSetLayout sharedLayout;
    static VkDescriptorSet sharedSet;
    static int alignedSharedSize;
    static s_uniformBuffer* sharedOrthoUniform;

    static std::vector<int> initialAllocationSizes;
    static std::vector<int> avlAllocLastLoc;
    static std::vector<int*> freeIndicesList;
    static std::vector<int64_t> alignedSizes;

    static std::vector<s_uniformBuffer*> commonStorageBuffers;
    static void createCommonStorageBuffers(ThinDrawer* thinDrawer);
    static void init(ThinDrawer* thinDrawer);
    static void setSharedLayout(ThinDrawer* thinDrawer);
    static void writeSharedDescriptors(ThinDrawer* thinDrawer);

    ThinDrawer* thinDrawer;
    VkDevice logicalDevice;

    ShaderBase(ThinDrawer* thinDrawer);

    virtual void prepareVertexData() = 0;
    virtual void prepareUniforms() = 0;
    virtual void setupDescriptorSetLayout() = 0;
    virtual void preparePipeline() = 0;
    virtual void setupDescriptorSet() = 0;

    void init();
};