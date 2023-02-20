#include <BaseTestCase.h>

class TestCase7 : public BaseTestCase
{
public:
    TestCase7(ThinDrawer* td) : BaseTestCase(td) {
        testCaseItem = 6;
        
        beforeCreate();
        onCreate();
        afterCreate();
    }

    void onCreate(){
        b2Vec2 gravity(0.0f, -40.00f);
        pWorld->SetGravity(gravity);

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

        createEdge(b2Vec2(-roomWidth * 2 - 1, bottomPos), b2Vec2(-roomWidth * 2 - 1, roomHeight + bottomPos), pWorld, b2_staticBody);
        createEdge(b2Vec2(-1, bottomPos), b2Vec2(-1, roomHeight + bottomPos), pWorld, b2_staticBody);

        createEdge(b2Vec2(+roomWidth * 2 + 1, bottomPos), b2Vec2(+roomWidth * 2 + 1, roomHeight + bottomPos), pWorld, b2_staticBody);
        createEdge(b2Vec2(+1, bottomPos), b2Vec2(+1, roomHeight + bottomPos), pWorld, b2_staticBody);

        for (int i = 0; i < numOfCubes; i++){
            float sizex = getRand() * 0.5f + 0.8f;
            float sizey = getRand() * 0.5f + 0.8f;
            b2Body* b = createBox(b2Vec2(-roomWidth - 1, 0.0f), b2Vec2(sizex, sizey), pWorld, b2_dynamicBody);
            PortalBody* pb = pWorld->createPortalBody(b);
            thinDrawer->addPortalBody(pb);
        }

        for (int i = 0; i < numOfCircles; i++){
            float size = getRand() * 0.5f + 0.8f;
            b2Body* b = createCircle(b2Vec2(-roomWidth - 1, 0.0f), size, pWorld, b2_dynamicBody);
            PortalBody* pb = pWorld->createPortalBody(b);
            thinDrawer->addPortalBody(pb);
        }

        b2Vec2 p = b2Vec2(roomWidth + 1.0f, +9.4f);
        b2Vec2 s = b2Vec2(roomWidth * 0.75f, 0.3f);
        b2Body* body4 = createBox(p, s, pWorld, b2_dynamicBody, 0.0f);
        PortalBody* pb = pWorld->createPortalBody(body4);
        thinDrawer->addPortalBody(pb);
        b2Vec2 vel(0.0f, 0.0f);
        body4->SetLinearVelocity(vel);
        body4->SetAngularVelocity(2.0f);
        body4->SetBullet(true);
        body4->SetLinearDamping(0.0f);
    }
};
