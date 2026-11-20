// Vortex 5.0.0 build 800 "vortexldd41" - VxCompositeController (restored header).
//
// A stepping controller made of child controllers and the connections between their fields.
// Every VxAssemblyBase embeds one (Mac +0x54, Win +0x70); the assembly forwards its name, its
// universe and its three step phases to it, and exposes fields of its parts through it.
// VxControllerGraph walks the composites of all assemblies to build the execution order.
//
// Mac SUB-TU 442e "VxCompositeController.cpp" (0x4083ac-0x40aa90), Win 0x78b450-0x78d230.
// The Mac build dead-stripped the child / template management that LDD never calls
// (addController, removeController, the templates); those are restored from Win.
#ifndef VX_VXCOMPOSITECONTROLLER_H
#define VX_VXCOMPOSITECONTROLLER_H

#include "Vx/VxSteppingController.h"
#include "Vx/VxConnection.h"
#include <string>

namespace Vx {

// --------------------------------------------------------------------------------------
// InternalConnection (Mac vtable 0x75d1c0; D0 0x42c22c / D1 0x42c264 emitted in
// VxAssemblyBase.cpp; Win 0x728cc0): the connection kind a composite creates for its own
// exposed fields. Adds nothing to VxConnection.
class InternalConnection : public VxConnection {
public:
    InternalConnection() {}
    virtual ~InternalConnection() {}
};

// --------------------------------------------------------------------------------------
// VxControllerTemplate (Win-only RTTI, vtable slot 0 Win 0x7fdf40; dead-stripped on Mac where
// only the composite's array of them is left): a named description of a controller set-up.
// Layout beyond the name is not understood; the sizes are Mac 0x28 / Win 0x58 (Mac 0x408de2
// destroys the elements with a 0x28 stride, Win 0x78b8e0 with 0x58).
class VxControllerTemplate {
public:
    VxControllerTemplate() {}
    virtual ~VxControllerTemplate() {}                // Win 0x7fdf40
    const VxID& getName() const { return mName; }      // Win 0x7fd5b0 (name inferred)

    VxID mName;          // +0x04
    std::string m0c;     // Mac +0x0c, Win +0x24 (meaning unknown)
    int m10[6];          // Mac +0x10, Win +0x40 (meaning unknown)
};

// --------------------------------------------------------------------------------------
// VxCompositeController (Mac vtable 0x75ade8, 22 slots; Win 21). It keeps VxSteppingController's
// class id (0x180): slots 0/1 are not overridden. Mac 0x354 bytes, Win 0x444.
class VxCompositeController : public VxSteppingController {
public:
    VxCompositeController();                           // Mac C2 0x408510 / C1 0x4085d2, Win 0x78b5a0
    virtual ~VxCompositeController();                  // slot 2/3 Mac 0x409066 (-> 0x408de2) / 0x40907a, Win 0x78d230 (-> 0x78b8e0)
    virtual const char* getClassName() const;          // slot 4 Mac 0x40a41c, Win 0x78ce60
    virtual void update();                             // slot 6 Mac 0x4083ac, Win 0x78b450 (empty)
    virtual void onActivate();                         // slot 11 Mac 0x408758, Win 0x78b810
    virtual void onDeactivate();                       // slot 12 Mac 0x408700, Win 0x78b840
    virtual void onReset();                            // slot 13 Mac 0x4087b0, Win 0x78b870
    virtual void preStep();                            // slot 16 Mac 0x408aa6, Win 0x78ba80
    virtual void step();                               // slot 17 Mac 0x40898c, Win 0x78bac0
    virtual void postStep();                           // slot 18 Mac 0x4085e6, Win 0x78bb00
    virtual void setUniverse(VxUniverse* u);           // slot 19 Mac 0x40845c, Win 0x78b7c0
    // slot 21 Mac 0x408436, Win 0x78b4b0 (bound to a Field<bool> delegate: RTTI of
    // DelegateCB<Field<bool> const&, bind_t<void, mf1<void, VxCompositeController, bool>, ...>>)
    // (name inferred)
    virtual void onDoneChanged(bool done);

    // --- children (names inferred) ---
    int getControllerCount() const { return mControllers.size(); }
    VxControllerBase* getController(int i) const;      // Mac 0x4083b2, Win 0x78b4f0 (0 when out of range)
    void addController(VxControllerBase* c);           // Win 0x78bc10 (dead-stripped on Mac)
    void removeController(VxControllerBase* c);        // Win 0x78b6a0 (dead-stripped on Mac)
    // a child's Enable input follows the composite (Mac 0x4083e3, Win 0x78b680)
    static void setChildEnabled(VxControllerBase* child, bool enabled);
    // add / remove a child to / from the set enabled by onActivate, after its
    // setSteppingEnabled (Mac 0x40980e, Win 0x78bbb0)
    void updateSteppingSet(VxControllerBase* child);

    // --- connections (names inferred) ---
    int getConnectionCount() const { return mConnections.size(); }
    VxConnection* getConnection(int i) const;          // Win 0x78b520
    bool addConnection(VxConnection* c);               // Mac 0x408bc0, Win 0x78bb40 (false if present)
    bool removeConnection(VxConnection* c);            // Mac 0x40888e, Win 0x78b610 (false if absent)
    void deleteConnection(VxConnection* c);            // Mac 0x40893c, Win 0x78b780: remove + delete
    // new InternalConnection from -> to, added to this composite (Mac 0x408d3e, Win 0x78bd20)
    VxConnection* createConnection(const FieldInfo& from, const FieldInfo& to, int type);

    // --- exposing a field of a part / child as a field of the composite (names inferred) ---
    // The field is cloned into this controller's container under `name` (or, with makeUnique,
    // under a name made unique among the existing ones; an existing field of that name is
    // reused when makeUnique is false) and connected to the original (inputs and parameters:
    // composite -> field; outputs: field -> composite). Called by VxAssemblyBase's field
    // exposure (Mac 0x428bcc).
    void exposeInput(const VxID& name, VxData::FieldBase* field, int kind, bool makeUnique);     // Mac 0x4099c0, Win 0x78bda0
    void exposeOutput(const VxID& name, VxData::FieldBase* field, int kind, bool makeUnique);    // Mac 0x409ec2, Win 0x78c060
    void exposeParameter(const VxID& name, VxData::FieldBase* field, int kind, bool makeUnique); // Mac 0x409304, Win 0x78c310

    // --- templates (Win only) (names from the Win warning strings) ---
    int getTemplateCount() const { return mTemplates.size(); }
    VxControllerTemplate* getTemplate(int i);          // Win 0x78b550
    void addTemplate(const VxControllerTemplate& t);   // Win 0x78c8a0
    void removeTemplate(const VxControllerTemplate& t); // Win 0x78c980

    VxArray<VxControllerBase*> mControllers;           // Mac +0x304, Win +0x3f4
    VxArray<VxSteppingController*> mSteppingControllers; // Mac +0x314, Win +0x404
    VxArray<VxConnection*> mConnections;               // Mac +0x324, Win +0x414
    VxArray<VxControllerTemplate> mTemplates;          // Mac +0x334, Win +0x424
    VxArray<VxControllerBase*> mEnabledControllers;    // Mac +0x344, Win +0x434 (stepping-enabled children)

private:
    // the common body of the three expose functions (inlined three times in both builds)
    void expose(VxData::Container& container, const VxID& name, VxData::FieldBase* field, int kind,
                bool makeUnique, int exposedKind);
};

} // namespace Vx

#endif
