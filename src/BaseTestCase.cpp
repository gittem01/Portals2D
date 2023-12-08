#pragma once

#include "mouseJointHandler.h"
#include "ContactListener.h"
#include "ThinDrawer.h"
#include <Game.h>
#include <BaseTestCase.h>

BaseTestCase* BaseTestCase::testCase = nullptr;

BaseTestCase::BaseTestCase(Game* game, const char* name)
{
    this->g = game;
    this->td = game->thinDrawer;
    this->name = name;
    refreshItem = 0;
    shouldGetNewTest = false;
    testCase = this;
}

void BaseTestCase::destroy()
{
    td->releaseRenderResources();

    delete g->mjh;
    delete g->pWorld;
    delete tp;

    g->pWorld = nullptr;
    td->pWorld = nullptr;
}

void BaseTestCase::imguiRender()
{
    const ImGuiIO io = ImGui::GetIO();

    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    ImGui::Begin("Debug Window", NULL, ImGuiWindowFlags_AlwaysAutoResize);

    ImGui::Text("fps: %.2f", io.Framerate);
    ImGui::SameLine();
    ImGui::Text("dt: %.4f", io.DeltaTime * 1000.0f);

    int lastItem = testCaseItem;
    ImGui::Combo(" ", &testCaseItem, testCases.data(), testCases.size());
    ImGui::SameLine();
    if (ImGui::Button("Reset"))
    {
        shouldGetNewTest = true;
    }

    if (lastItem != testCaseItem)
    {
        shouldGetNewTest = true;
    }

    if (ImGui::RadioButton("Follow Player", followPlayer))
    {
        followPlayer = !followPlayer;
    }

    if (ImGui::RadioButton("Draw Releases", td->drawReleases))
    {
        td->drawReleases = !td->drawReleases;
    }

    if (ImGui::RadioButton("Draw Debug Bodies", td->drawDebugBodies))
    {
        td->drawDebugBodies = !td->drawDebugBodies;
    }

    if (ImGui::RadioButton("Draw Debug Edges", td->drawDebugEdges))
    {
        td->drawDebugEdges = !td->drawDebugEdges;
    }

    if (ImGui::RadioButton("Draw Debug Portals", td->drawDebugPortals))
    {
        td->drawDebugPortals = !td->drawDebugPortals;
    }

    if (g->isPaused)
    {
        if (ImGui::Button("Resume"))
        {
            g->isPaused = false;
        }
    }
    else
    {
        if (ImGui::Button("Stop"))
        {
            g->isPaused = true;
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("Tick"))
    {
        g->isPaused = true;
        g->tick = true;
    }

    float oldVal = g->mjh->bodyRadius;
    ImGui::SliderFloat("MouseSelectionArea", &g->mjh->bodyRadius, g->mjh->radiusLimits[0], g->mjh->radiusLimits[1], "%.2f", 1);
    if (oldVal != g->mjh->bodyRadius)
    {
        g->mjh->createMouseBody();
    }

    ImGui::End();

    ImGui::Render();

    td->drawData = ImGui::GetDrawData();
}

void BaseTestCase::beforeCreate(bool createTP)
{
    td->debugLineRegistry.clear();

    if (!g->contactListener)
    {
        g->contactListener = new ContactListener();
    }
    g->pWorld = new PortalWorld();
    g->pWorld->SetContactListener(g->contactListener);
    b2Vec2 gravity(0.0f, -10.0f);
    g->pWorld->SetGravity(gravity);
    g->pWorld->SetAllowSleeping(true);

    g->mjh = new mouseJointHandler(g->pWorld, td->wh);

    td->pWorld = g->pWorld;
    td->beforeCreate();

    if (createTP)
    {
        tp = new TestPlayer(g);
    }
}

void BaseTestCase::afterCreate()
{
    td->afterCreate();
}

void BaseTestCase::onUpdate()
{
    double physicsTickTime = glfwGetTime();

    td->debugLineRegistry.clear();

    int requiredIters = std::max((int)std::round(g->deltaTime / requestedDtPerStep), 1);
    double requiredDt = g->deltaTime / requiredIters;
    requiredIters = std::min(requiredIters, 20);

    if (!g->pWorld)
    {
        return;
    }

    //requiredDt = 1.0 / 60.0;
    //requiredIters = 1;
    float dt = requiredDt * 2.0;
    for (int i = 0; i < requiredIters; i++)
    {
        g->mjh->mouseHandler(g->frame, requiredIters);

        tp->update(dt, requiredIters);

        g->pWorld->PortalStep(dt, 8, 3);
    }
    
    if (followPlayer) tp->camHandle();

    //printf("dt: %f\n", glfwGetTime() - physicsTickTime);
}

void BaseTestCase::keyHandler(WindowHandler* wh)
{
    extern BaseTestCase* testCase;

    g->tick = false;
    if (wh->keyData[GLFW_KEY_P] == 2)
    {
        g->isPaused = !g->isPaused;
    }
    else if (wh->keyData[GLFW_KEY_O] == 2)
    {
        g->isPaused = true;
        g->tick = true;
    }
    if (wh->keyData[GLFW_KEY_S] == 2)
    {
        td->drawReleases ^= 1;
    }
}




// helpers

float getRand()
{
    return ((double)rand() / RAND_MAX) - 0.5f;
}

// O shaped body
b2Body* createObody(b2World* world, b2Vec2 bodyPos, float degree, int n, float thickness, float rOut, float fd, float isMixed) {
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
        if (dec % 2 == 0) {
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
        else {
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
        if (isMixed) {
            dec++;
        }
    }

    free(vertices);

    return body;
}

b2Body* createWbody(b2World* world, b2Vec2 bodyPos, float degree, int n, float circleR, float rOut, float fd) {
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

b2Body* createEdge(b2Vec2 p1, b2Vec2 p2, b2World* world, b2BodyType type, bool oneSided) {
    b2BodyDef bd;
    bd.type = type;
    b2Body* edgeBody = world->CreateBody(&bd);

    b2Vec2 dir = p1 - p2;
    b2EdgeShape shape;
    shape.SetOneSided(p1 + dir, p1, p2, p2 - dir);
    shape.m_oneSided = oneSided;

    b2FixtureDef fixDef;
    fixDef.shape = &shape;

    b2Fixture* fix = edgeBody->CreateFixture(&fixDef);

    return edgeBody;
}

b2Body* createPortalCube(b2Vec2 pos, b2Vec2 size, b2World* world, b2BodyType type, float density) {
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
    for (int i = 0; i < 4; i++) {
        b2PolygonShape cornerShape;
        cornerShape.Set(vertices.data(), (int)vertices.size());
        for (auto& vertex : vertices) {
            if (!mirrorStatus) {
                vertex.x *= -1;
            }
            else {
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

b2Body* createBox(b2Vec2 pos, b2Vec2 size, b2World* world, b2BodyType type, float density)
{
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

b2Body* createCircle(b2Vec2 pos, float size, b2World* world, b2BodyType type, float density, bool sensor) {
    b2CircleShape shape;
    shape.m_radius = size;

    b2FixtureDef fDef;
    fDef.shape = &shape;
    fDef.density = density;
    fDef.restitution = 0.2f;
    fDef.friction = 0.2f;
    fDef.isSensor = sensor;

    b2BodyDef def;
    def.type = type;
    def.position = pos;
    def.linearDamping = 0.0f;

    b2Body* body = world->CreateBody(&def);
    body->CreateFixture(&fDef);

    return body;
}