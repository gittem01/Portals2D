#pragma once

#include <vector>
#include <functional>
#include <mutex>
#include "SDL3/SDL.h"
#include "PopUp.h"
#include "steam_api.h"

class Game
{
public:

	static SDL_Scancode jumpKey;
	static SDL_Scancode leftKey;
	static SDL_Scancode rightKey;
	static SDL_Scancode downKey;
	static SDL_Scancode rotateKey;
	static SDL_Scancode holdKey;
	static int limitFramerate;

	static int lastLevel;
	static int maxLevel;

	static Game* object;

	bool isPaused;
	bool tick;
	int frame;
	bool shouldQuit;

	double gameTime;
	double deltaTime;
	double lastTime;
	double dtMult;

	std::vector<std::function<void(void)>> singleTaskRegistry;
	std::vector<std::function<void(void)>> singleTaskRegistry2;
	std::mutex taskRegistryMutex;

	// steam

	uint32_t appid;
	CCallResult<Game, CreateItemResult_t> createItemResult;
	CCallResult<Game, SubmitItemUpdateResult_t> updateItemResult;

	// steam_end

	class BaseTestCase* testCase;

	class PortalWorld* pWorld;
	class mouseJointHandler* mjh;
	class ContactListener* contactListener;
	class ThinDrawer* thinDrawer;
	class Sounder* sounder;
	PopUp pop;

	class Leveler* leveler;
	class Menu* menu;

	void(Game::* gameStater)(void);
	void(Game::* lastState)(void) = nullptr;

	Game();
	
	void onItemCreated(CreateItemResult_t* pCallback, bool bIOFailure);
	void onItemSubmitted(SubmitItemUpdateResult_t* pCallback, bool bIOFailure);

	void loadGameData();
	void saveGameData();

	void runSingleTaskRegistry();
	void runSingleTaskRegistry2();
	void loop();

	void timeHandle();

	void stateStarterAnimation(float time = 1.0f);

	void LevelerState();
	void TestState();
	void MenuState();
};

double SDL_GetTicksSec();