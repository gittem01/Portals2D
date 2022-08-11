#include <BaseTestCase.h>

class TestCase3 : public BaseTestCase
{
public:
    TestCase3(WindowPainter* wh) : BaseTestCase(wh){
        testCaseItem = 2;
        
        beforeCreate();
        onCreate();
        afterCreate();
    }

    void onCreate(){
        b2Vec2 gravity(0.0f, -9.81f);
        pWorld->SetGravity(gravity);
        float r = 10.0f;
        const int n = 15; // n * 2 portals will be created
        float anglePlus = glm::radians(0.0f);
        auto circlePortals = (Portal**)malloc(2 * n * sizeof(Portal*));

        for (int i = 0; i < n * 2; i++){
            float angle1 = (float)i * (b2_pi / n) + anglePlus;
            float angle2 = (float)(i + 1) * (b2_pi / n) + anglePlus;
            b2Vec2 p1 = b2Vec2(sin(angle1) * r, cos(angle1) * r);
            b2Vec2 p2 = b2Vec2(sin(angle2) * r, cos(angle2) * r);
            float l = (p1 - p2).Length();
            b2Vec2 pos = 0.5f * (p1 + p2);
            Portal* portal = pWorld->createPortal(pos, -pos, l / 2.0f);
            circlePortals[i] = portal;
        }
        for (int i = 0; i < n; i++){
            circlePortals[i]->connect(circlePortals[n + i]);
        }
        free(circlePortals);

        float sizeM = 0.2f;
        for (int i = 0; i < 150; i++){
            b2Vec2 size = b2Vec2(sizeM + sizeM * (rand() / (float)RAND_MAX), sizeM + sizeM * (rand() / (float)RAND_MAX));
            float r = sizeM + 2.0f * sizeM * (rand() / (float)RAND_MAX);

            b2Body* body1 = createBox(b2Vec2(0, 0), size, pWorld, b2_dynamicBody);
            b2Body* body2 = createCircle(b2Vec2(rand() % 5 + r, rand() % 5), r, pWorld, b2_dynamicBody);

            PortalBody* pb1 = pWorld->createPortalBody(body1);
            PortalBody* pb2 = pWorld->createPortalBody(body2);

            renderer->addPortalBody(pb1, b2Color(0.25f, 1.0f, 0.9f, 0.5f));
            renderer->addPortalBody(pb2, b2Color(1.0f, 0.25f, 0.7f, 0.5f));
        }
    }
};
