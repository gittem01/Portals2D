#include <BaseTestCase.h>
#include <GameBase.h>

class TestCase8 : public BaseTestCase
{
public:
    Portal* portal1;
    Portal* portal2;

    float t = 0.0f;

    std::vector<PortalBody*>* pbi1;
    std::vector<PortalBody*>* pbi2;

    TestCase8(ThinDrawer* td) : BaseTestCase(td)
    {
        testCaseItem = 7;

        beforeCreate();
        onCreate();
        afterCreate();
    }

    void makePortalDynamic(Portal* portal)
    {
        b2FixtureDef fDef;
        fDef.density = 100.0f;
        fDef.restitution = 0.2f;
        fDef.friction = 0.5f;
        b2CircleShape cShape;
        cShape.m_radius = 0.1f;
        cShape.m_p = b2Vec2(0.0f, 2.1f);
        fDef.shape = &cShape;
        portal->body->CreateFixture(&fDef);
        cShape.m_p = b2Vec2(0.0f, -2.1f);
        portal->body->CreateFixture(&fDef);
    }

    void onCreate()
    {
        srand(2);
        //pWorld->SetGravity(b2Vec2(0.0f, 0.0f));
        //pWorld->SetGravity(b2Vec2(0.0f, -15.0f));

        b2Vec2 startPos = b2Vec2(-10.0f, -10.0f);
        
        int n = 550;
        b2Body* body;
        b2Body* lastBody = nullptr;
        g_EdgeData* lastData = nullptr;
        for (int i = 0; i < n; i++)
        {
            float inc = 0.02f + getRand() * 0.2f + 0.08f;
            body = createEdge(startPos, startPos + b2Vec2(inc, 0.0f), pWorld);
            body->GetFixtureList()->SetFriction(0.0f);
            startPos.x += inc;
            fixtureData* fixData;
            g_EdgeData* edgeData;
            createEdgeData(&fixData, &edgeData);
            edgeData->gel = (GEL_TYPE)(NONE << (int)(getRand() * 4.0f + 2.0f));
            //edgeData->gel = WHITE;
            edgeData->wall = getRand() > -0.45f ? PORTALLABLE : NOT_PORTALLABLE;
            //edgeData->wall = PORTALLABLE;
            edgeData->fix = body->GetFixtureList();

            edgeData->fix->GetUserData().pointer = (uintptr_t)fixData;

            edgeData->edges[0] = lastData;
            edgeData->edges[1] = nullptr;
            if (lastData)
            {
                lastData->edges[1] = edgeData;
            }

            lastData = edgeData;
        }

        createEdge(b2Vec2(-10.0f, -10.0f), b2Vec2(-10.0f, 10.0f), pWorld);
        createEdge(b2Vec2(+40.0f, -10.0f), b2Vec2(+40.0f, 10.0f), pWorld);
        createEdge(b2Vec2(-10.0f, +10.0f), b2Vec2(+40.0f, 10.0f), pWorld);

        body = createEdge(b2Vec2(-8.0f, -3.0f), b2Vec2(-8.0f, -8.0f), pWorld, b2_kinematicBody);
        body->SetLinearVelocity(b2Vec2(3.0f, 0.0f));
        fixtureData* fixData;
        g_EdgeData* edgeData;
        createEdgeData(&fixData, &edgeData);
        edgeData->gel = NONE;
        edgeData->wall = PORTALLABLE;
        edgeData->fix = body->GetFixtureList();
        edgeData->fix->GetUserData().pointer = (uintptr_t)fixData;

        /*portal1 = pWorld->createPortal(b2Vec2(-2.0f, +3.0f), b2Vec2(1.0f, 0.0f), 2.0f);
        portal2 = pWorld->createPortal(b2Vec2(-2.0f, -3.0f), portal1->dir, portal1->size);

        Portal* portal3 = pWorld->createPortal(b2Vec2(-0.0f, +3.0f), b2Vec2(1.0f, 0.0f), 2.0f);
        Portal* portal4 = pWorld->createPortal(b2Vec2(-0.0f, -3.0f), portal1->dir, portal1->size);

        Portal* portal5 = pWorld->createPortal(b2Vec2(+2.0f, +3.0f), b2Vec2(1.0f, 0.0f), 2.0f);
        Portal* portal6 = pWorld->createPortal(b2Vec2(+2.0f, -3.0f), portal1->dir, portal1->size);

        Portal* portal7 = pWorld->createPortal(b2Vec2(+4.0f, +3.0f), b2Vec2(1.0f, 0.0f), 2.0f);
        Portal* portal8 = pWorld->createPortal(b2Vec2(+4.0f, -3.0f), portal1->dir, portal1->size);

        portal1->connect(portal2, true, 0, 0);
        portal1->connect(portal2, false, 1, 1);

        portal3->connect(portal4, true, 0, 0);
        portal3->connect(portal4, false, 1, 1);

        portal5->connect(portal6, true, 0, 0);
        portal5->connect(portal6, false, 1, 1);

        portal7->connect(portal8, true, 0, 0);
        portal7->connect(portal8, false, 1, 1);
        
        makePortalDynamic(portal1);
        makePortalDynamic(portal2);
        makePortalDynamic(portal3);
        makePortalDynamic(portal4);
        makePortalDynamic(portal5);
        makePortalDynamic(portal6);
        makePortalDynamic(portal7);
        makePortalDynamic(portal8);*/

        b2Vec2 p = b2Vec2(1.0f, -3.0f);
        b2Vec2 s = b2Vec2(0.4f, 0.4f);
        b2Body* body1 = createPortalCube(p, s, pWorld, b2_dynamicBody);
        //b2Body* body1 = createBox(p, s, pWorld, b2_dynamicBody);

        p.y = +3.0f;
        b2Body* body2 = createPortalCube(p, s, pWorld, b2_dynamicBody);

        body1->SetLinearDamping(0.0f);
        body2->SetLinearDamping(0.0f);

        PortalBody* pb1 = pWorld->createPortalBody(body1);
        PortalBody* pb2 = pWorld->createPortalBody(body2);

        thinDrawer->addPortalBody(pb1);
        thinDrawer->addPortalBody(pb2);

        pbi1 = &pb1->worldIndex->pbIndices;
        pbi2 = &pb2->worldIndex->pbIndices;

        startPos = b2Vec2(35.0f, 4.0f);
        lastBody = createBox(startPos, b2Vec2(0.5f, 0.25f), pWorld, b2_dynamicBody);
        PortalBody* lastPb = pWorld->createPortalBody(lastBody);
        thinDrawer->addPortalBody(lastPb);
        lastBody->SetLinearDamping(0.0f);
    }

    void onUpdate()
    {
        BaseTestCase::onUpdate();

        const float speedAddition = 1.5f;
        const float angSpeedAddition = 0.5f;

        b2Vec2 setSpeed = b2Vec2(0.0f, 0.0f);
        float setAngle = 0.0f;

        if (td->wh->keyData[GLFW_KEY_LEFT])
        {
            setAngle += angSpeedAddition;
            //setSpeed += speedAddition * portal2->rotDir;
        }
        if (td->wh->keyData[GLFW_KEY_RIGHT])
        {
            setAngle -= angSpeedAddition;
            //setSpeed -= speedAddition * portal2->rotDir;
        }
        if (td->wh->keyData[GLFW_KEY_DOWN])
        {
            setSpeed -= speedAddition * portal2->dir;
        }
        if (td->wh->keyData[GLFW_KEY_UP])
        {
            setSpeed += speedAddition * portal2->dir;
        }

        //portal2->setLinVel(setSpeed);
        //portal2->setAngVel(setAngle);
    }
};
