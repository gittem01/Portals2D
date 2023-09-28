#pragma once

#include <vector>
#include <box2d/box2d.h>
#include "mouseJointHandler.h"
#include "ContactListener.h"
#include "ThinDrawer.h"

#include <thread>
#include <chrono>
#include <random>

class BaseTestCase
{
public:
    std::vector<const char*> testCases =
    {
        "Random Portals",
        "Squares",
        "Circles",
        "Speeder",
        "Elevator",
        "Squisher and Spirals",
        "Mixer",
        "ShootShoot Place",
    };

    static BaseTestCase* testCase;

	int refreshItem;
    const char* name;
    bool followPlayer = false;
    bool shouldQuit = false;

    class Game* g;
    class ThinDrawer* td;
    class TestPlayer* tp;
    bool shouldGetNewTest;
    int testCaseItem;
    
    double requestedDtPerStep = 0.001;

    void destroy();

    BaseTestCase(Game* game, const char* name = "Test Debug Window");

    void imguiRender();

    void beforeCreate(bool createTP=true);

    void afterCreate();

    void onUpdate();

    virtual void perFrameUpdate() {}

    void keyHandler(class WindowHandler* wh);

    virtual void onCreate() = 0;
};



// helpers

float getRand();
// O shaped body
b2Body* createObody(b2World* world, b2Vec2 bodyPos = b2Vec2(0, 0), float degree = 270.0f, int n = 20, float thickness = 0.3f, float rOut = 1.2f, float fd = 0.0f, float isMixed = false);

b2Body* createWbody(b2World* world, b2Vec2 bodyPos = b2Vec2(0, 0), float degree = 270.0f, int n = 20, float circleR = 0.15f, float rOut = 1.2f, float fd = 0.0f);

b2Body* createEdge(b2Vec2 p1, b2Vec2 p2, b2World* world, b2BodyType type = b2_staticBody, bool oneSided = false);

b2Body* createPortalCube(b2Vec2 pos, b2Vec2 size, b2World* world, b2BodyType type, float density = 1.0f);

b2Body* createBox(b2Vec2 pos, b2Vec2 size, b2World* world, b2BodyType type, float density = 1.0f);

b2Body* createCircle(b2Vec2 pos, float size, b2World* world, b2BodyType type, float density = 1.0f, bool sensor = false);