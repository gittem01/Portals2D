#include <BaseTestCase.h>

class TestCase4 : public BaseTestCase
{
public:
    TestCase4(WindowPainter* wh) : BaseTestCase(wh){
        testCaseItem = 3;
        
        beforeCreate();
        onCreate();
        afterCreate();
    }

    void onCreate(){
        b2Vec2 gravity(0.0f, 0.0f);
        pWorld->SetGravity(gravity);

        float yPos = -4.0f;
        float portalSize = 3.0f;
        float d = 0.3f;
        const int n = 50;
        float bias = n * d * 0.5f;

        Portal* linePortals[n];

        createEdge(b2Vec2(-100.0f, yPos - portalSize), b2Vec2(+100.0f, yPos - portalSize), pWorld, b2_staticBody);

        b2Body* body2 = createCircle(b2Vec2(-n * d - 2.0f + bias, yPos + 1.5f), 1, pWorld, b2_dynamicBody);
        b2Body* body3 = createCircle(b2Vec2(d + 2.0f + bias, yPos + 1.5f), 1, pWorld, b2_dynamicBody);

        b2Body* body4 = createCircle(b2Vec2((-n * d - 7.0f + bias) * 0.5f + 1.48f, yPos + 6.0f), 1, pWorld, b2_dynamicBody);
        b2Body* body5 = createCircle(b2Vec2(d + 2.5f + bias, yPos + 6.0f), 1, pWorld, b2_dynamicBody);

        body2->SetBullet(true);
        body4->SetBullet(true);

        b2Vec2 vel(3, 0);
        body2->SetLinearVelocity(vel);
        body4->SetLinearVelocity(vel);

        PortalBody* pb = pWorld->createPortalBody(body2);
        renderer->addPortalBody(pb, b2Color(1.0f, 1.0f, 0.0f, 0.5f));

        pb = pWorld->createPortalBody(body3);
        renderer->addPortalBody(pb, b2Color(1.0f, 0.0f, 1.0f, 0.5f));

        body3->SetLinearVelocity(-vel);
        body5->SetLinearVelocity(-vel);

        float p = -n * d + bias;
        for (int i = 0; i < n; i++){
            Portal* portal = pWorld->createPortal(b2Vec2(p, yPos), b2Vec2(+1.0f, +0.0f), portalSize);
            linePortals[i] = portal;
            p += d;
        }
        Portal* endPortal1 = pWorld->createPortal(b2Vec2(p + 3.0f, yPos), b2Vec2(-1.0f, +0.0f), portalSize);
        Portal* endPortal2 = pWorld->createPortal(b2Vec2(-n * d + bias - 3.0f, yPos), b2Vec2(+1.0f, +0.0f), portalSize);
        endPortal1->connect(endPortal1);
        endPortal2->connect(endPortal2);

        for (int i = 0; i < n; i+=2){
            linePortals[i]->connect(linePortals[i + 1], true, (i + 1) % 2, 0);
        }
    }
};
