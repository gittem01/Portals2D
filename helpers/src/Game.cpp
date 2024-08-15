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
    Turret::staticInit();
    Gel::staticInit();
    Platform::staticInit();
    ForceField::staticInit();
    HazardField::staticInit();

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

        if ((!isPaused || tick) && testCase)
        {
            deltaTime = glfwGetTime() - lastTime;
            gameTime += deltaTime;
            lastTime = glfwGetTime();

            testCase->onUpdate();
            Turret::perFrameUpdate();
            Gel::update();
            Platform::update();
            ForceField::update();
        }
        else
        {
            deltaTime = 0.0;
            lastTime = glfwGetTime();
        }

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
    
    if (leveler)
    {
        gameStater = &Game::LevelerState;
        if (testCase)
        {
            testCase->shouldQuit = false;

            testCase->destroy();
            Turret::turrets.clear();
            Platform::numPlatforms = 0;
            thinDrawer->wallGel->numWallLines = 0;
            delete testCase;
        }

        leveler->pushRenderFunctions();
        leveler->ol->pushRenderFunctions();
    }
    else
    {
        gameStater = nullptr;
    }
}

void Game::loop()
{
    while (gameStater)
    {
        (this->*gameStater)();
    }
}