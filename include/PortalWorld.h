#pragma once

#include <box2d/box2d.h>
#include <vector>
#include <stdio.h>
#include <algorithm>
#include <vector>
#include <map>
#include <set>
#include <unordered_set>

class PortalBody;
class Portal;
class DebugDrawer;

class PortalWorld{

private:
    DebugDrawer* drawer;

    std::vector<PortalBody*> portalBodies;
    std::vector<Portal*> portals;
    std::set<PortalBody*> destroyBodies;

    bool isLeft(b2Vec2& a, b2Vec2& b, b2Vec2& c, float t);
    float vecAngle(b2Vec2 v1, b2Vec2 v2);
    float getDist(b2Vec2& a, b2Vec2& b, b2Vec2& c);
    float calcAngle2(b2Vec2 vec);
    void normalize(b2Vec2* vec);

public:
    bool drawReleases;
    b2Color releaseColor;

    b2World* world;

    PortalWorld(b2World* world, DebugDrawer* drawer);

    void portalUpdate();
    void globalPostHandle();
    void drawUpdate();

    Portal* createPortal(b2Vec2 pos, b2Vec2 dir, float size);
    PortalBody* createPortalBody(b2Body* body, b2Color bodyColor=b2Color(1.0f, 1.0f, 1.0f, 0.5f));

    friend class PortalBody;
    friend class Portal;
};