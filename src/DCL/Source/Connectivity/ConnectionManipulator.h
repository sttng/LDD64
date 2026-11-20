// LEGO Digital Designer — restored source
// LEGO::ConnectionManipulator — interactive manipulation of one joint (turning a hinge or a ball
// joint; hinge tool, TU 056) and its implementations PrimitiveConnectionManipulator
// (PrimitiveElement::CreateManipulator, Mac 0x9dea0: cycles the element's primitive connections,
// may release a rigid one for the time) and RigidConnectionManipulator
// (RigidElement::CreateManipulator, Mac 0xa2840: cycles the body's rigid connections). The Scene
// keeps the active one (Scene::m_manipulator, Mac +0x11c, not referenced) and calls Detach before
// structural changes.
// ConnectionManipulator.cpp = sub-TU 025i of the original Scene.cpp (Mac 0x9cc30-0x9d7f0, + 025s
// 0xc40b0-0xc41c0, 0xc53a0-0xc5d10, 0xc74a0-0xc79c0, + the two CreateManipulator functions; Win
// 0x475230-0x475ed0, 0x470330-0x470460). Mac 0x9d050 (Win 0x48ac40) is
// Scene::CollectManipulatedElements.
// RTTI: class LEGO::ConnectionManipulator : Object (non-virtual base).
//   Mac sizeof 0x3c, vtable 0x70681c {D1 0xc40b0, D0 0xc4150, OnFinalRelease 0x9cef0, AddRef,
//       Release, pure SelectNext, pure Detach, pure Reset}.
//   Win sizeof 0x40 (the double is 8-aligned: m_position +0x18, m_connections +0x20, m_elements
//       +0x30); vftable {sdd, OnFinalRelease 0x4753c0, AddRef, Release, …}; constructor 0x470330,
//       destructor 0x4703c0 (sdd 0x470460).
// RTTI: class LEGO::PrimitiveConnectionManipulator : ConnectionManipulator. Mac sizeof 0x4c,
//   vtable 0x762270 {0xc53a0, 0xc5440, 0x9cef0, …, 0xc54f0, 0xc5510, 0xc5570}; Win sizeof 0x50
//   {sdd 0x475ba0, …, 0x475440, 0x475870, 0x4758c0}.
// RTTI: class LEGO::RigidConnectionManipulator : ConnectionManipulator. Mac sizeof 0x44,
//   vtable 0x762388 {0xc74a0, 0xc7540, 0x9cef0, …, 0xc75f0, 0xc7610, 0xc7700}; Win sizeof 0x48
//   {sdd 0x475eb0, …, 0x475bc0, 0x475e70, …}, destructor 0x475e20.
// Every member and method name is inferred.
#pragma once
#include "LEGO/Types.h"
#include "Object.h"
#include "BasicArray.h"
#include "BrickPhysics.h"
#include "Scene.h"

namespace LEGO {

class RigidConnection;
class RigidElement;
class PrimitiveElement;
class PrimitiveConnection;

// The joint interfaces queried from RigidConnection::m_joint (function-local static ids of
// Mac 0x9cc30 / 0x9cda0): PhysicsHingeConnection and PhysicsBallConnection
// (Physics/BrickPhysics.h).

class ConnectionManipulator : public Object {
public:
    // Mac inlined into 0x9dea0 / 0xa2840, Win 0x470330
    explicit ConnectionManipulator(Scene* scene);
    // Mac 0xc40b0 / 0xc4150, Win 0x4703c0 / sdd 0x470460
    virtual ~ConnectionManipulator();

    // --- slot 5: the next connection of the element / body that can move (cycling;
    // `wrapped` (optional): another connection was selected). False when none is active.
    virtual bool SelectNext(bool* wrapped) = 0;
    // --- slot 6: ends the manipulation (restores a temporarily released connection) and
    // leaves Scene::m_manipulator.
    virtual void Detach() = 0;
    // --- slot 7: forgets everything (scene, element, connection) without restoring.
    virtual void Reset() = 0;

    // Turns the active hinge joint by `delta`; m_position follows. Mac 0x9cc30, Win 0x475230
    bool   Rotate(bool interactive, Real32 delta);
    // Current angle of the active hinge joint (0 if it is none). Mac 0x9cd00, Win 0x4752b0
    Real32 GetAngle() const;
    // Turns the active ball joint by `delta` about `axis` (PhysicsBallConnection::Rotate).
    // Mac 0x9cda0, Win 0x475300
    bool   Slide(bool interactive, Real32 delta, const Vector3* axis);
    // The three angles of the active ball joint (PhysicsBallConnection::GetAngles into the
    // outputs). NOTE: returns false in both builds. Mac 0x9ce60, Win 0x475370
    bool   SlideUnknown(Real32* a, Real32* b, Real32* c);

    RigidConnection* GetConnection() const { return m_connection; }
    RigidElement*    GetFixedElement() const { return m_fixed; }
    Real64           GetPosition() const { return m_position; }
    const BasicArray<Scene::ActiveRigidElement>& GetActiveElements() const { return m_elements; }

protected:
    // Mac 0x9cef0, Win 0x4753c0 (calls Detach)
    virtual void OnFinalRelease();

    // Makes `connection` the active joint (`fixed`: the body that stays; m_position = the
    // hinge angle) and collects the bodies it moves. Mac 0x9cf10, Win 0x4753d0
    void SetConnection(RigidConnection* connection, RigidElement* fixed);
    // No active joint any more (only when there is one). Win 0x475cc0 (Mac inlined)
    void ResetConnection();
    // Slot 7 of both implementations without the element fields. (inline)
    void ResetAll();

public:
    // ---- data (Mac offsets) -------------------------------------------------------------------
    RigidConnection*                      m_connection;   // +0x08 active joint
    RigidElement*                         m_fixed;        // +0x0c
    Scene*                                m_scene;        // +0x10
    Real64                                m_position;     // +0x14 (Win +0x18) hinge angle
    BasicArray<RigidConnection*>          m_connections;  // +0x1c (Win +0x20)
    BasicArray<Scene::ActiveRigidElement> m_elements;     // +0x2c (Win +0x30)

private:
    ConnectionManipulator(const ConnectionManipulator&);
    ConnectionManipulator& operator=(const ConnectionManipulator&);
};

class PrimitiveConnectionManipulator : public ConnectionManipulator {
public:
    // Starts at the element's first connection. Mac inlined into 0x9dea0
    explicit PrimitiveConnectionManipulator(PrimitiveElement* element);
    virtual ~PrimitiveConnectionManipulator();                  // Mac 0xc53a0 / 0xc5440, Win sdd 0x475ba0
    // Mac 0xc54f0 (-> 0xc5720), Win 0x475440
    virtual bool SelectNext(bool* wrapped);
    // Mac 0xc5510, Win 0x475870
    virtual void Detach();
    // Mac 0xc5570, Win 0x4758c0
    virtual void Reset();

    // Cycles from m_current to the next selectable connection. Mac 0xc5720
    bool Next(bool* wrapped);
    // Selects m_current when its joint can move. A rigid primitive connection inside a
    // primitive rigid element that is only held by its suspension state 2 is released for the
    // manipulation (suspension 0, the elements split); when that gives no joint it is
    // restored. Mac 0xc5940
    bool TrySelect();
    // Ends the selection; a released connection is suspended again and the elements merged
    // back. Mac 0xc5810
    void Restore();
    // Merges the two primitive rigid elements of m_current and lets the Scene rebuild their
    // connections. Mac 0xc5aa0, Win 0x475a70
    void Merge();
    // Splits the primitive rigid elements of m_current. Mac 0xc5d10, Win 0x475920
    void Split();

    PrimitiveElement*    m_element;      // +0x3c
    PrimitiveConnection* m_current;      // +0x40 the element's selected connection
    UInt32               m_suspended;    // +0x44 m_current's suspension before the release
                                         //       (NOTE: not initialised by the constructor)
    bool                 m_released;     // +0x48 m_current is released (TrySelect)
    bool                 m_changed;      // +0x49 Scene::NotifyRigidElementChanges pending
};

class RigidConnectionManipulator : public ConnectionManipulator {
public:
    // Starts at the body's first connection. Mac inlined into 0xa2840, Win 0x475be0
    explicit RigidConnectionManipulator(RigidElement* element);
    virtual ~RigidConnectionManipulator();                      // Mac 0xc74a0 / 0xc7540, Win 0x475e20 / sdd 0x475eb0
    // Mac 0xc75f0 (-> 0xc77c0), Win 0x475bc0 (-> 0x475d20)
    virtual bool SelectNext(bool* wrapped);
    // Mac 0xc7610, Win 0x475e70
    virtual void Detach();
    // Mac 0xc7700
    virtual void Reset();

    // Cycles from m_current to the next selectable connection. Mac 0xc77c0, Win 0x475d20
    bool Next(bool* wrapped);
    // Selects m_current when it has a real field connection (field kind below 999000) and its
    // joint is not fixed. Win 0x475c70 (Mac inlined)
    bool TrySelect();

    RigidElement*    m_element;          // +0x3c
    RigidConnection* m_current;          // +0x40 the body's selected connection
};

} // namespace LEGO
