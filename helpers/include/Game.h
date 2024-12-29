#pragma once

#include <vector>
#include <functional>
#include <mutex>
#include "SDL3/SDL.h"
#include "PopUp.h"

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
	
	void loadGameData();
	void saveGameData();

	void runSingleTaskRegistry();
	void runSingleTaskRegistry2();
	void loop();

	void timeHandle();

	void LevelerState();
	void TestState();
	void MenuState();
};