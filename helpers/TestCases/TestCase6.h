#include <BaseTestCase.h>

class TestCase6 : public BaseTestCase
{
public:
    TestCase6(Game* game) : BaseTestCase(game)
    {
        testCaseItem = 5;
        
        beforeCreate(false);
        onCreate();
        afterCreate();
    }

    void onCreate(){
        createEdge(b2Vec2(+10000.0f, -10.0f), b2Vec2(-10000.0f, -10.0f), g->pWorld, b2_staticBody);

        Portal* portal1 = g->pWorld->createPortal(b2Vec2(0.0f, -3.0f), b2Vec2(0.0f, +1.0f), 5.0f);
        Portal* portal2 = g->pWorld->createPortal(b2Vec2(0.0f, +3.0f), b2Vec2(0.0f, -1.0f), 5.0f);
        
        portal1->connect(portal2);
        portal1->connect(portal2, true, 1, 1);
        
        b2Body* body1 = createWbody(g->pWorld, b2Vec2(+6.0f, -7.5f), 540.0f, 100, 0.16f, 3.0f, 0.008f);
        PortalBody* b1 = g->pWorld->createPortalBody(body1);

        b2Body* body2 = createObody(g->pWorld, b2Vec2(-15.0f, -5.0f), 1080.0f, 50, 0.3f, 3.0f, 0.01f, false);
        PortalBody* b2 = g->pWorld->createPortalBody(body2);

        b2Body* body3 = createBox(b2Vec2(0, 0), b2Vec2(30, 0.5), g->pWorld, b2_dynamicBody);
        PortalBody* b3 = g->pWorld->createPortalBody(body3);

        td->addPortalBody(b3, {"long.png"});

        td->debug_addPortalBody(b1);
        td->debug_addPortalBody(b2);
        td->debug_addPortalBody(b3);

        b2Vec2 startPos = b2Vec2(29.0f, 20.0f);
        b2Body* lastBody = createBox(startPos, b2Vec2(0.5f, 0.2f), g->pWorld, b2_dynamicBody);

        PortalBody* pb = g->pWorld->createPortalBody(lastBody);
        td->debug_addPortalBody(pb);
        for (int i = 0; i < 100; i++){
            startPos -= b2Vec2(0.7f, 0.0f);
            
            b2Body* jointBody = createBox(startPos, b2Vec2(0.5f, 0.2f), g->pWorld, b2_dynamicBody);
            pb = g->pWorld->createPortalBody(jointBody);
            td->addPortalBody(pb, { "chain.png" });
            td->debug_addPortalBody(pb);

            b2RevoluteJointDef revDef;
            revDef.Initialize(jointBody, lastBody, startPos + b2Vec2(0.35f, 0.0f));
            revDef.calculateAnchorHelpers();
            revDef.enableLimit = true;
            revDef.upperAngle = +b2_pi * 0.1f;
            revDef.lowerAngle = -b2_pi * 0.1f;
            g->pWorld->CreateJoint(&revDef);

            lastBody = jointBody;
        }
    }
};
