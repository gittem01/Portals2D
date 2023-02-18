#pragma once

#include <helpers.h>
#include <WindowHandler.h>

#define ENABLE_TESTPLAYER 0

#if ENABLE_TESTPLAYER
    TestPlayer* tp;
#endif

class BaseTestCase
{
protected:
	int refreshItem;
    const char* name;

public:
    WindowHandler* wh;
    bool shouldGetNewTest;
    int testCaseItem;

    void destroy()
    {
        delete mjh;
        delete pWorld;
    #if ENABLE_TESTPLAYER
        delete tp;
    #endif

        pWorld = NULL;
    }

    BaseTestCase(WindowHandler* wh, const char* name="Test Debug Window")
    {
        this->wh = wh;
        this->name = name;
		refreshItem = 0;
        shouldGetNewTest = false;
    }

    virtual ~BaseTestCase() { }

    void beforeCreate()
    {
        if (!contactListener)
        {
            contactListener = new ContactListener();
        }
        pWorld = new PortalWorld();
        pWorld->SetContactListener(contactListener);

        mjh = new mouseJointHandler(pWorld, wh);

        b2Vec2 gravity(0.0f, -10.0f);
        pWorld->SetGravity(gravity);

    #if ENABLE_TESTPLAYER
        tp = new TestPlayer(pWorld, wh);
    #endif
    }

    void afterCreate(){
        // manual random coloring for now
        for (Portal* p : pWorld->portals){
            b2Color c = b2Color(1, getRand() + 0.5f, getRand() + 0.5f, 1.0f);
        }

        for (int i = 0; i < 2; i++){
            for (Portal* p : pWorld->portals){
                p->evaluateWorld();
            }
        }
    }

    virtual void onUpdate(){
    #if ENABLE_TESTPLAYER
        tp->camHandle();
    #endif
        if (!pWorld) return;
        for (int i = 0; i < totalIter; i++) {
			// speeded up 2x times
            float dt = (2.0f / 120) / totalIter;
            mjh->mouseHandler(frame, totalIter);
        #if ENABLE_TESTPLAYER
            tp->update(dt, totalIter);
        #endif
            pWorld->PortalStep(dt, 8, 3);
        }
    }

    virtual void onCreate() = 0;
};

BaseTestCase* testCase = NULL;
