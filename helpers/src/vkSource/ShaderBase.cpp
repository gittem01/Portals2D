#include <ShaderBase.h>
#include <ThinDrawer.h>
#include <SwapChain.h>
#include <vkInit.h>

#define ALLOCATION_COUNT_ALL 2000
#define ALLOCATION_COUNT_POLY 2000

ShaderBase::ShaderBase(ThinDrawer* thinDrawer)
{
    this->thinDrawer = thinDrawer;
    this->logicalDevice = thinDrawer->logicalDevice;
}

void ShaderBase::init(ThinDrawer* thinDrawer)
{
    createCommonStorageBuffers(thinDrawer);
    setSharedLayout(thinDrawer);
    writeSharedDescriptors(thinDrawer);
}

void ShaderBase::setSharedLayout(ThinDrawer* thinDrawer)
{
    const int s = 32;
    alignedSharedSize = thinDrawer->pad_uniform_buffer_size(sizeof(s_sharedUniformData));
    sharedOrthoUniform = (s_uniformBuffer*)malloc(sizeof(s_uniformBuffer));
    thinDrawer->uniformHelper(alignedSharedSize * s, sharedOrthoUniform, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, s);

    VkDescriptorSetLayoutBinding layoutBinding0 =
        vkinit::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 1, 0);

    VkDescriptorSetLayoutBinding bindings[] = { layoutBinding0 };

    VkDescriptorSetLayoutCreateInfo descriptorLayout = vkinit::descriptorSetLayoutCreateInfo(bindings, sizeof(bindings) / sizeof(bindings[0]));
    CHECK_RESULT_VK(vkCreateDescriptorSetLayout(thinDrawer->logicalDevice, &descriptorLayout, VK_NULL_HANDLE, &sharedLayout));
}

void ShaderBase::writeSharedDescriptors(ThinDrawer* thinDrawer)
{
    VkDescriptorSetAllocateInfo allocInfo = vkinit::descriptorSetAllocateInfo(thinDrawer->descriptorPool, &sharedLayout, 1);
    CHECK_RESULT_VK(vkAllocateDescriptorSets(thinDrawer->logicalDevice, &allocInfo, &sharedSet));

    VkWriteDescriptorSet writeDescriptorSet = { };

    writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet.dstSet = sharedSet;
    writeDescriptorSet.descriptorCount = 1;
    writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;

    writeDescriptorSet.pBufferInfo = &ShaderBase::sharedOrthoUniform->descriptor;
    writeDescriptorSet.dstBinding = 0;
    vkUpdateDescriptorSets(thinDrawer->logicalDevice, 1, &writeDescriptorSet, 0, VK_NULL_HANDLE);
}

void ShaderBase::createCommonStorageBuffers(ThinDrawer* thinDrawer)
{
    const int bufferSize = 2;

    int imCount = thinDrawer->swapChain->imageCount;

    commonStorageBuffers.resize(imCount);

    int alignedSizeFS = thinDrawer->pad_storage_buffer_size(sizeof(s_uboAll));
    int alignedSizePoly = thinDrawer->pad_storage_buffer_size(sizeof(s_uboPolyData));

    alignedSizes = { alignedSizeFS, alignedSizePoly };

    commonStorageBuffers.resize(bufferSize);

    for (int j = 0; j < bufferSize; j++)
    {
        commonStorageBuffers[j] = (s_uniformBuffer*)malloc(sizeof(s_uniformBuffer));
    }

    initialAllocationSizes.resize(bufferSize);
    avlAllocLastLoc.resize(bufferSize);
    freeIndicesList.resize(bufferSize);

    initialAllocationSizes[0] = ALLOCATION_COUNT_ALL;
    initialAllocationSizes[1] = ALLOCATION_COUNT_POLY;

    avlAllocLastLoc[0] = initialAllocationSizes[0] - 1;
    avlAllocLastLoc[1] = initialAllocationSizes[1] - 1;

    for (int j = 0; j < bufferSize; j++)
    {
        freeIndicesList[j] = (int*)malloc(sizeof(int) * initialAllocationSizes[j]);

        for (int k = 0; k < initialAllocationSizes[j]; k++)
        {
            freeIndicesList[j][initialAllocationSizes[j] - k - 1] = k;
        }

        thinDrawer->uniformHelper(alignedSizeFS * initialAllocationSizes[j],
            commonStorageBuffers[j], VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, initialAllocationSizes[j]);
    }
}

void ShaderBase::init()
{
    prepareVertexData();
    prepareUniforms();
    setupDescriptorSetLayout();
    preparePipeline();
    setupDescriptorSet();
}

VkDescriptorSetLayout ShaderBase::sharedLayout;
VkDescriptorSet ShaderBase::sharedSet;
int ShaderBase::alignedSharedSize;
s_uniformBuffer* ShaderBase::sharedOrthoUniform;

std::vector<s_uniformBuffer*> ShaderBase::commonStorageBuffers;
std::vector<int> ShaderBase::initialAllocationSizes;
std::vector<int> ShaderBase::avlAllocLastLoc;
std::vector<int*> ShaderBase::freeIndicesList;
std::vector<int64_t> ShaderBase::alignedSizes;