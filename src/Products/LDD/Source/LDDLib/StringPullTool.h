// LEGO Digital Designer — restored source
// LEGO::StringPullTool — pulls flexible parts (strings, hoses) by anchors ("stringPullTool"): a
//   click on a flexible part frees the clicked element (DisconnectElement) and adds an anchor of
//   the anchor solver at the clicked point (StringPullSolverCallback: the anchor pulls its point
//   to the target and aligns an axle near the target with an axle of another part). Each anchor
//   target shows three axis handles (x red, y green, z blue; 0.5 long, radius 0.1); dragging a
//   handle with the left button moves the target along that axis (with key 0x39 held: in the
//   plane normal to it). Delete (key 0xd3) removes the active anchor.
// LEGO::StringPullTool::Anchor — one anchor (Object; Mac sizeof 0x9c).
//
// Home TU 304 "StringPullTool.cpp" (Mac 0x2b14a0-0x2b3b1b; Win 0x66bc55-0x66c9ac).
// RTTI: StringPullTool : BrickConnectSceneControlTool +0, UpdateObserver +0x2c (subject +0x30);
//   virtual base Object +0x78 (Mac sizeof 0x80, Win 0x98).
// Mac vtable (primary): base slots 0-26 with D1 0x2b33a0, D0 0x2b3400, OnFinalRelease 0x2b3470,
//   OnAttached 0x2b1830, OnDetached 0x2b1870, OnEvent06 0x2b1f30, OnEvent19 0x2b1b50 (empty);
//   then 27 OnMouseMove 0x2b18f0 (thunk 0x2b1b30), 28 Update 0x2b1b60 (thunk 0x2b1bf0),
//   29 OnAttached(Updater*) 0x2b1c80 (thunk 0x2b1d00), 30 OnDetached(Updater*) 0x2b1d80 (thunk
//   0x2b1f10), 31 OnMousePressed 0x2b2160 (thunk 0x2b3140), 32 OnKeyPressed 0x2b3160 (thunk
//   0x2b3270). Win sdd 0x62c2b7.
// The constructor is inline: emitted in EditMode.cpp (Mac 0x18c8f0, Win 0x62b100).
// Method and member names inferred.
#pragma once
#include "LEGO/Types.h"
#include "Object.h"
#include "BasicArray.h"
#include "Geometry.h"
#include "Updater.h"
#include "Mesh.h"
#include "VertexBuffer.h"
#include "BrickConnectSceneControlTool.h"

namespace LEGO {

class Render;
class BrickConnectSceneManager;
class PrimitiveElement;
class SingleMesh;
class VertexBuffer;
class StringPullSolverCallback;
class AnchorPhysicsSolverAnchor;

class StringPullTool : public BrickConnectSceneControlTool, public UpdateObserver {
public:
    // One anchor. Mac vtable 0x744d28 {D1 0x2b32b0, D0 0x2b3320}, Win sdd 0x66bc96.
    class Anchor : public Object {
    public:
        Anchor() : m_element(0), m_handle(0) {}
        // The solver anchor goes (AnchorPhysicsSolverAnchor::Remove, Release).
        // Mac 0x2b32b0 / 0x2b3320, Win 0x66bc55 / 0x66bc96
        virtual ~Anchor();

        PrimitiveElement*          m_element;   // +0x08 the clicked element (not referenced)
        Matrix3D                   m_local;     // +0x0c the point in body space
        Matrix3D                   m_target;    // +0x4c the target in the world
        Vector3                    m_grab;      // +0x8c the grabbed handle point (target space)
        AnchorPhysicsSolverAnchor* m_handle;    // +0x98 one reference (CreateAnchor's; may be NULL)
    };

    // Mac 0x18c8f0 (inline, emitted in EditMode.cpp), Win 0x62b100
    StringPullTool()
        : BrickConnectSceneControlTool(false), m_flag34(false), m_lastTime(0), m_activeAnchor(-1),
          m_activeAxis(3), m_callback(0) {}
    virtual ~StringPullTool() {}                                            // Mac 0x2b33a0 / 0x2b3400, Win sdd 0x62c2b7

    // The line (2 vertices, white at half alpha) and axes meshes. Always true (NOTE: the vertex
    // buffer is used without a check). Mac 0x2b14a0, Win 0x66c1a8
    bool Create(Render* render, BrickConnectSceneManager* sceneManager);

    // ---- BrickConnectSceneControlObserver ----
    // Per anchor: a line from its point to its target and the target axes, first without and
    // then with the depth test. Mac 0x2b1f30, Win 0x66bec7
    virtual void OnEvent06(Render* render);
    // Empty. Mac 0x2b1b50
    virtual void OnEvent19();

    // ---- ControlObserver overriders (Mac primary 27 / 31 / 32) ----
    // Dragging a handle (left button held): the target moves along the handle's axis (key 0x39:
    // in the plane normal to it). Mac 0x2b18f0, Win 0x66bd0b
    virtual void OnMouseMove(Control* control, const Vector2& position);
    // Left click: the nearest axis handle hit becomes active; else a click on a flexible part
    // adds an anchor (nothing: no handle active). Mac 0x2b2160, Win 0x66c534
    virtual void OnMousePressed(Control* control, const Vector2& position, UInt32 button);
    // 0xd3 (Delete): the active anchor goes. Mac 0x2b3160, Win 0x66c41e
    virtual void OnKeyPressed(Control* control, UInt32 key, UInt32 modifiers, bool repeat);

    // ---- UpdateObserver (the scene control's manager while pulling; Mac primary 28-30) ----
    // One solver step of the elapsed time (NOTE: 1 / milliseconds, both builds).
    // Mac 0x2b1b60, Win 0x66c358
    virtual void Update();

    // ---- state (test access) ----
    const BasicArray<RefPointer<Anchor> >& GetAnchors() const { return m_anchors; }
    Int32  GetActiveAnchor() const { return m_activeAnchor; }
    UInt32 GetActiveAxis() const { return m_activeAxis; }
    StringPullSolverCallback* GetCallback() const { return m_callback; }
    SingleMesh* GetAxesMesh() const { return m_axesMesh; }
    SingleMesh* GetLineMesh() const { return m_lineMesh; }

protected:
    // Mac 0x2b3470
    virtual void OnFinalRelease();
    // Clears the selection, cursor 4. Mac 0x2b1830
    virtual void OnAttached(BrickConnectSceneControl* control);
    // The anchors go, the updater is left. Mac 0x2b1870, Win 0x66c50e
    virtual void OnDetached(BrickConnectSceneControl* control);
    // The pull starts: the control is modal, the transforms are kept, the scene's
    // StringPullSolverCallback is created. Mac 0x2b1c80
    virtual void OnAttached(Updater* updater);
    // The pull ends: the callback goes, the solver stops, the changed transforms are one undo
    // step, the control is no longer modal. Mac 0x2b1d80
    virtual void OnDetached(Updater* updater);
    using BrickConnectSceneControlTool::OnAttached;
    using BrickConnectSceneControlTool::OnDetached;

private:
    // The undo step (UndoMoveSnapshot) of the transforms changed since the pull started.
    // Mac 0x2b1e10
    void FinishMove();

protected:
    bool                                         m_flag34;       // +0x34 false (not used)
    UInt64                                       m_lastTime;     // +0x38 GetMilliseconds of the last step
    BasicArray<Pair<PrimitiveElement*, Matrix3D> > m_transforms; // +0x40 at the start of the pull
    BasicArray<RefPointer<Anchor> >              m_anchors;      // +0x50
    Int32                                        m_activeAnchor; // +0x60 -1: none
    UInt32                                       m_activeAxis;   // +0x64 0-2, 3: none
    RefPointer<SingleMesh>                       m_axesMesh;     // +0x68
    RefPointer<VertexBuffer>                     m_lineBuffer;   // +0x6c
    RefPointer<SingleMesh>                       m_lineMesh;     // +0x70
    StringPullSolverCallback*                    m_callback;     // +0x74 one reference while pulling
                                                                 // Object +0x78
};

} // namespace LEGO
