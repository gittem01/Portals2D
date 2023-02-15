#include "mouseJointHandler.h"
#include "ContactListener.h"

#include <thread>
#include <chrono>

const char* refreshRates[] = { "60", "90", "120", "144", "240" };
const int correspondingValues[] = { 60, 90, 120, 144, 240 };
const char* testCases[] =   { 
                                "Random Portals",
                                "Squares",
                                "Circles",
                                "Speeder",
                                "Elevator",
                                "Squisher and Spirals",
                                "Mixer",
                                "ShootShoot Place",
                            };

const int refreshSize = sizeof(refreshRates) / sizeof(refreshRates[0]);
const int numTestCases = sizeof(testCases) / sizeof(testCases[0]);

std::random_device rd;
std::mt19937 mt(rd());
std::uniform_real_distribution<double> randF(-0.5f, 0.5f);

std::vector<PortalBody*>* indexToTrack;

float getRand(){
    //return (rand() / RAND_MAX) - 0.5f;
    randF(mt);
    return randF(mt);
}

bool isPaused = false;
bool tick = false;
int frame = 0;
PortalWorld* pWorld = nullptr;
Renderer* renderer = nullptr;
mouseJointHandler* mjh = nullptr;
DebugDrawer* drawer = nullptr;
ContactListener* contactListener = nullptr;
int totalIter = 5;

// O shaped body
b2Body* createObody(b2World* world, b2Vec2 bodyPos=b2Vec2(0, 0), float degree=270.0f, int n=20, float thickness=0.3f, float rOut=1.2f, float fd=0.0f, float isMixed=false) {
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
        if (isMixed){
            dec++;
        }
    }

    free(vertices);

    return body;
}

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
    fixtureData* fData = new fixtureData();
    fixDef.userData.pointer = (uintptr_t)fData;

    edgeBody->CreateFixture(&fixDef);

    return edgeBody;
}

b2Body* createPortalCube(b2Vec2 pos, b2Vec2 size, b2World* world, b2BodyType type, float density=1.0f){
    const float mult = (1.0f / 1.2f);
    std::vector<b2Vec2> vertices = 
        {
            mult * 2 * b2Vec2(-0.1f * size.x, +0.5f * size.y),
            mult * 2 * b2Vec2(-0.2f * size.x, +0.6f * size.y),
            mult * 2 * b2Vec2(-0.5f * size.x, +0.6f * size.y),
            mult * 2 * b2Vec2(-0.6f * size.x, +0.5f * size.y),
            mult * 2 * b2Vec2(-0.6f * size.x, +0.2f * size.y),
            mult * 2 * b2Vec2(-0.5f * size.x, +0.1f * size.y)
        };
    b2PolygonShape boxShape;
    boxShape.SetAsBox(size.x * mult, size.y * mult);

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
        renderer->drawReleases ^= 1;
}