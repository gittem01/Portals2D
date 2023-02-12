#include <box2d/box2d.h>

class PortalGunRayCast : public b2RayCastCallback
{
public:
    b2World* world;
    bool found;
    b2Fixture* edgeFix;
    b2Vec2 rayNormal;
    b2Vec2 rayPoint;
    float closestFraction = 1.1f;

    PortalGunRayCast(b2World* world)
    {
        this->world = world;
    }

    float ReportFixture(b2Fixture* fixture, const b2Vec2& point, const b2Vec2& normal, float fraction)
    {
        if (fixture->GetBody()->GetType() == b2_staticBody && fixture->GetType() == b2Shape::e_edge && fraction < closestFraction)
        {
            found = true;
            rayNormal = normal;
            rayPoint = point;

            closestFraction = fraction;
        }
        return -1.0f;
    }

    void cast(b2Vec2 p1, b2Vec2 p2)
    {
        found = false;
        edgeFix = nullptr;
        world->RayCast(this, p1, p2);
    }
};