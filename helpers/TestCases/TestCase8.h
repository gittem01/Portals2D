#include <BaseTestCase.h>
#include <GameBase.h>
#include <DefaultObjects/DefaultCube.h>
#include <DefaultObjects/Cyber.h>
#include <Game.h>
#include <Portal.h>
#include <helpers.h>
#include <DefaultObjects/Turret.h>

class TestCase8 : public BaseTestCase
{
public:
    Portal* portal1;
    Portal* portal2;
    b2Body* bodyToMove;
    std::vector<PortalBody*>* platform;


    float t = 0.0f;

    TestCase8(Game* game) : BaseTestCase(game)
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
        b2Body* body = g->pWorld->CreateBody(&bDef);

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
        g->pWorld->SetGravity(b2Vec2(0.0f, 0.0f));
        g->pWorld->SetGravity(b2Vec2(0.0f, -12.5f));

        b2Vec2 startPos = b2Vec2(-10.0f, -10.0f);

        b2Body* body;
        b2Body* lastBody = nullptr;

        createEdge(b2Vec2(-10.0f, -10.0f), b2Vec2(-10.0f, 10.0f), g->pWorld);
        createEdge(b2Vec2(+40.0f, +10.0f), b2Vec2(+40.0f, -10.0f), g->pWorld);
        createEdge(b2Vec2(+40.0f, -10.0f), b2Vec2(-10.0f, -10.0f), g->pWorld);
        createEdge(b2Vec2(-10.0f, +10.0f), b2Vec2(+40.0f, 10.0f), g->pWorld);

        const float bSize = 0.1;
        for (float i = -30; i < 30; i+=3)
        {
            for (float j = -200; j < 200; j+=3)
            {
                //b2Body* b = createBox(b2Vec2(i, j), b2Vec2(0.5, 0.5), g->pWorld, b2_staticBody);
                /*createEdge(b2Vec2(j - bSize, i + bSize), b2Vec2(j + bSize, i + bSize), g->pWorld, b2_staticBody, false);
                createEdge(b2Vec2(j + bSize, i + bSize), b2Vec2(j + bSize, i - bSize), g->pWorld, b2_staticBody, false);
                createEdge(b2Vec2(j + bSize, i - bSize), b2Vec2(j - bSize, i - bSize), g->pWorld, b2_staticBody, false);
                createEdge(b2Vec2(j - bSize, i - bSize), b2Vec2(j - bSize, i + bSize), g->pWorld, b2_staticBody, false);*/
            }
        }

        body = createEdge(b2Vec2(0.0f, +3.0f), b2Vec2(0.0f, -3.0f), g->pWorld);
        body->SetTransform(b2Vec2(6.0f, 0.0f), -b2_pi * 0.0f);
        body->SetType(b2_kinematicBody);

        createEdge(b2Vec2(12.0f, +3.0f), b2Vec2(6.0f, +3.0f), g->pWorld, b2_kinematicBody, true);

        b2EdgeShape eShape;
        eShape.SetOneSided(b2Vec2(-7.5f, -10.0f), b2Vec2(-8.0f, +5.0f), b2Vec2(-8.0f, -5.0f), b2Vec2(-7.5f, +10.0f));
        b2BodyDef bDef;
        bDef.type = b2_staticBody;
        b2Body* eb = g->pWorld->CreateBody(&bDef);
        eb->CreateFixture(&eShape, 0.0f);

        bodyToMove = body;
        /*body->SetLinearVelocity(b2Vec2(3.0f, 0.0f));
        body->SetAngularVelocity(0.1f);*/

        portal1 = g->pWorld->createPortal(b2Vec2(-2.0f, +3.0f), b2Vec2(1.0f, 0.0f), 2.0f);
        portal2 = g->pWorld->createPortal(b2Vec2(-2.0f, -3.0f), portal1->dir, portal1->size);

        portal1->connect(portal2, true, 0, 0);

        lastBody = createBox(b2Vec2(10.0f, 0.0f), b2Vec2(2.49, 2.49), g->pWorld, b2_dynamicBody, 10.0);
        lastBody->SetBullet(true);

        Portal* portalDown = g->pWorld->createPortal(b2Vec2(10.0f, -2.5f), b2Vec2(0.0f, -1.0f), 2.5f, lastBody);
        Portal* portalUp = g->pWorld->createPortal(b2Vec2(10.0f, +2.5f), b2Vec2(0.0f, +1.0f), 2.5f, lastBody);

        Portal* portalRight = g->pWorld->createPortal(b2Vec2(12.5f, 0.0f), b2Vec2(1.0f, 0.0f), 2.5f, lastBody);
        Portal* portalLeft = g->pWorld->createPortal(b2Vec2(7.5f, 0.0f), b2Vec2(-1.0f, 0.0f), 2.5f, lastBody);

        portalDown->connect(portalRight);
        portalUp->connect(portalLeft);

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
        lastBody = createCircle(startPos, 0.4f, g->pWorld, b2_dynamicBody);
        PortalBody* lastPb = g->pWorld->createPortalBody(lastBody);
        td->debug_addPortalBody(lastPb);
        td->addPortalBody(lastPb, {"sphereEnabled.png"});


        b2Vec2 circP = b2Vec2(20.0, 0.0);
        const int cnt = 20;
        const float circR = 0.2f;
        for (int i = 0; i < cnt; i++)
        {
            for (int j = 0; j < cnt; j++)
            {
                b2Body* bdy = createCircle(circP + circR * b2Vec2(i, j), circR, g->pWorld, b2_dynamicBody);
                PortalBody* circPb = g->pWorld->createPortalBody(bdy);
                td->addPortalBody(circPb, { "sphereEnabled.png" });
            }
        }

        lastBody->SetLinearDamping(0.0f);

        startPos = b2Vec2(0.0f, -9.0f);
        lastBody = createBox(startPos, b2Vec2(6.7f, 0.2f), g->pWorld, b2_dynamicBody, 0.0f);
        lastPb = g->pWorld->createPortalBody(lastBody);
        platform = &lastPb->worldIndex->pbIndices;
        lastBody->SetLinearVelocity(b2Vec2(2.0, 0.0));
        lastBody->SetLinearDamping(0.0);
        td->debug_addPortalBody(lastPb);
        td->addPortalBody(lastPb, {"grad2.png"});
        names = {};

        /*b2Body* bdy = randomBody();
        lastPb = g->pWorld->createPortalBody(bdy);
        td->debug_addPortalBody(lastPb);*/

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
        b2Body* bodyX = g->pWorld->CreateBody(&bDefX);
        bodyX->CreateFixture(&pShape, 1.0f);

        PortalBody* pbx = g->pWorld->createPortalBody(bodyX);

        td->debug_addPortalBody(pbx);
        td->addPortalBody(pbx, {});

        DefaultCube* dc1 = new DefaultCube(g->pWorld);
        DefaultCube* dc2 = new DefaultCube(g->pWorld);
        dc2->pBody->at(0)->body->SetTransform(b2Vec2(5.0, 0.0), 0.0);

        Cyber* cyb = new Cyber(g->pWorld, b2Vec2(0.0, 0.0));

        Turret* tr = new Turret(g->pWorld);
        tr->pBody->at(0)->body->SetTransform(b2Vec2(-3.0f, 3.0f), 0.0);
    }

    void onUpdate()
    {
        static int counter = 0;

        BaseTestCase::onUpdate();

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

        if (td->wh->keyData[GLFW_KEY_K] == 2)
        {
            glm::vec2 mp = td->wh->cam->getMouseCoords();
            b2Body* lastBody = createCircle(b2Vec2(mp.x, mp.y), 0.5f, g->pWorld, b2_dynamicBody, 1.0, true);
            PortalBody* pbx = g->pWorld->createPortalBody(lastBody);
            td->debug_addPortalBody(pbx);
            counter++;
        }

        //printf("%d\n", counter);

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
