// Vortex 5.0.0 build 800 "vortexldd41" - VxVisitor (restored header).
// Accepted by the frame / universe / assembly hierarchy (VxFrame Mac 0x453d74 walks the universes;
// VxUniverse Mac 0x480d42: slot 2, then - unless it returned non-zero - the root mechanism (Mac
// 0x425434), then slot 3). 0 = continue; the walk stops on 2. Default bodies (Mac 0x485364-0x4853a0,
// all "return 0"; Win 0x7c61f0) are in VxUniverse.cpp. Slot meanings beyond 2-4 inferred from the
// overriders: GatherAllCollisionSensor / FirstDynamicPartFinder / LineSegIntersectVisitor slot 6,
// GatherAllConstraints slot 7, NotificationVisitor<VxUniverse> slots 4-7.
#ifndef VX_VXVISITOR_H
#define VX_VXVISITOR_H

namespace Vx {

class VxUniverse;
class VxAssemblyBase;
class VxEntity;
class VxConstraint;

// VxVisitor (Mac 9 slots, pure virtual destructor (Mac D2 0x486cb2); Win 8) (method names inferred)
class VxVisitor {
public:
    virtual ~VxVisitor() = 0;                               // slot 0/1 (pure; body Mac 0x486cb2)
    virtual int visitUniverse(VxUniverse* u);               // slot 2 Mac 0x485364, Win 0x7c61f0
    virtual int endVisitUniverse(VxUniverse* u);            // slot 3 Mac 0x48536e
    virtual int visitAssembly(VxAssemblyBase* a);           // slot 4 Mac 0x485378
    virtual int endVisitAssembly(VxAssemblyBase* a);        // slot 5 Mac 0x485382
    virtual int visitEntity(VxEntity* e);                   // slot 6 Mac 0x48538c
    virtual int visitConstraint(VxConstraint* c);           // slot 7 Mac 0x485396
    virtual int visitOther(void* o);                        // slot 8 Mac 0x4853a0 (meaning unknown)
};

} // namespace Vx

#endif
