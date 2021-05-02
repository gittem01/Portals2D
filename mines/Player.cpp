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
}

void Player::update() {
	handleInput();
}

void Player::handleInput() {
	if (wp->keyData[GLFW_KEY_SPACE] == 1) {
		shape->body->ApplyForceToCenter(b2Vec2(0.0f, shape->body->GetMass() * 500.0f), true);
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
}

void Player::swapShape(Shape* newShape) {
	shape = newShape;
	shape->defaultrestitution = 0.0f;
	shape->defaultAngularDamping = 2.0f;
	shape->defaultFriction = 100.0f;
	shape->isControllable = true;
	shape->controlClass = this;

	shape->body->GetFixtureList()->SetRestitution(shape->defaultrestitution);
	shape->body->GetFixtureList()->SetFriction(shape->defaultFriction);
	shape->body->SetAngularDamping(shape->defaultAngularDamping);
}