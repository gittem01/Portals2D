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
        thinDrawer->addPortalBody(tp->pBody->at(0));
        thinDrawer->addPortalBody(tp->gun->at(0));
    #endif
    }

    void afterCreate(){
        // manual random coloring for now
        for (Portal* p : pWorld->portals){
            b2Color c = b2Color(1, getRand() + 0.5f, getRand() + 0.5f, 1.0f);
        }

        /*for (int i = 0; i < 2; i++){
            for (Portal* p : pWorld->portals){
                p->evaluateWorld();
            }
        }*/
    }

    virtual void onUpdate()
    {
#if ENABLE_TESTPLAYER
        //tp->camHandle();
#endif
        if (!pWorld)
        {
            return;
        }
        for (int i = 0; i < totalIter; i++)
        {
			// speeded up 2x times
            float dt = (2.0f / 144.0f) / totalIter;
            mjh->mouseHandler(frame, totalIter);
            pWorld->PortalStep(dt, 8, 3);
#if ENABLE_TESTPLAYER
            tp->update(dt, totalIter);
#endif
        }
    }

    virtual void onCreate() = 0;
};

BaseTestCase* testCase = NULL;
