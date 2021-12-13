#include "Portal.h"
#include "glm/common.hpp"

#define CIRCLE_POINTS 50
#define DRAW_RELEASES 0

std::vector<PortalBody*> PortalBody::portalBodies;

PortalBody::PortalBody(b2Body* body, b2World* world, b2Vec3 bodyColor){
    this->world = world;
    this->bodyMaps[body] = new std::vector<void*>();
    
    bodyData* bd = (bodyData*)malloc(sizeof(bodyData));
    bd->data = this;
    bd->type = PORTAL_BODY;
    body->GetUserData().pointer = (uintptr_t)bd;

    this->bodyColor = bodyColor;
    PortalBody::portalBodies.push_back(this);
}

void PortalBody::collisionBegin(b2Contact* contact, b2Fixture* fix1, b2Fixture* fix2){
    bodyData* bData = (bodyData*)fix2->GetBody()->GetUserData().pointer;
    if (bData && bData->type == PORTAL){
        Portal* p = (Portal*)bData->data;
        int out = p->collisionBegin(contact, fix2, fix1);
    }
}

void PortalBody::collisionEnd(b2Contact* contact, b2Fixture* fix1, b2Fixture* fix2){
    bodyData* bData = (bodyData*)fix2->GetBody()->GetUserData().pointer;
    if (bData && bData->type == PORTAL){
        Portal* p = (Portal*)bData->data;
        int out = p->collisionEnd(contact, fix2, fix1);
    }
}

void PortalBody::preCollision(b2Contact* contact, b2Fixture* fix1, b2Fixture* fix2){
    bodyData* bData = (bodyData*)fix2->GetBody()->GetUserData().pointer;

    if (bData && bData->type == PORTAL){
        Portal* p = (Portal*)bData->data;
        int out = p->preCollision(contact, fix2, fix1);
    }
}

// 0 for no rendering
// 1 for partial rendering
// 2 for full renndering
int PortalBody::getRenderStatus(b2Fixture* fix, void** portal, int* side){
    // unoptimized version
    for (int i = 0; i < 2; i++){
        for (Portal* p : Portal::portals){
            if (p->releaseFixtures[i].find(fix) != p->releaseFixtures[i].end()){
                return 0;
            }
            if (p->collidingFixtures[i].find(fix) != p->collidingFixtures[i].end()){
                *((Portal**)portal) = p;
                *side = i;
                return i + 1;
            }
        }
    }

    return 3;
}

void PortalBody::drawBodies(){
    for (std::map<b2Body*, std::vector<void*>*>::iterator iter = bodyMaps.begin(); iter != bodyMaps.end(); iter++){
        for (b2Fixture* fix = iter->first->GetFixtureList(); fix != nullptr; fix = fix->GetNext()){
            if (fix->GetType() == b2Shape::Type::e_polygon){
                drawPolygonFix(fix);
            }
            else if (fix->GetType() == b2Shape::Type::e_circle){
                drawCircleFix(fix);
            }
        }
    }
}

void PortalBody::adjustVertices(b2Vec2* vertices, int vertexCount, b2Vec2** retVertices1, b2Vec2** retVertices2, 
                                int* size1, int* size2, void* p, int side){
    Portal* portal = (Portal*)p;
    *retVertices1 = (b2Vec2*)calloc(vertexCount + 1, sizeof(b2Vec2));
    *retVertices2 = (b2Vec2*)calloc(vertexCount + 1, sizeof(b2Vec2));

    int index1 = 0;
    int index2 = 0;

    int pSide;
    int lastSide = portal->getPointSide(vertices[0]);
    for (int i = 0; i < vertexCount + 1; i++){
        int vertexIndex = i % vertexCount;
        pSide = portal->getPointSide(vertices[vertexIndex]);
        if (pSide == side && lastSide == 1 - side){
            b2Vec2 intersectionPoint = getLineIntersection(portal->points[0], portal->points[1], vertices[i - 1], vertices[vertexIndex]);
            (*retVertices1)[index1++] = intersectionPoint;
            (*retVertices2)[index2++] = intersectionPoint;
        }
        else if (pSide == 1 - side && lastSide == side){
            b2Vec2 intersectionPoint = getLineIntersection(portal->points[0], portal->points[1], vertices[i - 1], vertices[vertexIndex]);
            (*retVertices1)[index1++] = intersectionPoint;
            (*retVertices2)[index2++] = intersectionPoint;
        }

        if (pSide == side && i != vertexCount){
            (*retVertices1)[index1++] = vertices[vertexIndex];
        }
        else if (i != vertexCount){
            (*retVertices2)[index2++] = vertices[vertexIndex];
        }

        lastSide = pSide;
    }
    *size1 = index1;
    *size2 = index2;
}

void PortalBody::portalRender(b2Fixture* fix, b2Vec2* vertices, int vertexCount){
    b2Body* body = fix->GetBody();

    Portal* portal = NULL;
    int side;
    int size1, size2;
    int renderStatus = getRenderStatus(fix, (void**)&portal, &side);

    if (renderStatus == 0){
#if DRAW_RELEASES == 1
            b2Vec3 oldColor = bodyColor;
            bodyColor = b2Vec3(0.1f, 0.1f, 0.1f);
            drawVertices(body, vertices, vertexCount);
            bodyColor = oldColor;
#endif
    }
    else if (renderStatus == 1 || renderStatus == 2){
        b2Vec2* drawVecs = NULL;
        b2Vec2* releaseVecs = NULL;
        if (portal) adjustVertices(vertices, vertexCount, &drawVecs, &releaseVecs, &size1, &size2, portal, side);
        if (drawVecs){
            b2Vec3 oldColor = bodyColor;
            bodyColor = b2Vec3(1.0f, 1.0f, 1.0f);
            drawVertices(body, drawVecs, size1);
            bodyColor = oldColor;
        }
        if (releaseVecs){
#if DRAW_RELEASES == 1
            b2Vec3 oldColor = bodyColor;
            bodyColor = b2Vec3(0.1f, 0.1f, 0.1f);
            drawVertices(body, releaseVecs, size2);
            bodyColor = oldColor;
#endif
        }
        if (portal){ free(drawVecs); free(releaseVecs); }
    }

    else if (renderStatus == 3) drawVertices(body, vertices, vertexCount);

    if (renderStatus == 0) bodyColor.x = 1.0f;
    else if (renderStatus == 1) bodyColor.y = 1.0f;
    else if (renderStatus == 2) bodyColor.z = 1.0f;

    free(vertices);
}

void PortalBody::drawPolygonFix(b2Fixture* fix){
    b2Body* body = fix->GetBody();
    b2PolygonShape* shape = (b2PolygonShape*)fix->GetShape();

    int vertexCount = shape->m_count;
    b2Vec2* vertices = (b2Vec2*)malloc(sizeof(b2Vec2*) * vertexCount);

	for (int i = 0; i < vertexCount; i++) {
        b2Vec2 p1 = b2Vec2((shape->m_vertices + i)->x, (shape->m_vertices + i)->y);
        p1 = body->GetWorldPoint(p1);
        vertices[i] = p1;
	}

    portalRender(fix, vertices, vertexCount);
}

void PortalBody::drawCircleFix(b2Fixture* fix){
    b2Body* body = fix->GetBody();
    b2CircleShape* shape = (b2CircleShape*)fix->GetShape();
    float r = shape->m_radius;

    b2Vec2* vertices = (b2Vec2*)malloc(sizeof(b2Vec2*) * CIRCLE_POINTS);

    float angle = 0;
	for (int i = 0; i < CIRCLE_POINTS; i++) {
        float angle0 = ((float)i / CIRCLE_POINTS) * b2_pi * 2;
        b2Vec2 p = b2Vec2(sin(angle0) * r, cos(angle0) * r) + shape->m_p;
        p = body->GetWorldPoint(p);
        vertices[i] = p;
	}

    portalRender(fix, vertices, CIRCLE_POINTS);
}

void PortalBody::drawVertices(b2Body* body, b2Vec2* vertices, int vertexCount){
    float transparency = 1.0f;
    if (!body->IsAwake()){
        transparency /= 2;
    }
    
    glColor4f(bodyColor.x, bodyColor.y, bodyColor.z, 0.5f * transparency);
	glBegin(GL_POLYGON);
	for (int i = 0; i < vertexCount; i++) {
		glVertex2d((vertices + i)->x, (vertices + i)->y);
	}
	glEnd();

	glLineWidth(1.0f);
	glColor4f(bodyColor.x, bodyColor.y, bodyColor.z, transparency);
	glBegin(GL_LINES);
	for (int i = 0; i < vertexCount; i++) {
		glVertex2d((vertices + i)->x, (vertices + i)->y);
		glVertex2d((vertices + (i + 1) % vertexCount)->x, (vertices + (i + 1) % vertexCount)->y);
	}
	glEnd();
}

b2Vec2 PortalBody::getLineIntersection(b2Vec2 p1, b2Vec2 p2, b2Vec2 p3, b2Vec2 p4){
    float d = (p1.x - p2.x) * (p3.y - p4.y) - (p1.y - p2.y) * (p3.x - p4.x);
    float up1 = (p1.x * p2.y - p1.y * p2.x) * (p3.x - p4.x) - (p1.x - p2.x) * (p3.x * p4.y - p3.y * p4.x);
    float up2 = (p1.x * p2.y - p1.y * p2.x) * (p3.y - p4.y) - (p1.y - p2.y) * (p3.x * p4.y - p3.y * p4.x);

    return b2Vec2(up1 / d, up2 / d);
}