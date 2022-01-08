#pragma once

#include <box2d/box2d.h>
#include <vector>
#include <stdio.h>
#include <algorithm>
#include <vector>
#include <map>
#include <set>
#include <unordered_set>
#include <PortalBody.h>

class PortalWorld{

private:
    std::vector<PortalBody*> portalBodies;
    std::set<PortalBody*> destroyBodies;
    std::set<Portal*> portals;

    bool isLeft(b2Vec2& a, b2Vec2& b, b2Vec2& c, float t);
    float vecAngle(b2Vec2 v1, b2Vec2 v2);
    float getDist(b2Vec2& a, b2Vec2& b, b2Vec2& c);
    float calcAngle2(b2Vec2 vec);
    void normalize(b2Vec2* vec);

public:
    bool drawReleases;
    b2Color releaseColor;

    b2World* world;

    PortalWorld(b2World* world);

    void portalUpdate();
    void globalPostHandle();
    void drawUpdate();

    friend class PortalBody;
    friend class Portal;
};