// LEGO Digital Designer — restored source
// LEGO::ClickPullTool — pulls a flexible part by a clicked point ("clickPullTool"): the click
//   frees the clicked element (DisconnectElement) and starts the anchor solver through the
//   scene's ClickPullSolverCallback (one anchor dragged by the mouse ray; the callback aligns a
//   field of the pulled body with a compatible field of another part). Parts it could connect
//   (GetState 2) are framed in green; the second click connects (Scene::Reconnect, up to 20
//   solver steps of 1/60 s first) or restores the transforms.
// LEGO::ClickPullTool::Anchor — the anchor of the click (Object; Mac sizeof 0x9c).
//
// Home TU 350 "ClickPullTool.cpp" (Mac 0x3305f0-0x33260b; Win 0x66c9ad-0x66d1xx).
// RTTI: ClickPullTool : BrickConnectSceneControlTool +0, UpdateObserver +0x2c (subject +0x30);
//   virtual base Object +0x78 (Mac sizeof 0x80, Win 0x98).
// Mac vtable (primary): base slots 0-26 with D1 0x331eb0, D0 0x331f10, OnFinalRelease 0x331f80,
//   OnAttached 0x330670, OnDetached 0x3306b0, OnEvent05 0x330ec0; then 27 OnMouseMove 0x330780
//   (thunk 0x3307e0), 28 Update 0x330850 (thunk 0x330bf0), 29 OnAttached(Updater*) 0x330c10
//   (thunk 0x330c90), 30 OnDetached(Updater*) 0x330d10 (thunk 0x330ea0), 31 OnMousePressed
//   0x330f30 (thunk 0x331da0). Win sdd 0x62c2d9.
// The constructor is inline: emitted in EditMode.cpp (Mac 0x18ca20, Win 0x62b2e1).
// Method and member names inferred.
#pragma once
#include "LEGO/Types.h"
#include "Object.h"
#include "BasicArray.h"
#include "Geometry.h"
#include "Updater.h"
#include "Mesh.h"
#include "BrickConnectSceneControlTool.h"

namespace LEGO {

class Render;
class RenderQueue;
class BrickConnectSceneManager;
class PrimitiveElement;
class SingleMesh;
class ClickPullSolverCallback;
class AnchorPhysicsSolverAnchor;

class ClickPullTool : public BrickConnectSceneControlTool, public UpdateObserver {
public:
    // The anchor of a click. Mac vtable 0x74c548 {D1 0x331dc0, D0 0x331e30}, Win sdd 0x66ca05.
    class Anchor : public Object {
    public:
        Anchor() : m_element(0), m_handle(0) {}
        // The solver anchor goes (AnchorPhysicsSolverAnchor::Remove, Release).
        // Mac 0x331dc0 / 0x331e30, Win 0x66c9ad / 0x66ca05
        virtual ~Anchor();

        PrimitiveElement*          m_element;   // +0x08 the clicked element (not referenced)
        Matrix3D                   m_local;     // +0x0c the point in body space
        Matrix3D                   m_target;    // +0x4c the point in the world
        Vector3                    m_grab;      // +0x8c (not used by this tool)
        AnchorPhysicsSolverAnchor* m_handle;    // +0x98 one reference (AddAnchor's)
    };

    // Mac 0x18ca20 (inline, emitted in EditMode.cpp), Win 0x62b2e1
    ClickPullTool()
        : BrickConnectSceneControlTool(false), m_flag34(false), m_lastTime(0), m_callback(0) {}
    virtual ~ClickPullTool() {}                                             // Mac 0x331eb0 / 0x331f10, Win sdd 0x62c2d9

    // The green box mesh of the connect candidates. Always true. Mac 0x3305f0, Win 0x66cace
    bool Create(Render* render, BrickConnectSceneManager* sceneManager);

    // ---- BrickConnectSceneControlObserver ----
    // The green boxes of the parts the pulled body would connect to. Mac 0x330ec0, Win 0x66cb06
    virtual void OnEvent05(const Matrix3D& transform, Render* render, RenderQueue* overlayQueue,
                           RenderQueue* sceneQueue);

    // ---- ControlObserver overriders (Mac primary 27 / 31) ----
    // While pulling: the anchors follow the mouse ray. Mac 0x330780, Win 0x66ca21
    virtual void OnMouseMove(Control* control, const Vector2& position);
    // Left click in the scene: connect / restore while pulling, else start pulling the clicked
    // flexible part. Mac 0x330f30, Win 0x66cbe4
    virtual void OnMousePressed(Control* control, const Vector2& position, UInt32 button);

    // ---- UpdateObserver (the scene control's manager while pulling; Mac primary 28-30) ----
    // One solver step of the elapsed time (NOTE: 1 / milliseconds, both builds); the connect
    // candidates. Mac 0x330850, Win 0x66cf51
    virtual void Update();

    // ---- state (test access) ----
    const BasicArray<RefPointer<Anchor> >& GetAnchors() const { return m_anchors; }
    const BasicArray<RefPointer<PrimitiveElement> >& GetHighlighted() const { return m_highlighted; }
    ClickPullSolverCallback* GetCallback() const { return m_callback; }
    SingleMesh* GetBoxMesh() const { return m_boxMesh; }

protected:
    // Mac 0x331f80, Win (Object-in entry)
    virtual void OnFinalRelease();
    // Clears the selection, cursor 4. Mac 0x330670
    virtual void OnAttached(BrickConnectSceneControl* control);
    // Restores the transforms of a running pull, forgets anchors and candidates, leaves the
    // updater. Mac 0x3306b0, Win 0x66cb9f
    virtual void OnDetached(BrickConnectSceneControl* control);
    // The pull starts: the control is modal, the transforms are kept, the scene's
    // ClickPullSolverCallback is created. Mac 0x330c10, Win 0x66ca75
    virtual void OnAttached(Updater* updater);
    // The pull ends: the callback goes, the solver stops, the changed transforms are one undo
    // step, the control is no longer modal. Mac 0x330d10
    virtual void OnDetached(Updater* updater);
    using BrickConnectSceneControlTool::OnAttached;
    using BrickConnectSceneControlTool::OnDetached;

private:
    // The undo step (UndoMoveSnapshot) of the transforms changed since the pull started.
    // Mac 0x330da0
    void FinishMove();

protected:
    bool                                         m_flag34;       // +0x34 false (not used)
    UInt64                                       m_lastTime;     // +0x38 GetMilliseconds of the last step
    BasicArray<Pair<PrimitiveElement*, Matrix3D> > m_transforms; // +0x40 at the start of the pull
    BasicArray<RefPointer<PrimitiveElement> >    m_highlighted;  // +0x50 connect candidates
    RefPointer<SingleMesh>                       m_boxMesh;      // +0x60
    BasicArray<RefPointer<Anchor> >              m_anchors;      // +0x64
    ClickPullSolverCallback*                     m_callback;     // +0x74 one reference while pulling
                                                                 // Object +0x78
};

} // namespace LEGO
