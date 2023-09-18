#include <box2d/box2d.h>
#include <PortalBody.h>

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
        if (fixture->GetType() == b2Shape::e_edge && fraction < closestFraction)
        {
            fixtureData* fixData = (fixtureData*)fixture->GetUserData().pointer;
            if (fixData && fixData->type & F_FEDGE)
            {
				return -1.0f;
			}

            found = true;
            rayNormal = normal;
            rayPoint = point;
            edgeFix = fixture;
            closestFraction = fraction;

            return fraction;
        }

        return -1.0f;
    }

    void cast(b2Vec2 p1, b2Vec2 p2)
    {
        found = false;
        edgeFix = nullptr;
        closestFraction = 1.1f;
        world->RayCast(this, p1, p2);
    }
};