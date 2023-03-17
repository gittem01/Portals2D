#pragma once

#include <helpers.h>
#include <WindowHandler.h>

#define ENABLE_TESTPLAYER 1

#if ENABLE_TESTPLAYER
    TestPlayer* tp;
#endif

class BaseTestCase
{
protected:
	int refreshItem;
    const char* name;

public:
    ThinDrawer* td;
    bool shouldGetNewTest;
    int testCaseItem;
    
    double lastTime = 0.0;
    double requestedDtPerStep = 0.002;
    double deltaTime;

    void destroy()
    {
        td->releaseRenderResources();

        delete mjh;
        delete pWorld;
    #if ENABLE_TESTPLAYER
        delete tp;
    #endif

        pWorld = NULL;
    }

    BaseTestCase(ThinDrawer* td, const char* name="Test Debug Window")
    {
        this->td = td;
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

        if (ImGui::RadioButton("Draw Releases", td->drawReleases))
        {
            td->drawReleases = !td->drawReleases;
        }
        ImGui::SameLine();
        if (isPaused)
        {
            if (ImGui::Button("Resume"))
            {
                isPaused = false;
            }
        }
        else
        {
            if (ImGui::Button("Stop"))
            {
                isPaused = true;
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Tick"))
        {
            isPaused = true;
            tick = true;
        }

        float oldVal = mjh->bodyRadius;
        ImGui::SliderFloat("MouseSelectionArea", &mjh->bodyRadius, mjh->radiusLimits[0], mjh->radiusLimits[1], "%.2f", 1);
        if (oldVal != mjh->bodyRadius)
        {
            mjh->createMouseBody();
        }

        ImGui::End();

        ImGui::Render();

        td->drawData = ImGui::GetDrawData();
    }

    void beforeCreate()
    {
        if (!contactListener)
        {
            contactListener = new ContactListener();
        }
        pWorld = new PortalWorld();
        thinDrawer->pWorld = pWorld;

        pWorld->SetContactListener(contactListener);

        mjh = new mouseJointHandler(pWorld, td->wh);

        b2Vec2 gravity(0.0f, -10.0f);
        pWorld->SetGravity(gravity);

    #if ENABLE_TESTPLAYER
        extern ThinDrawer* thinDrawer;
        tp = new TestPlayer(pWorld, td->wh);
        thinDrawer->debug_addPortalBody(tp->pBody->at(0));
        thinDrawer->debug_addPortalBody(tp->gun->at(0));

        std::vector<std::pair<std::string, std::string>> empty;
        thinDrawer->addPortalBody(tp->pBody->at(0), empty);
        thinDrawer->addPortalBody(tp->gun->at(0), empty);
    #endif
    }

    void afterCreate()
    {
        lastTime = glfwGetTime();
    }

    virtual void onUpdate()
    {
        deltaTime = glfwGetTime() - lastTime;
        lastTime = glfwGetTime();
        if (deltaTime > 1.0) deltaTime = 1.0;

        int requiredIters = fmax(deltaTime / requestedDtPerStep, 1);
        double requiredDt = deltaTime / requiredIters;

        printf("%d, %f, %.4f\n", requiredIters, requiredDt, requiredIters * requiredDt);

#if ENABLE_TESTPLAYER
        tp->camHandle();
#endif
        if (!pWorld)
        {
            return;
        }
        for (int i = 0; i < requiredIters; i++)
        {
			// speeded up 2x times
            float dt = requiredDt * 2.0;
            mjh->mouseHandler(frame, totalIter);
            pWorld->PortalStep(dt, 8, 3);
#if ENABLE_TESTPLAYER
            tp->update(dt, totalIter);
#endif
        }
    }

    void keyHandler(WindowHandler* wh)
    {
        extern BaseTestCase* testCase;

        tick = false;
        if (wh->keyData[GLFW_KEY_P] == 2)
        {
            isPaused = !isPaused;
            lastTime = glfwGetTime() - 0.01;
        }
        else if (wh->keyData[GLFW_KEY_O] == 2)
        {
            isPaused = true;
            tick = true;
            lastTime = glfwGetTime() - 0.01;
        }
        if (wh->keyData[GLFW_KEY_S] == 2)
        {
            thinDrawer->drawReleases ^= 1;
        }
    }

    virtual void onCreate() = 0;
};

BaseTestCase* testCase = NULL;
