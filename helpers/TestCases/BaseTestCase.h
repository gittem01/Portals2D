#pragma once

#include <helpers.h>

#define ENABLE_TESTPLAYER 0

#if ENABLE_TESTPLAYER
    TestPlayer* tp;
#endif

class BaseTestCase
{
private:
    static int namerCount;
	int refreshItem;
    const char* name;
    WindowPainter* wh;

public:
    bool shouldGetNewTest;
    int testCaseItem;

    void debugDraw() {
		ImGui::Begin(name, NULL, ImGuiWindowFlags_AlwaysAutoResize);

		ImGui::Combo("Refresh Rate", &refreshItem, refreshRates, refreshSize);

		renderer->refreshRate = correspondingValues[refreshItem];

        int lastItem = testCaseItem;
        ImGui::Combo(" ", &testCaseItem, testCases, numTestCases);
        ImGui::SameLine();
        if (ImGui::Button("Reset")){
            destroy();
            shouldGetNewTest = true;
        }

        if (lastItem != testCaseItem){
            destroy();
            shouldGetNewTest = true;
        }

		if (ImGui::RadioButton("Draw Releases", renderer->drawReleases)) {
			renderer->drawReleases = !renderer->drawReleases;
		}
        ImGui::SameLine();
        if (isPaused){
            if (ImGui::Button("Resume")){
                isPaused = false;
            }
        }
        else{
            if (ImGui::Button("Stop")){
                isPaused = true;
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Tick")){
            isPaused = true;
            tick = true;
        }

        float oldVal = mjh->bodyRadius;
        ImGui::SliderFloat("MouseSelectionArea", &mjh->bodyRadius, mjh->radiusLimits[0], mjh->radiusLimits[1], "%.2f", 1);
        if (oldVal != mjh->bodyRadius){
            mjh->createMouseBody();
        }

        ImGui::SliderFloat("Particle Sizes", &renderer->sizeMult, 0.3f, 2.0f, "%.2f", 1);
        ImGui::SliderFloat("Particle Blurriness", &renderer->blurriness, 0.0f, 3.0f, "%.2f", 1);

		ImGui::End();
	}

    void destroy(){
        for (int i = 0; i < pWorld->portals.size(); i++){
            pWorld->destroyPortal(pWorld->portals.at(i));
            i--;
        }
        
        delete mjh;
        delete pWorld;
    #if ENABLE_TESTPLAYER
        delete tp;
    #endif

        pWorld = NULL;
        renderer->reset();
    }

    BaseTestCase(WindowPainter* wh, const char* name="Test Debug Window"){
        this->wh = wh;
        this->name = name;
		refreshItem = 0;
        shouldGetNewTest = false;
        
        if (!renderer){
            renderer = new Renderer(pWorld, wh->cam);
        }

		for (int i = 0; i < refreshSize; i++) {
			if (renderer->refreshRate == correspondingValues[i]) {
				refreshItem = i;
				break;
			}
		}
    }

    virtual ~BaseTestCase(){ }

    void beforeCreate(){
        renderer->sizeMult = 1.0f;
        if (!contactListener){
            contactListener = new ContactListener();
        }
        pWorld = new PortalWorld();
        pWorld->SetContactListener(contactListener);

        if (!drawer){
            drawer = new DebugDrawer(renderer);
        }

        pWorld->SetDebugDraw(drawer);
        drawer->SetFlags(b2Draw::e_centerOfMassBit);

        mjh = new mouseJointHandler(pWorld, wh, renderer);

        b2Vec2 gravity(0.0f, -10.0f);
        pWorld->SetGravity(gravity);

    #if ENABLE_TESTPLAYER
        tp = new TestPlayer(pWorld, wh);
        renderer->addPortalBody(tp->pBody->at(0), {}, "assets/textures/playerTest.png");
    #endif
    }

    void afterCreate(){
        // manual random coloring for now
        for (Portal* p : pWorld->portals){
            b2Color c = b2Color(1, getRand() + 0.5f, getRand() + 0.5f, 1.0f);
            renderer->addPortal(p, c);
        }

        renderer->portalAddEnd();

        for (int i = 0; i < 2; i++){
            for (Portal* p : pWorld->portals){
                p->evaluateWorld();
                p->queryClass->mode++;
            }
        }
    }

    void onDraw(){
        if (!pWorld) return;
        if(!isPaused) mjh->drawMouseBody();
    }

    void onUpdate(){
    #if ENABLE_TESTPLAYER
        tp->camHandle();
    #endif
        if (!pWorld) return;
        for (int i = 0; i < totalIter; i++) {
            float dt = (2.0f / renderer->refreshRate) / totalIter;
            mjh->mouseHandler(frame, totalIter);
        #if ENABLE_TESTPLAYER
            tp->update(dt, totalIter);
        #endif
			// speeded up 2x times
            pWorld->PortalStep(dt, 8, 3);
        }
    }

    virtual void onCreate() = 0;
};

int BaseTestCase::namerCount = 0;
BaseTestCase* testCase = NULL;
