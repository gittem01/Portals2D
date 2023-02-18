#pragma once

#include <vulkan/vulkan.h>
#include <vector>
#include <fstream>

class Shader {
public:
    Shader(VkDevice device, std::vector<std::string> fileNames);
    ~Shader();
    std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
    std::vector<VkShaderModule> shaderModules;
    VkDevice device;

private:
    VkShaderModule load_shader_module(VkDevice device, const char* filePath);
};