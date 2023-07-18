#define STB_TRUETYPE_IMPLEMENTATION

#include <CharDrawer.h>
#include <ThinDrawer.h>
#include <glm/gtc/type_ptr.hpp>

std::vector<uint32_t> utf8ToUnicode(const std::string& utf8String)
{
    std::vector<uint32_t> codePoints;
    int i = 0;

    while (i < utf8String.length())
    {
        uint32_t codePoint = 0;

        int numBytes = 0;
        char ch = utf8String[i];
        if ((ch & 0x80) == 0)
        {
            // Single-byte character (0xxxxxxx)
            codePoint = ch;
            numBytes = 1;
        }
        else if ((ch & 0xE0) == 0xC0)
        {
            // Two-byte character (110xxxxx 10xxxxxx)
            codePoint = ch & 0x1F;
            numBytes = 2;
        }
        else if ((ch & 0xF0) == 0xE0)
        {
            // Three-byte character (1110xxxx 10xxxxxx 10xxxxxx)
            codePoint = ch & 0x0F;
            numBytes = 3;
        }
        else if ((ch & 0xF8) == 0xF0)
        {
            // Four-byte character (11110xxx 10xxxxxx 10xxxxxx 10xxxxxx)
            codePoint = ch & 0x07;
            numBytes = 4;
        }
        else
        {
            break;
        }

        // Extract the remaining bytes and construct the code point
        for (int j = 1; j < numBytes; j++)
        {
            if (i + j >= utf8String.length() || (utf8String[i + j] & 0xC0) != 0x80)
            {
                break;
            }
            codePoint = (codePoint << 6) | (utf8String[i + j] & 0x3F);
        }

        codePoints.push_back(codePoint);
        i += numBytes;
    }

    return codePoints;
}

s_texture* tex;

CharDrawer::CharDrawer(ThinDrawer* thinDrawer, const char* fontFile)
{
    IOHelper::getAssetsFolder();
    //fontFile = "Roboto-Regular.ttf";
    std::vector<unsigned char> fontData;
    stbtt_fontinfo fontInfo;
    loadFontFile(fontFile, fontData, fontInfo);

    this->thinDrawer = thinDrawer;

    int targetWidth = 1000;
    int targetHeight = 1000;

    std::string file_path = IOHelper::assetsFolder + "texts/testText.txt";

    // Open the file in binary mode
    std::ifstream file(file_path, std::ios::binary);

    // Read the file contents as a sequence of bytes
    std::string utf8String((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

    // Convert the UTF-8 encoded bytes to Unicode code points
    std::vector<uint32_t> codePoints = utf8ToUnicode(utf8String);

    // Print the Unicode code points
    for (uint32_t codepoint : codePoints)
    {
        if (codepoint == 32)
        {
            renderDatas.push_back({ {0.3, 0.0, 0.0, 0.0}, nullptr });
            continue;
        }

        float scale = stbtt_ScaleForPixelHeight(&fontInfo, targetHeight);

        int x0, y0, x1, y1;
        stbtt_GetCodepointBitmapBox(&fontInfo, codepoint, scale, scale, &x0, &y0, &x1, &y1);

        int ascent;
        stbtt_GetFontVMetrics(&fontInfo, &ascent, 0, 0);
        ascent = roundf(ascent * scale);

        int ax;
        int lsb;
        stbtt_GetCodepointHMetrics(&fontInfo, codepoint, &ax, &lsb);

        int y = ascent + y0;

        // Render the character into the bitmap
        int glyph = stbtt_FindGlyphIndex(&fontInfo, codepoint);
        int width, height, xOffset, yOffset;
        unsigned char* bitmap = stbtt_GetGlyphBitmap(&fontInfo, scale, scale, glyph, &width, &height, &xOffset, &yOffset);

        float toMid = 0.5 - ((float)y / targetHeight + ((float)height * 0.5f / targetHeight));
        float left = lsb * scale / targetWidth;

        s_textureData texData;
        texData.data = bitmap;
        texData.width = width;
        texData.height = height;
        texData.numChannels = 1;
        tex = (s_texture*)malloc(sizeof(s_texture));
        thinDrawer->loadTextureFromMemory(&texData, tex, true, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER);
        IOHelper::updateTextureSet(tex);

        s_charRenderData rData;
        rData.size_leftBotMargin = glm::vec4((float)width / targetWidth, (float)height / targetHeight, left, toMid);
        rData.tex = tex;

        renderDatas.push_back(rData);

        stbtt_FreeBitmap(bitmap, nullptr);
    }

    fontShader = new FontShader(thinDrawer);
    thinDrawer->shaders.push_back(fontShader);
    fontShader->init();

    thinDrawer->drawFuncs.push_back({ draw, this });
}

void CharDrawer::draw(VkCommandBuffer cmdBuff, void* ptr)
{
    CharDrawer* cd = (CharDrawer*)ptr;
    const VkDeviceSize offsets = 0;
    const uint32_t camOffset = ShaderBase::alignedSharedSize * cd->thinDrawer->lastSwapChainImageIndex;

    vkCmdBindPipeline(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS, cd->fontShader->pipeline);

    vkCmdBindVertexBuffers(cmdBuff, 0, 1, &cd->fontShader->buffers[0]->buffer, &offsets);
    
    vkCmdBindDescriptorSets(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS,
        cd->fontShader->pipelineLayout, 0, 1, &ShaderBase::sharedSet, 1, &camOffset);

    float x = 0.0;
    s_charRenderData prevData;
    prevData.size_leftBotMargin = { 0, 0, 0, 0 };
    for (auto& data : cd->renderDatas)
    {
        if (data.tex == nullptr)
        {
            x += data.size_leftBotMargin.x + prevData.size_leftBotMargin.x * 0.5;
            prevData.size_leftBotMargin = { 0, 0, 0, 0 };
            continue;
        }

        x += data.size_leftBotMargin.x * 0.5 + prevData.size_leftBotMargin.x * 0.5 + data.size_leftBotMargin.z;
        glm::vec4 d = data.size_leftBotMargin;
        d.x = x;
        d.y = data.size_leftBotMargin.w;
        d.z = data.size_leftBotMargin.x;
        d.w = data.size_leftBotMargin.y;
        vkCmdPushConstants(cmdBuff, cd->fontShader->pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(s_textData), (void*)glm::value_ptr(d));
        vkCmdBindDescriptorSets(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS,
            cd->fontShader->pipelineLayout, 1, 1, &data.tex->set, 0, VK_NULL_HANDLE);

        vkCmdDraw(cmdBuff, cd->fontShader->buffers[0]->count, 2, 0, 0);
        prevData = data;
    }
}

bool CharDrawer::loadFontFile(const std::string& filename, std::vector<unsigned char>& fontData, stbtt_fontinfo& fontInfo)
{
    std::string newFname = IOHelper::assetsFolder + "fonts/" + filename;
    std::ifstream file(newFname, std::ios::binary);
    if (!file)
    {
        printf("Failed to open font file: %s\n", filename.c_str());
        return false;
    }

    file.seekg(0, std::ios::end);
    std::streamsize fileSize = file.tellg();
    file.seekg(0, std::ios::beg);

    fontData.resize(fileSize);
    if (!file.read(reinterpret_cast<char*>(fontData.data()), fileSize))
    {
        printf("Failed to read font file: %s\n", filename.c_str());
        return false;
    }

    if (!stbtt_InitFont(&fontInfo, fontData.data(), 0))
    {
        printf("Failed to initialize font: %s\n", filename.c_str());
        return false;
    }

    return true;
}