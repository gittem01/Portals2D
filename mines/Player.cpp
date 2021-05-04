#include "Portal.h"

Player::Player(b2World* world, b2Vec2 pos, WindowPainter* wp) {
	this->shape = new Shape(world, pos);
	shape->defaultrestitution = 0.0f;
	shape->defaultAngularDamping = 2.0f;
	shape->defaultFriction = 100.0f;
	shape->isControllable = true;
	shape->controlClass = this;
	this->shape->createCircle(radius, b2_dynamicBody);
	this->wp = wp;
	this->gun = new Gun(pos, pos + b2Vec2(5.0f, 0.0f), world);
	playerPortals[0] = NULL; playerPortals[1] = NULL;
}

void Player::update() {
	handleInput();
	gun->pos = shape->body->GetPosition();
	gun->update();
}

void Player::handleInput() {
	if (wp->keyData[GLFW_KEY_SPACE] == 2) {
		shape->body->ApplyForceToCenter(b2Vec2(0.0f, shape->body->GetMass() * 500.0f * (1.0f + portalCollision)), true);
	}
	float w = 0.0f;
	b2Vec2 l = b2Vec2(0.0f, 0.0f);
	if (wp->keyData[GLFW_KEY_D]) {
		w += -10.0f;
		l += b2Vec2(+1.0f * shape->body->GetMass() * 10.f, 0.0f);
	}
	if (wp->keyData[GLFW_KEY_A]) {
		w += +10.0f;
		l += b2Vec2(-1.0f * shape->body->GetMass() * 10.f, 0.0f);
	}
	shape->body->SetAngularVelocity(w);
	shape->body->ApplyForceToCenter(l, true);

	glm::vec2 mp = wp->cam->getMouseCoords();
	gun->targetPos = b2Vec2(mp.x, mp.y);

	for (int i = 0; i < 2; i++) {
		if (wp->mouseData[i+2] == 2 && gun->currentTarget->GetBody()->GetType() == b2_staticBody) {
			if (playerPortals[i]) {
				Portal* p = (Portal*)playerPortals[i];
				
				if (portalCollision) {
					if (((Portal*)playerPortals[0])->correspondingBodies.find(shape->body) !=
						((Portal*)playerPortals[0])->correspondingBodies.end()) {
						shape->world->DestroyBody(((Portal*)playerPortals[0])->correspondingBodies[shape->body]);
					}
					else {
						shape->world->DestroyBody(((Portal*)playerPortals[1])->correspondingBodies[shape->body]);
					}
				}
				
				p->~Portal();
				playerPortals[i] = NULL;
			}
			playerPortals[i] = new Portal(gun->currentCollisionPos, gun->currentNormal, 1.0f, shape->world);
			if (playerPortals[1-i]) {
				((Portal*)playerPortals[i])->connect((Portal*)playerPortals[1-i]);
			}
			((Portal*)playerPortals[i])->color = b2Color(i, 0.5f, 1.0f-i, 1.0f);
		}
	}
}

void Player::swapShape(Shape* newShape) {
	shape = newShape;
	shape->defaultrestitution = 0.0f;
	shape->defaultAngularDamping = 2.0f;
	shape->defaultFriction = 100.0f;
	portalCollision = false;
}