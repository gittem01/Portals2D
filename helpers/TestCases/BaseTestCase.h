#pragma once

#include <helpers.h>
#include <Game.h>

#define ENABLE_TESTPLAYER 1

#if ENABLE_TESTPLAYER
    TestPlayer* tp;
#endif

class BaseTestCase
{
protected:
	int refreshItem;
    const char* name;
    bool followPlayer = false;

public:
    Game* g;
    ThinDrawer* td;
    bool shouldGetNewTest;
    int testCaseItem;
    
    double requestedDtPerStep = 0.002;

    void destroy()
    {
        td->releaseRenderResources();

        delete g->mjh;
        delete g->pWorld;
    #if ENABLE_TESTPLAYER
        delete tp;
    #endif

        g->pWorld = NULL;
    }

    BaseTestCase(Game* game, const char* name="Test Debug Window")
    {
        this->g = game;
        this->td = game->thinDrawer;
        this->name = name;
		refreshItem = 0;
        shouldGetNewTest = false;
    }

    virtual ~BaseTestCase() { }

    void imguiRender()
    {
        ImGuiIO io = ImGui::GetIO();

        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGui::Begin("Debug Window", NULL, ImGuiWindowFlags_AlwaysAutoResize);

        ImGui::Text("fps: %.2f", io.Framerate);
        ImGui::SameLine();
        ImGui::Text("dt: %.4f", io.DeltaTime * 1000.0f);

        int lastItem = testCaseItem;
        ImGui::Combo(" ", &testCaseItem, testCases, numTestCases);
        ImGui::SameLine();
        if (ImGui::Button("Reset"))
        {
            shouldGetNewTest = true;
        }

        if (lastItem != testCaseItem)
        {
            shouldGetNewTest = true;
        }

        if (ImGui::RadioButton("Follow Player", followPlayer))
        {
            followPlayer = !followPlayer;
        }

        if (ImGui::RadioButton("Draw Releases", td->drawReleases))
        {
            td->drawReleases = !td->drawReleases;
        }

        if (ImGui::RadioButton("Draw Debug Bodies", td->drawDebugBodies))
        {
            td->drawDebugBodies = !td->drawDebugBodies;
        }

        if (ImGui::RadioButton("Draw Debug Edges", td->drawDebugEdges))
        {
            td->drawDebugEdges = !td->drawDebugEdges;
        }

        if (ImGui::RadioButton("Draw Debug Portals", td->drawDebugPortals))
        {
            td->drawDebugPortals = !td->drawDebugPortals;
        }

        if (g->isPaused)
        {
            if (ImGui::Button("Resume"))
            {
                g->isPaused = false;
            }
        }
        else
        {
            if (ImGui::Button("Stop"))
            {
                g->isPaused = true;
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Tick"))
        {
            g->isPaused = true;
            g->tick = true;
        }

        float oldVal = g->mjh->bodyRadius;
        ImGui::SliderFloat("MouseSelectionArea", &g->mjh->bodyRadius, g->mjh->radiusLimits[0], g->mjh->radiusLimits[1], "%.2f", 1);
        if (oldVal != g->mjh->bodyRadius)
        {
            g->mjh->createMouseBody();
        }

        ImGui::End();

        ImGui::Render();

        td->drawData = ImGui::GetDrawData();
    }

    void beforeCreate()
    {
        td->debugLineRegistry.clear();

        if (!g->contactListener)
        {
            g->contactListener = new ContactListener();
        }
        g->pWorld = new PortalWorld();
        g->pWorld->SetContactListener(g->contactListener);
        b2Vec2 gravity(0.0f, -10.0f);
        g->pWorld->SetGravity(gravity);
        g->pWorld->SetAllowSleeping(true);

        g->mjh = new mouseJointHandler(g->pWorld, td->wh);

        td->pWorld = g->pWorld;
        td->beforeCreate();

    #if ENABLE_TESTPLAYER
        tp = new TestPlayer(g);
    #endif
    }

    void afterCreate()
    {
        td->afterCreate();
    }

    virtual void onUpdate()
    {
        double physicsTickTime = glfwGetTime();

        td->debugLineRegistry.clear();

        int requiredIters = fmax(g->deltaTime / requestedDtPerStep, 1);
        double requiredDt = g->deltaTime / requiredIters;
        requiredIters = fmin(requiredIters, 10);
#if ENABLE_TESTPLAYER
        if (followPlayer) tp->camHandle();
#endif
        if (!g->pWorld)
        {
            return;
        }

        for (int i = 0; i < requiredIters; i++)
        {
            float dt = requiredDt * 1.0;
            g->mjh->mouseHandler(g->frame, requiredIters);
#if ENABLE_TESTPLAYER
            tp->update(dt, requiredIters);
#endif
            g->pWorld->PortalStep(dt, 1, 1);
        }

        printf("dt: %f\n", glfwGetTime() - physicsTickTime);
    }

    void keyHandler(WindowHandler* wh)
    {
        extern BaseTestCase* testCase;

        g->tick = false;
        if (wh->keyData[GLFW_KEY_P] == 2)
        {
            g->isPaused = !g->isPaused;
        }
        else if (wh->keyData[GLFW_KEY_O] == 2)
        {
            g->isPaused = true;
            g->tick = true;
        }
        if (wh->keyData[GLFW_KEY_S] == 2)
        {
            td->drawReleases ^= 1;
        }
    }

    virtual void onCreate() = 0;
};

BaseTestCase* testCase = NULL;
