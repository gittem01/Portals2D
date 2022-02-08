#include "DebugDrawer.h"
#include "mouseJointHandler.h"
#include "ContactListener.h"
#include "TestPlayer.h"

#include <chrono>
#include <thread>

bool isPaused = false;
bool tick = false;
int totalIter = 10;
PortalWorld* pWorld;

// O shaped body
b2Body* createObody(b2World* world, b2Vec2 bodyPos=b2Vec2(0, 0), float degree=270.0f, float thickness=0.3f, float rOut = 1.2f) {
    b2BodyDef bodyDef;
    bodyDef.position = bodyPos;
    bodyDef.type = b2_dynamicBody;
    bodyDef.angularDamping = 0.1f;
    bodyDef.linearDamping = 0.1f;

    b2Body* body = world->CreateBody(&bodyDef);

    b2Vec2* vertices = (b2Vec2*)malloc(sizeof(b2Vec2) * 4);

    degree /= 180 / glm::pi<float>();
    float n = 20;
    float incr = degree / n;

    for (float currD = 0.0f; currD < degree;) {
        b2FixtureDef fDef;
        b2PolygonShape shape;

        float nextD = currD + incr;

        float x1 = sin(currD) * rOut;
        float y1 = cos(currD) * rOut;

        float x2 = sin(currD) * (rOut - thickness);
        float y2 = cos(currD) * (rOut - thickness);

        float x3 = sin(nextD) * rOut;
        float y3 = cos(nextD) * rOut;

        float x4 = sin(nextD) * (rOut - thickness);
        float y4 = cos(nextD) * (rOut - thickness);

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

    free(vertices);

    return body;
}

// Weird body (???)
b2Body* createWbody(b2World* world, b2Vec2 bodyPos=b2Vec2(0, 0), float degree=270.0f, float circleR=0.15f, float rOut = 1.2f) {
    b2BodyDef bodyDef;
    bodyDef.position = bodyPos;
    bodyDef.type = b2_dynamicBody;
    bodyDef.angularDamping = 0.1f;
    bodyDef.linearDamping = 0.1f;

    b2Body* body = world->CreateBody(&bodyDef);

    b2FixtureDef fDef;
    b2CircleShape shape;
    
    degree /= 180 / glm::pi<float>();
    float n = 20;
    float incr = degree / n;

    for (float currD = 0.0f; currD < degree;) {
        b2FixtureDef fDef;
        b2CircleShape cShape;

        float nextD = currD + incr;

        float x = sin(currD) * (rOut - circleR);
        float y = cos(currD) * (rOut - circleR);

        cShape.m_p = b2Vec2(x, y);
        cShape.m_radius = circleR;

        fDef.shape = &cShape;
        fDef.density = 0.2f;

        body->CreateFixture(&fDef);

        currD = nextD;
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
    bodyData data = { OTHER, NULL };
    fixDef.userData.pointer = (uintptr_t)&data;

    edgeBody->CreateFixture(&fixDef);

    return edgeBody;
}

b2Body* createPortalCube(b2Vec2 pos, b2Vec2 size, b2World* world, b2BodyType type, float density=1.0f){
    std::vector<b2Vec2> vertices = 
        {   b2Vec2(-0.1 * size.x, +0.5 * size.y), b2Vec2(-0.2 * size.x, +0.6 * size.y), b2Vec2(-0.5 * size.x, +0.6 * size.y),
            b2Vec2(-0.6 * size.x, +0.5 * size.y), b2Vec2(-0.6 * size.x, +0.2 * size.y), b2Vec2(-0.5 * size.x, +0.1 * size.y)};
    b2PolygonShape boxShape;
    boxShape.SetAsBox(0.5 * size.x, 0.5 * size.y);

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

    bool mirrorStatus = 0;
    for (int i = 0; i < 4; i++){
        b2PolygonShape cornerShape;
        cornerShape.Set(vertices.data(), vertices.size());
        for (int i = 0; i < vertices.size(); i++){
            if (!mirrorStatus){
                vertices.at(i).x *= -1;
            }
            else{
                vertices.at(i).y *= -1;
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
    def.linearDamping = 0.1f;    

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
    return ((float)rand()) / RAND_MAX - 0.5f;
}

void classicTest(PortalWorld* pWorld){
    b2Vec2 gravity(0.0f, -20.00f);
    pWorld->world->SetGravity(gravity);

    float xSize = 7.98f;
    float ySize = 4.48f;
    float width = 0.5f;
    float m = 1.0f;

    Portal* portal1 = pWorld->createPortal(b2Vec2(0.0f, -ySize), b2Vec2(+0.0f, +1.0f), ySize * m);
    Portal* portal2 = pWorld->createPortal(b2Vec2(0.0f, +ySize), b2Vec2(+0.0f, -1.0f), ySize * m);
    Portal* portal3 = pWorld->createPortal(b2Vec2(-xSize, 0.0f), b2Vec2(+1.0f, +0.0f), ySize * m);
    Portal* portal4 = pWorld->createPortal(b2Vec2(+xSize, 0.0f), b2Vec2(-1.0f, +0.0f), ySize * m);

    portal1->connect(portal2);
    portal3->connect(portal4);
    
    // extras
    // portal1->connect(portal3, 0, 1);
    // portal1->connect(portal4, 0, 1);

    createEdge(b2Vec2(-xSize, -ySize), b2Vec2(+xSize, -ySize), pWorld->world, b2_staticBody);
    createEdge(b2Vec2(-xSize, -ySize), b2Vec2(-xSize, +ySize), pWorld->world, b2_staticBody);
    createEdge(b2Vec2(-xSize, +ySize), b2Vec2(+xSize, +ySize), pWorld->world, b2_staticBody);
    createBox(b2Vec2(xSize + width, 0), b2Vec2(width, ySize), pWorld->world, b2_staticBody);

    b2Vec2 p = b2Vec2(0.0f, 0.0f);
    b2Vec2 s = b2Vec2(3.0f, 0.2f);
    b2Body* body4 = createBox(p, s, pWorld->world, b2_dynamicBody, 0.0f);
    (pWorld->createPortalBody(body4))->bodyColor = b2Color(0.0f, 0.0f, 1.0f, 0.5f);
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
                                (getRand() + sizeM) / div, pWorld->world, b2_dynamicBody);
        pWorld->createPortalBody(b, b2Color(1.0f, 0.25f, 0.7f, 0.5f));
    }
    for (int i = 0; i < numPolygons; i++) {
        b2Body* b = createBox(b2Vec2(getRand() * xSize * 1.9f, getRand() * ySize * 1.9f),
                            b2Vec2(((getRand() + sizeM) / div), (getRand() + sizeM) / div), pWorld->world, b2_dynamicBody);
        pWorld->createPortalBody(b, b2Color(0, 1, 1, 0.5));
    }

    for (int i = 0; i < 5; i++) {
        b2Body* b1 = createCircle(b2Vec2(getRand() * xSize * 1.0f, 10.0f),
                                (getRand() + sizeM) / div, pWorld->world, b2_dynamicBody);
        pWorld->createPortalBody(b1, b2Color(1.0f, 0.25f, 0.7f, 0.5f));

        b2Body* b2 = createBox(b2Vec2(getRand() * xSize * 1.0f, 10.0f),
                            b2Vec2(((getRand() + sizeM) / div), (getRand() + sizeM) / div), pWorld->world, b2_dynamicBody);
        pWorld->createPortalBody(b2, b2Color(0, 1, 1, 0.5));
    }
}

void testRooms(PortalWorld* pWorld){
    b2Vec2 gravity(0.0f, -40.00f);
    pWorld->world->SetGravity(gravity);

    float roomWidth = 10.0f;
    float roomHeight = 20.0f;
    float bottomPos = -10.0f;
    int numOfCubes = 100;
    int numOfCircles = 100;

    // bottom portals
    Portal* portal1 = pWorld->createPortal(b2Vec2(-roomWidth - 1.0f, bottomPos), b2Vec2(0.0f, +1.0f), roomWidth);
    Portal* portal2 = pWorld->createPortal(b2Vec2(+roomWidth + 1.0f, bottomPos), b2Vec2(0.0f, +1.0f), roomWidth);

    // top portals
    Portal* portal3 = pWorld->createPortal(b2Vec2(-roomWidth - 1.0f, bottomPos + roomHeight), b2Vec2(0.0f, -1.0f), roomWidth);
    Portal* portal4 = pWorld->createPortal(b2Vec2(+roomWidth + 1.0f, bottomPos + roomHeight), b2Vec2(0.0f, -1.0f), roomWidth);

    portal1->connect(portal2);
    portal3->connect(portal4);

    createEdge(b2Vec2(-roomWidth * 2 - 1, bottomPos), b2Vec2(-roomWidth * 2 - 1, roomHeight + bottomPos), pWorld->world, b2_staticBody);
    createEdge(b2Vec2(-1, bottomPos), b2Vec2(-1, roomHeight + bottomPos), pWorld->world, b2_staticBody);

    createEdge(b2Vec2(+roomWidth * 2 + 1, bottomPos), b2Vec2(+roomWidth * 2 + 1, roomHeight + bottomPos), pWorld->world, b2_staticBody);
    createEdge(b2Vec2(+1, bottomPos), b2Vec2(+1, roomHeight + bottomPos), pWorld->world, b2_staticBody);

    for (int i = 0; i < numOfCubes; i++){
        double sizex = ((double)rand() / RAND_MAX) * 0.5f + 0.3f;
        double sizey = ((double)rand() / RAND_MAX) * 0.5f + 0.3f;
        b2Body* b = createBox(b2Vec2(-roomWidth - 1, 0.0f), b2Vec2(sizex, sizey), pWorld->world, b2_dynamicBody);
        pWorld->createPortalBody(b, b2Color(1.0f, 1.0f, 0.25f, 0.5f));
    }

    for (int i = 0; i < numOfCircles; i++){
        double size = ((double)rand() / RAND_MAX) * 0.5f + 0.3f;
        b2Body* b = createCircle(b2Vec2(-roomWidth - 1, 0.0f), size, pWorld->world, b2_dynamicBody);
        pWorld->createPortalBody(b, b2Color(0.25f, 1.0f, 0.7f, 0.5f));
    }

    srand(77);
    for (int i = 0; i < 10; i++){
        b2Vec2 p = b2Vec2(2 + i * 2, 0.0f);
        b2Vec2 s = b2Vec2(roomWidth / 10, 0.1f);
        b2Body* body4 = createBox(p, s, pWorld->world, b2_dynamicBody, 0.0f);
        (pWorld->createPortalBody(body4))->bodyColor = b2Color(1.0f, 1.0f, 1.0f, 1.0f);
        b2Vec2 vel(0.0f, ((double)rand() / RAND_MAX) * 10.0f + 5.0f);
        body4->SetLinearVelocity(vel);
        body4->SetBullet(true);
        body4->SetLinearDamping(0.0f);
    }
}

void testCase1(PortalWorld* pWorld){
    b2Vec2 gravity(0.0f, -40.00f);
    pWorld->world->SetGravity(gravity);

    float yPos = -4.0f;
    float portalSize = 3.0f;

    Portal* portal1 = pWorld->createPortal(b2Vec2(-6.0f, yPos), b2Vec2(+1.0f, +0.0f), portalSize);
    Portal* portal2 = pWorld->createPortal(b2Vec2(+6.0f, yPos - portalSize + 4.0f), b2Vec2(0.0f, -1.0f), portalSize);
    Portal* portal3 = pWorld->createPortal(b2Vec2(+6.0f, yPos - portalSize + 0.0f), b2Vec2(0.0f, +1.0f), portalSize);
    Portal* portal41 = pWorld->createPortal(b2Vec2(+10.0f + 0.2f, yPos), b2Vec2(+1.0f, 0.0f), portalSize);
    Portal* portal42 = pWorld->createPortal(b2Vec2(+10.0f + 0.2f, yPos + portalSize * 2), b2Vec2(+1.0f, 0.0f), portalSize);

    Portal* portal5 = pWorld->createPortal(b2Vec2(-12.0f, yPos - portalSize + 0.0f), b2Vec2(0.0f, +1.0f), portalSize);
    Portal* portal6 = pWorld->createPortal(b2Vec2(-20.0f, yPos - portalSize + 0.0f), b2Vec2(0.0f, +1.0f), portalSize);

    portal3->connect(portal2);
    portal41->connect(portal1, 0, 0);
    portal42->connect(portal1, 0, 1);
    portal5->connect(portal6, 0, 0);

    createEdge(b2Vec2(-100.0f, yPos - portalSize), b2Vec2(+100.0f, yPos - portalSize), pWorld->world, b2_staticBody);

    PortalBody* b1 = pWorld->createPortalBody(createObody(pWorld->world, b2Vec2(0.0f, 3.0f)));
    PortalBody* b2 = pWorld->createPortalBody(createWbody(pWorld->world, b2Vec2(0.0f, 6.0f)));

    b1->bodyColor = b2Color(0.0f, 1.0f, 1.0f, 0.5f);
    b2->bodyColor = b2Color(1.0f, 0.0f, 1.0f, 0.5f);

    b2Vec2 p(5.0f, 0.0f);
    b2Vec2 s(0.75f, 0.75f);
    b2Body* body2 = createPortalCube(p, s, pWorld->world, b2_dynamicBody);
    (pWorld->createPortalBody(body2))->bodyColor = b2Color(1.0f, 0.6f, 0.5f, 0.5f);

    // artificial kinematic body
    // p = b2Vec2(5.0f, -6.0f);
    // s = b2Vec2(10.0f, 0.5f);
    // b2Body* body4 = createBox(p, s, pWorld->world, b2_dynamicBody, 0.0f);
    // (pWorld->createPortalBody(body4))->bodyColor = b2Color(0.0f, 0.0f, 1.0f, 0.5f);
    // b2Vec2 vel(-8.0f, 0.0f);
    // body4->SetLinearVelocity(vel);
    // body4->SetBullet(true); // for mimicing static body continuous collision
    // body4->SetLinearDamping(0.0f);

    p = b2Vec2(0.0f, 0.0f);
    float r = 0.7f;
    b2Body* body3 = createCircle(p, r, pWorld->world, b2_dynamicBody);
    (pWorld->createPortalBody(body3))->bodyColor = b2Color(1.0f, 1.0f, 0.0f, 0.5f);

    b2Vec2 bPos(10.0f, 5.0f);
    s = b2Vec2(0.2f, -yPos + portalSize + bPos.y);
    b2Body* statBody = createBox(bPos, s, pWorld->world, b2_staticBody);
}

void testCase2(PortalWorld* pWorld){
    b2Vec2 gravity(0.0f, -9.81f);
    pWorld->world->SetGravity(gravity);
    Portal* portal1 = pWorld->createPortal(b2Vec2(-7.0f, 0.0f), b2Vec2(+1.0f, 0.0f), 7.0f);
    Portal* portal2 = pWorld->createPortal(b2Vec2(+7.0f, 0.0f), b2Vec2(-1.0f, 0.0f), 7.0f);
    Portal* portal3 = pWorld->createPortal(b2Vec2(0.0f, -7.0f), b2Vec2(0.0f, +1.0f), 7.0f);
    Portal* portal4 = pWorld->createPortal(b2Vec2(0.0f, +7.0f), b2Vec2(0.0f, -1.0f), 7.0f);

    portal1->connect(portal2);
    portal3->connect(portal4);

    for (int i = 0; i < 300; i++){
        b2Body* body = createBox(b2Vec2(0, 0), b2Vec2(0.3f, 0.3f), pWorld->world, b2_dynamicBody);

        (pWorld->createPortalBody(body))->bodyColor = b2Color(0.0f, 1.0f, 1.0f, 0.5f);
    }
}

void testCase3(PortalWorld* pWorld){
    b2Vec2 gravity(0.0f, -9.81f);
    pWorld->world->SetGravity(gravity);
    float r = 10.0f;
    const int n = 15; // n * 2 portals will be created
    float anglePlus = glm::radians(0.0f);
    Portal** circlePortals = (Portal**)malloc(2 * n * sizeof(Portal*));

    for (int i = 0; i < n * 2; i++){
        float angle1 = i * (b2_pi / n) + anglePlus;
        float angle2 = (i + 1) * (b2_pi / n) + anglePlus;
        b2Vec2 p1 = b2Vec2(sin(angle1) * r, cos(angle1) * r);
        b2Vec2 p2 = b2Vec2(sin(angle2) * r, cos(angle2) * r);
        float l = (p1 - p2).Length();
        b2Vec2 pos = 0.5f * (p1 + p2);
        Portal* portal = pWorld->createPortal(pos, -pos, l / 2.0f);
        circlePortals[i] = portal;
    }
    for (int i = 0; i < n; i++){
        circlePortals[i]->connect(circlePortals[n + i]);
    }
    free(circlePortals);

    float sizeM = 0.2f;
    for (int i = 0; i < 150; i++){
        b2Vec2 size = b2Vec2(sizeM + sizeM * (rand() / (double)RAND_MAX), sizeM + sizeM * (rand() / (double)RAND_MAX));
        float r = sizeM + 2.0f * sizeM * (rand() / (double)RAND_MAX);

        b2Body* body1 = createBox(b2Vec2(0, 0), size, pWorld->world, b2_dynamicBody);
        b2Body* body2 = createCircle(b2Vec2(rand() % 5 + r, rand() % 5), r, pWorld->world, b2_dynamicBody);

        (pWorld->createPortalBody(body1))->bodyColor = b2Color(0.25f, 1.0f, 0.9f, 0.5f);
        (pWorld->createPortalBody(body2))->bodyColor = b2Color(1.0f, 0.25f, 0.7f, 0.5f);
    }
}

void testCase4(PortalWorld* pWorld){
    b2Vec2 gravity(0.0f, 0.0f);
    pWorld->world->SetGravity(gravity);

    float bias = 6.0f;
    float yPos = -4.0f;
    float portalSize = 3.0f;
    float d = 0.5f;
    int n = 51;

    Portal** linePortals = (Portal**)malloc(n * sizeof(Portal*));

    createEdge(b2Vec2(-100.0f, yPos - portalSize), b2Vec2(+100.0f, yPos - portalSize), pWorld->world, b2_staticBody);

    b2Body* body2 = createCircle(b2Vec2(-n * d - 7.0f + bias, yPos), 1, pWorld->world, b2_dynamicBody);

    b2Vec2 vel(4, 0);
    body2->SetLinearVelocity(vel);

    (pWorld->createPortalBody(body2))->bodyColor = b2Color(1.0f, 1.0f, 0.0f, 0.5f);

    PortalBody* b1 = pWorld->createPortalBody(createObody(pWorld->world, b2Vec2(-n * d - 10.0f + bias, yPos)));
    PortalBody* b2 = pWorld->createPortalBody(createWbody(pWorld->world, b2Vec2(-n * d - 13.0f + bias, yPos)));

    b1->body->SetLinearVelocity(vel);
    b2->body->SetLinearVelocity(vel);

    b1->bodyColor = b2Color(0.0f, 1.0f, 1.0f, 0.5f);
    b2->bodyColor = b2Color(1.0f, 0.0f, 1.0f, 0.5f);

    Portal* portal1 = pWorld->createPortal(b2Vec2(-1 * d - n * d + bias, yPos), b2Vec2(-1.0f, +0.0f), portalSize);

    float p = -n*d + bias;
    for (int i = 0; i < n; i++){
        Portal* portal = pWorld->createPortal(b2Vec2(p, yPos), b2Vec2(+1.0f, +0.0f), portalSize);
        linePortals[i] = portal;
        p += d;
    }

    portal1->connect(linePortals[0]);
    for (int i = 0; i < n - 1; i++){
        linePortals[i]->connect(linePortals[i + 1], i % 2, 0);
    }
}