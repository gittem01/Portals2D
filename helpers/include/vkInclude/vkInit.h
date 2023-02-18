#include <vulkan/vulkan.h>

namespace vkinit
{
    inline VkBufferCreateInfo bufferCreateInfo(
            VkDeviceSize size, VkBufferUsageFlags usage, VkSharingMode sharingMode=VK_SHARING_MODE_EXCLUSIVE)
    {
        VkBufferCreateInfo bufferCreateInfo = { };
        bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferCreateInfo.size = size;
        bufferCreateInfo.usage = usage;
        bufferCreateInfo.sharingMode = sharingMode;

        return bufferCreateInfo;
    }

    inline VkMemoryAllocateInfo memoryAllocateInfo()
    {
        VkMemoryAllocateInfo memoryAllocateInfo = { };
        memoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;

        return memoryAllocateInfo;
    }

    inline VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo(
            VkDescriptorSetLayoutBinding* bindings, uint32_t bindingCount, VkDescriptorSetLayoutCreateFlags flags= 0)
    {
        VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo = { };
        descriptorSetLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        descriptorSetLayoutCreateInfo.bindingCount = bindingCount;
        descriptorSetLayoutCreateInfo.flags = flags;
        descriptorSetLayoutCreateInfo.pBindings = bindings;

        return descriptorSetLayoutCreateInfo;
    }

    inline VkDescriptorSetLayoutBinding descriptorSetLayoutBinding(
            VkDescriptorType descriptorType, VkShaderStageFlags stateFlags, uint32_t descriptorCount, uint32_t binding)
    {
        VkDescriptorSetLayoutBinding descriptorSetLayoutBinding = { };
        descriptorSetLayoutBinding.descriptorType = descriptorType;
        descriptorSetLayoutBinding.descriptorCount = descriptorCount;
        descriptorSetLayoutBinding.stageFlags = stateFlags;
        descriptorSetLayoutBinding.binding = binding;

        return descriptorSetLayoutBinding;
    }

    inline VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo(VkDescriptorSetLayout* layouts, uint32_t layoutCount)
    {
        VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = { };
        pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutCreateInfo.setLayoutCount = layoutCount;
        pipelineLayoutCreateInfo.pSetLayouts = layouts;

        return pipelineLayoutCreateInfo;
    }

    inline VkDescriptorSetAllocateInfo descriptorSetAllocateInfo(
            VkDescriptorPool descriptorPool, VkDescriptorSetLayout* setLayouts, uint32_t descriptorSetCount)
    {
        VkDescriptorSetAllocateInfo descriptorSetAllocateInfo = { };
        descriptorSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        descriptorSetAllocateInfo.descriptorPool = descriptorPool;
        descriptorSetAllocateInfo.descriptorSetCount = descriptorSetCount;
        descriptorSetAllocateInfo.pSetLayouts = setLayouts;

        return descriptorSetAllocateInfo;
    }

    inline VkRenderPassBeginInfo renderPassBeginInfo(VkRenderPass renderPass, uint32_t width, uint32_t height, VkClearValue* clearValue)
    {
        VkRenderPassBeginInfo renderPassBeginInfo = { };
        renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassBeginInfo.renderPass = renderPass;
        renderPassBeginInfo.renderArea.offset.x = 0;
        renderPassBeginInfo.renderArea.offset.y = 0;
        renderPassBeginInfo.renderArea.extent.width = width;
        renderPassBeginInfo.renderArea.extent.height = height;

        renderPassBeginInfo.clearValueCount = 1;
        renderPassBeginInfo.pClearValues = clearValue;

        return renderPassBeginInfo;
    }

    inline VkCommandBufferBeginInfo commandBufferBeginInfo()
    {
        VkCommandBufferBeginInfo cmdBufInfo = { };
        cmdBufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

        return cmdBufInfo;
    }

    inline VkFenceCreateInfo fenceCreateInfo(VkFenceCreateFlags flags=0)
    {
        VkFenceCreateInfo fenceCreateInfo = { };
        fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceCreateInfo.flags = flags;

        return fenceCreateInfo;
    }
}