#pragma once

#include <helpers.h>

class BaseTestCase
{
protected:
    static std::vector<BaseTestCase> testCases;
    static int namerCount;

    const char* name;

    BaseTestCase(const char* name="\0"){
        this->name = name;
    }

    void beforeCreate(WindowPainter* wh){
        if (!contactListener){
            contactListener = new ContactListener();
        }
        pWorld = new PortalWorld();
        pWorld->SetContactListener(contactListener);

        if (!renderer){
            renderer = new Renderer(pWorld, wh->cam);
        }
        if (!drawer){
            drawer = new DebugDrawer(renderer);
        }

        pWorld->SetDebugDraw(drawer);
        drawer->SetFlags(b2Draw::e_centerOfMassBit);

        mjh = new mouseJointHandler(pWorld, wh, renderer);

        b2Vec2 gravity(0.0f, -10.0f);
        pWorld->SetGravity(gravity);
    }

    void afterCreate(){
        // manual random coloring for now
        for (Portal* p : pWorld->portals){
            b2Color c = b2Color(1, getRand() + 0.5f, getRand() + 0.5f, 1.0f);
            renderer->addPortal(p, c);
        }

        renderer->portalAddEnd();

        for (int i = 0; i < 2; i++){
            for (Portal* p : pWorld->portals){
                p->evaluateWorld();
                p->queryClass->mode++;
            }
        }
    }

    virtual void onCreate(WindowPainter* wh) = 0;
    virtual void onDestroy() = 0;
    virtual void onUpdate() = 0;
    virtual void onDraw() = 0;
};

int BaseTestCase::namerCount = 0;
std::vector<BaseTestCase> BaseTestCase::testCases;