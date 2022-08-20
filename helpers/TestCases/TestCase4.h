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
        float d = 0.65f;
        const int n = 30;
        float bias = n * d * 0.5f;

        Portal* linePortals[n];

        createEdge(b2Vec2(-100.0f, yPos - portalSize), b2Vec2(+100.0f, yPos - portalSize), pWorld, b2_staticBody);

        b2Body* body2 = createCircle(b2Vec2(-n * d - 2.0f + bias, yPos - 1.5f), 1, pWorld, b2_dynamicBody);
        b2Body* body3 = createCircle(b2Vec2(2.0f + bias, yPos + 1.5f), 1, pWorld, b2_dynamicBody);

        body2->SetLinearDamping(0.0f);
        body3->SetLinearDamping(0.0f);

        b2Vec2 vel(3, 0);
        body2->SetLinearVelocity(vel);

        PortalBody* pb2 = pWorld->createPortalBody(body2);
        renderer->addPortalBody(pb2, b2Color(1.0f, 1.0f, 0.0f, 0.5f));

        PortalBody* pb3 = pWorld->createPortalBody(body3);
        renderer->addPortalBody(pb3, b2Color(1.0f, 0.0f, 1.0f, 0.5f));

        float p = -n * d + bias;
        for (int i = 0; i < n; i++){
            Portal* portal = pWorld->createPortal(b2Vec2(p, yPos), b2Vec2(+1.0f, +0.0f), portalSize);
            linePortals[i] = portal;
            p += d;
        }

        for (int i = 0; i + 1 < n; i += 2){
            linePortals[i]->connect(linePortals[i + 1], true, 1, 0);
        }

        for (int i = 1; i + 1 < n; i += 2){
            linePortals[i]->connect(linePortals[i + 1], true, 1, 0);
        }
        
        createEdge(b2Vec2(p + d, yPos + portalSize), b2Vec2((-n - 2) * d + bias, yPos + portalSize), pWorld, b2_staticBody);

        const int iter = 120;
        for (int i = 0; i < iter; i++){
            pWorld->PortalStep(1.0f / iter, 8, 3);
        }
        Portal* endPortal1 = pWorld->createPortal(b2Vec2(p, yPos), b2Vec2(-1.0f, +0.0f), portalSize);
        Portal* endPortal2 = pWorld->createPortal(b2Vec2((-n - 1) * d + bias, yPos), b2Vec2(+1.0f, +0.0f), portalSize);

        endPortal1->connect(endPortal2);
        
        endPortal1->connect(linePortals[n - 1], true, 1, 1);
        endPortal2->connect(linePortals[0], true, 1, 0);

        body3->SetLinearVelocity(-vel);
        for (PortalBody* portalBody : *pb2->worldIndex){
            portalBody->body->SetLinearVelocity(b2Vec2(0.0f, 0.0f));
        }
        for (int i = 0; i < iter; i++){
            pWorld->PortalStep(1.0f / iter, 8, 3);
        }
        for (PortalBody* portalBody : *pb2->worldIndex){
            portalBody->body->SetLinearVelocity(vel);
        }
        
        Portal* endPortal3 = pWorld->createPortal(b2Vec2(p + d, yPos), b2Vec2(-1.0f, +0.0f), portalSize);
        Portal* endPortal4 = pWorld->createPortal(b2Vec2((-n - 2) * d + bias, yPos), b2Vec2(+1.0f, +0.0f), portalSize);
        endPortal3->connect(endPortal4);

        renderer->sizeMult = 0.6f;
    }
};
