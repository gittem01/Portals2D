#include "Player.h"

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
}

void Player::update() {
	handleInput();
	gun->pos = shape->body->GetPosition();
	gun->update();
}

void Player::handleInput() {
	if (wp->keyData[GLFW_KEY_SPACE] == 1) {
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
}

void Player::swapShape(Shape* newShape) {
	shape = newShape;
	shape->defaultrestitution = 0.0f;
	shape->defaultAngularDamping = 2.0f;
	shape->defaultFriction = 100.0f;
	portalCollision = false;
}