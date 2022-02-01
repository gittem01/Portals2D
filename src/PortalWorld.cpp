#include "PortalWorld.h"
#include "PortalBody.h"
#include "Portal.h"

PortalWorld::PortalWorld(b2World* world, DebugDrawer* drawer){
    this->world = world;
    this->drawer = drawer;

    this->drawReleases = false;
    this->releaseColor = b2Color(1.0f, 1.0f, 1.0f, 0.2f);
}

Portal* PortalWorld::createPortal(b2Vec2 pos, b2Vec2 dir, float size){
    Portal* portal = new Portal(pos, dir, size, this);
    portals.push_back(portal);

    return portal;
}

PortalBody* PortalWorld::createPortalBody(b2Body* body, b2Color bodyColor){
    PortalBody* pBody = new PortalBody(body, this, bodyColor);
    portalBodies.push_back(pBody);

    createPortalBody_i(pBody, NULL, true);

    return pBody;
}

void PortalWorld::createPortalBody_i(PortalBody* pBody, PortalBody* baseBody, bool isNew){
    if (isNew){
        std::vector<PortalBody*>* pbIndices = new std::vector<PortalBody*>();
        pbIndices->push_back(pBody);
        bodyIndices.push_back(pbIndices);
        pBody->worldIndex = pbIndices;
    }
    else{
        baseBody->worldIndex->push_back(pBody);
        pBody->worldIndex = baseBody->worldIndex;
    }
}

void PortalWorld::portalUpdate(){
    for (int i = 0; i < portalBodies.size(); i++){
        PortalBody* body = portalBodies.at(i);
        body->postHandle();
    }

    for (Portal* p : portals) {
        p->postHandle();
    }

    globalPostHandle();
}

void PortalWorld::drawUpdate(){
    for (PortalBody* body : portalBodies){
        body->drawBodies();
    }
    for (Portal* p : portals) {
        p->draw();
    }
}

void PortalWorld::globalPostHandle(){
    for (PortalBody* b : destroyBodies){
        for (int i = 0 ; i < portalBodies.size(); i++){
            if (portalBodies.at(i) == b){
                portalBodies.erase(portalBodies.begin() + i);
                break;
            }
        }
        delete b;
    }

    destroyBodies.clear();
}

b2Vec2 rotateVec(b2Vec2 vec, float angle){
    float x = cos(angle) * vec.x - sin(angle) * vec.y;
    float y = sin(angle) * vec.x + cos(angle) * vec.y;

    return {x, y};
}

std::vector<PortalBody*> PortalWorld::createCloneBody(bodyStruct* s){
    PortalBody* pBody = s->pBody;
    int side = s->side;
    Portal* collPortal = s->collPortal;
    b2Body* body1 = s->pBody->body;
    b2Vec2 dir1 = side ? -collPortal->dir : collPortal->dir;
    b2Vec2 posDiff = collPortal->pos - body1->GetPosition();
    b2Vec2 speed = body1->GetLinearVelocity();

    std::vector<PortalBody*> retBodies;

    for (portalConnection* c : collPortal->connections[side]){
        Portal* portal2 = c->portal2;
        
        b2Vec2 dir2 = c->side2 ? -portal2->dir : portal2->dir;
        float angleRot = -calcAngle2(dir1) + calcAngle2(-dir2);

        b2Vec2 localPos = rotateVec(posDiff, angleRot + b2_pi);
        if (c->isReversed)
            localPos = -localPos - 2.0f * (b2Dot(-localPos, dir2)) * dir2;

        b2BodyDef def;
        def.type = b2_dynamicBody;
        def.position = portal2->pos + localPos;
        def.fixedRotation = body1->IsFixedRotation();
        def.linearDamping = body1->GetLinearDamping();
        def.angularDamping = body1->GetAngularDamping();
        def.bullet = body1->IsBullet();

        b2Body* body2 = world->CreateBody(&def);
        
        PortalBody* npb = new PortalBody(body2, this, pBody->bodyColor);
        portalBodies.push_back(npb);

        retBodies.push_back(npb);

        bodyCollisionStatus* bcs = new bodyCollisionStatus;
        bcs->body = pBody;
        npb->bodyMaps->push_back(bcs);

        for (portalConnection* conn : portal2->connections[c->side2]){
            if (conn->portal2 == collPortal && conn->side2 == c->side1){
                bcs->connection = conn;
                break;
            }
        }

        bodyCollisionStatus* t_bcs = new bodyCollisionStatus;
        t_bcs->body = npb;
        pBody->bodyMaps->push_back(t_bcs);
        t_bcs->connection = c;

        body2->SetLinearVelocity(rotateVec(speed, angleRot));
        body2->SetAngularVelocity(body1->GetAngularVelocity());
        body2->SetTransform(body2->GetPosition(), body1->GetTransform().q.GetAngle());

        bool allOut = true;
        for (b2Fixture* fix = body1->GetFixtureList(); fix; fix = fix->GetNext()){
            b2Fixture* f;
            if (fix->GetType() == b2Shape::Type::e_polygon){
                b2PolygonShape* polyShape = (b2PolygonShape*)fix->GetShape();
                b2FixtureDef fDef;
                fDef.density = fix->GetDensity();
                fDef.restitution = fix->GetRestitution();
                fDef.friction = fix->GetFriction();
                b2PolygonShape newShape;
                fDef.shape = &newShape;

                newShape.m_count = polyShape->m_count;
                b2Vec2* vertices = (b2Vec2*)malloc(sizeof(b2Vec2) * polyShape->m_count);

                for (int i = 0; i < polyShape->m_count; i++){
                    vertices[i] = rotateVec(polyShape->m_vertices[i], angleRot);
                }
                newShape.Set(vertices, polyShape->m_count);
                
                f = body2->CreateFixture(&fDef);

                free(vertices);
            }
            else{
                b2CircleShape* circleShape = (b2CircleShape*)fix->GetShape();
                b2FixtureDef fDef;
                fDef.density = fix->GetDensity();
                fDef.restitution = fix->GetRestitution();
                fDef.friction = fix->GetFriction();
                b2CircleShape newShape;
                fDef.shape = &newShape;

                newShape.m_radius = circleShape->m_radius;
                newShape.m_p = rotateVec(circleShape->m_p, angleRot);
                
                f = body2->CreateFixture(&fDef);
            }

            npb->fixtureCollisions[f] = new std::set<portalCollision*>();
            npb->allParts[f] = new std::vector<std::vector<b2Vec2>*>();

            for (Portal* portal : portals){
                std::vector<b2Vec2> ps = portal->getCollisionPoints(portal->collisionSensor, f);
                if (ps.size() > 0) npb->prepareMaps[f].insert(portal);
            }

            portalCollision* col = (portalCollision*)malloc(sizeof(portalCollision));;

            if (collPortal->collidingFixtures[side].find(fix) != collPortal->collidingFixtures[side].end()){
                bool rayCheck;
                int side = collPortal->getFixtureSide(fix);
                
                if (side == (1 ^ c->side1)) rayCheck = collPortal->rayCheck(fix);
                else rayCheck = 1;

                if (rayCheck){
                    portal2->collidingFixtures[c->side2].insert(f);
                    col->portal = portal2;
                    col->side = c->side2;
                    col->status = 1;
                    npb->fixtureCollisions[f]->insert(col);
                    allOut = false;
                }
                else{
                    free(col);
                }
            }
            else if (collPortal->releaseFixtures[side].find(fix) == collPortal->releaseFixtures[side].end()){
                portal2->releaseFixtures[c->side2].insert(f);
                col->portal = portal2;
                col->side = c->side2;
                col->status = 0;
                npb->fixtureCollisions[f]->insert(col);
            }
            else{
                free(col);
            }
        }
        createPortalBody_i(npb, pBody, false);
        if (allOut) destroyBodies.insert(pBody);
        else connectBodies(body1, body2, c, side);
    }

    return retBodies;
}

void PortalWorld::connectBodies(b2Body* body1, b2Body* body2, portalConnection* connection, int side) {
    b2PrismaticJointDef prismDef;
    prismDef.Initialize(body1, body2, b2Vec2(0.0f, 0.0f), b2Vec2(0.0f, 0.0f));
    prismDef.collideConnected = true;
    prismDef.maxMotorForce = 0.0f;

    world->CreateJoint(&prismDef);

    b2Vec2 dirClone1 = connection->side1 == 0 ? connection->portal1->dir : -connection->portal1->dir;
    b2Vec2 dirClone2 = connection->side2 == 0 ? connection->portal2->dir : -connection->portal2->dir;

    float mult = 100000.0f;

    b2PulleyJointDef pulleyDef;

    b2MassData data1;
    b2MassData data2;
    body1->GetMassData(&data1);
    body2->GetMassData(&data2);
    
    b2Vec2 center1 = body1->GetWorldPoint(data1.center);
    b2Vec2 center2 = body2->GetWorldPoint(data2.center);

    b2Vec2 anchor1 = center1;
    b2Vec2 anchor2 = center2;
    b2Vec2 groundAnchor1(dirClone1.x * mult, dirClone1.y * mult);
    b2Vec2 groundAnchor2(dirClone2.x * mult, dirClone2.y * mult);
    pulleyDef.Initialize(body1, body2, groundAnchor1, groundAnchor2, anchor1, anchor2, 1.0f);
    world->CreateJoint(&pulleyDef);

    dirClone1 = rotateVec(dirClone1, b2_pi / 2.0f);
    if (connection->isReversed)
        dirClone2 = rotateVec(dirClone2, -b2_pi / 2.0f);
    else
        dirClone2 = rotateVec(dirClone2, b2_pi / 2.0f);

    anchor1 = center1;
    anchor2 = center2;
    groundAnchor1 = b2Vec2(dirClone1.x * mult, dirClone1.y * mult);
    groundAnchor2 = b2Vec2(dirClone2.x * mult, dirClone2.y * mult);
    pulleyDef.Initialize(body1, body2, groundAnchor1, groundAnchor2, anchor1, anchor2, 1.0f);
    world->CreateJoint(&pulleyDef);
}

bool PortalWorld::isLeft(b2Vec2& a, b2Vec2& b, b2Vec2& c, float t){
     return ((b.x - a.x)*(c.y - a.y) - (b.y - a.y)*(c.x - a.x)) >= t;
}

float PortalWorld::vecAngle(b2Vec2 v1, b2Vec2 v2){
    return abs(calcAngle2(v1) - calcAngle2(v2));
}

float PortalWorld::getDist(b2Vec2& a, b2Vec2& b, b2Vec2& c){
     return ((b.x - a.x)*(c.y - a.y) - (b.y - a.y)*(c.x - a.x));
}

float PortalWorld::calcAngle2(b2Vec2 vec) {
    float angle = atan2(vec.y, vec.x);
    if (angle < 0) angle += b2_pi * 2.0f;

    return angle;
}

void PortalWorld::normalize(b2Vec2* vec){
    float length = vec->Length();
    if (length == 0) { return; }
    
    vec->x /= length;
    vec->y /= length;
}