#include <Game.h>
#include <TestCaseIncludes.h>
#include <Sounder.h>
#include <Leveler.h>
#include <Overlayer.h>
#include <DefaultObjects/Turret.h>
#include <DefaultObjects/Gel.h>
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

Game* Game::object;

Game::Game()
{
    Game::object = this;

	isPaused = false;
	tick = false;
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

    gameStater = &Game::LevelerState;

    glfwShowWindow(thinDrawer->wh->window);
}

void Game::LevelerState()
{
    if (!leveler)
    {
        leveler = new Leveler(this);
    }

    lastTime = glfwGetTime();
    while (!glfwWindowShouldClose(thinDrawer->wh->window) && !leveler->shouldQuit)
    {
        thinDrawer->wh->looper();
        thinDrawer->wh->cam->update();

        frame++;

        deltaTime = glfwGetTime() - lastTime;
        gameTime += deltaTime;
        lastTime = glfwGetTime();

        leveler->update();
        sounder->update();

        thinDrawer->gameTime = gameTime;
        thinDrawer->renderLoop();
    }

    if (leveler->shouldQuit)
    {
        leveler->shouldQuit = false;

        gameStater = &Game::TestState;
    }
    else
    {
        gameStater = nullptr;
    }
}

void Game::TestState()
{
    dtMult = 1.0;
    Turret::staticInit();
    Gel::staticInit();
    Platform::staticInit();
    ForceField::staticInit();
    HazardField::staticInit();
    Geller::staticInit();
    UseButton::staticInit();
    DownButton::staticInit();
    CubeButton::staticInit();
    CircleButton::staticInit();
    LaserEmitter::staticInit();
    WallLaserReceiver::staticInit();
    GndLaserReceiver::staticInit();
    Funnel::staticInit();

    if (!leveler)
    {
        testCase = newTest(this, 8);

        thinDrawer->whiteWallShader->testFill();
        thinDrawer->blackWallShader->testFill();
        thinDrawer->wallLight->testFill();
        thinDrawer->diagonalLiner->testFill();
    }

    lastTime = glfwGetTime();
    while (!glfwWindowShouldClose(thinDrawer->wh->window) && (testCase && !testCase->shouldQuit || !testCase))
    {
        thinDrawer->wh->looper();
        thinDrawer->wh->cam->update();

        frame++;

        double timeNow = glfwGetTime();
        deltaTime = std::min((timeNow - lastTime) * dtMult, 0.1);
        lastTime = timeNow;
        if ((!isPaused || tick) && testCase)
        {
            gameTime += deltaTime;
            
            testCase->onUpdate();

            Turret::perFrameUpdate();
            Platform::update();
            ForceField::update();
            Geller::update();
            Gel::perFrameUpdate();
            UseButton::update();
            LaserEmitter::update();
            WallLaserReceiver::update();
            GndLaserReceiver::update();
            Funnel::update();
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
        
            testCase->perFrameUpdate();
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

    if (leveler)
    {
        gameStater = &Game::LevelerState;

        leveler->pushRenderFunctions();
        leveler->ol->pushRenderFunctions();
    }
}

void Game::loop()
{
    while (gameStater)
    {
        (this->*gameStater)();
    }
}