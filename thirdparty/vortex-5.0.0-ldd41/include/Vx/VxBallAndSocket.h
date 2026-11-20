// Vortex 5.0.0 build 800 "vortexldd41" - VxBallAndSocket (restored header;
// completed in V4.3).
// Mac 0x90 bytes, Win 0xa8 (glue operator new sizes). Class id 0x118 (Mac 0x42f0dc).
#ifndef VX_VXBALLANDSOCKET_H
#define VX_VXBALLANDSOCKET_H

#include "Vx/VxConstraint.h"
#include "Vx/VxTransform.h"
#include "Vx/VxConstraintFriction.h"

namespace Vx {

class VxBallAndSocket : public VxConstraint {
public:
    VX_DECLARE_ALLOCATOR();                             // new Mac 0x42f2d3, Win 0x72c010, delete Mac 0x42f1d5
    static int getStaticClassType();                    // Mac 0x42f0dc: 0x118
    static const VxArray<int>& getStaticClassTypes();   // (function-local static list, see the .cpp)
    VxBallAndSocket();                      // C1 Mac 0x42fe24 -> C2 0x42fd4e, Win 0x72c510; MdtBSJointCreate Mac 0x48cd73
    virtual bool isOfType(int type) const;              // slot 0 Mac 0x42f66e, Win 0x72c270
    virtual bool isOfClassType(int type) const;         // slot 1 Mac 0x42f0e6, Win 0x72c1d0
    virtual ~VxBallAndSocket();             // slot 2/3 Mac 0x42f2c0/0x42f1ec, Win 0x72c9d0
    virtual const char* getClassName() const;           // slot 4 Mac 0x42ff1c, Win 0x72c9c0
    virtual void resetParts();                          // slot 16 Mac 0x42f168, Win 0x72c0c0
    virtual bool getEquationFrictionIndex(int equation, int* friction) const; // slot 17 Mac 0x42f0fc, Win 0x72c150

    // A friction that drives the three angular frictions of the Mdt constraint together (Mac
    // vtable 0x75d268; Mac 0x40 bytes, Win 0x48). The getters read the first one, getMaxForce
    // the largest. src/Vx/VxBallAndSocket.cpp (Mac 0x42ff2e-0x43039e, Win 0x72c6e0-0x72c990).
    class AngularFriction : public VxConstraintFriction {
    public:
        AngularFriction() { mFriction[0] = mFriction[1] = mFriction[2] = 0; } // inline (Mac 0x42fd4e)
        virtual ~AngularFriction();                         // Mac D1 0x43039e / D0 0x430374, Win 0x72c990 (ICF)
        virtual void setProportional(bool b);               // slot 2  Mac 0x42ff2e, Win 0x72c6e0
        virtual bool isProportional() const;                // slot 3  Mac 0x42ff7c, Win 0x72c720
        virtual void setMaxForce(VxReal f);                 // slot 4  Mac 0x42ff96, Win 0x72c730
        virtual VxReal getMaxForce() const;                 // slot 5  Mac 0x42fff2, Win 0x72c780
        virtual void setLoss(VxReal loss);                  // slot 6  Mac 0x430042, Win 0x72c7d0
        virtual VxReal getLoss() const;                     // slot 7  Mac 0x43009e, Win 0x72c820
        virtual void setCoefficient(VxReal mu);             // slot 8  Mac 0x4300b4, Win 0x72c830
        virtual VxReal getCoefficient() const;              // slot 9  Mac 0x430110, Win 0x72c880
        virtual void setScale(VxReal s);                    // slot 10 Mac 0x430126, Win 0x72c890
        virtual VxReal getScale() const;                    // slot 11 Mac 0x430182, Win 0x72c8e0
        virtual void setEnabled(bool b);                    // slot 12 Mac 0x430198, Win 0x72c8f0
        virtual bool isEnabled() const;                     // slot 13 Mac 0x4301e6, Win 0x72c930
        virtual void setAxis(int axis);                     // slot 14 Mac 0x430200, Win 0x72c940
        virtual int getAxis() const;                        // slot 15 Mac 0x43024a, Win 0x72c980

        VxConstraintFriction* mFriction[3];                 // Mac +0x34, Win +0x38 (name inferred)
    };
    AngularFriction* getAngularFriction() const { return mAngularFriction; } // (inline, name inferred)

    // Win only (Mac dead-stripped):
    VxBallAndSocket(VxPart* part0, VxPart* part1, const VxVector3& position, const VxVector3* axis); // Win 0x72c5c0
    void setPosition(const VxVector3& p);   // Win 0x72c0e0 (name inferred)
    void setAxis(const VxVector3& a);       // Win 0x72c110 (name inferred)
    // Win 0x72c1a0: MdtBSJoint +0xae8 = the part whose frame gives the position rows (-1 = world) (name inferred)
    void setPositionEquationReferencePart(int partIndex);

    AngularFriction* mAngularFriction; // Mac +0x8c, Win +0xa4

private:
    void addAccessors();                    // Mac 0x42f6b4, Win 0x72c2a0: the equation / coordinate accessors (name inferred)
};

} // namespace Vx

#endif
