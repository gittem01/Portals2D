#include "Game.h"
#include "TestCaseIncludes.h"
#include "Sounder.h"
#include "Leveler.h"
#include "Menu.h"
#include "Overlayer.h"
#include "DefaultObjects/Turret.h"
#include "DefaultObjects/Gel.h"
#include "GameObjects/Platform.h"
#include "GameObjects/ForceField.h"
#include "GameObjects/HazardField.h"
#include "GameObjects/Geller.h"
#include "GameObjects/UseButton.h"
#include "GameObjects/DownButton.h"
#include "GameObjects/CubeButton.h"
#include "GameObjects/CircleButton.h"
#include "GameObjects/LaserEmitter.h"
#include "GameObjects/WallLaserReceiver.h"
#include "GameObjects/GndLaserReceiver.h"
#include "GameObjects/Funnel.h"
#include "GameObjects/LightBridge.h"
#include "GameObjects/Catapult.h"
#include <mutex>

Game* Game::object;

SDL_Scancode Game::jumpKey = SDL_SCANCODE_W;
SDL_Scancode Game::leftKey = SDL_SCANCODE_A;
SDL_Scancode Game::rightKey = SDL_SCANCODE_D;
SDL_Scancode Game::downKey = SDL_SCANCODE_S;
SDL_Scancode Game::rotateKey = SDL_SCANCODE_Q;
SDL_Scancode Game::holdKey = SDL_SCANCODE_E;

int Game::lastLevel = 1;
int Game::maxLevel = 27;

Game::Game()
{
    Game::object = this;

	isPaused = false;
	tick = false;
    shouldQuit = false;
	frame = 0;
    deltaTime = 0.0;
    lastTime = 0.0;
    gameTime = 0.0;
    dtMult = 1.0;
    testCase = nullptr;
    pWorld = nullptr;

    contactListener = nullptr;

	IOHelper::getAssetsFolder();

	thinDrawer = new ThinDrawer(pWorld);

	sounder = new Sounder();

    leveler = new Leveler(this);
    
    menu = new Menu();

    thinDrawer->releaseRenderResources();

    loadGameData();

    gameStater = &Game::MenuState;
}

void Game::loadGameData()
{
	std::string fileName = "gameData.bin";
	std::ifstream file(fileName, std::ios::binary);

    const uint32_t expectedFileSize = 16;

    // get file size
    file.seekg(0, file.end);
    int length = file.tellg();
    file.seekg(0, file.beg);

	if (file.is_open() && length >= expectedFileSize)
	{
        uint16_t arr[expectedFileSize];

		file.read((char*)arr, sizeof(arr));
		file.close();

		jumpKey = (SDL_Scancode)arr[0];
		leftKey = (SDL_Scancode)arr[1];
		rightKey = (SDL_Scancode)arr[2];
		downKey = (SDL_Scancode)arr[3];
		rotateKey = (SDL_Scancode)arr[4];
		holdKey = (SDL_Scancode)arr[5];

		sounder->generalVolume = arr[6] / 1024.0;
		sounder->musicVolume = arr[7] / 1024.0;
		sounder->effectVolume = arr[8] / 1024.0;

		lastLevel = arr[9];
	}
	else
	{
		saveGameData();
	}
}

void Game::saveGameData()
{
    static std::mutex saveMutex;

	std::thread saveThread([this]()
	{
        if (!saveMutex.try_lock())
        {
			return;
        }

		std::string fileName = "gameData.bin";
		std::ofstream file(fileName, std::ios::binary);

		if (file.is_open())
		{
			uint16_t arr[16];

			arr[0] = jumpKey;
			arr[1] = leftKey;
			arr[2] = rightKey;
			arr[3] = downKey;
			arr[4] = rotateKey;
			arr[5] = holdKey;

			arr[6] = (uint16_t)std::round(sounder->generalVolume * 1024);
			arr[7] = (uint16_t)std::round(sounder->musicVolume * 1024);
			arr[8] = (uint16_t)std::round(sounder->effectVolume * 1024);

			arr[9] = lastLevel;

			file.write((char*)arr, sizeof(arr));
			file.close();
		}
		else
		{
            // call on main thread
			//SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", "Error creating save file", thinDrawer->wh->window);
		}

		saveMutex.unlock();
	});
    
	saveThread.detach();
}

void Game::LevelerState()
{
    leveler->ol->overlayOverlay = LEVELER;

    if (lastState == &Game::MenuState)
    {
        thinDrawer->releaseRenderResources();

        leveler->pushRenderFunctions();
		leveler->ol->pushRenderFunctions();
        leveler->renderInit();
    }
    else
    {
        leveler->renderInit();
    }

    SDL_ShowWindow(thinDrawer->wh->window);

    lastTime = SDL_GetTicksSec();
    while (!shouldQuit && !leveler->shouldQuit)
    {
        shouldQuit = thinDrawer->wh->looper();
        if (!leveler->ol->overlayOverlay)
            thinDrawer->wh->cam->update();

        frame++;

        deltaTime = SDL_GetTicksSec() - lastTime;
        gameTime += deltaTime;
        lastTime = SDL_GetTicksSec();

        leveler->update();
        sounder->update();

        thinDrawer->gameTime = gameTime;
        thinDrawer->renderLoop();
    }

    shouldQuit = false;

    if (leveler->shouldQuit && gameStater == &Game::LevelerState)
    {
        leveler->shouldQuit = false;
        gameStater = &Game::TestState;
    }
    else if (gameStater == &Game::LevelerState)
    {
        gameStater = nullptr;
    }
    else
    {
        thinDrawer->releaseRenderResources();
    }

	lastState = &Game::LevelerState;
}

void Game::TestState()
{
    dtMult = 1.0;
    if (lastState == &Game::LevelerState)
    {
        LightBridge::staticInit();
        Turret::staticInit();
        Gel::staticInit();
        LaserEmitter::staticInit();
        Platform::staticInit();
        ForceField::staticInit();
        HazardField::staticInit();
        Geller::staticInit();
        UseButton::staticInit();
        DownButton::staticInit();
        CubeButton::staticInit();
        CircleButton::staticInit();
        WallLaserReceiver::staticInit();
        GndLaserReceiver::staticInit();
        Funnel::staticInit();
        Catapult::staticInit();
        TestPlayer::object->renderInit();
    }
    else
    {
        testCase = newTest(this, 3);

        thinDrawer->whiteWallShader->testFill();
        thinDrawer->blackWallShader->testFill();
        thinDrawer->wallLight->testFill();
        thinDrawer->diagonalLiner->testFill();
    }

    SDL_ShowWindow(thinDrawer->wh->window);

    lastTime = SDL_GetTicksSec();
    while (!shouldQuit && (testCase && !testCase->shouldQuit || !testCase))
    {
        shouldQuit = thinDrawer->wh->looper();
        thinDrawer->wh->cam->update();

        frame++;

        double timeNow = SDL_GetTicksSec();
        deltaTime = std::min((timeNow - lastTime) * dtMult, 0.1);
        lastTime = timeNow;
        if ((!isPaused || tick) && testCase)
        {
            gameTime += deltaTime;
            
            testCase->onUpdate();

            ForceField::update();
            Geller::update();
            Gel::perFrameUpdate();
            UseButton::update();
            LaserEmitter::update();
            WallLaserReceiver::update();
            GndLaserReceiver::update();
            Funnel::update();
            LightBridge::update();
        }

        for (auto func : singleTaskRegistry)
        {
            func();
        }
        singleTaskRegistry.clear();

        sounder->update();

        if (testCase)
        {
            testCase->imguiRender();

            testCase->keyHandler(thinDrawer->wh);

            if (testCase->shouldGetNewTest)
            {
                testCase->destroy();
                int nextTestCase = testCase->testCaseItem + 1;
                delete testCase;
                testCase = newTest(this, nextTestCase);
            }
            else
            {
                testCase->perFrameUpdate();
            }
        }

        thinDrawer->gameTime = gameTime;
        thinDrawer->renderLoop();
    }

    if (testCase)
    {
        if (lastState == &Game::LevelerState)
        {
            ForceField::toLeveler();
            HazardField::toLeveler();
            Platform::toLeveler();
            UseButton::toLeveler();
            DownButton::toLeveler();
            CubeButton::toLeveler();
            CircleButton::toLeveler();
            Funnel::toLeveler();
            LightBridge::toLeveler();
            Gel::toLeveler();
            Catapult::toLeveler();
        }

        testCase->shouldQuit = false;

        testCase->destroy();
        Turret::turrets.clear();
        Platform::numPlatforms = 0;
        thinDrawer->wallGel->numWallLines = 0;
        thinDrawer->wallGel->numWallGels = 0;
        DefaultObject::defaultObjects.clear();
        Geller::numGellers = 0;
        Gel::gelCount = 0;
        Gel::splashCount = 0;
        UseButton::checkButtons.clear();
        LaserEmitter::numLaserEmitters = 0;
        WallLaserReceiver::disableChecks.clear();
        WallLaserReceiver::enableChecks.clear();
        GndLaserReceiver::disableChecks.clear();
        GndLaserReceiver::enableChecks.clear();
        GndLaserReceiver::numGndLaserReceivers = 0;
        Funnel::funnels.clear();

        delete testCase;
    }

    if (shouldQuit)
    {
		gameStater = nullptr;
        return;
	}

    if (lastState == &Game::LevelerState)
    {
        gameStater = &Game::LevelerState;
        shouldQuit = false;
        leveler->pushRenderFunctions();
        leveler->ol->pushRenderFunctions();
    }

	lastState = &Game::TestState;
}

void Game::MenuState()
{
    menu->initDefaults();

    menu->renderInit();

    menu->initPlay();
    menu->initLeveler();
    menu->initSettings();
    menu->initCredits();
    menu->exitButton = menu->initWithName("exit", glm::vec4(1.0, 0.0, 0.0, 0.2));

    SDL_ShowWindow(thinDrawer->wh->window);

    lastTime = SDL_GetTicksSec();
    while (!shouldQuit && gameStater == &Game::MenuState)
    {
        shouldQuit = thinDrawer->wh->looper();

        frame++;

        deltaTime = SDL_GetTicksSec() - lastTime;
        gameTime += deltaTime;
        lastTime = SDL_GetTicksSec();

        sounder->update();

        menu->update();

        thinDrawer->gameTime = gameTime;
        thinDrawer->renderLoop();
    }

    if (gameStater == &Game::MenuState)
    {
        gameStater = nullptr;
    }
    else
    {
        shouldQuit = false;
    }

	lastState = &Game::MenuState;
}

void Game::loop()
{
    while (gameStater)
    {
        (this->*gameStater)();
    }
}