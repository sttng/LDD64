// Vortex 5.0.0 build 800 "vortexldd41" - VxControllerBase (restored header).
//
// The base of Vortex's controller framework. Every controller has the inputs "Enable" / "Reset",
// the outputs "Done" / "Reset" / "Active" and a three-state machine (inactive / active / done)
// driven by the Enable and Reset inputs. Every VxAssemblyBase (VxMechanism) embeds a
// VxCompositeController; VxControllerGraph (Vx/VxControllerGraph.h) is a VxFrame stepper that
// runs all controllers of the frame in dependency order each step. In LDD only the embedded
// composites exist (no concrete stepping controller is linked), but the graph runs every frame
// step.
//
// Mac TU 446 "VxControllerBase.cpp" (0x417d2c-0x41d6bf), Win 0x7f9890-0x7fb3e0 (+ the shared
// helpers 0x78bbb0-0x78cf10 that /LTCG placed next to VxCompositeController).
#ifndef VX_VXCONTROLLERBASE_H
#define VX_VXCONTROLLERBASE_H

#include "Vx/VxBase.h"
#include "Vx/VxArray.h"
#include "Vx/VxConnection.h"
#include "Vx/VxInputOutput.h"

namespace Vx {

class VxControllerBase;
class VxStateBase;

// --------------------------------------------------------------------------------------
// Observer of a controller (no RTTI survives: no implementation is linked) (all names
// inferred). The controller owns its listeners (deleted by ~VxControllerBase). Mac slots:
// 0/1 dtor, 2-7 below; the controller calls them through member-function pointers
// (Mac "mem_fun" offsets 9, 0xd, 0x11, 0x15, 0x19, 0x1d).
class VxControllerListener {
public:
    virtual ~VxControllerListener() {}
    virtual void preExecute(VxControllerBase* c) = 0;    // slot 2: before the graph runs c
    virtual void postExecute(VxControllerBase* c) = 0;   // slot 3: after the graph ran c
    virtual void onReset(VxControllerBase* c) = 0;       // slot 4
    virtual void onActivate(VxControllerBase* c) = 0;    // slot 5
    virtual void onDeactivate(VxControllerBase* c) = 0;  // slot 6
    virtual void onDone(VxControllerBase* c) = 0;        // slot 7
};

// --------------------------------------------------------------------------------------
// VxControllerBase (Mac vtable 0x75c868, 16 slots; Win 15; class id 0x17f Mac 0x417d2c)
// Mac 0x2fc bytes, Win 0x3ec. Abstract (slot 4 getClassName and slot 6 update are pure).
class VxControllerBase : public VxBaseUserData {
public:
    // the argument the Enable / Reset input delegates are bound with (RTTI name
    // "Vx::VxControllerBase::eStateChange"; enumerator names inferred)
    enum eStateChange {
        kEnableChanged = 0,
        kResetChanged = 1,
        kDoneChanged = 2
    };
    // mState (names inferred)
    enum State {
        kStateInactive = 0,
        kStateActive = 1,
        kStateDone = 2
    };
    // VxConnection::FieldInfo::kind values returned by getInput/getOutput/getParameter
    enum FieldKind { // (names inferred)
        kFieldInput = 1,
        kFieldOutput = 2,
        kFieldParameter = 3
    };
    typedef VxConnection::FieldInfo FieldInfo;

    VX_DECLARE_CLASS_TYPES();                      // slots 0/1: Mac 0x4188d0 / 0x417d36, Win 0x7f9df0 / 0x7f99f0; types Mac 0x418548
    VxControllerBase();                            // Mac 0x418dc0, Win 0x7f9ee0
    virtual ~VxControllerBase();                   // slot 2/3 Mac 0x418dac (-> 0x418b20) / 0x41948c, Win 0x7fb3e0 (-> 0x7fa180)
    // slot 4 getClassName: pure; slot 5 setName: VxBase's
    virtual void update() = 0;                     // slot 6: run once by the graph after an input changed (name inferred)
    virtual FieldInfo getInput(const VxID& name);      // slot 7 Mac 0x419a68, Win 0x78ce70 (name inferred)
    virtual FieldInfo getParameter(const VxID& name);  // slot 8 Mac 0x41a23a, Win 0x78cec0 (name inferred)
    virtual FieldInfo getOutput(const VxID& name);     // slot 9 Mac 0x41a1f4, Win 0x78cf10 (name inferred)
    virtual VxStateBase* createState();            // slot 10 Mac 0x4184ce, Win 0x7f9890: new VxControllerState(this) (name inferred)
    virtual void onActivate();                     // slot 11 Mac 0x419756, Win 0x7076a0 (empty) (name inferred)
    virtual void onDeactivate();                   // slot 12 Mac 0x41975c, Win 0x7076a0 (empty) (name inferred)
    virtual void onReset();                        // slot 13 Mac 0x419762, Win 0x7076a0 (empty) (name inferred)
    virtual void onDone();                         // slot 14 Mac 0x419768, Win 0x7076a0 (empty) (name inferred)
    virtual void setParent(VxBase* parent);        // slot 15 Mac 0x4184a6, Win 0x7f9940 (name inferred)

    // --- state machine (names inferred) ---
    // the callback of the Enable / Reset input delegates (boostVx::bind(&stateChanged, this,
    // kEnableChanged / kResetChanged, _1)) and of setDone (Mac 0x417eda, Win 0x7f9a50)
    void stateChanged(eStateChange change, bool value);
    void reset();                                  // Mac 0x4182e4, Win 0x7f9d00: stateChanged(kResetChanged, true)
    void setDone();                                // Mac 0x418308, Win 0x7f9c80: stateChanged(kDoneChanged, true)
    // Mac 0x40a3aa (emitted in VxCompositeController.cpp, used as a member-function pointer)
    bool isActive() const { return mState == kStateActive && sControllersEnabled; }
    int getState() const { return mState; }

    // the graph's per-step call (Mac 0x417e82, Win 0x7f9e20) (name inferred)
    void execute();
    // Mac 0x417d4c (used by the graph through a member-function pointer): notify all inputs
    // (Inputs container slot 16) (name inferred)
    void notifyInputs();

    // --- stepping set of the parent composite (Mac 0x418442, Win 0x7f9900) (name inferred) ---
    void setSteppingEnabled(bool enabled);
    bool isSteppingEnabled() const { return mSteppingEnabled; }

    VxBase* getParent() const { return mParent; }
    bool getInputsChanged() const { return mInputsChanged; }           // Mac 0x41fae6 (in VxControllerGraph.cpp)
    void setInputsChanged(bool changed) { mInputsChanged = changed; }  // (connections, V1.2)

    // listeners (Win 0x7f9ea0 add-if-absent, 0x7f9a10 remove; inlined on Mac) (names inferred)
    void addListener(VxControllerListener* l);
    void removeListener(VxControllerListener* l);

    VxInputOutputContainer& getFields() { return mFields; }
    VxInput<bool>& getEnableInput() { return mEnable; }
    VxInput<bool>& getResetInput() { return mReset; }
    VxOutput<bool>& getDoneOutput() { return mDone; }
    VxOutput<bool>& getResetOutput() { return mResetOut; }
    VxOutput<bool>& getActiveOutput() { return mActive; }

    // Mac 0x779184 (__data, = 1): a global switch read by isActive / execute / onReset
    // (no writer in the linked code) (name inferred)
    static bool sControllersEnabled;

    VxInputOutputContainer mFields;   // Mac +0x20, Win +0x38 ("ControllerFields")
    VxInput<bool> mEnable;            // Mac +0xd4, Win +0x14c ("Enable", true)
    VxInput<bool> mReset;             // Mac +0x13c, Win +0x1cc ("Reset", false)
    VxOutput<bool> mDone;             // Mac +0x1a4, Win +0x24c ("Done", false)
    VxOutput<bool> mResetOut;         // Mac +0x20c, Win +0x2cc ("Reset", false; mirrors the Reset input)
    VxOutput<bool> mActive;           // Mac +0x274, Win +0x34c ("Active", true)
    VxBase* mParent;                  // Mac +0x2dc, Win +0x3cc (a VxCompositeController or 0)
    bool mInputsChanged;              // Mac +0x2e0, Win +0x3d0 (= true)
    int mState;                       // Mac +0x2e4, Win +0x3d4 (State, = kStateActive)
    bool mSteppingEnabled;            // Mac +0x2e8, Win +0x3d8 (= true)
    VxArray<VxControllerListener*> mListeners; // Mac +0x2ec, Win +0x3dc

private:
    void notifyListeners(void (VxControllerListener::*fn)(VxControllerBase*));
    VxControllerBase(const VxControllerBase&);
    VxControllerBase& operator=(const VxControllerBase&);
};

} // namespace Vx

#endif
