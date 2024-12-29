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
int Game::limitFramerate = 1001;

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

    sounder = new Sounder();

    loadGameData();

	thinDrawer = new ThinDrawer(pWorld);

    leveler = new Leveler(this);
    
    menu = new Menu();

    thinDrawer->releaseRenderResources();

    gameStater = &Game::MenuState;

    pop.construct();
}

void Game::loadGameData()
{
	std::string fileName = IOHelper::assetsFolder + "gamedata/basicdata.bin";
	std::ifstream file(fileName, std::ios::binary);

    const uint32_t expectedFileSize = 16;

    // get file size
    file.seekg(0, file.end);
    int length = file.tellg();
    file.seekg(0, file.beg);

	if (file.is_open() && length >= expectedFileSize)
	{
        uint64_t arr[expectedFileSize];

		file.read((char*)arr, sizeof(arr));
		file.close();

		Game::jumpKey = (SDL_Scancode)arr[0];
		Game::leftKey = (SDL_Scancode)arr[1];
		Game::rightKey = (SDL_Scancode)arr[2];
		Game::downKey = (SDL_Scancode)arr[3];
		Game::rotateKey = (SDL_Scancode)arr[4];
		Game::holdKey = (SDL_Scancode)arr[5];

		Sounder::generalVolume = arr[6] / 1024.0;
		Sounder::musicVolume = arr[7] / 1024.0;
		Sounder::effectVolume = arr[8] / 1024.0;

		lastLevel = arr[9];
        
        WindowHandler::windowCreateFlags = arr[10];
        Game::limitFramerate = arr[11];
		SwapChain::enableVsync = arr[12];
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

		std::string fileName = IOHelper::assetsFolder + "gamedata/basicdata.bin";
		std::ofstream file(fileName, std::ios::binary);

		if (file.is_open())
		{
			uint64_t arr[16];

			arr[0] = Game::jumpKey;
			arr[1] = Game::leftKey;
			arr[2] = Game::rightKey;
			arr[3] = Game::downKey;
			arr[4] = Game::rotateKey;
			arr[5] = Game::holdKey;

			arr[6] = (uint64_t)std::round(Sounder::generalVolume * 1024);
			arr[7] = (uint64_t)std::round(Sounder::musicVolume * 1024);
			arr[8] = (uint64_t)std::round(Sounder::effectVolume * 1024);

			arr[9] = Game::lastLevel;
            arr[10] = WindowHandler::windowCreateFlags;
            arr[11] = Game::limitFramerate;
			arr[12] = SwapChain::enableVsync;

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

void Game::timeHandle()
{
    double timeNow = SDL_GetTicksSec();

    float baseDeltaTime = std::min((timeNow - lastTime), 0.1);
    deltaTime = baseDeltaTime * dtMult;
    thinDrawer->gameTime += baseDeltaTime;
    thinDrawer->deltaTime = baseDeltaTime;
    lastTime = timeNow;
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
    else if (lastState != &Game::TestState)
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

        timeHandle();

        leveler->update();
        sounder->update();

        runSingleTaskRegistry();

        thinDrawer->renderLoop();

        runSingleTaskRegistry2();
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
        leveler->renderClear();
        thinDrawer->releaseRenderResources();
        leveler->quitLeveler();
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
        testCase = newTest(this, 1);

        thinDrawer->whiteWallShader->testFill();
        thinDrawer->blackWallShader->testFill();
        thinDrawer->wallLight->testFill();
        thinDrawer->diagonalLiner->testFill();
    }

    SDL_ShowWindow(thinDrawer->wh->window);

    lastTime = SDL_GetTicksSec();
    while (!shouldQuit && (testCase && !testCase->shouldQuit || !testCase))
    {
        uint64_t t0 = SDL_GetTicksNS();

        shouldQuit = thinDrawer->wh->looper();
        
        if (dtMult > 0.0000000001) thinDrawer->wh->cam->update();

        frame++;

        timeHandle();

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

        runSingleTaskRegistry();

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

        thinDrawer->renderLoop();

		uint64_t t1 = SDL_GetTicksNS();

        if (limitFramerate < 1001)
        {
            uint64_t timePassed = t1 - t0;
            uint64_t timeToWait = (double)1000000000 / limitFramerate - timePassed;

            if (timeToWait > 0)
            {
                SDL_DelayNS(timeToWait);
            }
        }
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
        WallLaserReceiver::numWallLaserReceivers = 0;
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

    ALuint menuMusic = sounder->playLoopingSound(sounder->loadSound("Menu.wav"), 0.0);
    sounder->addFader(menuMusic, 0.0f, 0.15f, 1.0f);

    lastTime = SDL_GetTicksSec();
    while (!shouldQuit && gameStater == &Game::MenuState)
    {
        shouldQuit = thinDrawer->wh->looper();

        frame++;

        timeHandle();

        sounder->update();

        menu->update();

        runSingleTaskRegistry();

        thinDrawer->renderLoop();

        runSingleTaskRegistry2();
    }

    if (gameStater == &Game::MenuState)
    {
        gameStater = nullptr;
    }
    else
    {
        shouldQuit = false;
    }

    sounder->addFader(menuMusic, 0.15f, 0.0f, 1.0f, true);

	lastState = &Game::MenuState;
}

void Game::runSingleTaskRegistry()
{
    if (singleTaskRegistry.size() == 0)
    {
        return;
    }

    taskRegistryMutex.lock();

    for (auto func : singleTaskRegistry)
    {
        func();
    }

    singleTaskRegistry.clear();

    taskRegistryMutex.unlock();
}

void Game::runSingleTaskRegistry2()
{
    if (singleTaskRegistry2.size() == 0)
    {
        return;
    }

    taskRegistryMutex.lock();

    for (auto func : singleTaskRegistry2)
    {
        func();
    }

    singleTaskRegistry2.clear();

    taskRegistryMutex.unlock();
}

void Game::loop()
{
    while (gameStater)
    {
        (this->*gameStater)();
    }
}

double SDL_GetTicksSec()
{
    return SDL_GetTicksNS() / 1000000000.0;
}