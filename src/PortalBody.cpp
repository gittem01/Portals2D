#include "PortalBody.h"
#include "Portal.h"
#include "PortalWorld.h"
#include "glm/common.hpp"

#define RENDER_COLOURFUL 0
#define CIRCLE_POINTS 50
#define ODD_MASK 0b10101010101010101010101010101010

#if RENDER_COLOURFUL
    b2Color colors[] = {
        b2Color(0, 1, 0, 0.5f),
        b2Color(0, 0, 1, 0.5f),
        b2Color(0, 1, 1, 0.5f),
        b2Color(1, 1, 0, 0.5f),
        b2Color(1, 1, 1, 0.2f)
    };
#endif

PortalBody::PortalBody(b2Body* bBody, PortalWorld* pWorld, b2Color bodyColor){
    this->pWorld = pWorld;
    this->body = bBody;
    this->bodyMaps = new std::vector<bodyCollisionStatus*>();

    this->body->SetGravityScale(0.0f);
    this->offsetAngle = 0;
    this->bodyColor = bodyColor;

    bodyData* bd = (bodyData*)malloc(sizeof(bodyData));
    bd->data = this;
    bd->type = PORTAL_BODY;
    bd->extraData = nullptr;
    body->GetUserData().pointer = (uintptr_t)bd;


    this->numFixtures = 0;
    for (b2Fixture* fix = body->GetFixtureList(); fix; fix = fix->GetNext()){
        fixtureCollisions[fix] = new std::set<portalCollision*>();
        allParts[fix] = new std::vector<std::vector<b2Vec2>*>();
        numFixtures++;
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

    for (int i = 0; i < worldIndex->size(); i++){
        if (worldIndex->at(i) == this){
            worldIndex->erase(worldIndex->begin() + i);
            break;
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
        delete bodyCollStatus;
    }
    delete bodyMaps;

    for (b2Fixture* fix = body->GetFixtureList(); fix; fix = fix->GetNext()){
        for (std::vector<b2Vec2>* vec : *allParts[fix]){
            delete vec;
        }
        delete allParts[fix];
        
        for (portalCollision* coll : *fixtureCollisions[fix]){
            free(coll);
        }
        delete fixtureCollisions[fix];
    }

    uintptr_t bData = body->GetUserData().pointer;
    body->GetUserData().pointer = 0;
    free((void*)bData);

    b2JointEdge* jointEdge = body->GetJointList();
    for ( ; jointEdge ; ){
        b2JointEdge* nextEdge = jointEdge->next;
        if (jointEdge->joint->GetType() == e_unknownJoint){
            pWorld->DestroyRotationJoint((RotationJoint*)jointEdge->joint);
        }
        jointEdge = nextEdge;
    }

    pWorld->DestroyBody(body);
}

void PortalBody::collisionBegin(b2Contact* contact, b2Fixture* fix1, b2Fixture* fix2){
    bodyData* bData = (bodyData*)fix2->GetBody()->GetUserData().pointer;
    
    bool shouldCollide = this->shouldCollide(contact, fix1, fix2, bData);
    if (!shouldCollide){
        return;
    }

    if (bData && bData->type == PORTAL){
        Portal* p = (Portal*)bData->data;
        PortalCollisionType out = p->collisionBegin(contact, fix2, fix1);
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
        PortalCollisionType out = p->collisionEnd(contact, fix2, fix1);
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
        PortalCollisionType out = portal->preCollision(contact, fix2, fix1);
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
        if (coll->portal->collidingFixtures[coll->side].find(fix2) != coll->portal->collidingFixtures[coll->side].end() &&
            coll->portal->collidingFixtures[coll->side].find(fix1) != coll->portal->collidingFixtures[coll->side].end())
        {
            return true;
        }

        bool reverse = false;
        auto iter1 = coll->portal->releaseFixtures[coll->side].find(fix1);
        auto iter2 = coll->portal->releaseFixtures[coll->side].find(fix2);
        bool found1 = iter1 != coll->portal->releaseFixtures[coll->side].end();
        bool found2 = iter2 != coll->portal->releaseFixtures[coll->side].end();
        if (coll->status == 0){
            if (found2){
                int diff = iter1->second - iter2->second;
                if (diff == 0){
                    return true;
                }
                else if (abs(diff) > 1){
                    return false;
                }
                else{
                    if (iter1->second % 2 == 0){
                        if (iter2->second > iter1->second){
                            reverse = true;
                        }
                    }
                    else if (iter1->second > iter2->second){
                        reverse = true;
                    }   
                }
            }
            else if (coll->portal->collidingFixtures[coll->side].find(fix2) == coll->portal->collidingFixtures[coll->side].end()){
                return false;
            }
        }

        if (!reverse){
            if ((found1 && iter1->second == 1 &&
                coll->portal->collidingFixtures[coll->side].find(fix2) !=
                coll->portal->collidingFixtures[coll->side].end()) ||
                (found2 && iter2->second == 1 &&
                coll->portal->collidingFixtures[coll->side].find(fix1) !=
                coll->portal->collidingFixtures[coll->side].end()))
            {
                reverse = true;
            }        
        }
        if ((   found1 && iter1->second > 1 &&
                coll->portal->collidingFixtures[coll->side].find(fix2) !=
                coll->portal->collidingFixtures[coll->side].end()) ||
            (   found2 && iter2->second > 1 &&
                coll->portal->collidingFixtures[coll->side].find(fix1) !=
                coll->portal->collidingFixtures[coll->side].end()) ||
                (found1 && found2 && abs(iter1->second - iter2->second) > 1))
        {
            return false;
        }

        bool shouldCollide = coll->portal->shouldCollide(contact, fix1, fix2, coll);
        if (!shouldCollide) return reverse;
        else if (reverse) return false;
    }

    if (fix2->GetBody()->GetType() == b2_staticBody && !fix2->IsSensor() && prepareMaps.find(fix1) != prepareMaps.end()){
        auto mapsFix = prepareMaps[fix1];
        for (auto iter = mapsFix.begin(); iter != mapsFix.end(); iter++){
            (*iter)->prepareCollisionCheck(contact, fix1, fix2);
        }
    }

    return true;
}

void PortalBody::outHelper(b2Fixture* fix, Portal* portal, int status, PortalCollisionType out){
    int side = (uint32_t)out & ODD_MASK ? 1 : 0;
    if (status == 0){
        outFixtures[side][portal]--;
    }
    else{
        if (out & (RELEASE_COLLIDING_0 | RELEASE_COLLIDING_1)){
            outFixtures[side][portal]++;
        }
        else if (out & (NONE_COLLIDING_0 | NONE_COLLIDING_1)){
            if (outFixtures[side].find(portal) == outFixtures[side].end()){
                outFixtures[side][portal] = numFixtures - 1;
            }
            else{
                outFixtures[side][portal]--;
            }
        }
    }

    // Extra void portal logic
    if (portal->isVoid[side] && status == 0){
#if 1
        if (outFixtures[side][portal] == -numFixtures){
            pWorld->destroyBodies.insert(this);
            return;
        }
#else
        bool found = false;
        for (b2Fixture* f = fix->GetBody()->GetFixtureList(); f; f = f->GetNext()){
            if (f == fix) continue;
            if (portal->releaseFixtures[side].find(f) == portal->releaseFixtures[side].end())
            {
                found = true;
                f = nullptr;
                break;
            }
        }
        if (!found){
            pWorld->destroyBodies.insert(this);
            return;
        }
#endif
    }
    // -------------------------------------------------

    for (auto& col : *fixtureCollisions[fix]){
        if (col->portal == portal){
            col->status = status;
            return;
        }
    }

    portalCollision* portCol = (portalCollision*)malloc(sizeof(portalCollision));
    portCol->portal = portal;
    portCol->status = status;
    portCol->side = side;
    fixtureCollisions[fix]->insert(portCol);
}

std::vector<PortalBody*> PortalBody::postHandle(){
    for (b2Fixture* fix = body->GetFixtureList(); fix; fix = fix->GetNext()){
        calculateParts(fix);
    }

    std::vector<PortalBody*> retBodies;

    for (bodyStruct* s : createBodies){
        std::vector<PortalBody*> bodies = pWorld->createCloneBody(s);
        for (int i = 0; i < bodies.size(); i++){
            retBodies.push_back(bodies.at(i));
        }
        free(s);
    }
    createBodies.clear();

    return retBodies;
}

bool PortalBody::shouldCreate(b2Body* bBody, Portal* portal, PortalCollisionType out){
    int side = (uint32_t)out & ODD_MASK ? 1 : 0;
    if (portal->isVoid[side]) return false;

    for (bodyCollisionStatus* s : *bodyMaps){
        if (s->connection->portal1 == portal){
            const auto iter = pWorld->destroyBodies.find(s->body);
            if (iter != pWorld->destroyBodies.end()){
                pWorld->destroyBodies.erase(iter);
            }
        }
    }
    // Experimental
#if 0
    const auto iter = outFixtures[side].find(portal);
    if (iter != outFixtures[side].end() &&
        iter->second != numFixtures){
        return false;
    }
#else
    for (bodyCollisionStatus* s : *bodyMaps){
        if (s->connection->portal1 == portal && s->connection->side1 == side)
            return false;
    }
#endif

    for (bodyStruct* bs : createBodies){
        if (bs->collPortal == portal && bs->side == side)
            return false;
    }

    return true;
}

void PortalBody::releaseOut2(b2Fixture* fix, Portal* portal){
    b2Body* body = fix->GetBody();
    for (b2Fixture* fix = body->GetFixtureList(); fix; fix = fix->GetNext()){
        auto iter0 = portal->releaseFixtures[0].find(fix);
        auto iter1 = portal->releaseFixtures[1].find(fix);
        if (iter0 != portal->releaseFixtures[0].end()){
            if (iter0->second < 3){
                return;
            }
        }
        if (iter1 != portal->releaseFixtures[1].end()){
            if (iter1->second < 3){
                return;
            }
        }
    }
    pWorld->destroyBodies.insert(this);
}

void PortalBody::handleOut(b2Fixture* fix, Portal* portal, PortalCollisionType out){
    switch (out)
    {
    case NONE_COLLIDING_0:
    case NONE_COLLIDING_1:
    case RELEASE_COLLIDING_0:
    case RELEASE_COLLIDING_1:
        if (shouldCreate(fix->GetBody(), portal, out)){
            int side = (uint32_t)out & ODD_MASK ? 1 : 0;
            bodyStruct* s = (bodyStruct*)malloc(sizeof(bodyStruct));
            *s = {this, portal, side};
            createBodies.push_back(s);
        }
        outHelper(fix, portal, 1, out);
        break;
    
    case COLLIDING_RELEASE_0:
        outHelper(fix, portal, 0, out);
        break;
    case COLLIDING_RELEASE_1:
        outHelper(fix, portal, 0, out);
        break;

    case PREPARE_IN:
        prepareMaps[fix].insert(portal);
        break;
    case PREPARE_OUT:
        prepareMaps[fix].erase(portal);
        break;

    case E_COLLIDING_RELEASE_0:
    case E_COLLIDING_RELEASE_1:
        releaseOut2(fix, portal);
        break;

    case COLLIDING_NONE_0:
    case COLLIDING_NONE_1:
        destroyCheck(fix, portal, out);
        break;

    default:
        break;
    }
}

void PortalBody::destroyCheck(b2Fixture* f, Portal* portal, PortalCollisionType out){
    int side = (uint32_t)out & ODD_MASK ? 1 : 0;
    outFixtures[side][portal]++;

    std::set<portalCollision*>* coll = fixtureCollisions[f];
    for (auto& a : *coll){
        if (a->portal == portal && a->status == 1){
            coll->erase(a);
            free(a);
            break;
        }
    }

// Experimental
// Disable it if it is causing problems
#if 1
    if (outFixtures[side][portal] == numFixtures){
        for (bodyCollisionStatus* s : *bodyMaps){
            if (s->connection->portal1 == portal && s->connection->side1 == side){
                pWorld->destroyBodies.insert(s->body);
            }
        }
    }
#else
    for (b2Fixture* fix = f->GetBody()->GetFixtureList(); fix; fix = fix->GetNext()){
        if (fix == f) continue;
        for (portalCollision* coll : *fixtureCollisions[fix]){
            if (coll->portal == portal){
                auto iter = portal->releaseFixtures[coll->side].find(fix);
                if (iter == portal->releaseFixtures[coll->side].end()){
                    return;
                }
            }
        }
    }

    for (bodyCollisionStatus* s : *bodyMaps){
        if (s->connection->portal1 == portal){
            pWorld->destroyBodies.insert(s->body);
        }
    }
#endif
}

void rotateVec(b2Vec2* vec, float angle){
    float x = cos(angle) * vec->x - sin(angle) * vec->y;
    float y = sin(angle) * vec->x + cos(angle) * vec->y;
    vec->x = x;
    vec->y = y;
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
#if 0
    b2Transform t = body->GetTransform();
    t.q.Set(offsetAngle + body->GetAngle());
    pWorld->drawer->DrawTransform(t);
#endif
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

    float rv1 = x / f + off.x;
    float rv2 = y / f + off.y;

    // Validity check
    if (!b2IsValid(rv1) || !b2IsValid(rv2)){
        return b2Vec2(0, 0);
    }

    return b2Vec2(rv1, rv2);
}

void PortalBody::applyGravity(b2Fixture* fix, int status){
    auto apf = allParts[fix];
    if (apf->size() == 0 || status == 0) return;

    std::vector<b2Vec2>* vecs = apf->at(0);
    if (vecs->size() == 0) return;

    float area = getArea(fix, status);

    if (area == 0) return;

    b2Vec2 center = findCentroid(vecs);

    b2Vec2 force = area * fix->GetDensity() * pWorld->GetGravity();

    fix->GetBody()->ApplyForce(((float)bodyMaps->size() + 1) * force, center, false);
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
        applyGravity(fix, status);
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
            else
                free(releaseVecs);

            setIter++;
            if (setIter == (*fixIter).second->end()) break;

            drawVecs->clear();
        }
    }
    delete(vertices);

    applyGravity(fix, status);
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
            b2Color oldColor = bodyColor;
#if RENDER_COLOURFUL
            bodyColor = b2Color(1, 0, 1, 0.5f);
#endif
            drawVertices(bBody, *allParts[fix]->at(0));
            bodyColor = oldColor;
        }
        if (pWorld->drawReleases){
            for (int i = 1; i < allParts[fix]->size(); i++){
                b2Color oldColor = bodyColor;
                auto& vecs = allParts[fix]->at(i);                
#if RENDER_COLOURFUL
                bodyColor = b2Color(1, 0, 0, 0.5f);
#else
                bodyColor = pWorld->releaseColor;
#endif
                drawVertices(bBody, *vecs);
                bodyColor = oldColor;
            }
        }
    }
    else if (pWorld->drawReleases && renderStatus == 0){
            b2Color oldColor = bodyColor;

#if RENDER_COLOURFUL
            auto iter = fixtureCollisions[fix]->begin();
            int val = (*iter)->portal->releaseFixtures[(*iter)->side][fix];
            if (val <= sizeof(colors) / sizeof(colors[0])){
                bodyColor = colors[val - 1];
            }

#else
            bodyColor = pWorld->releaseColor;
#endif
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
