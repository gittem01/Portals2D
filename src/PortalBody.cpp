#include "PortalBody.h"
#include "Portal.h"
#include "PortalWorld.h"
#include "glm/common.hpp"

#define CIRCLE_POINTS 50

PortalBody::PortalBody(b2Body* bBody, PortalWorld* pWorld, b2Color bodyColor){
    this->pWorld = pWorld;
    this->body = bBody;
    this->bodyMaps = new std::vector<bodyCollisionStatus*>();

    bodyData* bd = (bodyData*)malloc(sizeof(bodyData));
    bd->data = this;
    bd->type = PORTAL_BODY;
    body->GetUserData().pointer = (uintptr_t)bd;

    this->bodyColor = bodyColor;

    for (b2Fixture* fix = body->GetFixtureList(); fix; fix = fix->GetNext()){
        fixtureCollisions[fix] = new std::set<portalCollision*>();
        allParts[fix] = new std::vector<std::vector<b2Vec2>*>();
    }
}

PortalBody::~PortalBody(){
    for (b2Fixture* fix = body->GetFixtureList(); fix; fix = fix->GetNext()){
        for (Portal* p : pWorld->portals){
            for (int i = 0; i < 2; i++){
                p->collidingFixtures[i].erase(fix);
                p->releaseFixtures[i].erase(fix);
            }
        }
    }

    for (int i = 0; i < bodyMaps->size(); i++){
        bodyCollisionStatus* bodyCollStatus = bodyMaps->at(i);
        for (auto iter = bodyCollStatus->body->bodyMaps->begin(); iter != bodyCollStatus->body->bodyMaps->end(); iter++){
            if ((*iter)->body == this){
                delete *iter;
                bodyCollStatus->body->bodyMaps->erase(iter--);
            }
        }
        //free(bodyCollStatus);
    }
    //delete bodyMaps;

    for (b2Fixture* fix = body->GetFixtureList(); fix; fix = fix->GetNext()){
        for (auto vec : *allParts[fix]){
            delete vec;
        }
        delete allParts[fix];
        
        for (portalCollision* coll : *fixtureCollisions[fix]){
            //free(coll);
        }
        //delete fixtureCollisions[fix];
    }

    uintptr_t bData = body->GetUserData().pointer;
    free((void*)bData);

    pWorld->world->DestroyBody(body);
}

void PortalBody::collisionBegin(b2Contact* contact, b2Fixture* fix1, b2Fixture* fix2){
    bodyData* bData = (bodyData*)fix2->GetBody()->GetUserData().pointer;
    
    bool shouldCollide = this->shouldCollide(contact, fix1, fix2, bData);
    if (!shouldCollide){
        return;
    }

    if (bData && bData->type == PORTAL){
        Portal* p = (Portal*)bData->data;
        int out = p->collisionBegin(contact, fix2, fix1);
        handleOut(fix1, p, out);
    }
}

void PortalBody::collisionEnd(b2Contact* contact, b2Fixture* fix1, b2Fixture* fix2){
    bodyData* bData = (bodyData*)fix2->GetBody()->GetUserData().pointer;
    
    bool shouldCollide = this->shouldCollide(contact, fix1, fix2, bData);
    if (!shouldCollide){
        return;
    }

    if (bData && bData->type == PORTAL){
        Portal* p = (Portal*)bData->data;
        int out = p->collisionEnd(contact, fix2, fix1);
        handleOut(fix1, p, out);
    }
}

void PortalBody::preCollision(b2Contact* contact, b2Fixture* fix1, b2Fixture* fix2){
    bodyData* bData = (bodyData*)fix2->GetBody()->GetUserData().pointer;

    bool sc = shouldCollide(contact, fix1, fix2, bData);
    if (!sc){
        contact->SetEnabled(false);
        return;
    }

    if (bData && bData->type == PORTAL){
        Portal* portal = (Portal*)bData->data;
        int out = portal->preCollision(contact, fix2, fix1);
        handleOut(fix1, portal, out);
    }
}

bool PortalBody::shouldCollide(b2Contact* contact, b2Fixture* fix1, b2Fixture* fix2, bodyData* bData){
    std::set<portalCollision*>* collidingPortals = fixtureCollisions[fix1];
    if (bData && bData->type == PORTAL){
        Portal* portal = (Portal*)bData->data;
        for (portalCollision* coll : *collidingPortals){
            if (portal == coll->portal) return true;
        }
    }
    
    for (portalCollision* coll : *collidingPortals){
        if (coll->portal->releaseFixtures[coll->side].find(fix2) != coll->portal->releaseFixtures[coll->side].end() ||
            coll->portal->collidingFixtures[coll->side].find(fix2) != coll->portal->collidingFixtures[coll->side].end())
        {
            return true;
        }

        if (coll->status == 0) return false;
        bool shouldCollide = coll->portal->shouldCollide(contact, fix1, fix2, coll);
        if (!shouldCollide) return false;
    }

    if (fix2->GetBody()->GetType() == b2_staticBody && !fix2->IsSensor() && prepareMaps.find(fix1) != prepareMaps.end()){
        auto mapsFix = prepareMaps[fix1];
        for (auto iter = mapsFix.begin(); iter != mapsFix.end(); iter++){
            (*iter)->prepareCollisionCheck(contact, fix1, fix2);
        }
    }

    return true;
}

void PortalBody::outHelper(b2Fixture* fix, Portal* portal, int status, int side){
    for (auto& col : *fixtureCollisions[fix]){
        if (col->portal == portal){
            fixtureCollisions[fix]->erase(col);
            free(col);
            break;
        }
    }
    portalCollision* portCol = (portalCollision*)malloc(sizeof(portalCollision));
    portCol->portal = portal;
    portCol->status = status;
    portCol->side = side;
    fixtureCollisions[fix]->insert(portCol);
}

void PortalBody::postHandle(){
    for (bodyStruct* s : createBodies){
        createCloneBody(s);
        free(s);
    }

    body->ApplyForceToCenter(body->GetMass() * -pWorld->world->GetGravity(), true);

    for (b2Fixture* fix = body->GetFixtureList(); fix; fix = fix->GetNext()){
        calculateParts(fix);
    }
    createBodies.clear();
}

bool PortalBody::shouldCreate(b2Body* bBody, Portal* portal, int side){
    for (bodyCollisionStatus* s : *bodyMaps){
        if (s->connection->portal1 == portal && s->connection->side1 == side)
            return false;
    }

    for (bodyStruct* bs : createBodies){
        if (bs->collPortal == portal && bs->side == side)
            return false;
    }

    return true;
}

void PortalBody::handleOut(b2Fixture* fix, Portal* portal, int out){
    switch (out)
    {
    case 0:
        if (shouldCreate(fix->GetBody(), portal, 0)){
            bodyStruct* s = (bodyStruct*)malloc(sizeof(bodyStruct));
            *s = {fix->GetBody(), portal, 0};
            createBodies.push_back(s);
        }
        outHelper(fix, portal, 1, 0);
        break;
    case 1:
        if (shouldCreate(fix->GetBody(), portal, 1)){
            bodyStruct* s = (bodyStruct*)malloc(sizeof(bodyStruct));
            *s = {fix->GetBody(), portal, 1};
            createBodies.push_back(s);
        }
        outHelper(fix, portal, 1, 1);
        break;
    
    case 2:
        outHelper(fix, portal, 0, 0);
        break;
    case 3:
        outHelper(fix, portal, 0, 1);
        break;

    case 5:
        prepareMaps[fix].insert(portal);
        break;
    case 6:
        prepareMaps[fix].erase(portal);
        break;

    case 4:
        std::set<portalCollision*>* coll = fixtureCollisions[fix];
        for (auto& a : *coll){
            if (a->portal == portal && a->status == 1){
                coll->erase(a);
                free(a);
                break;
            }
        }
        destroyCheck(fix, portal);
        break;
    }
}

void PortalBody::destroyCheck(b2Fixture* f, Portal* portal){
    for (b2Fixture* fix = f->GetBody()->GetFixtureList(); fix; fix = fix->GetNext()){
        if (fix == f) continue;
        for (portalCollision* coll : *fixtureCollisions[fix]){
            if (coll->portal == portal){
                return;
            }
        }
    }

    for (bodyCollisionStatus* s : *bodyMaps){
        if (s->connection->portal1 == portal){
            pWorld->destroyBodies.insert(s->body);
        }
    }
}

void rotateVec(b2Vec2* vec, float angle){
    float x = cos(angle) * vec->x - sin(angle) * vec->y;
    float y = sin(angle) * vec->x + cos(angle) * vec->y;
    vec->x = x;
    vec->y = y;
}

b2Vec2 rotateVec(b2Vec2 vec, float angle){
    float x = cos(angle) * vec.x - sin(angle) * vec.y;
    float y = sin(angle) * vec.x + cos(angle) * vec.y;

    return {x, y};
}

void PortalBody::createCloneBody(bodyStruct* s){
    int side = s->side;
    Portal* collPortal = s->collPortal;
    b2Body* body1 = s->body;
    b2Vec2 dir1 = side ? -collPortal->dir : collPortal->dir;
    b2Vec2 posDiff = collPortal->pos - body1->GetPosition();
    b2Vec2 speed = body1->GetLinearVelocity();

    for (portalConnection* c : collPortal->connections[side]){
        Portal* portal2 = c->portal2;
        
        b2Vec2 dir2 = c->side2 ? -portal2->dir : portal2->dir;
        float angleRot = -pWorld->calcAngle2(dir1) + pWorld->calcAngle2(-dir2);

        b2Vec2 localPos = rotateVec(posDiff, angleRot + b2_pi);
        if (c->isReversed)
            localPos = -localPos - 2.0f * (b2Dot(-localPos, dir2)) * dir2;

        b2BodyDef def;
        def.type = b2_dynamicBody;
        def.position = portal2->pos + localPos;
        
        def.linearDamping = body1->GetLinearDamping();
        def.angularDamping = body1->GetAngularDamping();
        def.bullet = body1->IsBullet();

        b2Body* body2 = pWorld->world->CreateBody(&def);

        bodyData* bd = (bodyData*)malloc(sizeof(bodyData));
        bd->data = this;
        bd->type = PORTAL_BODY;
        body2->GetUserData().pointer = (uintptr_t)bd;

        PortalBody* npb = pWorld->createPortalBody(body2, bodyColor);
        
        bodyCollisionStatus* bcs = new bodyCollisionStatus;
        bcs->body = this;
        npb->bodyMaps->push_back(bcs);

        for (portalConnection* conn : portal2->connections[c->side2]){
            if (conn->portal2 == collPortal && conn->side2 == c->side1){
                bcs->connection = conn;
                break;
            }
        }

        bodyCollisionStatus* t_bcs = new bodyCollisionStatus;
        t_bcs->body = npb;
        this->bodyMaps->push_back(t_bcs);
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

            for (Portal* portal : pWorld->portals){
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
        }

        if (allOut) pWorld->destroyBodies.insert(this);
        else connectBodies(body1, body2, c, side);
    }
}

void PortalBody::connectBodies(b2Body* body1, b2Body* body2, portalConnection* connection, int side) {
    b2PrismaticJointDef prismDef;
    prismDef.Initialize(body1, body2, b2Vec2(0.0f, 0.0f), b2Vec2(0.0f, 0.0f));
    prismDef.collideConnected = true;
    prismDef.maxMotorForce = 0.0f;

    pWorld->world->CreateJoint(&prismDef);

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
    pWorld->world->CreateJoint(&pulleyDef);

    rotateVec(&dirClone1, b2_pi / 2.0f);
    if (connection->isReversed)
        rotateVec(&dirClone2, -b2_pi / 2.0f);
    else
        rotateVec(&dirClone2, b2_pi / 2.0f);

    anchor1 = center1;
    anchor2 = center2;
    groundAnchor1 = b2Vec2(dirClone1.x * mult, dirClone1.y * mult);
    groundAnchor2 = b2Vec2(dirClone2.x * mult, dirClone2.y * mult);
    pulleyDef.Initialize(body1, body2, groundAnchor1, groundAnchor2, anchor1, anchor2, 1.0f);
    pWorld->world->CreateJoint(&pulleyDef);
}

void PortalBody::drawBodies(){
    for (b2Fixture* fix = body->GetFixtureList(); fix != nullptr; fix = fix->GetNext()){
        if (fix->GetType() == b2Shape::Type::e_polygon){
            drawPolygonFix(fix);
        }
        else if (fix->GetType() == b2Shape::Type::e_circle){
            drawCircleFix(fix);
        }
    }
}

void PortalBody::adjustVertices(std::vector<b2Vec2>* vertices, std::vector<b2Vec2>* retVertices1,
                                std::vector<b2Vec2>* retVertices2, Portal* p, int side)
{
    int pSide;
    int lastSide = p->getPointSide(vertices->at(0));

    for (int i = 0; i < vertices->size() + 1; i++){
        int vertexIndex = i % vertices->size();
        int vertexBefore = i ? i - 1 : 0;
        pSide = p->getPointSide(vertices->at(vertexIndex));

        if (pSide == side && lastSide == 1 - side){
            b2Vec2 intersectionPoint = getLineIntersection(p->points[0], p->points[1], vertices->at(i - 1), vertices->at(vertexIndex));
            retVertices1->push_back(intersectionPoint);
            retVertices2->push_back(intersectionPoint);
        }
        else if (pSide == 1 - side && lastSide == side){
            b2Vec2 intersectionPoint = getLineIntersection(p->points[0], p->points[1], vertices->at(i - 1), vertices->at(vertexIndex));
            retVertices1->push_back(intersectionPoint);
            retVertices2->push_back(intersectionPoint);
        }

        if (pSide == side && i != vertices->size()){
            retVertices1->push_back(vertices->at(vertexIndex));
        }
        else if (i != vertices->size()){
            retVertices2->push_back(vertices->at(vertexIndex));
        }

        lastSide = pSide;
    }
}

float PortalBody::getArea(b2Fixture* fix, int status){
    auto apf = allParts[fix];

    float baseArea = 0.0f;
    float finalArea = 0.0f;
    float baseDensity = fix->GetDensity();
    if (fix->GetType() == b2Shape::Type::e_polygon){
        b2PolygonShape* pShape = (b2PolygonShape*)fix->GetShape();
        for (int i = 0; i < pShape->m_count; i++){
            baseArea += pShape->m_vertices[i].x * pShape->m_vertices[(i + 1) % pShape->m_count].y;
            baseArea -= pShape->m_vertices[(i + 1) % pShape->m_count].x * pShape->m_vertices[i].y;
        }
        baseArea *= 0.5f;
    }
    else{
        b2CircleShape* cShape = (b2CircleShape*)fix->GetShape();
        baseArea = pow(cShape->m_radius, 2) * b2_pi;
    }

    if (status == 2) return baseArea;

    std::vector<b2Vec2>* vecs = apf->at(0);

    for (int i = 0; i < vecs->size(); i++){
        b2Vec2 v0 = vecs->at(i);
        b2Vec2 v1 = vecs->at((i + 1) % vecs->size());

        finalArea += v0.x * v1.y;
        finalArea -= v1.x * v0.y;
    }
    finalArea /= 2.0f;
    
    return finalArea;
}

b2Vec2 findCentroid(std::vector<b2Vec2>* vecs){
    b2Vec2 off = vecs->at(0);
    float twicearea = 0;
    float x = 0;
    float y = 0;
    b2Vec2 p1, p2;
    float f;
    for (int i = 0, j = vecs->size() - 1; i < vecs->size(); j = i++) {
        p1 = vecs->at(i);
        p2 = vecs->at(j);
        f = (p1.x - off.x) * (p2.y - off.y) - (p2.x - off.x) * (p1.y - off.y);
        twicearea += f;
        x += (p1.x + p2.x - 2 * off.x) * f;
        y += (p1.y + p2.y - 2 * off.y) * f;
    }

    f = twicearea * 3;

    return b2Vec2(x / f + off.x, y / f + off.y);
}

b2Vec2 PortalBody::getCenterOfMass(b2Fixture* fix, int status){
    auto apf = allParts[fix];
    if (apf->size() == 0 || status == 0) return b2Vec2();

    std::vector<b2Vec2>* vecs = apf->at(0);
    if (vecs->size() == 0) return b2Vec2();

    float area = getArea(fix, status);

    b2Vec2 center = findCentroid(vecs);

    b2Vec2 force = area * fix->GetDensity() * pWorld->world->GetGravity();

    fix->GetBody()->ApplyForce(((float)bodyMaps->size() + 1) * force, center, true);

    return center;
}

void PortalBody::calculateParts(b2Fixture* fix){
    std::map<b2Fixture*, std::set<portalCollision*>*>::iterator fixIter = fixtureCollisions.find(fix);
    std::set<portalCollision*>::iterator setIter = (*fixIter).second->begin();

    std::vector<b2Vec2>* vertices = new std::vector<b2Vec2>;

    int status = 2;
    for (portalCollision* coll : *(*fixIter).second){
        if (coll->status == 0){
            status = 0;
            break;
        }
        else{
            status = 1;
        }
    }

    if (fix->GetType() == b2Shape::Type::e_polygon){
        b2PolygonShape* shape = (b2PolygonShape*)fix->GetShape();
        for (int i = 0; i < shape->m_count; i++) {
            b2Vec2 p = b2Vec2((shape->m_vertices + i)->x, (shape->m_vertices + i)->y);
            p = fix->GetBody()->GetWorldPoint(p);
            vertices->push_back(p);
        }
    }
    else{
        b2CircleShape* shape = (b2CircleShape*)fix->GetShape();
        float r = shape->m_radius;

        float angle = 0;
        for (int i = 0; i < CIRCLE_POINTS; i++) {
            float angle0 = -((float)i / CIRCLE_POINTS) * b2_pi * 2;
            b2Vec2 p = b2Vec2(sin(angle0) * r, cos(angle0) * r) + shape->m_p;
            p = fix->GetBody()->GetWorldPoint(p);
            vertices->push_back(p);
        }
    }

    std::vector<std::vector<b2Vec2>*>* apf = allParts[fix];

    for (std::vector<b2Vec2>* vec : *apf){
        delete vec;
    }

    apf->clear();

    if (status == 2){
        apf->push_back(vertices);
        getCenterOfMass(fix, status);
        return;
    }

    std::vector<b2Vec2>* drawVecs = new std::vector<b2Vec2>;
    apf->push_back(drawVecs);
    
    if (setIter != (*fixIter).second->end()){
        for(;;){
            if (vertices->size() == 0) break;

            std::vector<b2Vec2>* releaseVecs = new std::vector<b2Vec2>;

            adjustVertices(vertices, drawVecs, releaseVecs, (*setIter)->portal, (*setIter)->side);

            vertices->clear();
            for (b2Vec2 v : *drawVecs){
                vertices->push_back(v);
            }
            if (releaseVecs->size() != 0)
                apf->push_back(releaseVecs);
            
            setIter++;
            if (setIter == (*fixIter).second->end()) break;

            drawVecs->clear();
        }
    }
    delete(vertices);

    getCenterOfMass(fix, status);
}

void PortalBody::portalRender(b2Fixture* fix, std::vector<b2Vec2>& vertices){
    b2Body* bBody = fix->GetBody();

    int side;
    int renderStatus = 2;

    std::map<b2Fixture*, std::set<portalCollision*>*>::iterator fixIter = fixtureCollisions.find(fix);
    std::set<portalCollision*>::iterator iter = (*fixIter).second->begin();
    
    void* portal = NULL;
    int s = -1;
    if (iter != (*fixIter).second->end()){
        portal = (*iter)->portal;
        s = (*iter)->side;
        renderStatus = (*iter)->status;
    }
    for (portalCollision* coll : *(*fixIter).second){
        if (coll->status == 0){
            renderStatus = 0;
            break;
        }
        else{
            renderStatus = 1;
        }
    }

    if (renderStatus == 1){
        if (allParts[fix]->at(0)->size() > 0){
            drawVertices(bBody, *allParts[fix]->at(0));
        }
        if (pWorld->drawReleases){
            for (int i = 1; i < allParts[fix]->size(); i++){
                auto& vecs = allParts[fix]->at(i);
                b2Color oldColor = bodyColor;
                bodyColor = pWorld->releaseColor;
                drawVertices(bBody, *vecs);
                bodyColor = oldColor;
            }
        }
    }
    else if (pWorld->drawReleases && renderStatus == 0){
            b2Color oldColor = bodyColor;
            bodyColor = pWorld->releaseColor;
            drawVertices(bBody, vertices);
            bodyColor = oldColor;
    }

    else if (renderStatus == 2) drawVertices(bBody, *allParts[fix]->at(0));
}

void PortalBody::drawPolygonFix(b2Fixture* fix){
    b2Body* bBody = fix->GetBody();
    b2PolygonShape* shape = (b2PolygonShape*)fix->GetShape();

    int vertexCount = shape->m_count;
    std::vector<b2Vec2> vertices;

	for (int i = 0; i < vertexCount; i++) {
        b2Vec2 p = b2Vec2((shape->m_vertices + i)->x, (shape->m_vertices + i)->y);
        p = bBody->GetWorldPoint(p);
        vertices.push_back(p);
	}

    portalRender(fix, vertices);
}

void PortalBody::drawCircleFix(b2Fixture* fix){
    b2Body* bBody = fix->GetBody();
    b2CircleShape* shape = (b2CircleShape*)fix->GetShape();
    float r = shape->m_radius;

    std::vector<b2Vec2> vertices;

    float angle = 0;
	for (int i = 0; i < CIRCLE_POINTS; i++) {
        float angle0 = ((float)i / CIRCLE_POINTS) * b2_pi * 2;
        b2Vec2 p = b2Vec2(sin(angle0) * r, cos(angle0) * r) + shape->m_p;
        p = body->GetWorldPoint(p);
        vertices.push_back(p);
	}

    portalRender(fix, vertices);
}

void PortalBody::drawVertices(b2Body* bBody, std::vector<b2Vec2>& vertices){
    float transparency = 1.0f;
    if (!bBody->IsAwake()){
        transparency /= 1.5f;
    }
    
    glColor4f(bodyColor.r, bodyColor.g, bodyColor.b, bodyColor.a * transparency);
	glBegin(GL_POLYGON);
	for (int i = 0; i < vertices.size(); i++) {
		glVertex2d(vertices.at(i).x, vertices.at(i).y);
	}
	glEnd();

	glLineWidth(1.0f);
	glColor4f(bodyColor.r, bodyColor.g, bodyColor.b, bodyColor.a * 2.0f);
	glBegin(GL_LINES);
	for (int i = 0; i < vertices.size(); i++) {
		glVertex2d(vertices.at(i).x, vertices.at(i).y);
		glVertex2d(vertices.at((i + 1) % vertices.size()).x, vertices.at((i + 1) % vertices.size()).y);
	}
	glEnd();
}

b2Vec2 PortalBody::getLineIntersection(b2Vec2 p1, b2Vec2 p2, b2Vec2 p3, b2Vec2 p4){
    float d = (p1.x - p2.x) * (p3.y - p4.y) - (p1.y - p2.y) * (p3.x - p4.x);
    float up1 = (p1.x * p2.y - p1.y * p2.x) * (p3.x - p4.x) - (p1.x - p2.x) * (p3.x * p4.y - p3.y * p4.x);
    float up2 = (p1.x * p2.y - p1.y * p2.x) * (p3.y - p4.y) - (p1.y - p2.y) * (p3.x * p4.y - p3.y * p4.x);

    return b2Vec2(up1 / d, up2 / d);
}