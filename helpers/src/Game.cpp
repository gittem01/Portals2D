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

Game* Game::object;

SDL_Scancode Game::jumpKey = SDL_SCANCODE_W;
SDL_Scancode Game::leftKey = SDL_SCANCODE_A;
SDL_Scancode Game::rightKey = SDL_SCANCODE_D;
SDL_Scancode Game::downKey = SDL_SCANCODE_S;
SDL_Scancode Game::rotateKey = SDL_SCANCODE_Q;
SDL_Scancode Game::holdKey = SDL_SCANCODE_E;

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
    leveler = nullptr;
    pWorld = nullptr;

    contactListener = nullptr;

	IOHelper::getAssetsFolder();

	thinDrawer = new ThinDrawer(pWorld);

	sounder = new Sounder();

    loadGameData();

    gameStater = &Game::MenuState;
}

void Game::loadGameData()
{

}

void Game::saveGameData()
{

}

void Game::LevelerState()
{
    if (!leveler)
    {
        leveler = new Leveler(this);
        leveler->renderInit();
    }
    else if (lastState == &Game::MenuState)
    {
        thinDrawer->releaseRenderResources();

        leveler->pushRenderFunctions();
		leveler->ol->pushRenderFunctions();
        leveler->renderInit();
    }

    SDL_ShowWindow(thinDrawer->wh->window);

    lastTime = SDL_GetTicksSec();
    while (!shouldQuit && !leveler->shouldQuit)
    {
        shouldQuit = thinDrawer->wh->looper();
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

    if (leveler->shouldQuit)
    {
        leveler->shouldQuit = false;
        shouldQuit = false;
        gameStater = &Game::TestState;
    }
    else
    {
        gameStater = nullptr;
    }

	lastState = &Game::LevelerState;
}

void Game::TestState()
{
    dtMult = 1.0;
    if (leveler)
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
        if (leveler)
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

    if (leveler)
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
    if (!leveler)
    {
        leveler = new Leveler(this);
        thinDrawer->releaseRenderResources();
    }

	menu = new Menu();

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