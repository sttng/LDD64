// LEGO Digital Designer — restored source
// LEGO::HingeAlignTool — aligns two parts of a hinged assembly ("hingeAlignTool"): the first
//   click takes the hovered field points (planar / axle fields; cursor 0x72), the second those
//   of the other part; for a compatible pair of two rigid bodies of the same HingedElement
//   Scene::MoveTogether runs the solver that turns the hinges until the points meet (5 ms
//   steps, at most 100 ms per update, 25 s in all). Success is one undo step; failure shows the
//   second points with the "failed" texture. Points are 4 x 4 quads textured with
//   Graphics/EditScreen/H-A_arrowCircle1.png (hover / first), 2.png (second), 2B.png (failed).
//
// Home TU 066 "HingeAlignTool.cpp" (Mac 0x158120-0x15b6ab; Win 0x665301-0x66753f).
// RTTI: HingeAlignTool : BrickConnectSceneControlTool +0, UpdateObserver +0x2c (subject +0x30);
//   virtual base Object +0xa4 (Mac sizeof 0xac, Win 0xc0).
// Mac vtable (primary): base slots 0-26 with D1 0x15a5c0, D0 0x15a620, OnFinalRelease 0x15a690,
//   OnAttached 0x158e40, OnDetached 0x158e80, OnEvent06 0x1596f0, OnEvent18 0x159230,
//   OnEvent19 0x159290; then 27 OnMouseMove 0x158f70 (thunk 0x159210), 28 Update 0x159310
//   (thunk 0x1596d0), 29 OnMousePressed 0x159fa0 (thunk 0x15a4d0). Win sdd 0x629a2a.
// The constructor is inline: emitted in EditMode.cpp (Mac 0x18c750, Win 0x628efc).
// Method and member names inferred.
#pragma once
#include "LEGO/Types.h"
#include "Object.h"
#include "BasicArray.h"
#include "Geometry.h"
#include "Updater.h"
#include "Mesh.h"
#include "Scene.h"
#include "BrickConnectSceneControlTool.h"

namespace LEGO {

class Render;
class BrickConnectSceneManager;
class PrimitiveElement;

class HingeAlignTool : public BrickConnectSceneControlTool, public UpdateObserver {
public:
    typedef Pair<Scene::Feature, Scene::Feature> FeaturePair;

    // Mac 0x18c750 (inline, emitted in EditMode.cpp), Win 0x628efc
    HingeAlignTool()
        : BrickConnectSceneControlTool(false), m_failed(false), m_deadline(0), m_lastTime(0),
          m_accumulated(0), m_stepInterval(5), m_maxTime(100) {}
    virtual ~HingeAlignTool() {}                                            // Mac 0x15a5c0 / 0x15a620, Win sdd 0x629a2a

    // The quad (4 x 4, texture coordinates) and the three textured meshes. False when the
    // vertex buffer or a texture is missing (`sceneManager` is not used). Mac 0x158120,
    // Win 0x665a32
    bool Create(Render* render, BrickConnectSceneManager* sceneManager);

    // ---- BrickConnectSceneControlObserver ----
    // The first / second / hovered points (opacity 0.5 for a hover that is no selection).
    // Mac 0x1596f0, Win 0x665f69
    virtual void OnEvent06(Render* render);
    // The hover is updated while the mouse is over the scene control. Mac 0x159230, Win 0x666e62
    virtual void OnEvent18();
    // Forgets the points, cursor 0x71, the hover is updated. Mac 0x159290, Win 0x666ea1
    virtual void OnEvent19();

    // ---- ControlObserver overriders (Mac primary 27 / 29) ----
    // UpdateHover(position) (`control` not checked). Mac 0x158f70
    virtual void OnMouseMove(Control* control, const Vector2& position);
    // Left button (any control), no solver running: takes the hover as first / second points
    // and starts the solver; without a hover the selection is forgotten. Mac 0x159fa0,
    // Win 0x66701d
    virtual void OnMousePressed(Control* control, const Vector2& position, UInt32 button);

    // ---- UpdateObserver (the scene control's manager while the solver runs; Mac primary 28) ----
    // Solver steps (Scene::PhysicsCall5 with the float bits of 1.0) for the elapsed time in
    // m_stepInterval steps (the rest is kept), at most until m_maxTime ms have passed.
    // Mac 0x159310, Win 0x666efb
    virtual void Update();

    // The hovered points: the field points along the ray through `position` (planar and axle
    // fields only); with a first selection they must make a pair with it, else nothing is
    // hovered. Invalidates on a change. NOTE: copies m_hover first and does not use the copy
    // (both builds). Mac 0x158f90, Win 0x666e53
    void UpdateHover(const Vector2& position);
    // A pair of `first` and `second` points of two rigid bodies of the same HingedElement that
    // are compatible (ConnectionField::IsCompatible(index, other, otherIndex)): a non-axle pair
    // of a field type below 5 is taken at once; axle pairs are ranked (0.25: same kind parity,
    // 0.75: different) and the highest rank is taken. `pair` (when not NULL) receives it.
    // Mac 0x159a50
    static bool FindFeaturePair(const BasicArray<Scene::Feature>& first,
                                const BasicArray<Scene::Feature>& second, FeaturePair* pair);
    // The transform of a feature quad in body space: rigid element local * element local *
    // field transform * point offset; planar FLAG_FIXED points are raised by 0.176 along the
    // field's y axis. Mac 0x15a4f0, Win 0x665400
    static void GetFeatureTransform(const Scene::Feature& feature, Matrix3D& transform);

    // ---- state (test access) ----
    const BasicArray<Scene::Feature>& GetFirst() const { return m_first; }
    const BasicArray<Scene::Feature>& GetSecond() const { return m_second; }
    const BasicArray<Scene::Feature>& GetHover() const { return m_hover; }
    bool HasFailed() const { return m_failed; }
    SingleMesh* GetMesh(UInt32 index) const { return m_meshes[index]; }

protected:
    // Mac 0x15a690
    virtual void OnFinalRelease();
    // Clears the selection, cursor 0x71. Mac 0x158e40, Win 0x665301
    virtual void OnAttached(BrickConnectSceneControl* control);
    // Stops a running solver (Scene::PhysicsCall6(false), no undo), forgets the points.
    // Mac 0x158e80, Win 0x6664f0
    virtual void OnDetached(BrickConnectSceneControl* control);
    using BrickConnectSceneControlTool::OnAttached;
    using BrickConnectSceneControlTool::OnDetached;
    using UpdateObserver::OnAttached;
    using UpdateObserver::OnDetached;

private:
    // The solver succeeded: the points are forgotten, the changed transforms are one undo step.
    // Mac 0x159560
    void FinishMove();
    // The solver failed. (inlined twice in Mac 0x159310)
    void FailMove();
    // Draws the quad of the first of `features` with `mesh` (blend on, lighting, depth write and
    // culling off). Mac 0x159920, Win 0x665745
    void DrawFeatures(const BasicArray<Scene::Feature>& features, SingleMesh* mesh, Render* render);

    BasicArray<Scene::Feature>                   m_first;        // +0x34
    BasicArray<Scene::Feature>                   m_second;       // +0x44
    BasicArray<Scene::Feature>                   m_hover;        // +0x54
    RefPointer<SingleMesh>                       m_meshes[3];    // +0x64 arrowCircle1, 2, 2B
    bool                                         m_failed;       // +0x70
    BasicArray<Pair<PrimitiveElement*, Matrix3D> > m_transforms; // +0x74 at the start of the solver
    UInt64                                       m_deadline;     // +0x84 start + 25000 ms
    UInt64                                       m_lastTime;     // +0x8c
    UInt64                                       m_accumulated;  // +0x94 ms not yet stepped
    UInt32                                       m_stepInterval; // +0x9c 5 ms
    UInt32                                       m_maxTime;      // +0xa0 100 ms per Update
                                                                 // Object +0xa4
};

} // namespace LEGO
