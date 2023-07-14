#include <BaseTestCase.h>

class TestCase2 : public BaseTestCase
{
public:
    TestCase2(Game* game) : BaseTestCase(game)
    {
        testCaseItem = 1;
        
        beforeCreate();
        onCreate();
        afterCreate();
    }

    void onCreate(){
        b2Vec2 gravity(0.0f, -9.81f);
        g->pWorld->SetGravity(gravity);
        Portal* portal1 = g->pWorld->createPortal(b2Vec2(-7.0f, 0.0f), b2Vec2(+1.0f, 0.0f), 7.0f);
        Portal* portal2 = g->pWorld->createPortal(b2Vec2(+7.0f, 0.0f), b2Vec2(-1.0f, 0.0f), 7.0f);
        Portal* portal3 = g->pWorld->createPortal(b2Vec2(0.0f, -7.0f), b2Vec2(0.0f, +1.0f), 7.0f);
        Portal* portal4 = g->pWorld->createPortal(b2Vec2(0.0f, +7.0f), b2Vec2(0.0f, -1.0f), 7.0f);

        portal1->connect(portal2);
        portal3->connect(portal4);

        for (int i = 0; i < 300; i++){
            b2Body* body = createBox(b2Vec2(0, 0), b2Vec2(0.3f, 0.3f), g->pWorld, b2_dynamicBody);
            PortalBody* pb = g->pWorld->createPortalBody(body);
            td->debug_addPortalBody(pb);
        }
    }
};
