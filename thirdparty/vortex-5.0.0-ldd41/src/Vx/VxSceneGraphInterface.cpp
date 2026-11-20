// Vortex 5.0.0 build 800 "vortexldd41" - the default scene-graph and node interfaces (restored).
// Mac TU 492 0x458e66-0x459446 (file guess: the TU of
// (anonymous namespace)::TrivialSceneGraphInterface; static initialiser 0x4593e1 iostream only),
// Win 0x752d30-0x7533f0 (the empty / constant slots are ICF-shared Win stubs).
// LDD registers neither interface: only the defaults below run (the node interface through
// VxNode::addRef / release).
#include "Vx/VxSceneGraphInterface.h"

namespace Vx {

namespace {

// Mac vtable 0x75de68 (13 slots), 4 bytes (new(4) Mac 0x458fa9) (no RTTI name in the binaries
// beyond the anonymous class; name from the Ghidra export)
class TrivialSceneGraphInterface : public VxSceneGraphInterface {
public:
    virtual ~TrivialSceneGraphInterface() {}            // Mac D1 0x458f76, D0 0x458f4c, Win 0x7532c0
    virtual bool setNodeTransform(const VxNode&, const VxTransform&) { return false; } // Mac 0x458e66, Win 0x716340
    virtual bool setNodeTransformPtr(const VxNode& node, const VxReal* tm);            // Mac 0x459056, Win 0x752d80
    virtual bool getNodeTransform(const VxNode&, VxTransform&, VxVector3&, VxTransform*) { return false; } // Mac 0x458e70, Win 0x752d30
    virtual bool attachEntity(const VxNode&, VxEntity*) { return true; }               // Mac 0x458e84, Win 0x752d50
    virtual bool detachEntity(const VxNode&) { return false; }                         // Mac 0x458e8e, Win 0x7c2400
    virtual bool releaseNode(const VxNode&) { return false; }                          // Mac 0x458ea4, Win 0x7c61f0
    virtual void vf08() {}                              // Mac 0x458e98, Win 0x7076a0
    virtual void vf09() {}                              // Mac 0x458e9e, Win 0x7076a0
    virtual void vf10() {}                              // Mac 0x458fd4, Win 0x713eb0
    virtual bool vf11() { return true; }                // Mac 0x458fda, Win 0x752ce0
    virtual bool vf12() { return false; }               // Mac 0x458e7a, Win 0x752d40
};

// Mac 0x459056, Win 0x752d80
bool TrivialSceneGraphInterface::setNodeTransformPtr(const VxNode& node, const VxReal* tm)
{
    VxNode n = node;
    VxTransform nodeTM;
    VxVector3 scale;
    VxTransform parent;
    if (!getNodeTransform(n, nodeTM, scale, &parent))
        return false;
    VxReal px = parent.m[3][0] / scale[0];
    VxReal py = parent.m[3][1] / scale[1];
    VxReal pz = parent.m[3][2] / scale[2];
    VxTransform rel;
    for (int i = 0; i < 3; ++i) {
        const VxReal* w = tm + 4 * i;
        for (int j = 0; j < 3; ++j)
            rel.m[i][j] = w[0] * parent.m[j][0] + w[1] * parent.m[j][1] + w[2] * parent.m[j][2];
        rel.m[i][3] = 0;
    }
    VxReal dx = tm[12] - px, dy = tm[13] - py, dz = tm[14] - pz;
    for (int j = 0; j < 3; ++j)
        rel.m[3][j] = dx * parent.m[j][0] + dy * parent.m[j][1] + dz * parent.m[j][2];
    rel.m[3][3] = 1.0;
    VxNode n2 = node;
    return setNodeTransform(n2, rel);
}

// Mac 0x458fa9: the default factory
VxSceneGraphInterface* createTrivialSceneGraphInterface()
{
    return new TrivialSceneGraphInterface;
}

VxSceneGraphInterface::Factory sFactory = createTrivialSceneGraphInterface; // Mac 0x779374

// Mac 0x458fe4: the default node interface, a static object of the base class
VxNodeInterface* defaultNodeInterface()
{
    static VxNodeInterface sDefault; // Mac 0x779370 (guard 0x779368, atexit 0x458f8f)
    return &sDefault;
}

} // namespace

// Mac 0x458eae, Win 0x752d60
VxSceneGraphInterface::Factory& VxSceneGraphInterface::getFactory()
{
    return sFactory;
}

// Mac 0x458ebf, Win 0x753320
VxSceneGraphInterface* VxSceneGraphInterface::createDefault()
{
    return getFactory()();
}

// Mac 0x458eef
VxNodeInterface*& vxNodeInterfaceRef()
{
    static VxNodeInterface* sInterface = defaultNodeInterface(); // Mac 0x77fe60 (guard 0x77fe58)
    return sInterface;
}

// Mac 0x458f3d, Win 0x7533a0
VxNodeInterface* vxGetNodeInterface()
{
    return vxNodeInterfaceRef();
}

} // namespace Vx
