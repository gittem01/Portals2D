#include <box2d/box2d.h>
#include <stdio.h>

class RotationJoint : b2PrismaticJoint{

friend class PortalWorld;

private:

    bool isReversed;

public:
    RotationJoint(const b2PrismaticJointDef* def, const bool rev) : b2PrismaticJoint(def)
    {
        isReversed = rev;
    }

    void SolveVelocityConstraints(const b2SolverData& data) override
    {
        float wA = data.velocities[m_indexA].w;
        float wB = data.velocities[m_indexB].w;

        float mA = m_invMassA, mB = m_invMassB;
        float iA = m_invIA, iB = m_invIB;

        {
            b2Vec2 Cdot;
            Cdot.x = m_s2 * wB - m_s1 * wA;
            if (isReversed){
                Cdot.y = wB + wA;
            }
            else{
                Cdot.y = wB - wA;
            }

            b2Vec2 df = m_K.Solve(-Cdot);
            m_impulse += df;

            float LA = df.x * m_s1 + df.y;
            float LB = df.x * m_s2 + df.y;

            if (isReversed){
                wA += iA * LA;
            }
            else{
                wA -= iA * LA;
            }
            wB += iB * LB;
        }

        data.velocities[m_indexA].w = wA;
        data.velocities[m_indexB].w = wB;
    }

    bool SolvePositionConstraints(const b2SolverData& data) override
    {
        return true;
    }
};