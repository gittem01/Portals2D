#pragma once

#include <vulkan/vulkan.h>
#include <string>
#include <vector>

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

class ThinDrawer;

class ShaderBase
{
    friend class ThinDrawer;

protected:
    std::vector<VkDescriptorSetLayout> descriptorSetLayouts;
    std::vector<s_texture*> textureData;
    VkPipelineLayout pipelineLayout;
    VkPipeline pipeline;
    
    std::vector<std::vector<VkDescriptorSet>> descriptorSets;

    std::vector<s_buffers*> buffers;
    std::vector<std::vector<s_uniformBuffer*>> uniformBuffers;
    
    static std::vector<int> initialAllocationSizes;
    static std::vector<int> avlAllocLastLoc;
    static std::vector<int*> freeIndicesList;
    static std::vector<int64_t> alignedSizes;

    static std::vector<std::vector<s_uniformBuffer*>> commonStorageBuffers;
    static void createCommonStorageBuffers(ThinDrawer* thinDrawer);

    ThinDrawer* thinDrawer;
    VkDevice logicalDevice;

    ShaderBase(ThinDrawer* thinDrawer);

    virtual void prepareVertexData() = 0;
    virtual void prepareUniforms() = 0;
    virtual void setupDescriptorSetLayout() = 0;
    virtual void preparePipeline() = 0;
    virtual void setupDescriptorSet();
};