#pragma once

#include <Portal.h>

typedef enum
{
    ROTATION,
    POSITION,
} PortalJointType;

class BaseCustomJoint : b2Joint
{

friend class PortalWorld;
friend class RotationJoint;
friend class PositionJoint;

public:

    b2Vec2 GetAnchorA() const override { return b2Vec2(); };
	b2Vec2 GetAnchorB() const override { return b2Vec2(); };

	b2Vec2 GetReactionForce(float inv_dt) const override { return b2Vec2(); };
	float GetReactionTorque(float inv_dt) const override { return 0.0f; };

protected:
    PortalJointType jointType;

private:

    BaseCustomJoint(const b2JointDef* def) : b2Joint(def) {}
};