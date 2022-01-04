#pragma once

#include "box2d/box2d.h"
#include "WindowPainter.h"
#include <set>
#include <map>
#include <vector>
#include "Portal.h"
#include "DebugDrawer.h"

class mouseJointHandler;

class QueryCallback : public b2QueryCallback
{
public:
    mouseJointHandler* mjh;

    QueryCallback(mouseJointHandler* mjh) {
        this->mjh = mjh;
    }

	virtual bool ReportFixture(b2Fixture* fixture){
        fixture->GetBody()->SetAwake(true);

        return true;
    }
};

class mouseJointHandler
{
public:
	std::set<b2MouseJoint*> mouseJoints;
    std::map<b2MouseJoint*, b2Vec2> jointDiffs;
    std::map<b2MouseJoint*, b2Body*> mouseBodies;
    std::vector<b2Body*> collidingBodies;
    std::set<b2Body*> selectedBodies;

    QueryCallback* queryCallback;

    DebugDrawer* drawer;

	float frequencyHz = 5.0f;
	float dampingRatio = 0.5f;
    float bodyRadius = 0.0f;
    float radiusLimits[2] = { 0.0f, 2.5f };

    int lastFrame = INT_MIN;

    b2World* world;
	b2BodyDef bodyDef;
	b2Body* groundBody;
    b2Body* mouseBody;

	glm::vec2* clicks[2] = { NULL, NULL };
    WindowPainter* wh;

    mouseJointHandler(b2World* world, WindowPainter* wh, DebugDrawer* drawer) {
        this->world = world;
        this->groundBody = world->CreateBody(&bodyDef);
        this->wh = wh;
        this->drawer = drawer;
        mouseBody = NULL;
        createMouseBody();
        queryCallback = new QueryCallback(this);
    }

    void removeDuplicates(){
        if (collidingBodies.size() == 0) return;
        for (int i = 0; i < collidingBodies.size() - 1; i++){
            for (int j = i + 1; j < collidingBodies.size(); j++){
                if (collidingBodies.at(i) == collidingBodies.at(j)){
                    collidingBodies.erase(collidingBodies.begin() + j);
                    j--;
                }
            }
        }
    }

    void jointHandler(glm::vec2 mp, b2World* world) {
        removeDuplicates();
        b2Vec2 target = b2Vec2(mp.x, mp.y);
        for (b2Body* clickedBody : collidingBodies) {
            if (selectedBodies.find(clickedBody) != selectedBodies.end()){
                continue;
            }

            b2MassData mData;
            clickedBody->GetMassData(&mData);
            b2Vec2 bodyPos = clickedBody->GetWorldPoint(mData.center);

            b2Vec2 diff = bodyPos - target;

            selectedBodies.insert(clickedBody);

            bool isIn = isBodyFullyIn(clickedBody);

            b2MouseJointDef jd;
            jd.bodyA = groundBody;
            jd.bodyB = clickedBody;
            if (isIn)
                jd.target = target + diff;
            else
                jd.target = target;
            jd.maxForce = 10000.0f * clickedBody->GetMass();
            jd.collideConnected = true;
            
            b2LinearStiffness(jd.stiffness, jd.damping, frequencyHz, dampingRatio, jd.bodyA, jd.bodyB);

            b2MouseJoint* joint = (b2MouseJoint*)world->CreateJoint(&jd);
            mouseBodies[joint] = clickedBody;
            if (isIn)
                jointDiffs[joint] = diff;

            mouseJoints.insert(joint);
        }
    }

    void mouseHandler(int frame, int totalIter) {
        glm::vec2 mp = wh->cam->getMouseCoords();
        mouseBody->SetTransform(b2Vec2(mp.x, mp.y), 0.0f);

        b2Vec2 lowerBound = b2Vec2(mp.x - bodyRadius, mp.y - bodyRadius);
        b2Vec2 upperBound = b2Vec2(mp.x + bodyRadius, mp.y + bodyRadius);

        b2AABB aabb = { lowerBound, upperBound };
        world->QueryAABB(queryCallback, aabb);

        if (wh->mouseData[3] == 2 && !wh->mouseData[2] && frame != lastFrame){
            clicks[1] = clicks[0];
            clicks[0] = new glm::vec2(mp.x, mp.y);
        }
        if (clicks[1]) {
            b2EdgeShape shape;
            shape.SetTwoSided(b2Vec2(clicks[0]->x, clicks[0]->y), b2Vec2(clicks[1]->x, clicks[1]->y));
            groundBody->CreateFixture(&shape, 0.0f);

            delete(clicks[1]);
            clicks[0] = NULL; clicks[1] = NULL;
        }

        std::vector<b2MouseJoint*> remJoints;
        for (b2MouseJoint* mouseJoint : mouseJoints) {
            bool jointFound = false;
            for (b2Joint* joint = world->GetJointList(); joint; joint = joint->GetNext()) {
                if (joint == mouseJoint) {
                    if (jointDiffs.find((b2MouseJoint*)joint) != jointDiffs.end()) {
                        mouseJoint->SetTarget(b2Vec2(mp.x, mp.y) + jointDiffs[(b2MouseJoint*)joint]);
                    }
                    else {
                        mouseJoint->SetTarget(b2Vec2(mp.x, mp.y));
                    }
                    jointFound = true;
                    break;
                }
            }
            if (!jointFound) {
                remJoints.push_back(mouseJoint);
            }
        }

        for (b2MouseJoint* mouseJoint : remJoints){
            selectedBodies.erase(mouseBodies[mouseJoint]);
            mouseBodies.erase(mouseJoint);
            mouseJoints.erase(mouseJoint);
            jointDiffs.erase(mouseJoint);
        }

        if (wh->mouseData[2] == 2) {
            jointHandler(mp, world);
        }
        else if (wh->mouseData[2] == 0 && mouseJoints.size() > 0) {
            for (b2MouseJoint* mouseJoint : mouseJoints) {
                for (b2Joint* joint = world->GetJointList(); joint; joint = joint->GetNext()) {
                    if (joint == mouseJoint) {
                        world->DestroyJoint(mouseJoint);
                        break;
                    }
                }
            }
            mouseJoints.clear();
            jointDiffs.clear();
            mouseBodies.clear();
            selectedBodies.clear();
        }
        else if (wh->mouseData[2] == 1 && (wh->mouseData[3] == 2 || wh->keyData[GLFW_KEY_F])) {
            mouseJoints.clear();
            jointDiffs.clear();
            mouseBodies.clear();
            selectedBodies.clear();
        }

        if (wh->keyData[GLFW_KEY_LEFT_CONTROL] || wh->keyData[GLFW_KEY_RIGHT_CONTROL]) {
            if (wh->mouseData[5]) {
                bodyRadius += wh->mouseData[5] * 0.1f / totalIter;
                if (bodyRadius < radiusLimits[0]) bodyRadius = radiusLimits[0];
                else if (bodyRadius > radiusLimits[1]) bodyRadius = radiusLimits[1];
                createMouseBody();
            }
        }
        lastFrame = frame;
    }

    bool isPolyIn(b2Body* body, b2PolygonShape* shape) {
        b2Vec2 thisPos = mouseBody->GetPosition();
        b2Vec2 vert;
        for (int i = 0; i < shape->m_count; i++) {
            b2Vec2 vertPos = body->GetWorldPoint(shape->m_vertices[i]);

            if ((vertPos - thisPos).Length() > bodyRadius)
                return false;
        }
        return true;
    }

    bool isCircleIn(b2Body* body, b2CircleShape* shape) {
        b2Vec2 thisPos = mouseBody->GetPosition();
        b2MassData mData;
        body->GetMassData(&mData);
        b2Vec2 bodyPos = body->GetWorldPoint(mData.center);
        float r = bodyRadius - shape->m_radius;

        if ((bodyPos - thisPos).Length() > r)
            return false;
        else
            return true;
    }

    bool isFixtureIn(b2Fixture* fixture) {
        switch (fixture->GetShape()->GetType())
        {
        case b2Shape::e_polygon:
            return isPolyIn(fixture->GetBody(), (b2PolygonShape*)fixture->GetShape());
        case b2Shape::e_circle:
            return isCircleIn(fixture->GetBody(), (b2CircleShape*)fixture->GetShape());
        default:
            return false;
            break;
        }
    }

    bool isBodyFullyIn(b2Body* body) {
        for (b2Fixture* fix = body->GetFixtureList(); fix; fix = fix->GetNext()) {
            if (!isFixtureIn(fix)) {
                return false;
            }
        }

        return true;
    }

    void createMouseBody() {
        b2Vec2 oldMousePos = b2Vec2();
        if (mouseBody) {
            oldMousePos = mouseBody->GetTransform().p;
            world->DestroyBody(mouseBody);
        }
        b2BodyDef bodyDef;
        bodyDef.type = b2_kinematicBody;
        bodyData* bData = (bodyData*)malloc(sizeof(bodyData));
        *bData = { MOUSE, this };
        bodyDef.userData.pointer = (uintptr_t)bData;
        mouseBody = world->CreateBody(&bodyDef);

        mouseBody->SetTransform(oldMousePos, 0.0f);

        b2CircleShape circleShape;
        circleShape.m_radius = bodyRadius;

        b2FixtureDef fixtureDef;
        fixtureDef.shape = &circleShape;
        fixtureDef.isSensor = true;

        mouseBody->CreateFixture(&fixtureDef);
    }

    void drawMouseBody() {
        drawer->DrawSolidCircle(mouseBody->GetPosition(), bodyRadius,
            b2Vec2(0.0f, 0.0f), b2Color(1.0f, 1.0f, 1.0f, 0.2f));
    }
};