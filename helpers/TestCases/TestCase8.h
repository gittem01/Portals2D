#include <BaseTestCase.h>
#include <GameBase.h>
#include <DefaultObjects/DefaultCube.h>

class TestCase8 : public BaseTestCase
{
public:
    Portal* portal1;
    Portal* portal2;
    b2Body* bodyToMove;
    b2Body* rbdy;
    std::vector<PortalBody*>* platform;
    std::vector<glm::vec4> outerEdges;
    std::vector<point> outerVertices;
    std::vector <float> angles;

    float t = 0.0f;

    TestCase8(ThinDrawer* td) : BaseTestCase(td)
    {
        testCaseItem = 7;

        beforeCreate();
        onCreate();
        afterCreate();
    }

    b2Body* randomBody()
    {
        b2BodyDef bDef;
        bDef.type = b2_dynamicBody;
        bDef.position = b2Vec2(20.0, 0.0);
        b2Body* body = pWorld->CreateBody(&bDef);

        const int polyCount = 20;
        const int circleCount = 20;

        for (int i = 0; i < polyCount; i++)
        {
            b2Vec2 randPos = b2Vec2(getRand() * 5.0, getRand() * 5.0);
            b2PolygonShape pShape;

            float startAngle = getRand() * b2_pi;
            float r0 = b2_pi * 0.7;
            float r1 = b2_pi * 0.7;

            b2Vec2 points[3] = {
                randPos + 0.3f * b2Vec2(sin(startAngle), cos(startAngle)),
                randPos + 0.3f * b2Vec2(sin(startAngle + r0), cos(startAngle + r0)),
                randPos + 0.3f * b2Vec2(sin(startAngle + r0 + r1), cos(startAngle + r0 + r1)) };

            pShape.Set(points, 3);

            body->CreateFixture(&pShape, 1.0);
        }

        for (int i = 0; i < circleCount; i++)
        {
            b2CircleShape cShape;
            cShape.m_p = b2Vec2(getRand() * 5.0, getRand() * 5.0);
            cShape.m_radius = getRand() * 0.3 + 0.2;

            body->CreateFixture(&cShape, 1.0);
        }

        return body;
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
        srand(glfwGetTime() * 1000);
        pWorld->SetGravity(b2Vec2(0.0f, 0.0f));
        pWorld->SetGravity(b2Vec2(0.0f, -15.0f));

        b2Vec2 startPos = b2Vec2(-10.0f, -10.0f);

        int n = 0;
        b2Body* body;
        b2Body* lastBody = nullptr;
        g_EdgeData* lastData = nullptr;
        for (int i = 0; i < n; i++)
        {
            float inc = 0.1f + getRand() * 0.2f + 0.08f;
            body = createEdge(startPos + b2Vec2(inc, 0.0f), startPos, pWorld, b2_staticBody, true);
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

            edgeData->edges[1] = lastData;
            edgeData->edges[0] = nullptr;
            if (lastData)
            {
                lastData->edges[0] = edgeData;
            }

            lastData = edgeData;
        }

        createEdge(b2Vec2(-10.0f, -10.0f), b2Vec2(-10.0f, 10.0f), pWorld, b2_staticBody, true);
        createEdge(b2Vec2(+40.0f, +10.0f), b2Vec2(+40.0f, -10.0f), pWorld, b2_staticBody, true);
        createEdge(b2Vec2(+40.0f, -10.0f), b2Vec2(-10.0f, -10.0f), pWorld, b2_staticBody, true);
        createEdge(b2Vec2(-10.0f, +10.0f), b2Vec2(+40.0f, 10.0f), pWorld, b2_staticBody, true);

        body = createEdge(b2Vec2(0.0f, +3.0f), b2Vec2(0.0f, -3.0f), pWorld, b2_kinematicBody, true);
        body->SetTransform(b2Vec2(6.0f, 0.0f), -b2_pi * 0.0f);

        createEdge(b2Vec2(12.0f, +3.0f), b2Vec2(6.0f, +3.0f), pWorld, b2_kinematicBody, true);

        bodyToMove = body;
        /*body->SetLinearVelocity(b2Vec2(3.0f, 0.0f));
        body->SetAngularVelocity(0.1f);*/
        fixtureData* fixData;
        g_EdgeData* edgeData;
        createEdgeData(&fixData, &edgeData);
        edgeData->gel = NONE;
        edgeData->wall = PORTALLABLE;
        edgeData->fix = body->GetFixtureList();
        edgeData->fix->GetUserData().pointer = (uintptr_t)fixData;

        portal1 = pWorld->createPortal(b2Vec2(-2.0f, +3.0f), b2Vec2(1.0f, 0.0f), 2.0f);
        portal2 = pWorld->createPortal(b2Vec2(-2.0f, -3.0f), portal1->dir, portal1->size);

        portal1->connect(portal2, true, 0, 0);

        /*
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

        std::vector<std::pair<std::string, std::string>> names =
        {
            {"pCube.edge2", "edge2.png"},
            {"pCube.edge3", "edge3.png"},
            {"pCube.edge4", "edge4.png"},
            {"pCube.edge1", "edge1.png"},
            {"pCube.body",  "body.png" }
        };

        startPos = b2Vec2(10.0f, 4.0f);
        lastBody = createCircle(startPos, 0.5f, pWorld, b2_dynamicBody);
        PortalBody* lastPb = pWorld->createPortalBody(lastBody);
        thinDrawer->debug_addPortalBody(lastPb);

        rbdy = tp->pBody->at(0)->body;
        angles = tp->pBody->at(0)->worldIndex->angles;
        outerEdges = tp->pBody->at(0)->worldIndex->outerEdges;
        outerVertices = tp->pBody->at(0)->worldIndex->outerVertices;

        thinDrawer->addPortalBody(lastPb, {"circle.png"});

        lastBody->SetLinearDamping(0.0f);

        startPos = b2Vec2(0.0f, -9.0f);
        lastBody = createBox(startPos, b2Vec2(6.7f, 0.2f), pWorld, b2_dynamicBody, 0.0f);
        lastPb = pWorld->createPortalBody(lastBody);
        platform = &lastPb->worldIndex->pbIndices;
        lastBody->SetLinearVelocity(b2Vec2(2.0, 0.0));
        lastBody->SetLinearDamping(0.0);
        thinDrawer->debug_addPortalBody(lastPb);
        thinDrawer->addPortalBody(lastPb, {"grad2.png"});
        names = {};
        thinDrawer->addPortalBody_3d(lastPb, names);

        /*b2Body* bdy = randomBody();
        lastPb = pWorld->createPortalBody(bdy);
        thinDrawer->debug_addPortalBody(lastPb);*/

        b2PolygonShape pShape;
        b2Vec2 points[] = {
            b2Vec2(+0.1f, 8.0f),
            b2Vec2(-0.1f, 8.0f),
            b2Vec2(+0.0f, 12.0f),
        };
        pShape.Set(points, std::size(points));
        b2BodyDef bDefX;
        bDefX.position = b2Vec2(10.0f, 0.0f);
        bDefX.type = b2_dynamicBody;
        b2Body* bodyX = pWorld->CreateBody(&bDefX);
        bodyX->CreateFixture(&pShape, 1.0f);

        PortalBody* pbx = pWorld->createPortalBody(bodyX);

        thinDrawer->debug_addPortalBody(pbx);
        thinDrawer->addPortalBody(pbx, {});

        DefaultCube* dc1 = new DefaultCube(pWorld);
        DefaultCube* dc2 = new DefaultCube(pWorld);
        dc2->pBody->at(0)->body->SetTransform(b2Vec2(5.0, 0.0), 0.0);
    }

    void onUpdate()
    {
        BaseTestCase::onUpdate();

        /*for (auto ps : outerEdges)
        {
            b2Vec2 b2P1 = rbdy->GetWorldPoint(b2Vec2(ps.x, ps.y));
            b2Vec2 b2P2 = rbdy->GetWorldPoint(b2Vec2(ps.z, ps.w));
            thinDrawer->registerDebugLine(glm::vec4(b2P1.x, b2P1.y, b2P2.x, b2P2.y), glm::vec4(1.0, 0.0, 0.0, 1.0), 0.03, 0.5);
            thinDrawer->registerDebugLine(glm::vec4(b2P1.x, b2P1.y, b2P1.x, b2P1.y), glm::vec4(1.0, 0.0, 0.0, 1.0), 0.15, 0.5);
        }

        glm::vec2 mc = thinDrawer->wh->cam->getMouseCoords();
        b2Vec2 diff = b2Vec2(mc.x, mc.y) - rbdy->GetPosition();
        diff = PortalWorld::rotateVec(diff, -rbdy->GetAngle());
        diff.Normalize();
        float diffAng = PortalWorld::calcAngle2(diff);

        int s = 0;
        int e = angles.size() - 1;
        for ( ; e - s > 1; )
        {
            int p = (e - s) / 2 + s;
            if (angles[p] < diffAng)
            {
                s = p;
            }
            else
            {
                e = p;
            }
        }

        if (angles[0] > diffAng) e = 0;
        if (angles[angles.size() - 1] < diffAng) e = 0;

        b2Vec2 rotDiff = PortalWorld::rotateVec(diff, b2_pi * 0.5);
        float vDist = PortalWorld::getRealDist(b2Vec2(0.0, 0.0), rotDiff, b2Vec2(outerVertices[e].pos.x, outerVertices[e].pos.y)) + outerVertices[e].r;
        diff = PortalWorld::rotateVec(diff, rbdy->GetAngle());
        rotDiff = PortalWorld::rotateVec(diff, b2_pi * 0.5);
        b2Vec2 farPos = rbdy->GetPosition() + vDist * diff;
        b2Vec2 fp1 = farPos - 100.0 * rotDiff;
        b2Vec2 fp2 = farPos + 100.0 * rotDiff;
        thinDrawer->registerDebugLine(glm::vec4(fp1.x, fp1.y, fp2.x, fp2.y), glm::vec4(0.0, 0.0, 1.0, 1.0), 0.06, 0.5);*/

        const float speedAddition = 1.5f;
        const float angSpeedAddition = 0.51f;

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

        bodyToMove->SetLinearVelocity(setSpeed);
        bodyToMove->SetAngularVelocity(setAngle);

        if (td->wh->keyData[GLFW_KEY_Z] == 2)
        {
            platform->at(0)->body->SetLinearVelocity(b2Vec2(-2.0, 0.0));
        }
        if (td->wh->keyData[GLFW_KEY_X] == 2)
        {
            platform->at(0)->body->SetLinearVelocity(b2Vec2(+2.0, 0.0));
        }
    }
};
