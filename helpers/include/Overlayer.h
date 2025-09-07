#pragma once

#include <vulkan/vulkan.h>
#include <Camera.h>
#include <Leveler.h>
#include <CharDrawer.h>

typedef enum
{
	LEVELER = 0,
	LEVELER_MENU = 1,
	TESTER_MENU = 2,
	LOAD_LEVEL = 3,
	SAVE_LEVEL = 4
} overlayType;

typedef struct Pop
{
	std::vector<std::vector<int>> texts;
	std::vector<std::string> secondaryPops;
	glm::vec2 size;
	int mask;
} s_pop;

typedef struct RenderPop
{
	std::string popName;
	glm::vec2 pos;
} s_renderPop;

class Overlayer
{
public:
	class Game* game;
	class ThinDrawer* thinDrawer;
	class WindowHandler* wh;
	class ArrowShader* as;
	class ItemList* il;
	class OptionShader* os;
	class ManyTexture* mt;
	class CharDrawer* cd;
	class Sounder* sounder;
	class Camera cam;

	std::vector<s_gridItemData> items;
	std::map<std::string, s_pop> pops;
	std::vector<s_renderPop> renderPops;
	std::vector<std::string> polarizationString;

	const char numX = 4;
	const char numY = 8;
	const float leftX = -80.0;
	const float xSize = numX * 9.0f + 1.0;
	const float ySize = 72.0;
	const char totalNum = numX * numY;

	char selectedItemIndex = -1;
	char lastHoverIndex = -1;

	bool clickStartOnPlay = false;
	bool hoveringPlay = false;
	bool clickedPlay = false;
	overlayType overlayOverlay = LEVELER;

	float itemRatio = 0.0;
	bool itemDir = 0;
	std::string lastPopName = std::string("");
	std::string popShowName = std::string("");
	int popHoverIndices[2] = { -1, -1 };
	int popHeight;
	glm::vec2 popShowPos;
	glm::ivec2 levelerRCPos;
	s_gridData rightClickItem;
	int startAlloc;
	s_menuButton topLeftButtons[2];
	double topLeftHoverTimes[2];

	unsigned int switchSound;
	unsigned int clickSound;
	unsigned int bopSound;
	unsigned int removeSound;
	unsigned int undoSound;
	unsigned int redoSound;

	Overlayer(Game* game);

	void pushRenderFunctions();
	void eraseRenderFunctions();
	void addItem(const char* name, const char* fileName, levelObject objectType, std::string popName="none",
		glm::vec2 gridRenderTranslation = glm::vec2(0.0, 0.0), int numTextureObjects = 1, glm::ivec2 gridScale=glm::ivec2(1, 1),
		float gridRenderScale=1.0, bool canRotate=false, int requiredSideFill=-1, bool canMirror=false, float extraScale=0.0);
	void getPops();
	int addPop(std::string popName, std::vector<std::string> texts, std::vector<std::string> secondaryPops, int mask);
	void topLeftUpdate();
	void popUpdate();
	void handleHoverPlay();
	void update();

	static void camMap(void* ptr);
	static void maper(void* ptr);
	static void draw(VkCommandBuffer cmdBuff, void* ptr);
};