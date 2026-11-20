// Vortex 5.0.0 build 800 "vortexldd41" - scene-graph node handle and interface (restored header;
// the parts VxEntity needs; all names inferred).
//
// A VxEntity can be bound to a node of the application's scene graph (VxEntity::setNode, Mac
// 0x446d6e). The node is an opaque handle whose reference count is kept by the registered node
// interface (Mac 0x458f3d returns it: slot 2 addRef, slot 3 release). The scene-graph interface
// lives in VxFrame::mSceneGraphInterface (Mac +0xd4) and is created lazily (Mac 0x458ebf, the
// TrivialSceneGraphInterface factory, V4.5) - the original inlines that "get or create" into its
// callers (VxEntity Mac 0x446cda / 0x446d6e / 0x447a94).
//
// LDD never binds a node (no VX_GRAPHICS_REGISTER_NODE_INTERFACE): every path below is
// link-only at run time.
#ifndef VX_VXSCENEGRAPHINTERFACE_H
#define VX_VXSCENEGRAPHINTERFACE_H

#include "Vx/VxFrame.h"
#include "Vx/VxTransform.h"
#include "Vx/VxVector3.h"
#include <string>

namespace Vx {

class VxEntity;

// 4 bytes on both platforms (name inferred)
class VxNode {
public:
    VxNode() : mHandle(0) {}
    void addRef();              // Mac 0x4673f6 (TU 506, src/Vx/VxNode.cpp, V4.3): node interface slot 2
    void release();             // Mac 0x4673c6, Win 0x760b30 (V4.3): slot 3; clears the handle when it returns 0
    bool operator==(const VxNode& o) const { return mHandle == o.mHandle; }
    bool operator!=(const VxNode& o) const { return mHandle != o.mHandle; }
    static const VxNode sNull;  // Mac 0x77cdfc, Win 0xaf2ff8 (bss; defined in src/Vx/VxNodeInterface.cpp, V4.6)

    void* mHandle;              // +0x00
};

// The node interface (V4.3 / V4.6: src/Vx/VxNodeInterface.cpp = Mac TU 507 0x467482-0x467efd,
// Win 0x752d00-0x7534e0 + 0x7ac7a0). The default instance is a static object of this class
// itself (Mac vtable 0x75e340, in VxSceneGraphInterface.cpp), so every slot is concrete:
// addRef / release return 1, the others
// warn "VxNodeInterface: default implementation of %s() is used..." (VX_GRAPHICS_REGISTER_NODE_
// INTERFACE) and return a neutral value; only slot 17 does real work (through the scene-graph
// interface). Method names from the warning strings; parameter lists from the Win callee
// cleanup sizes (unknown 4-byte arguments are kept as ints) (V4.6).
struct VxNodeBoundingBox {     // (name inferred) 0x30 bytes: empty box = (DBL_MAX..., -DBL_MAX...)
    VxVector3 min;
    VxVector3 max;
};
struct VxNodeBoundingSphere {  // (name inferred) 0x20 bytes: empty sphere = radius -1
    VxVector3 center;
    VxReal radius;
};
class VxNodeInterface {
public:
    virtual ~VxNodeInterface();                                         // Mac D1 0x46764a / D0 0x467620, Win 0x752d00
    virtual int addRef(const VxNode& node);                             // slot 2 Mac 0x4675d8, Win 0x752cf0 (1)
    virtual int release(const VxNode& node);                            // slot 3 Mac 0x4675e2, Win 0x752cf0 (1; 0 = the node is gone)
    virtual std::string getName(const VxNode& node);                    // slot 4 Mac 0x467894, Win 0x7533f0 ("NoAccessToName")
    virtual void setName(const VxNode& node, const std::string& name);  // slot 5 Mac 0x467860, Win 0x752e60
    virtual std::string getClassName(const VxNode& node);               // slot 6 Mac 0x4677fc, Win 0x753440 ("NoAccessToName")
    virtual void addChild(const VxNode& parent, const VxNode& child, int a3, int a4, int a5); // slot 7 Mac 0x467ab8, Win 0x752e80
    virtual void removeChild(const VxNode& parent, const VxNode& child); // slot 8 Mac 0x467a84, Win 0x752ea0
    virtual int getNumChildren(const VxNode& node);                     // slot 9 Mac 0x467a4c, Win 0x752ec0 (0)
    virtual VxNode getChild(const VxNode& node, int index);             // slot 10 Mac 0x467a00, Win 0x752ee0 (sNull)
    virtual VxNode findChild(const VxNode& node, const std::string& name); // slot 11 Mac 0x4679b4, Win 0x752f10 (sNull)
    virtual bool hasChild(const VxNode& node, const VxNode& child);     // slot 12 Mac 0x46797c, Win 0x752f40 (false)
    virtual VxNode getParent(const VxNode& node, int index);            // slot 13 Mac 0x467930, Win 0x752f60 (sNull)
    virtual int getNumParents(const VxNode& node);                      // slot 14 Mac 0x4678f8, Win 0x752f90 (0)
    virtual void validateOrInsertTransform(const VxNode& node);         // slot 15 Mac 0x4675ec, Win 0x752fb0
    virtual VxTransform getTransform(const VxNode& node);               // slot 16 Mac 0x467e4c, Win 0x752fd0 (identity)
    // slot 17 Mac 0x4674a4, Win 0x7ac7a0: the node's transform from the frame's scene-graph
    // interface (getNodeTransform), identity for the null node; no warning (name inferred)
    virtual VxTransform getNodeTransform(const VxNode& node);
    virtual void setTransform(const VxNode& node, const VxTransform& tm); // slot 18 Mac 0x467e18, Win 0x753000
    virtual void enableCulling(const VxNode& node, bool b);             // slot 19 Mac 0x467de4, Win 0x753020
    virtual bool isCullingEnabled(const VxNode& node);                  // slot 20 Mac 0x467dac, Win 0x753040 (false)
    virtual void enableLighting(const VxNode& node, bool b);            // slot 21 Mac 0x467d78, Win 0x753060
    virtual bool isLightingEnabled(const VxNode& node);                 // slot 22 Mac 0x467d40, Win 0x753080 (false)
    virtual void enableFog(const VxNode& node, bool b);                 // slot 23 Mac 0x467d0c, Win 0x7530a0
    virtual bool isFogEnabled(const VxNode& node);                      // slot 24 Mac 0x467cd4, Win 0x7530c0 (false)
    virtual void hide(const VxNode& node, bool b, int a3, int a4);      // slot 25 Mac 0x467ca0, Win 0x7530e0
    virtual bool isHidden(const VxNode& node);                          // slot 26 Mac 0x467c68, Win 0x753100 (true)
    virtual bool isSwitchNodeON(const VxNode& node);                    // slot 27 Mac 0x467c30, Win 0x753120 (true)
    virtual VxNodeBoundingBox getBoundingBox(const VxNode& node);       // slot 28 Mac 0x467bba, Win 0x753140
    virtual VxNodeBoundingSphere getBoundingSphere(const VxNode& node); // slot 29 Mac 0x467b58, Win 0x753180
    virtual int getNodeType(const VxNode& node);                        // slot 30 Mac 0x467b20, Win 0x7531c0 (0)
    virtual void setEffect(const VxNode& node, int a2, int a3);         // slot 31 Mac 0x467aec, Win 0x7531e0
    virtual void setColor(const VxNode& node, const void* color);       // slot 32 Mac 0x4677c8, Win 0x753200
    virtual VxNode clone(const VxNode& node, int a2);                   // slot 33 Mac 0x46777c, Win 0x753220 (sNull)
    virtual VxNode load(const char* fileName, int a2);                  // slot 34 Mac 0x467730, Win 0x753250 (sNull)
    virtual void save(const VxNode& node, const char* fileName, int a3); // slot 35 Mac 0x4676fc, Win 0x753280
    virtual void updateNodeTreeFromParts(const VxNode& node);           // slot 36 Mac 0x4676c8, Win 0x7532a0
    virtual std::string getNodeTypeName(const VxNode& node);            // slot 37 Mac 0x467664, Win 0x753490 ("")
};
// Mac 0x458f3d, Win 0x7533a0: the registered node interface (V4.5, VxSceneGraphInterface.cpp)
VxNodeInterface* vxGetNodeInterface();

// Mac slots (D1, D0 first) 2 .. 12 (vtable of the trivial implementation Mac 0x75de68, V4.5:
// src/Vx/VxSceneGraphInterface.cpp = Mac TU 492, Win 0x752d30-0x7533f0). Return types from the
// trivial implementation (which returns 0 / 1 from slots 2, 4-7, 11, 12). (names inferred)
class VxSceneGraphInterface {
public:
    typedef VxSceneGraphInterface* (*Factory)();

    virtual ~VxSceneGraphInterface() {}
    virtual bool setNodeTransform(const VxNode& node, const VxTransform& tm) = 0;   // slot 2
    // slot 3 (Mac 0x459056 in the trivial class): a world transform made relative to the node's
    // parent (slot 4's parent transform, its translation divided by the node scale), then slot 2
    virtual bool setNodeTransformPtr(const VxNode& node, const VxReal* tm) = 0;
    // slot 4: the node's transform, scale and (when parentWorld != 0) its parent's world
    // transform; false when the node has none (VxEntity passes parentWorld = 0)
    virtual bool getNodeTransform(const VxNode& node, VxTransform& tm, VxVector3& scale, VxTransform* parentWorld) = 0;
    virtual bool attachEntity(const VxNode& node, VxEntity* entity) = 0;            // slot 5
    virtual bool detachEntity(const VxNode& node) = 0;                              // slot 6
    virtual bool releaseNode(const VxNode& node) = 0;                               // slot 7 (VxFrame Mac 0x453ec2)
    virtual void vf08() = 0;                            // slot 8 (meaning unknown; trivial: empty)
    virtual void vf09() = 0;                            // slot 9 (meaning unknown; trivial: empty)
    virtual void vf10() = 0;                            // slot 10 (meaning unknown; trivial: empty)
    virtual bool vf11() = 0;                            // slot 11 (meaning unknown; trivial: true)
    virtual bool vf12() = 0;                            // slot 12 (meaning unknown; trivial: false)

    // Mac 0x458ebf, Win 0x753320: the registered factory's interface (default: a new
    // TrivialSceneGraphInterface, Mac 0x458fa9)
    static VxSceneGraphInterface* createDefault();
    // Mac 0x458eae, Win 0x752d60: the factory (a static function pointer, Mac 0x779374)
    static Factory& getFactory();
};

// Mac 0x458eef: the registered node interface (a function-local static pointer, initialised
// with the default interface, Mac 0x458fe4: a static VxNodeInterface) (V4.5) (name inferred)
VxNodeInterface*& vxNodeInterfaceRef();

// The "get or create" the original inlines (Mac 0x446cda: frame +0xd4, then 0x458ebf).
inline VxSceneGraphInterface* vxSceneGraphInterface(VxFrame* frame)
{
    if (frame->mSceneGraphInterface == 0)
        frame->mSceneGraphInterface = VxSceneGraphInterface::createDefault();
    return (VxSceneGraphInterface*)frame->mSceneGraphInterface;
}

} // namespace Vx

#endif
