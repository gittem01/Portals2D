#include <helpers.h>
#include <BaseTestCase.h>

class TestCase1 : public BaseTestCase
{
private:
    b2Body* platform = nullptr;
    std::vector<PortalBody*>* snakeHead = nullptr;
    std::vector<PortalBody*>* snakeTail = nullptr;
    b2RevoluteJoint* snakeJoint = nullptr;

public:
    TestCase1(Game* game) : BaseTestCase(game){
        testCaseItem = 0;

        beforeCreate();
        onCreate();
        afterCreate();
    }

    void onCreate(){
        float yPos = -4.0f;
        float portalSize = 3.0f;

        Portal* portal1 = g->pWorld->createPortal(b2Vec2(-12.0f - portalSize, yPos), b2Vec2(+1.5f, -0.0f), portalSize);
        Portal* portal2 = g->pWorld->createPortal(b2Vec2(+6.0f, yPos - portalSize + 4.0f), b2Vec2(0.0f, -1.0f), portalSize);
        Portal* portal3 = g->pWorld->createPortal(b2Vec2(+6.0f, yPos - portalSize + 0.0f), b2Vec2(0.0f, +1.0f), portalSize);
        Portal* portal41 = g->pWorld->createPortal(b2Vec2(+10.0f + 0.2f, yPos), b2Vec2(+1.0f, 0.0f), portalSize);
        Portal* portal42 = g->pWorld->createPortal(b2Vec2(+10.0f + 0.2f, yPos + portalSize * 2), b2Vec2(+1.0f, 0.0f), portalSize);

        Portal* diagonalPortal1 = g->pWorld->createPortal(b2Vec2(-10.0f + 0.2f, yPos + portalSize), b2Vec2(+1.0f, 0.0f), portalSize);
        Portal* diagonalPortal2 = g->pWorld->createPortal(b2Vec2(-10.0f + 0.2f, yPos + portalSize * 2 + 5.0f), b2Vec2(+1.0f, -1.0f), portalSize);

        Portal* topPortal1 = g->pWorld->createPortal(b2Vec2(+5.0f, yPos + portalSize + 5.0f), b2Vec2(0.0f, -1.0f), portalSize);
        Portal* topPortal2 = g->pWorld->createPortal(b2Vec2(-5.0f, yPos + portalSize + 5.0f), b2Vec2(0.0f, -1.0f), portalSize);

        Portal* portal5 = g->pWorld->createPortal(b2Vec2(-12.0f, yPos - portalSize + 0.0f), b2Vec2(0.0f, +1.0f), portalSize);
        Portal* portal6 = g->pWorld->createPortal(b2Vec2(-20.0f, yPos - portalSize + 0.0f), b2Vec2(0.0f, +1.0f), portalSize);

//        Portal* destroyer = g->pWorld->createPortal(b2Vec2(-30.0f, 7.0f), b2Vec2(1.0f, +0.0f), 14.0f);
//        destroyer->setVoid(0);
//        destroyer->setVoid(1);

        portal3->connect(portal2, false);
        portal41->connect(portal1, true, 0, 0);
        portal42->connect(portal1, false, 0, 1);
        portal5->connect(portal6, true, 0, 0);
        diagonalPortal1->connect(diagonalPortal2, true, 0, 0);

        diagonalPortal2->connect(topPortal1, true, 1, 0);
        diagonalPortal2->connect(topPortal2, false, 1, 0);
        topPortal1->connect(topPortal2);

        createEdge(b2Vec2(+30.0f, yPos - portalSize), b2Vec2(+30.0f, 21), g->pWorld, b2_staticBody);
        createEdge(b2Vec2(-30.0f, 21), b2Vec2(-30.0f, yPos - portalSize), g->pWorld, b2_staticBody);
        createEdge(b2Vec2(+30.0f, 21), b2Vec2(-30.0f, 21), g->pWorld, b2_staticBody);

        createEdge(b2Vec2(-30.0f, yPos - portalSize), b2Vec2(portal6->points[0].x, yPos - portalSize), g->pWorld, b2_staticBody);
        createEdge(b2Vec2(portal6->points[1].x, yPos - portalSize), b2Vec2(portal5->points[0].x, yPos - portalSize), g->pWorld, b2_staticBody);
        createEdge(b2Vec2(portal5->points[1].x, yPos - portalSize), b2Vec2(portal3->points[0].x, yPos - portalSize), g->pWorld, b2_staticBody);
        createEdge(b2Vec2(portal3->points[1].x, yPos - portalSize), b2Vec2(portal42->points[1].x, yPos - portalSize), g->pWorld, b2_staticBody);
        createEdge(b2Vec2(portal42->points[1].x, yPos - portalSize), b2Vec2(+30.0f, yPos - portalSize), g->pWorld, b2_staticBody);

        createEdge(b2Vec2(10.2f, 5.0f), b2Vec2(10.2f, 16.0f), g->pWorld, b2_staticBody);

        td->pWorld = g->pWorld;

        PortalBody* b1 = g->pWorld->createPortalBody(createObody(g->pWorld, b2Vec2(0.0f, 3.0f)));
        PortalBody* b2 = g->pWorld->createPortalBody(createWbody(g->pWorld, b2Vec2(0.0f, 6.0f)));

        td->debug_addPortalBody(b1);
        td->debug_addPortalBody(b2);

        b2Vec2 p(5.0f, 0.0f);
        b2Vec2 s(0.75f, 0.75f);
        b2Body* body2 = createPortalCube(p, s, g->pWorld, b2_dynamicBody);
        b2Body* body20 = createPortalCube(p + b2Vec2(1, 0), s, g->pWorld, b2_dynamicBody);
        //b2Body* body21 = createPortalCube(p + b2Vec2(2, 0), s, g->pWorld, b2_dynamicBody);
        b2Body* body21 = createBox(p + b2Vec2(2, 0), s, g->pWorld, b2_dynamicBody);

        PortalBody* pb1 = g->pWorld->createPortalBody(body2);
        PortalBody* pb2 = g->pWorld->createPortalBody(body20);
        PortalBody* pb3 = g->pWorld->createPortalBody(body21);

        std::vector<std::string> cubeNames;
        for (int i = 0; i < 6; i++)
        {
            cubeNames.push_back("cube.png");
        }
        td->addPortalBody(pb1, cubeNames);

        td->debug_addPortalBody(pb1);
        td->debug_addPortalBody(pb2);
        td->debug_addPortalBody(pb3);
        
        PortalBody* pb;
    #if 1
        // artificial kinematic body
        p = b2Vec2(6.0f, -6.0f);
        s = b2Vec2(1.5f, 0.2f);
        platform = createBox(p, s, g->pWorld, b2_dynamicBody, 0.0f);
        pb = g->pWorld->createPortalBody(platform);
        td->debug_addPortalBody(pb);
        b2Vec2 vel(-10.0f, 0.0f);
        platform->SetLinearVelocity(vel);
        platform->SetBullet(true);
        platform->SetLinearDamping(0.0f);
    #endif

        p = b2Vec2(0.0f, 0.0f);
        float r = 0.7f;
        b2Body* body3 = createCircle(p, r, g->pWorld, b2_dynamicBody);
        pb = g->pWorld->createPortalBody(body3);
        td->debug_addPortalBody(pb);

        b2Vec2 startPos = b2Vec2(29.0f, 20.0f);
        b2Body* lastBody = createBox(startPos, b2Vec2(0.5f, 0.25f), g->pWorld, b2_dynamicBody);

        pb = g->pWorld->createPortalBody(lastBody);
        td->debug_addPortalBody(pb);
        snakeHead = &pb->worldIndex->pbIndices;
        for (int i = 0; i < 20; i++){
            startPos -= b2Vec2(0.7f, 0.0f);
            
            b2Body* jointBody = createBox(startPos, b2Vec2(0.5f, 0.2f), g->pWorld, b2_dynamicBody);
            jointBody->GetFixtureList()->SetFriction(0.0f);
            pb = g->pWorld->createPortalBody(jointBody);
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
        snakeTail = &pb->worldIndex->pbIndices;
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

        if (!snakeJoint && td->wh->keyData[GLFW_KEY_SPACE] == 2)
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
            snakeJoint = (b2RevoluteJoint*)g->pWorld->CreateJoint(&revDef);
        }
    }
};
