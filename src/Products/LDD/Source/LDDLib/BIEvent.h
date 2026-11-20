// LEGO Digital Designer — restored source
// LEGO::BIEvent — one event of a building guide as the generators and the building-guide modes
//   see it: a brick (a BIBrickPrimitive or a BIBrickSubstep) to add, a step boundary (with the
//   camera of the step), a marker, or a jump. BIStorage converts its stored steps into these
//   (BIStorage::GetEvents) and back (BIStorage::SetEvents).
//
// Home TU 357 "BIEvent.cpp" (Mac 0x33ed70-0x33f02f; modmap guess BIEventBrick.cpp). The base
// class has no vtable of its own on Mac (abstract, constructor inlined); Win vtable 0xa2b8d4
// {sdd 0x5531a0, OnFinalRelease, AddRef, Release, GetBrick 0x553130, pure, pure}.
// Layout (both): Object +0x00, m_type +0x08, subclass member +0x0c.
//
// Slots (Mac index / Win slot): GetBrick 5 / 4, Apply 6 / 5, Flag 7 / 6.
// Type values: 0 marker (BIEventUnknown), 1 brick, 3 step, 4 jumper (Win ctor 0x566160 passes 4
// to the base constructor 0x5530a0). Value 2 is never constructed in the restored code.
#pragma once
#include "LEGO/Types.h"
#include "Object.h"
#include "BasicArray.h"
#include "StorageScene2.h"

namespace LEGO {

class BIBrick;
class BGScene;

class BIEvent : public Object {
public:
    // m_type values (names inferred)
    enum Type {
        TypeMarker = 0,    // BIEventUnknown
        TypeBrick  = 1,    // BIEventBrick
        TypeStep   = 3,    // BIEventStep
        TypeJumper = 4     // BIEventJumper
    };

    // Inlined on Mac; Win 0x5530a0 (thiscall, type on the stack)
    explicit BIEvent(UInt32 type) : m_type(type) {}
    virtual ~BIEvent() {}

    // The brick of a brick event, else null (the other subclasses share Mac 0x33eec0,
    // Win 0x553130). (name inferred)
    virtual BIBrick* GetBrick() const { return 0; }
    // Applies the event to the building-guide scene: a brick event adds (`forward`) or removes
    // its brick; the other events do nothing (step / jumper report true, the marker false).
    // (name inferred)
    virtual bool Apply(BGScene* scene, bool forward) = 0;
    // Returns false for every event type (meaning unknown). (name inferred)
    virtual bool GetFlag() const = 0;

    UInt32 GetType() const { return m_type; }

    UInt32 m_type;   // +0x08 Type
};

// A brick to add. Mac vtable 0x74d030 {D1 0x33ee10, D0 0x33ee20, …, 0x33ee60, 0x33ed70,
// 0x33eda0}, Win vtable 0xa2b910 {sdd 0x5532a0, …, 0x5d5710, 0x577aa0, 0x577af0}.
// sizeof 0x10. Constructor inlined on Mac, Win 0x5531d0.
class BIEventBrick : public BIEvent {
public:
    // The brick is not reference counted (it belongs to the BIBrickRepository).
    explicit BIEventBrick(BIBrick* brick) : BIEvent(TypeBrick), m_brick(brick) {}
    virtual ~BIEventBrick();                                               // Mac 0x33ee10, Win 0x553240

    virtual BIBrick* GetBrick() const;                                     // Mac 0x33ee60, Win 0x5d5710
    virtual bool Apply(BGScene* scene, bool forward);                      // Mac 0x33ed70, Win 0x577aa0
    virtual bool GetFlag() const;                                          // Mac 0x33eda0, Win 0x577af0

    BIBrick* m_brick;   // +0x0c
};

// A step boundary; the camera (may be null) is the one of the step. Mac vtable 0x74d09c
// {D1 0x33eed0, D0 0x33ef30, …, 0x33eec0, 0x33edd0, 0x33ede0}, Win vtable 0xa2b994.
// sizeof 0x10. Constructor inlined on both builds.
class BIEventStep : public BIEvent {
public:
    explicit BIEventStep(StorageScene2::Camera* camera = 0) : BIEvent(TypeStep), m_camera(camera) {}
    virtual ~BIEventStep();                                                // Mac 0x33eed0, Win 0x55a480

    virtual bool Apply(BGScene* scene, bool forward);                      // Mac 0x33edd0, Win 0x577b00
    virtual bool GetFlag() const;                                          // Mac 0x33ede0, Win 0x577af0

    RefPointer<StorageScene2::Camera> m_camera;   // +0x0c
};

// A marker (BIGenerator::AppendMarker / RemoveMarker). Mac vtable 0x74d064 {D1 0x33ee70,
// D0 0x33ee80, …, 0x33eec0, 0x33edb0, 0x33edc0}, Win vtable 0xa2ba28. sizeof 0x0c.
class BIEventUnknown : public BIEvent {
public:
    BIEventUnknown() : BIEvent(TypeMarker) {}
    virtual ~BIEventUnknown();                                             // Mac 0x33ee70

    virtual bool Apply(BGScene* scene, bool forward);                      // Mac 0x33edb0, Win 0x57baa0
    virtual bool GetFlag() const;                                          // Mac 0x33edc0, Win 0x577af0
};

// A jump (BGAutomaticApplication::InsertJumper creates it with 2). Mac vtable 0x74d0d4
// {D1 0x33efa0, D0 0x33efb0, …, 0x33eec0, 0x33edf0, 0x33ee00}, Win vtable 0xa2beec;
// Win constructor 0x566160, destructor 0x5661d0. sizeof 0x10.
class BIEventJumper : public BIEvent {
public:
    explicit BIEventJumper(UInt32 jump) : BIEvent(TypeJumper), m_jump(jump) {}
    virtual ~BIEventJumper();                                              // Mac 0x33efa0, Win 0x5661d0

    virtual bool Apply(BGScene* scene, bool forward);                      // Mac 0x33edf0, Win 0x577b00
    virtual bool GetFlag() const;                                          // Mac 0x33ee00, Win 0x577af0

    UInt32 m_jump;   // +0x0c (name inferred)
};

} // namespace LEGO
