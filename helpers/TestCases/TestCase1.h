#include <BaseTestCase.h>

class TestCase1 : public BaseTestCase
{
private:
    b2Body* platform = nullptr;
    std::vector<PortalBody*>* snakeHead = nullptr;
    std::vector<PortalBody*>* snakeTail = nullptr;
    b2RevoluteJoint* snakeJoint = nullptr;

public:
    TestCase1(WindowPainter* wh) : BaseTestCase(wh){
        testCaseItem = 0;

        beforeCreate();
        onCreate();
        afterCreate();
    }

    void onCreate(){
        float yPos = -4.0f;
        float portalSize = 3.0f;

        Portal* portal1 = pWorld->createPortal(b2Vec2(-12.0f - portalSize, yPos), b2Vec2(+1.5f, -0.0f), portalSize);
        Portal* portal2 = pWorld->createPortal(b2Vec2(+6.0f, yPos - portalSize + 4.0f), b2Vec2(0.0f, -1.0f), portalSize);
        Portal* portal3 = pWorld->createPortal(b2Vec2(+6.0f, yPos - portalSize + 0.0f), b2Vec2(0.0f, +1.0f), portalSize);
        Portal* portal41 = pWorld->createPortal(b2Vec2(+10.0f + 0.2f, yPos), b2Vec2(+1.0f, 0.0f), portalSize);
        Portal* portal42 = pWorld->createPortal(b2Vec2(+10.0f + 0.2f, yPos + portalSize * 2), b2Vec2(+1.0f, 0.0f), portalSize);

        Portal* diagonalPortal1 = pWorld->createPortal(b2Vec2(-10.0f + 0.2f, yPos + portalSize), b2Vec2(+1.0f, 0.0f), portalSize);
        Portal* diagonalPortal2 = pWorld->createPortal(b2Vec2(-10.0f + 0.2f, yPos + portalSize * 2 + 5.0f), b2Vec2(+1.0f, -1.0f), portalSize);

        Portal* topPortal1 = pWorld->createPortal(b2Vec2(+5.0f, yPos + portalSize + 5.0f), b2Vec2(0.0f, -1.0f), portalSize);
        Portal* topPortal2 = pWorld->createPortal(b2Vec2(-5.0f, yPos + portalSize + 5.0f), b2Vec2(0.0f, -1.0f), portalSize);

        Portal* portal5 = pWorld->createPortal(b2Vec2(-12.0f, yPos - portalSize + 0.0f), b2Vec2(0.0f, +1.0f), portalSize);
        Portal* portal6 = pWorld->createPortal(b2Vec2(-20.0f, yPos - portalSize + 0.0f), b2Vec2(0.0f, +1.0f), portalSize);

//        Portal* destroyer = pWorld->createPortal(b2Vec2(-30.0f, 7.0f), b2Vec2(1.0f, +0.0f), 14.0f);
//        destroyer->setVoid(0);
//        destroyer->setVoid(1);

        portal3->connect(portal2, false);
        portal41->connect(portal1, true, 0, 0);
        portal42->connect(portal1, false, 0, 1);
        portal5->connect(portal6, true, 0, 0);
        diagonalPortal1->connect(diagonalPortal2, true, 0, 0);

        // TODO : do this special case automatically
        //diagonalPortal2->connect(topPortal1, true, 1, 0);
        //diagonalPortal2->connect(topPortal2, false, 1, 0);
        topPortal1->connect(topPortal2);

        createEdge(b2Vec2(-30.0f, yPos - portalSize), b2Vec2(+30.0f, yPos - portalSize), pWorld, b2_staticBody);
        createEdge(b2Vec2(+30.0f, yPos - portalSize), b2Vec2(+30.0f, 21), pWorld, b2_staticBody);
        createEdge(b2Vec2(+30.0f, 21), b2Vec2(-30.0f, 21), pWorld, b2_staticBody);
        createEdge(b2Vec2(-30.0f, 21), b2Vec2(-30.0f, yPos - portalSize), pWorld, b2_staticBody);

        PortalBody* b1 = pWorld->createPortalBody(createObody(pWorld, b2Vec2(0.0f, 3.0f)));
        PortalBody* b2 = pWorld->createPortalBody(createWbody(pWorld, b2Vec2(0.0f, 6.0f)));

        renderer->addPortalBody(b1, b2Color(0.0f, 1.0f, 1.0f, 0.5f));
        renderer->addPortalBody(b2, b2Color(1.0f, 0.0f, 1.0f, 0.5f));

        b2Vec2 p(5.0f, 0.0f);
        b2Vec2 s(0.75f, 0.75f);
        b2Body* body2 = createPortalCube(p, s, pWorld, b2_dynamicBody);
        b2Body* body20 = createPortalCube(p + b2Vec2(1, 0), s, pWorld, b2_dynamicBody);
        //b2Body* body21 = createPortalCube(p + b2Vec2(2, 0), s, pWorld, b2_dynamicBody);
        b2Body* body21 = createBox(p + b2Vec2(2, 0), s, pWorld, b2_dynamicBody);

        PortalBody* pb1 = pWorld->createPortalBody(body2);
        PortalBody* pb2 = pWorld->createPortalBody(body20);
        PortalBody* pb3 = pWorld->createPortalBody(body21);

        renderer->addPortalBody(pb1, b2Color(1.0f, 0.6f, 0.5f, 0.5f), "assets/textures/cube.png");
        renderer->addPortalBody(pb2, b2Color(0.5f, 0.6f, 1.0f, 0.5f), "assets/textures/cube.png");
        renderer->addPortalBody(pb3, b2Color(0.6f, 1.0f, 0.5f, 0.5f));

        PortalBody* pb;
    #if 1
        // artificial kinematic body
        p = b2Vec2(6.0f, -6.0f);
        s = b2Vec2(1.5f, 0.2f);
        platform = createBox(p, s, pWorld, b2_dynamicBody, 0.0f);
        pb = pWorld->createPortalBody(platform);
        renderer->addPortalBody(pb, b2Color(0.0f, 1.0f, 1.0f, 0.5f));

        b2Vec2 vel(-10.0f, 0.0f);
        platform->SetLinearVelocity(vel);
        platform->SetBullet(true);
        platform->SetLinearDamping(0.0f);
    #endif

        p = b2Vec2(0.0f, 0.0f);
        float r = 0.7f;
        b2Body* body3 = createCircle(p, r, pWorld, b2_dynamicBody);
        pb = pWorld->createPortalBody(body3);
        renderer->addPortalBody(pb, b2Color(1.0f, 1.0f, 0.0f, 0.5f));

        b2Vec2 bPos(10.0f, 5.0f);
        s = b2Vec2(0.2f, -yPos + portalSize + bPos.y);
        b2Body* statBody = createBox(bPos, s, pWorld, b2_staticBody);

        b2Vec2 startPos = b2Vec2(29.0f, 20.0f);
        b2Body* lastBody = createBox(startPos, b2Vec2(0.5f, 0.25f), pWorld, b2_dynamicBody);

        pb = pWorld->createPortalBody(lastBody);
        snakeHead = pb->worldIndex;
        renderer->addPortalBody(pb, b2Color(1, 1, 1, 0.5f), "assets/textures/head.png", GL_NEAREST);
        for (int i = 0; i < 20; i++){
            startPos -= b2Vec2(0.7f, 0.0f);
            
            b2Body* jointBody = createBox(startPos, b2Vec2(0.5f, 0.2f), pWorld, b2_dynamicBody);
            jointBody->GetFixtureList()->SetFriction(0.0f);
            pb = pWorld->createPortalBody(jointBody);
            renderer->addPortalBody(pb, b2Color(getRand() + 0.5f, getRand() + 0.5f, 1.0f, 0.5f), "assets/textures/body.png", GL_NEAREST);

            b2RevoluteJointDef revDef;
            revDef.Initialize(jointBody, lastBody, startPos + b2Vec2(0.35f, 0.0f));
            revDef.calculateAnchorHelpers();
            revDef.enableLimit = true;
            revDef.upperAngle = +b2_pi * 0.1f;
            revDef.lowerAngle = -b2_pi * 0.1f;
            pWorld->CreateJoint(&revDef);

            lastBody = jointBody;
        }
        snakeTail = pb->worldIndex;
    }

    void onUpdate()
    {
        BaseTestCase::onUpdate();
        if (platform)
        {
            if (platform->GetPosition().x < -10.0f)
            {
                platform->SetLinearVelocity(b2Vec2(3.0f, 0.0f));
            }
            else if (platform->GetPosition().x > 5.0f)
            {
                platform->SetLinearVelocity(b2Vec2(-3.0f, 0.0f));
            }
        }

        if (!snakeJoint && wh->keyData[GLFW_KEY_SPACE] == 2)
        {
            b2RevoluteJointDef revDef;
            revDef.bodyA = snakeHead->at(0)->body;
            revDef.bodyB = snakeTail->at(0)->body;
            revDef.localAnchorA = b2Vec2(0.35f, 0.0f);
            revDef.localAnchorB = b2Vec2(-0.35f, 0.0f);
            revDef.calculateAnchorHelpers();
            revDef.enableLimit = true;
            revDef.upperAngle = +b2_pi * 0.1f;
            revDef.lowerAngle = -b2_pi * 0.1f;
            snakeJoint = (b2RevoluteJoint*)pWorld->CreateJoint(&revDef);
        }
    }
};
