#pragma once

#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <map>

#include <stb_truetype.h>
#include <FontShader.h>

typedef struct CharRenderData
{
	glm::vec4 size_leftBotMargin;
	s_texture* tex;
} s_charRenderData;

typedef struct LineRenderData
{
	std::vector<s_charRenderData> charDatas;
	glm::vec4 posScale; // startX, startY, scale, unused
	glm::vec4 color;
} s_lineRenderData;

class CharDrawer
{
public:
	static int selectedLanguageIndex;
	
	class ThinDrawer* thinDrawer;
	FontShader* fontShader;
	std::vector<unsigned char> fontData;
	stbtt_fontinfo fontInfo;
	int creationHeight;
	std::string languageName;
	std::vector<std::string> allLanguages;

	std::vector<s_lineRenderData> renderDatas;
	std::map<std::pair<int, int>, s_charRenderData> codepointMap;

	CharDrawer(ThinDrawer* thinDrawer);

	std::vector<s_charRenderData> getRenderData(const std::string& text, int targetHeight, float& x,
		s_texture* prevTex=nullptr, int prevCodepoint =0);

	static float calcSize(std::vector<s_charRenderData> rd);
	static void draw(VkCommandBuffer cmdBuff, void* ptr);

	void recreate();
	bool loadFontFile(const std::string& filename, std::vector<unsigned char>& fontData, stbtt_fontinfo& fontInfo);
};