#include <BaseTestCase.h>

class TestCase8 : public BaseTestCase
{
public:
    Portal* portal1;
    Portal* portal2;

    std::vector<PortalBody*>* pbi1;
    std::vector<PortalBody*>* pbi2;

    TestCase8(WindowPainter* wh) : BaseTestCase(wh)
    {
        testCaseItem = 7;

        beforeCreate();
        onCreate();
        afterCreate();
    }

    void onCreate()
    {
        pWorld->SetGravity(b2Vec2(0.0f, 0.0f));
        //pWorld->SetGravity(b2Vec2(0.0f, -15.0f));
        pWorld->SetAllowSleeping(false);
        createEdge(b2Vec2(-10000.0f, -10.0f), b2Vec2(+10000.0f, -10.0f), pWorld, b2_staticBody);
        /*createEdge(b2Vec2(-10.0f, -10.0f), b2Vec2(-10.0f, 10.0f), pWorld, b2_staticBody);
        createEdge(b2Vec2(+10.0f, -10.0f), b2Vec2(+10.0f, 10.0f), pWorld, b2_staticBody);
        createEdge(b2Vec2(-10.0f, +10.0f), b2Vec2(+10.0f, 10.0f), pWorld, b2_staticBody);*/

        portal1 = pWorld->createPortal(b2Vec2(-2.0f, +3.0f), b2Vec2(1.0f, 0.0f), 2.0f);
        portal2 = pWorld->createPortal(b2Vec2(-2.0f, -3.0f), portal1->dir, portal1->size);

        b2FixtureDef fDef;
        fDef.density = 100.0f;
        fDef.restitution = 0.2f;
        fDef.friction = 0.5f;
        b2CircleShape cShape;
        cShape.m_radius = 0.1f;
        cShape.m_p = b2Vec2(0.0f, 2.1f);
        fDef.shape = &cShape;
        portal1->body->CreateFixture(&fDef);
        cShape.m_p = b2Vec2(0.0f, -2.1f);
        portal1->body->CreateFixture(&fDef);

        portal1->connect(portal2, true, 0, 1);
        portal1->connect(portal2, true, 1, 0);

        portal1->setLinVel(b2Vec2(0, 0));

        float randVal = getRand();
        b2Vec2 p = b2Vec2(1.0f, -5.0f + randVal * 0.5f);
        b2Vec2 s = b2Vec2(0.4f, 0.4f);
        //b2Body* body1 = createPortalCube(p, s, pWorld, b2_dynamicBody);
        b2Body* body1 = createBox(p, s, pWorld, b2_dynamicBody);

        p.y = +3.0f;
        b2Body* body2 = createPortalCube(p, s, pWorld, b2_dynamicBody);

        body1->SetLinearDamping(0.0f);
        body2->SetLinearDamping(0.0f);

        PortalBody* pb1 = pWorld->createPortalBody(body1);
        PortalBody* pb2 = pWorld->createPortalBody(body2);

        pbi1 = pb1->worldIndex;
        pbi2 = pb2->worldIndex;

        renderer->addPortalBody(pb1, b2Color(1.0f, 0.6f, 0.5f, 0.5f));
        renderer->addPortalBody(pb2, b2Color(0.5f, 0.6f, 1.0f, 0.5f), "assets/textures/cube.png");

        b2Vec2 startPos = b2Vec2(35.0f, 4.0f);
        b2Body* lastBody = createBox(startPos, b2Vec2(0.5f, 0.25f), pWorld, b2_dynamicBody);
        lastBody->SetLinearDamping(0.0f);

        PortalBody* pb = pWorld->createPortalBody(lastBody);
        renderer->addPortalBody(pb, b2Color(1, 1, 1, 0.5f), "assets/textures/head.png", GL_NEAREST);
        for (int i = 0; i < 50; i++)
        {
            startPos -= b2Vec2(0.7f, 0.0f);

            b2Body* jointBody = createBox(startPos, b2Vec2(0.5f, 0.2f), pWorld, b2_dynamicBody);
            jointBody->SetLinearDamping(0.0f);
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

    }

    void onUpdate()
    {
        BaseTestCase::onUpdate();

        const float speedAddition = 1.5f;
        const float angSpeedAddition = 0.5f;

        b2Vec2 setSpeed = b2Vec2(0.0f, 0.0f);
        float setAngle = 0.0f;

        if (wh->keyData[GLFW_KEY_LEFT])
        {
            setAngle += angSpeedAddition;
            //setSpeed += speedAddition * portal2->rotDir;
        }
        if (wh->keyData[GLFW_KEY_RIGHT])
        {
            setAngle -= angSpeedAddition;
            //setSpeed -= speedAddition * portal2->rotDir;
        }
        if (wh->keyData[GLFW_KEY_DOWN])
        {
            setSpeed -= speedAddition * portal2->dir;
        }
        if (wh->keyData[GLFW_KEY_UP])
        {
            setSpeed += speedAddition * portal2->dir;
        }

        portal2->setLinVel(setSpeed);
        portal2->setAngVel(setAngle);
    }
};
