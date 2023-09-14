#pragma once

#include <vector>

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