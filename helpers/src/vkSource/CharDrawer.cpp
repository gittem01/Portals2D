#define STB_TRUETYPE_IMPLEMENTATION

#include <CharDrawer.h>
#include <ThinDrawer.h>
#include <glm/gtc/type_ptr.hpp>
#include <vkInit.h>

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

CharDrawer::CharDrawer(ThinDrawer* thinDrawer)
{
	auto languages = IOHelper::loadLines("languages.txt");
	auto languageMap = IOHelper::mapVector(languages);
	selectedLanguageIndex = std::stoi(languageMap["<selected_language_index>"][0]);
	std::string s = "<" + std::to_string(selectedLanguageIndex) + ">";
	
	auto polarizations = IOHelper::loadLines("pops/polarization_language.txt");
	auto polarizationMap = IOHelper::mapVector(polarizations);

	for (auto& p : polarizationMap)
	{
		if (p.first.size() < 2 || p.first[1] < '0' || p.first[1] > '9') continue;
		std::string subString = p.first.substr(1, p.first.size() - 2);
		polarizationString[std::stoi(subString)] = p.second[0];
	}

	languageName = languageMap[s][0];
	loadFontFile(languageMap[s][1], fontData, fontInfo);

	this->thinDrawer = thinDrawer;

	fontShader = new FontShader(thinDrawer);
	thinDrawer->shaders.push_back(fontShader);
	fontShader->init();

	creationHeight = thinDrawer->wh->windowSizes.y;
}

void CharDrawer::draw(VkCommandBuffer cmdBuff, void* ptr)
{
	CharDrawer* cd = (CharDrawer*)ptr;
	const VkDeviceSize offsets = 0;
	const uint32_t camOffset = ShaderBase::alignedSharedSize * (cd->thinDrawer->frameIndex + 8);

	vkCmdBindPipeline(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS, cd->fontShader->pipeline);

	vkCmdBindVertexBuffers(cmdBuff, 0, 1, &cd->fontShader->buffers[0].buffer, &offsets);

	vkCmdBindDescriptorSets(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS,
		cd->fontShader->pipelineLayout, 0, 1, &ShaderBase::sharedSet, 1, &camOffset);

	for (const auto& data : cd->renderDatas)
	{
		const glm::vec2 posOffset = glm::vec2(data.posScale.x, data.posScale.y);
		const float scale = data.posScale.z;

		float x = 0.0;

		s_charRenderData prevData;
		prevData.size_leftBotMargin = { 0, 0, 0, 0 };

		s_textData texData;
		texData.color = data.color;

		for (auto& chrData : data.charDatas)
		{
			if (chrData.tex == nullptr)
			{
				x += chrData.size_leftBotMargin.x + prevData.size_leftBotMargin.x * 0.5;
				prevData.size_leftBotMargin = { 0, 0, 0, 0 };
				continue;
			}

			x += chrData.size_leftBotMargin.x * 0.5 + prevData.size_leftBotMargin.x * 0.5 + (x != 0 ? chrData.size_leftBotMargin.z : 0.0);

			texData.data.x = x * scale + posOffset.x;
			texData.data.y = chrData.size_leftBotMargin.w * scale + posOffset.y;
			texData.data.z = chrData.size_leftBotMargin.x * scale;
			texData.data.w = chrData.size_leftBotMargin.y * scale;

			vkCmdPushConstants(cmdBuff, cd->fontShader->pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(s_textData), (void*)&texData);
			vkCmdBindDescriptorSets(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS,
				cd->fontShader->pipelineLayout, 1, 1, &chrData.tex->set, 0, VK_NULL_HANDLE);

			vkCmdDraw(cmdBuff, cd->fontShader->buffers[0].count, 1, 0, 0);
			prevData = chrData;
		}
	}

	cd->renderDatas.clear();
}

float CharDrawer::calcSize(std::vector<s_charRenderData> rd)
{
	float x = 0.0f;
	float prevX = 0.0f;

	for (auto& chrData : rd)
	{
		if (chrData.tex == nullptr)
		{
			x += chrData.size_leftBotMargin.x + prevX * 0.5;
			prevX = 0.0f;
			continue;
		}

		x += chrData.size_leftBotMargin.x * 0.5 + prevX * 0.5 + (x != 0 ? chrData.size_leftBotMargin.z : 0.0);

		prevX = chrData.size_leftBotMargin.x;
	}

	x += prevX * 0.5;

	return x;
}

std::vector<s_charRenderData> CharDrawer::getRenderData(const std::string& text, int targetHeight, float& x)
{
	// Convert the UTF-8 encoded bytes to Unicode code points
	std::vector<uint32_t> codePoints = utf8ToUnicode(text);

	std::vector<s_charRenderData> rd;

	targetHeight = std::round(targetHeight * ThinDrawer::object->wh->dpiScaling);

	x = 0;
	float prevX = 0.0;
	for (uint32_t codepoint : codePoints)
	{
		if (codepoint == 32)
		{
			rd.push_back({ {0.25, 0.0, 0.0, 0.0}, nullptr });

			x += 0.25 + prevX * 0.5;
			prevX = 0.0;

			continue;
		}

		const auto& iterFind = codepointMap.find({ codepoint, targetHeight });
		if (iterFind != codepointMap.end())
		{
			rd.push_back((*iterFind).second);
			x += (*iterFind).second.size_leftBotMargin.x * 0.5 + prevX * 0.5 + (x != 0.0 ? (*iterFind).second.size_leftBotMargin.z : 0.0);
			prevX = (*iterFind).second.size_leftBotMargin.x;
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
		if (glyph == 0)
		{
			codepoint = 0;
			const auto& iterFind = codepointMap.find({ 0, targetHeight });
			if (iterFind != codepointMap.end())
			{
				rd.push_back((*iterFind).second);
				x += (*iterFind).second.size_leftBotMargin.x * 0.5 + prevX * 0.5 + (x != 0.0 ? (*iterFind).second.size_leftBotMargin.z : 0.0);
				prevX = (*iterFind).second.size_leftBotMargin.x;
				continue;
			}
		}

		stbtt_vertex* rdShape;
		int numVert = stbtt_GetGlyphShape(&fontInfo, glyph, &rdShape);

		if (numVert == 0) return rd;

		stbtt__bitmap rdBitmap;
		rdBitmap.w = x1 - x0;
		rdBitmap.h = y1 - y0;
		rdBitmap.stride = rdBitmap.w;
		rdBitmap.pixels = new uint8_t[rdBitmap.w * rdBitmap.h];
		stbtt_Rasterize(&rdBitmap, 0.01f, rdShape, numVert, scale, scale, 0.0f, 0.0f, x0, y0, 1, nullptr);

		float toMid = 0.5 - ((float)y / targetHeight + ((float)rdBitmap.h * 0.5f / targetHeight));
		float left = lsb * scale / targetHeight;

		s_textureData texData;
		texData.data = rdBitmap.pixels;
		texData.width = rdBitmap.w;
		texData.height = rdBitmap.h;
		texData.numChannels = 1;
		s_texture* tex = (s_texture*)malloc(sizeof(s_texture));

		thinDrawer->loadTextureFromMemory(&texData, tex, false, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER, 1, VK_FILTER_LINEAR);

		IOHelper::updateTextureSet(tex);

		s_charRenderData rData;
		rData.size_leftBotMargin = glm::vec4((float)rdBitmap.w / targetHeight, (float)rdBitmap.h / targetHeight, left, toMid);
		rData.tex = tex;

		x += rData.size_leftBotMargin.x * 0.5 + prevX * 0.5 + (x != 0.0 ? rData.size_leftBotMargin.z : 0.0);
		prevX = rData.size_leftBotMargin.x;

		codepointMap[{ codepoint, targetHeight }] = rData;

		rd.push_back(rData);

		delete rdBitmap.pixels;

		stbtt_FreeShape(&fontInfo, rdShape);
	}

	x += prevX * 0.5;

	return rd;
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