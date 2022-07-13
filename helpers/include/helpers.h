#include "DebugDrawer.h"
#include "mouseJointHandler.h"
#include "ContactListener.h"
#include "TestPlayer.h"

#include <thread>
#include <chrono>
#include <random>

std::random_device rd;
std::mt19937 mt(rd());
std::uniform_real_distribution<double> randF(-0.5f, 0.5f);

bool isPaused = false;
bool tick = false;
int totalIter = 10;
PortalWorld* pWorld;

// O shaped body
b2Body* createObody(b2World* world, b2Vec2 bodyPos=b2Vec2(0, 0), float degree=270.0f, int n=20, float thickness=0.3f, float rOut=1.2f, float fd=0.0f) {
    b2BodyDef bodyDef;
    bodyDef.position = bodyPos;
    bodyDef.type = b2_dynamicBody;
    bodyDef.angularDamping = 0.1f;
    bodyDef.linearDamping = 0.1f;

    b2Body* body = world->CreateBody(&bodyDef);

    auto* vertices = (b2Vec2*)malloc(sizeof(b2Vec2) * 4);

    degree /= 180 / b2_pi;
    // keep it even for non-symmetrical shape
    float incr = degree / n;

    int dec = 0;
    float f = 1.0f;
    for (float currD = 0.0f; currD < degree;) {
        if (dec % 2 == 0){
            b2FixtureDef fDef;
            b2PolygonShape shape;

            float nextD = currD + incr;

            float x1 = sin(currD) * rOut * (f + fd);
            float y1 = cos(currD) * rOut * (f + fd);

            float x2 = sin(currD) * (rOut * (f + fd) - thickness);
            float y2 = cos(currD) * (rOut * (f + fd) - thickness);

            float x3 = sin(nextD) * rOut * f;
            float y3 = cos(nextD) * rOut * f;

            float x4 = sin(nextD) * (rOut * f - thickness);
            float y4 = cos(nextD) * (rOut * f - thickness);

            vertices[0].x = x1; vertices[0].y = y1;
            vertices[1].x = x2; vertices[1].y = y2;
            vertices[2].x = x3; vertices[2].y = y3;
            vertices[3].x = x4; vertices[3].y = y4;

            shape.Set(vertices, 4);
            fDef.shape = &shape;
            fDef.density = 0.2f;

            body->CreateFixture(&fDef);

            currD = nextD;
        }
        else{
            b2FixtureDef fDef;
            b2CircleShape cShape;

            float nextD = currD + incr;

            float x = sin(0.5f * (currD + nextD)) * (rOut - thickness * 0.5f);
            float y = cos(0.5f * (currD + nextD)) * (rOut - thickness * 0.5f);

            cShape.m_p = f * b2Vec2(x, y);
            cShape.m_radius = thickness * 0.65f;

            fDef.shape = &cShape;
            fDef.density = 0.2f;

            body->CreateFixture(&fDef);

            currD = nextD;
        }
        f -= fd;
    }

    free(vertices);

    return body;
}

// Weird body (???)
b2Body* createWbody(b2World* world, b2Vec2 bodyPos=b2Vec2(0, 0), float degree=270.0f, int n=20, float circleR=0.15f, float rOut=1.2f, float fd=0.0f) {
    b2BodyDef bodyDef;
    bodyDef.position = bodyPos;
    bodyDef.type = b2_dynamicBody;
    bodyDef.angularDamping = 0.1f;
    bodyDef.linearDamping = 0.1f;

    b2Body* body = world->CreateBody(&bodyDef);

    b2FixtureDef fDef;
    b2CircleShape shape;
    
    degree /= 180 / b2_pi;
    float incr = degree / n;
    float f = 1.0f;
    
    for (float currD = 0.0f; currD < degree;) {
        b2CircleShape cShape;

        float nextD = currD + incr;

        float x = sin(currD) * (rOut - circleR);
        float y = cos(currD) * (rOut - circleR);

        cShape.m_p = f * b2Vec2(x, y);
        cShape.m_radius = circleR;

        fDef.shape = &cShape;
        fDef.density = 0.2f;

        body->CreateFixture(&fDef);

        currD = nextD;
        f -= fd;
    }

    return body;
}

b2Body* createEdge(b2Vec2 p1, b2Vec2 p2, b2World* world, b2BodyType type) {
    b2BodyDef bd;
    bd.type = type;
    b2Body* edgeBody = world->CreateBody(&bd);

    b2EdgeShape shape;
    shape.SetTwoSided(p1, p2);

    b2FixtureDef fixDef;
    fixDef.shape = &shape;
    bodyData data = { OTHER, nullptr };
    fixDef.userData.pointer = (uintptr_t)&data;

    edgeBody->CreateFixture(&fixDef);

    return edgeBody;
}

b2Body* createPortalCube(b2Vec2 pos, b2Vec2 size, b2World* world, b2BodyType type, float density=1.0f){
    std::vector<b2Vec2> vertices = 
        {
            b2Vec2(-0.1f * size.x, +0.5f * size.y),
            b2Vec2(-0.2f * size.x, +0.6f * size.y),
            b2Vec2(-0.5f * size.x, +0.6f * size.y),
            b2Vec2(-0.6f * size.x, +0.5f * size.y),
            b2Vec2(-0.6f * size.x, +0.2f * size.y),
            b2Vec2(-0.5f * size.x, +0.1f * size.y)
        };
    b2PolygonShape boxShape;
    boxShape.SetAsBox(0.5f * size.x, 0.5f * size.y);

    b2FixtureDef fDef1;
    fDef1.shape = &boxShape;
    fDef1.density = density;
    fDef1.restitution = 0.2f;
    fDef1.friction = 0.2f;

    b2BodyDef def;
    def.type = type;
    def.position = pos;
    def.linearDamping = 0.1f;

    b2Body* body = world->CreateBody(&def);
    body->CreateFixture(&fDef1);

    bool mirrorStatus = false;
    for (int i = 0; i < 4; i++){
        b2PolygonShape cornerShape;
        cornerShape.Set(vertices.data(), (int)vertices.size());
        for (auto & vertex : vertices){
            if (!mirrorStatus){
                vertex.x *= -1;
            }
            else{
                vertex.y *= -1;
            }
        }
        mirrorStatus ^= 1;
        b2FixtureDef fDef2;
        fDef2.shape = &cornerShape;
        fDef2.density = density;
        fDef2.restitution = 0.2f;
        fDef2.friction = 0.2f;
        body->CreateFixture(&fDef2);
    }

    return body;
}

b2Body* createBox(b2Vec2 pos, b2Vec2 size, b2World* world, b2BodyType type, float density=1.0f){
    b2PolygonShape shape;
    shape.SetAsBox(size.x, size.y);

    b2FixtureDef fDef;
    fDef.shape = &shape;
    fDef.density = density;
    fDef.restitution = 0.2f;
    fDef.friction = 0.2f;

    b2BodyDef def;
    def.type = type;
    def.position = pos;
    def.linearDamping = 0.1f;

    b2Body* body = world->CreateBody(&def);
    body->CreateFixture(&fDef);

    return body;
}

b2Body* createCircle(b2Vec2 pos, float size, b2World* world, b2BodyType type, float density=1.0f){
    b2CircleShape shape;
    shape.m_radius = size;

    b2FixtureDef fDef;
    fDef.shape = &shape;
    fDef.density = density;
    fDef.restitution = 0.2f;
    fDef.friction = 0.2f;

    b2BodyDef def;
    def.type = type;
    def.position = pos;
    def.linearDamping = 0.0f;    

    b2Body* body = world->CreateBody(&def);
    body->CreateFixture(&fDef);

    return body;
}

void keyHandler(WindowPainter* wh) {
    tick = false;
    if (wh->keyData[GLFW_KEY_P] == 2) {
        isPaused = !isPaused;
    }
    else if (wh->keyData[GLFW_KEY_O] == 2) {
        isPaused = true;
        tick = true;
    }
    if (wh->keyData[GLFW_KEY_S] == 2)
        pWorld->drawReleases ^= 1;
}

float getRand(){
    randF(mt);
    return randF(mt);
}

void classicTest(PortalWorld* portalWorld){
    b2Vec2 gravity(0.0f, -20.00f);
    portalWorld->SetGravity(gravity);

    float xSize = 7.98f;
    float ySize = 4.48f;
    float width = 0.5f;
    float m = 1.0f;

    Portal* portal1 = portalWorld->createPortal(b2Vec2(0.0f, -ySize), b2Vec2(+0.0f, +1.0f), ySize * m);
    Portal* portal2 = portalWorld->createPortal(b2Vec2(0.0f, +ySize), b2Vec2(+0.0f, -1.0f), ySize * m);
    Portal* portal3 = portalWorld->createPortal(b2Vec2(-xSize, 0.0f), b2Vec2(+1.0f, +0.0f), ySize * m);
    Portal* portal4 = portalWorld->createPortal(b2Vec2(+xSize, 0.0f), b2Vec2(-1.0f, +0.0f), ySize * m);

    portal1->connect(portal2);
    portal3->connect(portal4);

    createEdge(b2Vec2(-xSize, -ySize), b2Vec2(+xSize, -ySize), portalWorld, b2_staticBody);
    createEdge(b2Vec2(-xSize, -ySize), b2Vec2(-xSize, +ySize), portalWorld, b2_staticBody);
    createEdge(b2Vec2(-xSize, +ySize), b2Vec2(+xSize, +ySize), portalWorld, b2_staticBody);
    createBox(b2Vec2(xSize + width, 0), b2Vec2(width, ySize), portalWorld, b2_staticBody);

    b2Vec2 p = b2Vec2(0.0f, 0.0f);
    b2Vec2 s = b2Vec2(3.0f, 0.2f);
    b2Body* body4 = createBox(p, s, portalWorld, b2_dynamicBody, 0.0f);
    (portalWorld->createPortalBody(body4))->bodyColor = b2Color(0.0f, 0.0f, 1.0f, 0.5f);
    b2Vec2 vel(0.0f, 5.0f);
    body4->SetLinearVelocity(vel);
    body4->SetBullet(true); // for mimicing static body continuous collision
    body4->SetLinearDamping(0.0f);

    float sizeM = 1.0f;
    float div = 2.25f;
    uint32_t numCircles = 50;
    uint32_t numPolygons = 50;
    for (int i = 0; i < numCircles; i++) {
        b2Body* b = createCircle(b2Vec2(getRand() * xSize * 1.9f, getRand() * ySize * 1.9f),
                                (getRand() + sizeM) / div, portalWorld, b2_dynamicBody);
        portalWorld->createPortalBody(b, b2Color(1.0f, 0.25f, 0.7f, 0.5f));
    }
    for (int i = 0; i < numPolygons; i++) {
        b2Body* b = createBox(b2Vec2(getRand() * xSize * 1.9f, getRand() * ySize * 1.9f),
                              b2Vec2(((getRand() + sizeM) / div), (getRand() + sizeM) / div), portalWorld, b2_dynamicBody);
        portalWorld->createPortalBody(b, b2Color(0, 1, 1, 0.5));
    }

    for (int i = 0; i < 5; i++) {
        b2Body* b1 = createCircle(b2Vec2(getRand() * xSize * 1.0f, 10.0f),
                                (getRand() + sizeM) / div, portalWorld, b2_dynamicBody);
        portalWorld->createPortalBody(b1, b2Color(1.0f, 0.25f, 0.7f, 0.5f));

        b2Body* b2 = createBox(b2Vec2(getRand() * xSize * 1.0f, 10.0f),
                               b2Vec2(((getRand() + sizeM) / div), (getRand() + sizeM) / div), portalWorld, b2_dynamicBody);
        portalWorld->createPortalBody(b2, b2Color(0, 1, 1, 0.5));
    }
}

void testRooms(PortalWorld* portalWorld){
    b2Vec2 gravity(0.0f, -40.00f);
    portalWorld->SetGravity(gravity);

    float roomWidth = 10.0f;
    float roomHeight = 20.0f;
    float bottomPos = -10.0f;
    int numOfCubes = 100;
    int numOfCircles = 100;

    // bottom portals
    Portal* portal1 = portalWorld->createPortal(b2Vec2(-roomWidth - 1.0f, bottomPos), b2Vec2(0.0f, +1.0f), roomWidth);
    Portal* portal2 = portalWorld->createPortal(b2Vec2(+roomWidth + 1.0f, bottomPos), b2Vec2(0.0f, +1.0f), roomWidth);

    // top portals
    Portal* portal3 = portalWorld->createPortal(b2Vec2(-roomWidth - 1.0f, bottomPos + roomHeight), b2Vec2(0.0f, -1.0f), roomWidth);
    Portal* portal4 = portalWorld->createPortal(b2Vec2(+roomWidth + 1.0f, bottomPos + roomHeight), b2Vec2(0.0f, -1.0f), roomWidth);

    portal1->connect(portal2);
    portal3->connect(portal4);

    createEdge(b2Vec2(-roomWidth * 2 - 1, bottomPos), b2Vec2(-roomWidth * 2 - 1, roomHeight + bottomPos), portalWorld, b2_staticBody);
    createEdge(b2Vec2(-1, bottomPos), b2Vec2(-1, roomHeight + bottomPos), portalWorld, b2_staticBody);

    createEdge(b2Vec2(+roomWidth * 2 + 1, bottomPos), b2Vec2(+roomWidth * 2 + 1, roomHeight + bottomPos), portalWorld, b2_staticBody);
    createEdge(b2Vec2(+1, bottomPos), b2Vec2(+1, roomHeight + bottomPos), portalWorld, b2_staticBody);

    for (int i = 0; i < numOfCubes; i++){
        float sizex = getRand() * 0.5f + 0.8f;
        float sizey = getRand() * 0.5f + 0.8f;
        b2Body* b = createBox(b2Vec2(-roomWidth - 1, 0.0f), b2Vec2(sizex, sizey), portalWorld, b2_dynamicBody);
        portalWorld->createPortalBody(b, b2Color(1.0f, 1.0f, 0.25f, 0.5f));
    }

    for (int i = 0; i < numOfCircles; i++){
        float size = getRand() * 0.5f + 0.8f;
        b2Body* b = createCircle(b2Vec2(-roomWidth - 1, 0.0f), size, portalWorld, b2_dynamicBody);
        portalWorld->createPortalBody(b, b2Color(0.25f, 1.0f, 0.7f, 0.5f));
    }

    b2Vec2 p = b2Vec2(roomWidth + 1.0f, +9.4f);
    b2Vec2 s = b2Vec2(roomWidth * 0.75f, 0.3f);
    b2Body* body4 = createBox(p, s, portalWorld, b2_dynamicBody, 0.0f);
    (portalWorld->createPortalBody(body4))->bodyColor = b2Color(1.0f, 1.0f, 1.0f, 1.0f);
    b2Vec2 vel(0.0f, 0.0f);
    body4->SetLinearVelocity(vel);
    body4->SetAngularVelocity(5.0f);
    body4->SetBullet(true);
    body4->SetLinearDamping(0.0f);
}

void testCase1(PortalWorld* portalWorld){
    b2Vec2 gravity(0.0f, -10.0f);
    portalWorld->SetGravity(gravity);

    float yPos = -4.0f;
    float portalSize = 3.0f;

    Portal* portal1 = portalWorld->createPortal(b2Vec2(-6.0f, yPos), b2Vec2(+1.0f, +0.0f), portalSize);
    Portal* portal2 = portalWorld->createPortal(b2Vec2(+6.0f, yPos - portalSize + 4.0f), b2Vec2(0.0f, -1.0f), portalSize);
    Portal* portal3 = portalWorld->createPortal(b2Vec2(+6.0f, yPos - portalSize + 0.0f), b2Vec2(0.0f, +1.0f), portalSize);
    Portal* portal41 = portalWorld->createPortal(b2Vec2(+10.0f + 0.2f, yPos), b2Vec2(+1.0f, 0.0f), portalSize);
    Portal* portal42 = portalWorld->createPortal(b2Vec2(+10.0f + 0.2f, yPos + portalSize * 2), b2Vec2(+1.0f, 0.0f), portalSize);
    
    Portal* diagonalPortal1 = portalWorld->createPortal(b2Vec2(-10.0f + 0.2f, yPos + portalSize), b2Vec2(+1.0f, 0.0f), portalSize);
    Portal* diagonalPortal2 = portalWorld->createPortal(b2Vec2(-10.0f + 0.2f, yPos + portalSize * 2 + 5.0f), b2Vec2(+1.0f, -1.0f), portalSize);

    Portal* topPortal1 = portalWorld->createPortal(b2Vec2(+5.0f, yPos + portalSize + 5.0f), b2Vec2(0.0f, -1.0f), portalSize);
    Portal* topPortal2 = portalWorld->createPortal(b2Vec2(-5.0f, yPos + portalSize + 5.0f), b2Vec2(0.0f, -1.0f), portalSize);

    Portal* portal5 = portalWorld->createPortal(b2Vec2(-12.0f, yPos - portalSize + 0.0f), b2Vec2(0.0f, +1.0f), portalSize);
    Portal* portal6 = portalWorld->createPortal(b2Vec2(-20.0f, yPos - portalSize + 0.0f), b2Vec2(0.0f, +1.0f), portalSize);

    Portal* destroyer = portalWorld->createPortal(b2Vec2(-30.0f, 7.0f), b2Vec2(1.0f, +0.0f), 14.0f);
    destroyer->setVoid(0);
    destroyer->setVoid(1);

    portal3->connect(portal2, false);
    portal3->connect(diagonalPortal2, false, 0, 1);
    portal41->connect(portal1, true, 0, 0);
    portal42->connect(portal1, false, 0, 1);
    portal5->connect(portal6, true, 0, 0);
    diagonalPortal1->connect(diagonalPortal2, true, 0, 0);
    diagonalPortal2->connect(topPortal1, true, 1, 0);
    diagonalPortal2->connect(topPortal2, false, 1, 0);

    createEdge(b2Vec2(-30.0f, yPos - portalSize), b2Vec2(+30.0f, yPos - portalSize), portalWorld, b2_staticBody);
    createEdge(b2Vec2(+30.0f, yPos - portalSize), b2Vec2(+30.0f, 21), portalWorld, b2_staticBody);
    createEdge(b2Vec2(+30.0f, 21), b2Vec2(-30.0f, 21), portalWorld, b2_staticBody);

    PortalBody* b1 = portalWorld->createPortalBody(createObody(portalWorld, b2Vec2(0.0f, 3.0f)));
    PortalBody* b2 = portalWorld->createPortalBody(createWbody(portalWorld, b2Vec2(0.0f, 6.0f)));

    b1->bodyColor = b2Color(0.0f, 1.0f, 1.0f, 0.5f);
    b2->bodyColor = b2Color(1.0f, 0.0f, 1.0f, 0.5f);

    b2Vec2 p(5.0f, 0.0f);
    b2Vec2 s(0.75f, 0.75f);
    b2Body* body2 = createPortalCube(p, s, portalWorld, b2_dynamicBody);
    b2Body* body20 = createPortalCube(p + b2Vec2(1, 0), s, portalWorld, b2_dynamicBody);
    b2Body* body21 = createPortalCube(p + b2Vec2(2, 0), s, pWorld, b2_dynamicBody);
    //b2Body* body21 = createBox(p + b2Vec2(2, 0), s, portalWorld, b2_dynamicBody);

   (portalWorld->createPortalBody(body2))->bodyColor = b2Color(1.0f, 0.6f, 0.5f, 0.5f);
   (portalWorld->createPortalBody(body20))->bodyColor = b2Color(0.5f, 0.6f, 1.0f, 0.5f);
   (portalWorld->createPortalBody(body21))->bodyColor = b2Color(0.6f, 1.0f, 0.5f, 0.5f);

#if 0
    artificial kinematic body
    p = b2Vec2(6.0f, -6.0f);
    s = b2Vec2(0.5f, 0.2f);
    b2Body* body4 = createBox(p, s, pWorld, b2_dynamicBody, 0.0f);
    (pWorld->createPortalBody(body4))->bodyColor = b2Color(0.0f, 0.0f, 1.0f, 0.5f);
    b2Vec2 vel(0.0f, -10.0f);
    body4->SetLinearVelocity(vel);
    body4->SetBullet(true); // for mimicing static body continuous collision
    body4->SetLinearDamping(0.0f);
#endif

    p = b2Vec2(0.0f, 0.0f);
    float r = 0.7f;
    b2Body* body3 = createCircle(p, r, portalWorld, b2_dynamicBody);
    (portalWorld->createPortalBody(body3))->bodyColor = b2Color(1.0f, 1.0f, 0.0f, 0.5f);

    b2Vec2 bPos(10.0f, 5.0f);
    s = b2Vec2(0.2f, -yPos + portalSize + bPos.y);
    b2Body* statBody = createBox(bPos, s, portalWorld, b2_staticBody);
}

void testCase2(PortalWorld* portalWorld){
    b2Vec2 gravity(0.0f, -9.81f);
    portalWorld->SetGravity(gravity);
    Portal* portal1 = portalWorld->createPortal(b2Vec2(-7.0f, 0.0f), b2Vec2(+1.0f, 0.0f), 7.0f);
    Portal* portal2 = portalWorld->createPortal(b2Vec2(+7.0f, 0.0f), b2Vec2(-1.0f, 0.0f), 7.0f);
    Portal* portal3 = portalWorld->createPortal(b2Vec2(0.0f, -7.0f), b2Vec2(0.0f, +1.0f), 7.0f);
    Portal* portal4 = portalWorld->createPortal(b2Vec2(0.0f, +7.0f), b2Vec2(0.0f, -1.0f), 7.0f);

    portal1->connect(portal2);
    portal3->connect(portal4);

    for (int i = 0; i < 300; i++){
        b2Body* body = createBox(b2Vec2(0, 0), b2Vec2(0.3f, 0.3f), portalWorld, b2_dynamicBody);

        (portalWorld->createPortalBody(body))->bodyColor = b2Color(0.0f, 1.0f, 1.0f, 0.5f);
    }
}

void testCase3(PortalWorld* portalWorld){
    b2Vec2 gravity(0.0f, -9.81f);
    portalWorld->SetGravity(gravity);
    float r = 10.0f;
    const int n = 15; // n * 2 portals will be created
    float anglePlus = glm::radians(0.0f);
    auto** circlePortals = (Portal**)malloc(2 * n * sizeof(Portal*));

    for (int i = 0; i < n * 2; i++){
        float angle1 = (float)i * (b2_pi / n) + anglePlus;
        float angle2 = (float)(i + 1) * (b2_pi / n) + anglePlus;
        b2Vec2 p1 = b2Vec2(sin(angle1) * r, cos(angle1) * r);
        b2Vec2 p2 = b2Vec2(sin(angle2) * r, cos(angle2) * r);
        float l = (p1 - p2).Length();
        b2Vec2 pos = 0.5f * (p1 + p2);
        Portal* portal = portalWorld->createPortal(pos, -pos, l / 2.0f);
        circlePortals[i] = portal;
    }
    for (int i = 0; i < n; i++){
        circlePortals[i]->connect(circlePortals[n + i]);
    }
    free(circlePortals);

    float sizeM = 0.2f;
    for (int i = 0; i < 150; i++){
        b2Vec2 size = b2Vec2(sizeM + sizeM * (rand() / (float)RAND_MAX), sizeM + sizeM * (rand() / (float)RAND_MAX));
        float r = sizeM + 2.0f * sizeM * (rand() / (float)RAND_MAX);

        b2Body* body1 = createBox(b2Vec2(0, 0), size, portalWorld, b2_dynamicBody);
        b2Body* body2 = createCircle(b2Vec2(rand() % 5 + r, rand() % 5), r, portalWorld, b2_dynamicBody);

        (portalWorld->createPortalBody(body1))->bodyColor = b2Color(0.25f, 1.0f, 0.9f, 0.5f);
        (portalWorld->createPortalBody(body2))->bodyColor = b2Color(1.0f, 0.25f, 0.7f, 0.5f);
    }
}

void testCase4(PortalWorld* portalWorld){
    b2Vec2 gravity(0.0f, 0.0f);
    portalWorld->SetGravity(gravity);

    float bias = 10.0f;
    float yPos = -4.0f;
    float portalSize = 3.0f;
    float d = 0.5f;
    const int n = 50;

    auto** linePortals = (Portal**)malloc(n * sizeof(Portal*));

    createEdge(b2Vec2(-100.0f, yPos - portalSize), b2Vec2(+100.0f, yPos - portalSize), portalWorld, b2_staticBody);

    b2Body* body2 = createCircle(b2Vec2(-n * d - 7.0f + bias, yPos), 1, portalWorld, b2_dynamicBody);

    b2Vec2 vel(8, 0);
    body2->SetLinearVelocity(vel);

    (portalWorld->createPortalBody(body2))->bodyColor = b2Color(1.0f, 1.0f, 0.0f, 0.5f);

    PortalBody* b1 = portalWorld->createPortalBody(createObody(portalWorld, b2Vec2(-n * d - 10.0f + bias, yPos)));
    PortalBody* b2 = portalWorld->createPortalBody(createWbody(portalWorld, b2Vec2(-n * d - 13.0f + bias, yPos)));

    b1->body->SetLinearVelocity(vel);
    b2->body->SetLinearVelocity(vel);

    b1->bodyColor = b2Color(0.0f, 1.0f, 1.0f, 0.5f);
    b2->bodyColor = b2Color(1.0f, 0.0f, 1.0f, 0.5f);

    float p = -n * d + bias;
    for (int i = 0; i < n; i++){
        Portal* portal = portalWorld->createPortal(b2Vec2(p, yPos), b2Vec2(+1.0f, +0.0f), portalSize);
        linePortals[i] = portal;
        p += d;
    }

    for (int i = 0; i < n; i+=2){
        linePortals[i]->connect(linePortals[i + 1], true, (i + 1) % 2, 0);
    }
}

void multiReleaseTest(PortalWorld* portalWorld){
    createEdge(b2Vec2(-100.0f, -10.0f), b2Vec2(+100.0f, -10.0f), portalWorld, b2_staticBody);
    
    Portal* portal1 = portalWorld->createPortal(b2Vec2(0.0f, -3.0f), b2Vec2(0.0f, +1.0f), 5.0f);
    Portal* portal2 = portalWorld->createPortal(b2Vec2(0.0f, +3.0f), b2Vec2(0.0f, -1.0f), 5.0f);
    
    portal1->connect(portal2);
    portal1->connect(portal2, true, 1, 1);
    
    b2Body* body1 = createWbody(portalWorld, b2Vec2(+6.0f, -7.5f), 540.0f, 100, 0.16f, 3.0f, 0.008f);
    PortalBody* b1 = portalWorld->createPortalBody(body1);

    b2Body* body2 = createObody(portalWorld, b2Vec2(-15.0f, 0.0f), 1080.0f, 50, 0.3f, 3.0f, 0.01f);
    PortalBody* b2 = portalWorld->createPortalBody(body2);

    b2Body* body3 = createBox(b2Vec2(0, 0), b2Vec2(10, 1), portalWorld, b2_dynamicBody);
    PortalBody* b3 = portalWorld->createPortalBody(body3);
}
