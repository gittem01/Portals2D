#include <BaseTestCase.h>

class TestCase6 : public BaseTestCase
{
public:
    TestCase6(WindowPainter* wh) : BaseTestCase(wh){
        testCaseItem = 5;
        
        beforeCreate();
        onCreate();
        afterCreate();
    }

    void onCreate(){
        createEdge(b2Vec2(-100.0f, -10.0f), b2Vec2(+100.0f, -10.0f), pWorld, b2_staticBody);

        Portal* portal1 = pWorld->createPortal(b2Vec2(0.0f, -3.0f), b2Vec2(0.0f, +1.0f), 5.0f);
        Portal* portal2 = pWorld->createPortal(b2Vec2(0.0f, +3.0f), b2Vec2(0.0f, -1.0f), 5.0f);
        
        portal1->connect(portal2);
        portal1->connect(portal2, true, 1, 1);
        
        b2Body* body1 = createWbody(pWorld, b2Vec2(+6.0f, -7.5f), 540.0f, 100, 0.16f, 3.0f, 0.008f);
        PortalBody* b1 = pWorld->createPortalBody(body1);

        b2Body* body2 = createObody(pWorld, b2Vec2(-15.0f, -5.0f), 1080.0f, 50, 0.3f, 3.0f, 0.01f, false);
        PortalBody* b2 = pWorld->createPortalBody(body2);

        b2Body* body3 = createBox(b2Vec2(0, 0), b2Vec2(30, 0.5), pWorld, b2_dynamicBody);
        PortalBody* b3 = pWorld->createPortalBody(body3);

        renderer->addPortalBody(b1);
        renderer->addPortalBody(b2);
        renderer->addPortalBody(b3, b2Color(1, 1, 1, 0.5f), "assets/textures/long.png");

        b2Vec2 startPos = b2Vec2(29.0f, 20.0f);
        b2Body* lastBody = createBox(startPos, b2Vec2(0.5f, 0.2f), pWorld, b2_dynamicBody);

        PortalBody* pb = pWorld->createPortalBody(lastBody);
        renderer->addPortalBody(pb, b2Color(1, 1, 1, 0.5f), "assets/textures/chain.png", GL_NEAREST);
        for (int i = 0; i < 100; i++){
            startPos -= b2Vec2(0.7f, 0.0f);
            
            b2Body* jointBody = createBox(startPos, b2Vec2(0.5f, 0.2f), pWorld, b2_dynamicBody);
            pb = pWorld->createPortalBody(jointBody);
            renderer->addPortalBody(pb, b2Color(getRand() + 0.5f, getRand() + 0.5f, 1.0f, 0.5f), "assets/textures/chain.png", GL_NEAREST);

            b2RevoluteJointDef revDef;
            revDef.Initialize(jointBody, lastBody, startPos + b2Vec2(0.35f, 0.0f));
            // revDef.enableLimit = true;
            // revDef.upperAngle = +b2_pi * 0.2f;
            // revDef.lowerAngle = -b2_pi * 0.2f;
            b2RevoluteJoint* revJoint = (b2RevoluteJoint*)pWorld->CreateJoint(&revDef);

            lastBody = jointBody;
        }
    }
};
