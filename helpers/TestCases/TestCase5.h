#include <BaseTestCase.h>

class TestCase5 : public BaseTestCase
{
public:
    TestCase5(ThinDrawer* td) : BaseTestCase(td) {
        testCaseItem = 4;
        
        beforeCreate();
        onCreate();
        afterCreate();
    }

    void onCreate(){
        b2Vec2 gravity(0.0f, -20.00f);
        pWorld->SetGravity(gravity);

        float xSize = 7.98f;
        float ySize = 4.48f;
        float width = 0.5f;

        Portal* portal1 = pWorld->createPortal(b2Vec2(0.0f, -ySize), b2Vec2(+0.0f, +1.0f), ySize);
        Portal* portal2 = pWorld->createPortal(b2Vec2(0.0f, +ySize), b2Vec2(+0.0f, -1.0f), ySize);
        Portal* portal3 = pWorld->createPortal(b2Vec2(-xSize, 0.0f), b2Vec2(+1.0f, +0.0f), ySize);
        Portal* portal4 = pWorld->createPortal(b2Vec2(+xSize, 0.0f), b2Vec2(-1.0f, +0.0f), ySize);

        portal1->connect(portal2);
        portal3->connect(portal4);

        createEdge(b2Vec2(-xSize, -ySize), b2Vec2(portal1->points[0].x, -ySize), pWorld, b2_staticBody);
        createEdge(b2Vec2(portal1->points[1].x, -ySize), b2Vec2(+xSize, -ySize), pWorld, b2_staticBody);

        createEdge(b2Vec2(-xSize, +ySize), b2Vec2(portal2->points[1].x, +ySize), pWorld, b2_staticBody);
        createEdge(b2Vec2(portal2->points[0].x, +ySize), b2Vec2(+xSize, +ySize), pWorld, b2_staticBody);
        
        // fences
        createEdge(b2Vec2(-xSize, +ySize), b2Vec2(-xSize, +ySize + 5.0f), pWorld, b2_staticBody);
        createEdge(b2Vec2(+xSize, +ySize), b2Vec2(+xSize, +ySize + 5.0f), pWorld, b2_staticBody);

        b2Vec2 p = b2Vec2(0.0f, 0.0f);
        b2Vec2 s = b2Vec2(3.0f, 0.2f);
        b2Body* body4 = createBox(p, s, pWorld, b2_dynamicBody, 0.0f);
        PortalBody* pb = pWorld->createPortalBody(body4);
        thinDrawer->debug_addPortalBody(pb);
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
                                    (getRand() + sizeM) / div, pWorld, b2_dynamicBody);
            PortalBody* pb = pWorld->createPortalBody(b);
            thinDrawer->debug_addPortalBody(pb);
        }
        for (int i = 0; i < numPolygons; i++) {
            b2Body* b = createBox(b2Vec2(getRand() * xSize * 1.9f, getRand() * ySize * 1.9f),
                                b2Vec2(((getRand() + sizeM) / div), (getRand() + sizeM) / div), pWorld, b2_dynamicBody);
            PortalBody* pb = pWorld->createPortalBody(b);
            thinDrawer->debug_addPortalBody(pb);
        }

        for (int i = 0; i < 10; i++) {
            b2Body* b1 = createCircle(b2Vec2(getRand() * xSize * 1.0f, 10.0f),
                                    (getRand() + sizeM) / div, pWorld, b2_dynamicBody);
            PortalBody* pb = pWorld->createPortalBody(b1);
            thinDrawer->debug_addPortalBody(pb);

            b2Body* b2 = createBox(b2Vec2(getRand() * xSize * 1.0f, 10.0f),
                                b2Vec2(((getRand() + sizeM) / div), (getRand() + sizeM) / div), pWorld, b2_dynamicBody);
            pb = pWorld->createPortalBody(b2);
            thinDrawer->debug_addPortalBody(pb);
        }
    }
};
