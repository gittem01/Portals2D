#pragma once

#include <box2d/box2d.h>
#include "WindowPainter.h"
#include <set>
#include <vector>
#include "Shape.h"

class mouseJointHandler {
public:
	std::vector<b2MouseJoint*> mouseJoints;
    std::set<b2Body*> collidingBodies;

	float frequencyHz = 5.0f;
	float dampingRatio = 0.5f;
    float bodyRadius = 0.0f;
    float radiusLimits[2] = { 0.0f, 0.5f };

    b2World* world;
	b2BodyDef bodyDef;
	b2Body* groundBody;
    b2Body* mouseBody;

	glm::vec2* clicks[2] = { NULL, NULL };
    WindowPainter* wh;

    mouseJointHandler(b2World* world, WindowPainter* wh) {
        this->world = world;
        this->groundBody = world->CreateBody(&bodyDef);
        this->wh = wh;
        mouseBody = NULL;
        createMouseBody();
    }

    void jointHandler(glm::vec2 mp, b2World* world) {
        b2Vec2 target = b2Vec2(mp.x, mp.y);

        for (b2Body* clickedBody : collidingBodies) {
            b2MouseJointDef jd;
            jd.bodyA = groundBody;
            jd.bodyB = clickedBody;
            jd.target = target;
            jd.maxForce = 100.0f * clickedBody->GetMass();
            b2LinearStiffness(jd.stiffness, jd.damping, frequencyHz, dampingRatio, jd.bodyA, jd.bodyB);

            mouseJoints.push_back((b2MouseJoint*)world->CreateJoint(&jd));
            clickedBody->SetAwake(true);
        }
    }

    void mouseHandler() {
        glm::vec2 mp = wh->cam->getMouseCoords();
        mouseBody->SetTransform(b2Vec2(mp.x, mp.y), 0.0f);
        drawMouseBody();

        if (wh->mouseData[3] == 2 && !wh->mouseData[2]) {
            clicks[1] = clicks[0];
            clicks[0] = new glm::vec2(mp.x, mp.y);
        }
        if (clicks[1]) {
            b2BodyDef bd;
            b2Body* ground = world->CreateBody(&bd);

            b2EdgeShape shape;
            shape.SetTwoSided(b2Vec2(clicks[0]->x, clicks[0]->y), b2Vec2(clicks[1]->x, clicks[1]->y));

            ground->CreateFixture(&shape, 0.0f);
            clicks[0] = NULL; clicks[1] = NULL;
        }
        for (int i = 0; i < mouseJoints.size(); i++) {
            b2MouseJoint* mouseJoint = mouseJoints.at(i);
            bool jointFound = false;
            for (b2Joint* joint = world->GetJointList(); joint; joint = joint->GetNext()) {
                if (joint == mouseJoint) {
                    mouseJoint->SetTarget(b2Vec2(mp.x, mp.y));
                    jointFound = true;
                    break;
                }
            }
            if (!jointFound) mouseJoints.erase(mouseJoints.begin() + i);
        }


        if (wh->mouseData[2] == 2) {
            jointHandler(mp, world);
        }
        else if (wh->mouseData[2] == 0 && mouseJoints.size() > 0) {
            for (int i = 0; i < mouseJoints.size(); i++) {
                b2MouseJoint* mouseJoint = mouseJoints.at(i);
                for (b2Joint* joint = world->GetJointList(); joint; joint = joint->GetNext()) {
                    if (joint == mouseJoint) {
                        world->DestroyJoint(mouseJoint);
                        break;
                    }
                }
            }
            mouseJoints.clear();
        }
        else if (wh->mouseData[2] == 1 && wh->mouseData[3] == 2) {
            mouseJoints.clear();
        }

        if (wh->keyData[GLFW_KEY_LEFT_CONTROL] || wh->keyData[GLFW_KEY_RIGHT_CONTROL]) {
            if (wh->mouseData[5]) {
                bodyRadius += wh->mouseData[5] * 0.1f;
                if (bodyRadius < radiusLimits[0]) bodyRadius = radiusLimits[0];
                else if (bodyRadius > radiusLimits[1]) bodyRadius = radiusLimits[1];
                createMouseBody();
            }
        }
    }

    void createMouseBody() {
        if (mouseBody) {
            world->DestroyBody(mouseBody);
        }
        b2BodyDef bodyDef;
        bodyDef.type = b2_kinematicBody;
        bodyData* bData = (bodyData*)malloc(sizeof(bodyData));
        *bData = { MOUSE, this };
        bodyDef.userData.pointer = (uintptr_t)bData;
        mouseBody = world->CreateBody(&bodyDef);

        b2CircleShape circleShape;
        circleShape.m_radius = bodyRadius;

        b2FixtureDef fixtureDef;
        fixtureDef.shape = &circleShape;
        fixtureDef.isSensor = true;

        mouseBody->CreateFixture(&fixtureDef);
    }

    void drawMouseBody() {
        world->m_debugDraw->DrawSolidCircle(mouseBody->GetPosition(), bodyRadius,
            b2Vec2(0.0f, 0.0f), b2Color(1.0f, 1.0f, 1.0f, 0.2f));
    }
};