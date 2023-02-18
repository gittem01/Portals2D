#include <Shader.h>
#include <IOHelper.h>
#include <definitions.h>

Shader::Shader(VkDevice device, std::vector<std::string> fileNames)
{
    this->device = device;
    this->shaderStages.resize(fileNames.size());

    VkShaderStageFlagBits bits[3] = { VK_SHADER_STAGE_VERTEX_BIT, VK_SHADER_STAGE_FRAGMENT_BIT, VK_SHADER_STAGE_GEOMETRY_BIT };

    for (int i = 0; i < fileNames.size(); i++)
    {
        VkShaderModule shaderModule = load_shader_module(device, fileNames[i].c_str());
        shaderModules.push_back(shaderModule);
        VkPipelineShaderStageCreateInfo info = { };
        info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;

        info.stage = bits[i];
        info.module = shaderModule;
        info.pName = "main";
        shaderStages[i] = info;
    }
}

Shader::~Shader()
{
    for (int i = 0; i < shaderModules.size(); i++) {
        vkDestroyShaderModule(device, shaderModules.at(i), NULL);
    }
    shaderModules.clear();
    shaderStages.clear();
}

VkShaderModule Shader::load_shader_module(VkDevice device, const char* filePath)
{
    std::string finalPath = IOHelper::assetsFolder + filePath;
    
    std::ifstream file(finalPath.c_str(), std::ios::ate | std::ios::binary);
    if (!file.is_open()) {
        printf("Could not find shader file: %s\n", finalPath.c_str());
        return NULL;
    }

    size_t fileSize = (size_t)file.tellg();
    std::vector<uint32_t> buffer(fileSize / sizeof(uint32_t));
    file.seekg(0);
    file.read((char*)buffer.data(), fileSize);
    file.close();

    VkShaderModuleCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = buffer.size() * sizeof(uint32_t);
    createInfo.pCode = buffer.data();

    VkShaderModule shaderModule;
    CHECK_RESULT_VK(vkCreateShaderModule(device, &createInfo, NULL, &shaderModule));

    return shaderModule;
}