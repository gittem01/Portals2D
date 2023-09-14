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

    class Game* g;
    class ThinDrawer* td;
    class TestPlayer* tp;
    bool shouldGetNewTest;
    int testCaseItem;
    
    double requestedDtPerStep = 0.001;

    void destroy();

    BaseTestCase(Game* game, const char* name = "Test Debug Window");

    virtual ~BaseTestCase() { }

    void imguiRender();

    void beforeCreate();

    void afterCreate();

    virtual void onUpdate();

    void keyHandler(class WindowHandler* wh);

    virtual void onCreate() = 0;
};