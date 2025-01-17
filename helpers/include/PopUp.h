#pragma once

#include "ShaderHeaders/MenuButton.h"
#include "CharDrawer.h"
#include <map>
#include <string>
#include <optional>
#include <functional>

typedef enum LLoadEnum { L_NONE, L_LOADING, L_LOADED, L_ERROR } lLoadEnum;

typedef struct LevelSelectData
{
	std::string text;
	s_lineRenderData textData;
	std::vector<uint8_t> usedElements;
	lLoadEnum status = lLoadEnum::L_NONE;
} levelSelectData;

class PopUp
{
public:

	s_menuButton initialButton;
	s_menuButton finalButton;
	double startTime;
	double animationTime;
	double extraTextDisplayTime;
	double extraTextDisplayAmount;
	float scrollAmount;
	float targetScroll;
	int scrollClickIndex;
	float clickOffset;
	float totalSize;
	bool closeOnEnd;
	bool noAction;
	bool active;

	unsigned int sliderHitSound;

	std::optional<std::function<void()>> endFunction;
	std::map<std::string, void(*)(VkCommandBuffer, void*)> renderFunctions;
	std::map<std::string, void(PopUp::*)(void)> initFunctions;
	std::vector<std::string> renderTexts;
	std::vector<char> textSizes;

	PopUp();
	void construct();

	void init(s_menuButton initialButton, std::string popName, double animTime);

	void initSave();
	void initLoad();
	void initLanguage();
	void initDarkness();

	void renderAt(VkCommandBuffer cmdBuff, glm::vec4 posSize, glm::vec2 pos, levelSelectData data);

	static void saveRender(VkCommandBuffer cmdBuff, void* ptr);
	static void loadRender(VkCommandBuffer cmdBuff, void* ptr);
	static void languageRender(VkCommandBuffer cmdBuff, void* ptr);
	static void darknessRender(VkCommandBuffer cmdBuff, void* ptr);
};