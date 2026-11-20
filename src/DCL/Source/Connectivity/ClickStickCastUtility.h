// LEGO Digital Designer — restored source
// LEGO::ClickStickCastUtility — places the dragged scene of the click-stick / move tool: at a
//   transform ("2D": Scene::SnapDrag within 2.0) or along the mouse ray ("3D": Scene::SnapDragRay,
//   else a physics sweep of the drag scene, else where the ray meets the grid plane or leaves the
//   box around the scene; those last two then Scene::SnapDrag within 0.25). Keeps the result, the
//   placement and the grip offset; casts can be recorded / replayed ("cstRecording.dat",
//   developer mode, ClickStickTool::OnAttached).
// LEGO::ClickStickCastSimpleUtility — the implementation the tools use: grid height, the box
//   around the scene (±1000), the pivot rotation (arrow keys, Rotate) and the grip.
// TU 376 "ClickStickCastUtility.cpp" (Mac 0x363640-0x366b3a, Win 0x5aa470-0x5ac22f; Win code in
// the DCL range). Inline here, emitted elsewhere: slots 2 / 3 (Mac 0x15dfd0 / 0x15dfe0, MoveTool
// TU 067), slots 4 / 5 and the SimpleUtility accessors (Mac 0x146e90-0x146f20, ClickStickTool TU
// 062). Mac 0x366920 = <iostream> + five ProfileTimers "ClickStickCastUtility::CanConnect(2D|3D|
// 3D env|3D phy|3D phy down)".
// RTTI: class LEGO::ClickStickCastUtility (no base, abstract); class
//   LEGO::ClickStickCastSimpleUtility : ClickStickCastUtility. Not reference counted: a base of
//   ClickStickTool (Mac +0x44) and MoveTool (Mac +0x34).
//   Mac vtables 0x7504f8 / 0x778348 (16 entries: D1, D0, slots 2-15); Win 0x9d1fb4 / 0x9d1ff4
//   (one scalar deleting destructor, slots = Mac - 1).
// Layout (Mac = Win, sizeof 0xf0 / 0x198): see the data members. Names inferred, except the
//   class names (RTTI) and the timer strings.
#pragma once
#include "LEGO/Types.h"
#include "Object.h"
#include "BasicArray.h"
#include "Geometry.h"
#include "Scene.h"
#include "File.h"

namespace LEGO {

class Part;

class ClickStickCastUtility {
public:
    // Flags of a cast (the recording / playback compares them; the bits say which way the
    // placement was found). (names inferred)
    enum CastFlags {
        CastConnected      = 0x001,   // the ray / transform snap connected
        CastSnapped        = 0x002,   // ... found a placement without connections
        CastFree           = 0x004,   // nothing found (2D) / the physics sweep missed (3D)
        CastPhysics        = 0x008,   // 3D: the physics sweep found the placement
        CastSnapDrag       = 0x010,   // 3D: Scene::SnapDrag at the plane / physics hit was tried
        CastSnapDragHit    = 0x020,   // ... and connected
        CastOffset         = 0x040,   // the grip offset was measured on a connecting placement
        CastFirstOffset    = 0x080,   // the offset was the identity: taken over
        CastOffsetReset    = 0x100    // the offset changed too much: reset, cast again
    };

    ClickStickCastUtility();                                                // Mac 0x363640, Win 0x5aa630
    virtual ~ClickStickCastUtility();                                       // Mac 0x366880/0x366890, Win sdd 0x5aa530 (body 0x5aa470)

    // ---- virtual slots (Mac slot / Win slot) ----
    // The playback ended (true: the end of the file; false: a read failed). Empty.
    virtual void OnPlaybackFinished(bool /*success*/) {}                    // 2/1  Mac 0x15dfd0, Win 0x40be80
    // A replayed cast came out differently (flags or transform). Empty.
    virtual void OnPlaybackMismatch(UInt32 /*frame*/, UInt32 /*recordedFlags*/, UInt32 /*flags*/,
                                    const Matrix3D& /*recordedTransform*/,
                                    const Matrix3D& /*transform*/) {}       // 3/2  Mac 0x15dfe0, Win 0x5aa520
    // Extra per-frame data of the recording (none). Mac 0x146e90 / 0x146ea0, Win 0x52d7b0 (ICF)
    virtual bool ReadFrameData(UInt32 /*frame*/, File* /*file*/) { return true; }   // 4/3
    virtual bool WriteFrameData(UInt32 /*frame*/, File* /*file*/) { return true; }  // 5/4
    // Place without casting (CanConnectDrag at the current transform).
    virtual bool IsSimple() const = 0;                                      // 6/5
    // The y of the plane the 3D cast falls back to (the grid minus the drag scene's bottom).
    virtual Real32 GetPlaneHeight() const = 0;                              // 7/6
    // The point of the ray to place the grip at: where the ray leaves `box` (at most
    // `maxDistance` along it), or the hit of the plane y = `planeHeight` when that is nearer
    // (`onPlane` set then). Always true. Mac 0x365910, Win 0x5abd20
    virtual bool GetCastPoint(const Vector3& origin, const Vector3& direction, Real32 maxDistance,
                              const BoundingBox& box, Real32 planeHeight, Vector3& point,
                              bool& onPlane);                               // 8/7
    // The box the 3D cast clips the ray with.
    virtual const BoundingBox& GetBoundingBox() const = 0;                  // 9/8
    // The rotation of the dragged scene about its grip.
    virtual const Matrix3D& GetPivot() const = 0;                           // 10/9
    // The grip: the dragged scene relative to the cast point.
    virtual const Matrix3D& GetGrip() const = 0;                            // 11/10
    // How the physics fallback of the 3D cast places the scene (0 at the sweep hit, 1 on the
    // grid levels above it, 2 dropped from above). (name inferred; 1 in every tool)
    virtual UInt32 GetPhysicsMode() const = 0;                              // 12/11
    // SnapToGrid of the transform's translation. (not an overload of SnapToGrid: MSVC would
    // have swapped the two slots) Mac 0x365d10, Win 0x5abdf0
    virtual void SnapTransformToGrid(Matrix3D& transform);                  // 13/12
    // x / z to the 0.8 grid (+0.4 unless m_aligned), or to the 0.4 grid with m_fineGrid.
    // Mac 0x365d30, Win 0x5abe10
    virtual void SnapToGrid(Vector3& point);                                // 14/13
    // The drag scene's collision volumes (Scene::GetDebugInfo 0x804 at `transform`) into
    // `debugger` (category 'csu '). Mac 0x365ee0, Win 0x5abed0
    virtual void DrawDebugInfo(const Matrix3D& transform, VisualDebugger* debugger); // 15/14

    // ---- non-virtual ----
    // Starts dragging `dragScene` over `scene` (Scene::BeginDrag from `first`; `single`: only
    // that element snaps); false (nothing kept) for an empty drag scene. Resets the result, the
    // distance and the offset. Mac 0x363790, Win 0x5aa700
    bool Begin(Scene* scene, Scene* dragScene, PrimitiveElement* first, bool single);
    // Scene::EndDrag and both scenes released; `clear` also clears the result. Mac 0x363990,
    // Win 0x5aa7b0
    void End(bool clear);
    // The result, the transform and the connected flag. Mac 0x363a60, Win 0x5abb10
    void ClearResult();
    // ClearResult, the distance and the offset. Mac 0x363900, Win 0x5abb60
    void Reset();
    // Places the drag scene at (near) `transform` (Scene::SnapDrag within 2.0). The scene
    // listener gets 'csu ' first. Returns false when a playback failed. Mac 0x3651f0 (body
    // 0x365230), Win 0x5ab7e0 (body 0x5ab810)
    bool Cast(const Matrix3D& transform);
    // Places the drag scene along the ray `origin` + t `direction` (t <= `maxDistance`).
    // Mac 0x363ae0 (body 0x363b40), Win 0x5aa820 (body 0x5aa860)
    bool Cast(const Vector3& origin, const Vector3& direction, Real32 maxDistance);
    // The scene parts the last connecting result connects to (once each). Mac 0x3655d0
    void GetConnectedParts(BasicArray<RefPointer<Part> >& parts) const;
    // Recording / playback to / from `file` (header "CSCU002\0"). Mac 0x3657e0 / 0x365860,
    // Win 0x5abb90 / 0x5abc00
    bool StartRecording(File* file);
    bool StartPlayback(File* file);
    // Closes the file (a recording is truncated at the current position when `truncate`).
    // Mac 0x364eb0, Win 0x5abcb0
    void StopRecording(bool truncate);

    bool IsConnected() const                          { return m_connected; }
    const Scene::ConnectResult& GetResult() const     { return m_result; }
    const Matrix3D& GetTransform() const              { return m_transform; }
    bool IsRecording() const                          { return m_recording; }
    bool IsPlaying() const                            { return m_playing; }
    bool IsRecordingOrPlaying() const                 { return m_file != 0; }

    // ---- data (Mac offsets = Win offsets) ----
    RefPointer<Scene>     m_scene;        // +0x04 the scene placed into
    RefPointer<Scene>     m_dragScene;    // +0x08 the dragged scene
    Real32                m_distance;     // +0x0c ray distance of the last 3D placement (0: none)
    Scene::ConnectResult  m_result;       // +0x10 transform +0x10, connections +0x50
    Matrix3D              m_transform;    // +0x60 the placement
    Matrix3D              m_offset;       // +0xa0 grip offset of the connected placement
    bool                  m_connected;    // +0xe0
    bool                  m_hasPhysics;   // +0xe1 the physics scene can sweep (Begin)
    bool                  m_aligned;      // +0xe2 the bricks keep their grid position (no +0.4)
    bool                  m_fineGrid;     // +0xe3 0.4 instead of 0.8 grid
    bool                  m_recording;    // +0xe4
    bool                  m_playing;      // +0xe5
    UInt32                m_frame;        // +0xe8
    RefPointer<File>      m_file;         // +0xec

protected:
    // The 2D / 3D casts. Mac 0x365230 / 0x363b40, Win 0x5ab810 / 0x5aa860
    bool DoCast(const Matrix3D& transform);
    bool DoCast(const Vector3& origin, const Vector3& direction, Real32 maxDistance,
                VisualDebugger* debugger);
    // Records / checks the outcome of a cast, counts the frame; false when a playback failed.
    // Mac 0x364f30, Win 0x5ac230
    bool RecordFrame(UInt32 flags);
    // A failed playback read: everything reset, the file closed, OnPlaybackFinished(false).
    // (inlined three times, Mac 0x36475d / 0x364372 / 0x364f30)
    void AbortPlayback();
    // Where the ray leaves `box` (at most `maxDistance`). Mac 0x365a00, Win 0x5ac050
    static Vector3 ClipRayToBox(const Vector3& origin, const Vector3& direction, Real32 maxDistance,
                                const BoundingBox& box);

private:
    ClickStickCastUtility(const ClickStickCastUtility&);
    ClickStickCastUtility& operator=(const ClickStickCastUtility&);
};

class ClickStickCastSimpleUtility : public ClickStickCastUtility {
public:
    // The box ±1000, no pivot / grip, mode 1. Mac 0x366030
    ClickStickCastSimpleUtility();
    virtual ~ClickStickCastSimpleUtility();                                 // Mac 0x3668d0/0x3668e0, Win sdd 0x5aa610 (body 0x5aa550)

    virtual bool IsSimple() const { return m_simple; }                      // 6/5   Mac 0x146eb0, Win 0x5aa5a0
    virtual Real32 GetPlaneHeight() const { return m_planeHeight - m_heightOffset; } // 7/6 Mac 0x146ec0, Win 0x5aa5b0
    virtual const BoundingBox& GetBoundingBox() const { return m_box; }    // 9/8   Mac 0x146ef0, Win 0x5aa5d0
    virtual const Matrix3D& GetPivot() const { return m_pivot; }           // 10/9  Mac 0x146f00, Win 0x5aa5e0
    virtual const Matrix3D& GetGrip() const { return m_grip; }             // 11/10 Mac 0x146f10, Win 0x5aa5f0
    virtual UInt32 GetPhysicsMode() const { return m_physicsMode; }        // 12/11 Mac 0x146f20, Win 0x5aa600

    // Turns the pivot a quarter turn: `direction` 1 / 2 about the horizontal axis across the
    // view, 3 / 4 about the vertical axis, 5 / 6 about the view axis (signs and axes snapped
    // to the world axes from `up` and `view`); 0 clears the pivot. Resets the cast and updates
    // the height offset. Mac 0x366190
    void Rotate(UInt32 direction, const Vector3& up, const Vector3& view);
    // m_heightOffset = -(bottom of the drag scene's box, moved by the grip, rotated by the
    // pivot). Mac 0x366100
    void UpdateHeightOffset();

    // ---- data (Mac offsets = Win offsets) ----
    Real32       m_planeHeight;   // +0xf0 the grid plane (minus the grid height)
    Real32       m_heightOffset;  // +0xf4
    BoundingBox  m_box;           // +0xf8
    Matrix3D     m_pivot;         // +0x110
    Matrix3D     m_grip;          // +0x150
    UInt32       m_physicsMode;   // +0x190 1
    bool         m_simple;        // +0x194 developer key 'I' held
};

} // namespace LEGO
