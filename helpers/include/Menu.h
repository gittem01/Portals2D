#pragma once

#include "vulkan/vulkan.h"
#include "Game.h"
#include "ShaderHeaders/MenuButton.h"
#include "PortalWorld.h"
#include <bitset>
#include "CharDrawer.h"

typedef enum
{
	RENDER_ENABLED = 0,
	INTERACTABLE = 1,
	MOVE_NEXT = 2,
	TEXT_LEFT = 3,
} bitmapIndices;

typedef enum
{
	BUTTON_RECTANGLE = 1,
	BUTTON_CIRCLE = 2,
} buttonType;

class Button
{
public:

	Button();

	std::string strText = "";
	s_lineRenderData text; // 0 sized char datas for no text
	s_menuButton data;
	glm::vec2 vertices[4];
	double lastHoverTime = -100.0;
	float activeAmount = 0.0f;
	float openYMult = 0.0f;
	float openYAdder = 0.0f;
	float alphaMult = 1.0f;
	float baseAlpha = 0.2f;
	float baseX;
	float baseY;
	std::vector<Button*> dependentButtons;
	std::bitset<8> bools;
	buttonType type = BUTTON_RECTANGLE;

	void generateVertices();

	bool hoverCheck(glm::vec2 pos);
	bool rayCheck(b2Vec2 p21, b2Vec2 p22, float* len);
};

class Menu
{
public:
	class ThinDrawer* td;
	class Overlayer* ol;

	std::vector<Button> buttons;
	std::map<std::string, std::vector<std::string>> buttonTexts;

	double lastMouseTime;
	double activationTime;
	double upCamTime;
	double lastDataSaveTime;
	float upMargin;
	float currentY;
	glm::vec2 lastMousePos;

	Button* hoverButton;
	Button* clickButton;
	Button* activeButton;
	Button* qButton;
	Button* soundButton;
	Button* newEditorButton;
	Button* exitButton;
	Button* keyAssignmentButton;

	Menu();

	void initDefaults();
	void renderInit(bool fullInit=true);
	void renderDestroy();

	void initPlay(float w = 0.2f);
	void initLeveler(float w = 0.2f);
	void initSettings(float w = 0.2f);
	void initCredits(float w = 0.2f);
	Button* initWithName(std::string name, glm::vec4 clr); // simple only click button

	void addTextToButton(Button& b, std::string text);
	Button* addButton(glm::vec2 pos, glm::vec2 size, std::string text, glm::vec4 color=glm::vec4(1.0, 1.0, 1.0, 0.2), glm::vec2 data=glm::vec2(0.25f, 0.5f));
	void buttonPressed(Button* button);
	void update();

	static void draw(VkCommandBuffer cmdBuff, void* ptr);
};