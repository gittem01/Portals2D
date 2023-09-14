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

    g->pWorld = NULL;
}

void BaseTestCase::imguiRender()
{
    ImGuiIO io = ImGui::GetIO();

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

void BaseTestCase::beforeCreate()
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

    tp = new TestPlayer(g);
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

    if (followPlayer) tp->camHandle();

    if (!g->pWorld)
    {
        return;
    }

    for (int i = 0; i < requiredIters; i++)
    {
        float dt = requiredDt * 2.0;
        g->mjh->mouseHandler(g->frame, requiredIters);

        tp->update(dt, requiredIters);

        g->pWorld->PortalStep(dt, 1, 1);
    }

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