// LEGO Digital Designer — restored source
// LEGO::BrickConnectSceneControl — the main 3D view that shows and edits the model: Scene,
//   camera, lights, grid, skybox, selection, undo, wires, building instructions and the render
//   data (per rigid body a BIBrick of PlacedMeshes merged with GeometryAccum; LDDParts with
//   skinned surfaces for flexible parts). Notifies BrickConnectSceneControlObservers, follows
//   scene / undo manager / preferences / palette, receives loaded models, is the VisualDebugger.
//
// Mac TU 021 (one object file of 186 KB, Mac 0x2d5c0-0x5bccb; sub-TUs 021a-021h; Win block
// 0x6125c8-0x622b61 + scattered helpers). Sub-TUs:
//   021a/021d  LoadTerrain + AddSceneryElement (021a), the physics snapshot (Create /
//              RestorePhysicsSnapshot), clipboard / click-stick helpers, the Execute bodies of
//              the TU's undo actions (021d); the rest is header-inline / template code (Mesh.h,
//              DataStream.h, Texture.h, VertexBuffer.h, Notifier.h, the observers' headers,
//              BasicArray/BasicMap)
//   021b       constructor, Initialize, scene / selection / undo operations, camera, wires,
//              storage, statistics, physics helpers
//   021c       destructor, OnFinalRelease, IsInside, OnKeyboardFocusGained, UndoInsertSelection
//   021e/f/g   render path (RenderToBitmap, RenderScene, mesh building), frame Draw (021e 0x49440)
//   021f       Control overrides (rectangle, mouse, keys, buffering), observer overrides (scene,
//              undo, preferences, palette), incremental render data, debug keys, post-processor
//              switch, UndoRemoveSelection
//   021h       camera / state helpers 0x42650-0x43260 (RotateCamera, RotateCameraVertical,
//              ZoomCamera, FocusOnPart, NotifyUndoState, IsModified, SetGridVisible, SetSkybox),
//              picking (GetPickRay / PickElement / PickFeatures / PickFrustum), selection, hiding,
//              groups and part descriptions with undo, UpdateMissingBricks, debug text controls
// NOT restored: the debug render modes 1-6 and their helpers
//   (not restored: BuildDebugInfoMeshes 0x467d0, UpdateDebugInfoMeshes 0x47980, BuildOverlayMeshes
//   0x47a50, the random-colour material 0x46370 - named only). BuildRenderDataForParts 0x48940 /
//   RebuildPartRenderData 0x488e0 are restored.
//
// Bases (RTTI, Mac offsets / Win offsets): BufferedAbleRenderControl +0x000,
//   Notifier<BrickConnectSceneControlObserver> +0x194 / +0x21c, SceneObserver (private)
//   +0x1b8 / +0x1a8, UndoManagerObserver +0x1c0 / +0x1b4, PreferencesObserver +0x1c8 / +0x1c0,
//   ModelLoaderHandler +0x1d0 / +0x1cc, BrickPaletteObserver +0x1d8 / +0x1d8,
//   RenderVisualDebugger +0x1e0 / +0x1e4, virtual base Object +0x510 / +0x53c.
// Virtual slots: Mac primary vtable 0x7045bc (52 entries): BufferedAbleRenderControl's 38
//   (overridden: D1 0x44560, D0 0x44940, OnFinalRelease 0x44db0, IsInside 0x454a0,
//   OnRectangleChanged 0x4aed0, OnMouseMove 0x4d9c0, OnMousePressed 0x4db30, OnMouseReleased
//   0x4e7c0, OnMouseWheel 0x4eaa0, OnKeyboardFocusGained 0x454b0, OnKeyPressed 0x4eb00, OnChar
//   0x4eb30, OnBufferingEnabled 0x4c7f0, Draw 0x49440) and the Mac-only primary entries of the
//   secondary-base overriders 38-51 in declaration order (see the declarations). No new
//   virtual function (Win primary 0x9cf964 has BufferedAbleRenderControl's 35 entries).
// Member and method names are inferred unless noted; Mac offsets are given for every member
// (the Win object differs: Control +4, the observers interleaved differently).
#pragma once
#include "LEGO/Types.h"
#include "Object.h"
#include "BasicArray.h"
#include "BasicMap.h"
#include "ObjectRefPointer.h"
#include "LEGO/String.h"
#include "Geometry.h"
#include "Color.h"
#include "Render.h"
#include "Mesh.h"
#include "GeometryAccum.h"
#include "Notifier.h"
#include "BufferedAbleRenderControl.h"
#include "BrickConnectSceneControlObserver.h"
#include "Scene.h"
#include "UndoManager.h"
#include "Preferences.h"
#include "BrickPalette.h"
#include "ModelLoaderHandler.h"
#include "RenderVisualDebugger.h"
#include "Brand.h"
#include "RenderControl.h"
#include "BrickPhysics.h"
#include "PrimitiveElement.h"
#include "BrickGroup.h"
#include "Brick.h"
#include "Part.h"

namespace LEGO {

class RenderControlManager;
class BrickConnectSceneManager;
class Brick;
class Part;
class LDDPart;
class RigidElement;
class PrimitiveElement;
class PrimitiveElementBrick;
class Camera;
class Material;
class Texture;
class VertexBuffer;
class RenderQueue;
class Bitmap;
class Skybox;
class Grid;
class BI;
class BrickConnect;
class Path;
class PostProcessingEffect;
class LDDPostProcessor;
class RenderControl;
struct StorageScene2;

// One wire of the scene (Brick of a wire design, Scene::IsWire): its colour, whether its
// endpoints are shown, and the mesh of the endpoint triangles. Mac sizeof 0x20 {vptr,
// refcount, m_color +0x08, m_selected +0x18, m_mesh +0x1c}; vtable 0x760b50 (D1 0x56770,
// D0 0x567d0, Object slots). RTTI name WireData; member names inferred.
class WireData : public Object {
public:
    WireData(const Color& color, bool selected, SingleMesh* mesh)
        : m_color(color), m_selected(selected), m_mesh(mesh) {}
    Color                  m_color;      // +0x08
    bool                   m_selected;   // +0x18 the wire's endpoint meshes are drawn
    RefPointer<SingleMesh> m_mesh;       // +0x1c (CreateTriangleMesh)
};

class BrickConnectSceneControl : public BufferedAbleRenderControl,
                                 public Notifier<BrickConnectSceneControlObserver>,
                                 private SceneObserver,
                                 public UndoManagerObserver,
                                 public PreferencesObserver,
                                 public ModelLoaderHandler,
                                 public BrickPaletteObserver,
                                 public RenderVisualDebugger {
public:
    // One mesh of the scene render data at a transform (body space). sizeof 0x58 (both builds).
    struct PlacedMesh {
        PlacedMesh() : m_layer(0), m_opacity(1.0f), m_cullRadius(-2.0f) {}
        // Mac 0x2f4f0 (Win inlined)
        PlacedMesh(Mesh* mesh, const Vector3& center, const Matrix3D& transform, UInt32 layer,
                   Real32 opacity)
            : m_transform(transform), m_center(center), m_mesh(mesh), m_layer(layer),
              m_opacity(opacity), m_cullRadius(-2.0f) {}
        Matrix3D         m_transform;   // +0x00
        Vector3          m_center;      // +0x40 depth reference (the body's box centre)
        RefPointer<Mesh> m_mesh;        // +0x4c
        UInt32           m_layer;       // +0x50 RenderQueue layer
        Real32           m_opacity;     // +0x54
        // The view culling's sphere around the mesh's vertices, made at the first draw (radius -2:
        // not yet; < 0: none, never culled).
        Vector3          m_cullCenter;   // the centre of the vertices' box
        Real32           m_cullRadius;
        Vector3          m_cullHalf;     // half the box's size (fast navigation's boxes)
    };

    // The render data of one rigid body. sizeof 0x58 (both builds); copy Mac 0x59730,
    // destructor Mac 0x55b30.
    struct BIBrick {
        BIBrick() : m_body(0), m_field54(0) {}
        // Whether any of the five lists has a mesh (Mac 0x39df0 inline, Win 0x612d18).
        bool HasMeshes() const
        {
            return !m_meshes.IsEmpty() || !m_secondaryMeshes.IsEmpty() || !m_overlay20.IsEmpty() ||
                   !m_overlay30.IsEmpty() || !m_overlay40.IsEmpty();
        }
        // Drops every mesh (the five lists are emptied). Mac 0x4f590
        void ClearMeshes();
        BasicArray<PlacedMesh> m_meshes;           // +0x00 the body's meshes (drawn)
        BasicArray<PlacedMesh> m_secondaryMeshes;  // +0x10 CreateAccumulatedMeshes(.., true, ..)
        BasicArray<PlacedMesh> m_overlay20;        // +0x20 debug / overlay meshes (0x467d0, 0x47a50)
        BasicArray<PlacedMesh> m_overlay30;        // +0x30
        BasicArray<PlacedMesh> m_overlay40;        // +0x40
        RigidElement*          m_body;             // +0x50 (its transform places the meshes)
        UInt32                 m_field54;          // +0x54 (0)
        // Level of detail (LDD64, while LevelOfDetail is on): the body's elements by region, each
        // with its meshes at full detail (also in m_meshes), without studs and tubes, and as boxes.
        // Draw picks one set per region by distance.
        struct LodRegion {
            Vector3                center;         // body space: the centre of the region's elements
            BasicArray<PlacedMesh> full;
            BasicArray<PlacedMesh> reduced;
            BasicArray<PlacedMesh> boxes;
        };
        BasicArray<LodRegion>  m_lodRegions;
    };

    // A named element set (021h; RTTI BasicArray<BrickConnectSceneControl::Selection>,
    // element 0x20: destructor Mac 0x57810).
    struct Selection {
        String                       m_name;       // +0x00
        BasicArray<PrimitiveElement*> m_elements;  // +0x10
    };

    // Meshes added to the scene from outside with a physics object (RTTI
    // BasicArray<BrickConnectSceneControl::SceneryElement>, element 0x14).
    struct SceneryElement {
        SceneryElement() : m_object(0) {}
        BasicArray<PlacedMesh> m_meshes;   // +0x00
        PhysicsSceneryElement* m_object;   // +0x10 one reference; removed from the physics scene
                                           //       (slot 26) and released
    };

    // A light of the scene with its shadow / light camera (Light +0x60 holds the camera in the
    // original; kept as its own member here). Mac stride 0x64.
    struct SceneLight {
        Light              m_light;     // +0x00
        RefPointer<Camera> m_camera;    // (Mac +0x60 inside the light)
    };

    // Mac 0x30620 (C1 with VTT), Win 0x61e8f7: `undoManager` is referenced and observed (must
    // not be null), `palette` observed (may be null). Reads the preferences DEVELOPERMODE,
    // ZOOMTOSELECTION, INVERTEDCAMERAX / Y, creates the two BIs and the wire colours.
    BrickConnectSceneControl(RenderControlManager* manager, BrickConnectSceneManager* sceneManager,
                             UndoManager* undoManager, BrickPalette* palette);
    virtual ~BrickConnectSceneControl();                                    // Mac 0x44560/0x44940 (body 0x43260), Win sdd 0x621f82

    // The terrain texture (assets "Textures/…" when the preferences name one), the lights and
    // their cameras, the grids (false when the dynamic or the none grid fails to initialise),
    // the background colour, Control flags, the view camera (fitted, viewport), the selected
    // post-processing effect, the LDDPostProcessor. `scene` null: a new scene of the scene
    // manager's BrickConnect. Mac 0x32b90, Win 0x620991
    bool Initialize(Scene* scene);
    // The light / camera part of Initialize (lights, light cameras, view camera fitted to the
    // scene). Kept callable on its own for the picture tools (renderx, diff_render_model,
    // the smoke), which have no grid / post-processor. (HOST-ONLY helper split out of
    // Initialize; Mac 0x32ed2-0x333c1 + 0x335db-0x3365d)
    void InitializeLightsAndCamera();

    // ---- Control overrides ------------------------------------------------------------------
    // Always hit (the whole rectangle). Mac 0x454a0
    virtual bool IsInside(const Vector2& point) const;                      // 3/0
    // Viewport of the view and the first light camera, the buffer, the post-processor size.
    // Mac 0x4aed0, Win 0x6138b6
    virtual void OnRectangleChanged(const Vector2& oldPosition, const Vector2& oldSize); // 8/5
    // While a button is held (+0x124 bit 1): without modifier 2 the camera orbits
    // (RotateCamera / RotateCameraVertical with 0.005 per pixel), with it the camera pans;
    // a press farther than 3 px forgets the pressed element. Mac 0x4d9c0, Win 0x6148d1
    virtual void OnMouseMove(const Vector2& position);                      // 18/15
    // Button 1: counts the press, remembers the position and the element under it; button 2
    // in developer mode with the 'M' key: the measure features. Mac 0x4db30, Win 0x6219b2
    virtual void OnMousePressed(const Vector2& position, UInt32 button);    // 19/16
    // Button 1 released on the pressed element: focus the camera on its part (zoom-to-selection
    // preference). Mac 0x4e7c0, Win 0x621e46
    virtual void OnMouseReleased(const Vector2& position, UInt32 button);   // 20/17
    // Vertical wheel: ZoomCamera(5 * delta). Mac 0x4eaa0, Win 0x614a00
    virtual void OnMouseWheel(UInt32 axis, Real32 delta);                   // 23/20
    // Becomes the scene manager's scene control. Mac 0x454b0, Win 0x6124af
    virtual void OnKeyboardFocusGained(bool flag, Control* previous, UInt32 modifiers); // 26/23
    // Key 1 (Escape): back to the default tool. Mac 0x4eb00, Win 0x61269d
    virtual void OnKeyPressed(UInt32 key, UInt32 modifiers, bool repeat);   // 28/25
    // Developer-mode debug keys (render modes, culling flags, overlays, camera …).
    // Mac 0x4eb30, Win 0x622afc
    virtual void OnChar(UniChar16 c, UInt32 modifiers, bool repeated);      // 30/27
    // The buffer exists: tell the post-processor its size. Mac 0x4c7f0, Win 0x613937
    virtual bool OnBufferingEnabled();                                      // 31/28
    // The frame: clear (unless buffered), the lights, the post-processor's Begin, the skybox
    // (focal length / 4, at least 12), the developer axes / measure line, the scenery, the grid,
    // the debug meshes, the flexible parts and the bodies' meshes into the scene queue (render
    // queue 1 with a post-processor, else 0), the wires, the selection boxes, the tools (notify
    // 5), flush the scene queue, the post-processor's Apply, flush, notify 6, the 2D state, notify
    // 7. NOT restored: the physics debug overlay (debug key 'p'). Mac 0x49440, Win 0x61afcb
    virtual void Draw(ControlDrawArgs& args);                               // 33/30

    // ---- ModelLoaderHandler (Mac primary entries 38 / 39) ------------------------------------
    // Clears the scene and starts a ModelLoaderProgressBarThread for `storage`. Mac 0x33e90
    // (thunk 0x346a0)
    virtual void OnLoadStarted(StorageScene2* storage);                     // 38
    // The thread finished: flexible parts, wires, the stored camera, the lazy loads, the
    // building instructions (or ClearScene + the application's slot 17 on failure); notifies.
    // Mac 0x346c0 (thunk 0x35420), Win inlined
    virtual void OnLoadFinished();                                          // 39

private:
    // ---- SceneObserver (private base; Mac primary entries 40-43) -----------------------------
    // After a physics step: grid, flexible parts, measure distance, BI reset. Mac 0x4af80
    // (thunk 0x4b450), Win 0x619ceb
    virtual void OnSceneNotification5();                                    // 40
    // UpdateMissingBricks, notify slot 19, grid, BI reset. Mac 0x4b470 (thunk 0x4b7f0), Win 0x6200da
    virtual void OnStructureChanged();                                      // 41
    // Grid, render data (incrementally in render mode 0), notify slot 20. Mac 0x4b810 (thunk
    // 0x4bfd0), Win 0x622a7c
    virtual void OnRigidElementsChanged(const BasicArray<RigidElement*>& added,
                                        const BasicArray<RigidElement*>& changed,
                                        const BasicArray<RigidElement*>& removed); // 42
    // Empty. Mac 0x4c000 (thunk 0x4c010)
    virtual void OnGroupsChanged(const BasicArray<RefPointer<BrickGroup> >& added,
                                 const BasicArray<RefPointer<BrickGroup> >& changed,
                                 const BasicArray<RefPointer<BrickGroup> >& moved,
                                 const BasicArray<RefPointer<BrickGroup> >& removed); // 43

public:
    // ---- UndoManagerObserver (44 / 45) --------------------------------------------------------
    virtual void OnUndoAvailable(bool available);                           // 44 Mac 0x4c020 (thunk 0x4c030)
    virtual void OnRedoAvailable(bool available);                           // 45 Mac 0x4c050 (empty)
    // ---- PreferencesObserver (46) ---------------------------------------------------------------
    // DEVELOPERMODE, ZoomToSelection, SelectedPostProcessingEffect, INVERTEDCAMERAX / Y.
    // Mac 0x4c070 (thunk 0x4c560), Win 0x614681
    virtual void OnPreferenceChanged(const String& key);                    // 46
    // ---- Observer overloads (47 / 48) and BrickPaletteObserver (49-51) -------------------------
    // The modified flag follows the undo stack; NotifyUndoState. Mac 0x4c580 (thunk 0x4c5c0),
    // Win 0x6184bc
    virtual void OnAttached(UndoManager* undoManager);                      // 47
    // UpdateMissingBricks. Mac 0x4c600 (thunk 0x4c610), Win 0x620153
    virtual void OnAttached(BrickPalette* palette);                         // 48
    virtual void OnBagAdded(const BrickPalette::Bag* bag);                  // 49 Mac 0x4c630 (thunk 0x4c640)
    virtual void OnBrickAdded(const BrickPalette::Brick* brick);            // 50 Mac 0x4c660 (thunk 0x4c670)
    // Every brick of the scene gets its palette brick again. Mac 0x4c690 (thunk 0x4c7d0),
    // Win 0x620161
    virtual void OnPaletteChanged();                                        // 51
    using UndoManagerObserver::OnDetached;
    using BrickPaletteObserver::OnDetached;

    // References through the VisualDebugger (BrickConnectSceneManager Mac 0x5d370).
    using RenderVisualDebugger::AddRef;
    using RenderVisualDebugger::Release;

    // ---- scene ----------------------------------------------------------------------------------
    // `scene` (null: BrickConnect slot 0x38 creates one) becomes the control's scene: its
    // update flags, the control as listener and observer, UpdateMissingBricks, notify slot 8.
    // Mac 0x339d0
    void SetScene(Scene* scene);
    // Everything goes: tool, selection, part lists, render data, loaded parts, the scene, wires,
    // camera, building instructions, undo, modified; notify slot 12. Mac 0x341f0
    void ClearScene();
    // The scene has no hinged element. Mac 0x380e0, Win 0x6133ff
    bool IsSceneEmpty() const;
    // Removes `brick` (with undo). Mac 0x35da0
    void RemoveBrick(Brick* brick);
    // Removes bricks (UndoRemoveSelection; selection / flex parts / wires follow; notify slot 14
    // when the selection emptied, slot 15 when it changed). Mac 0x36040
    void RemoveBricks(const BasicArray<RefPointer<Brick> >& bricks);
    // RemoveBricks of the bricks of `parts`. Mac 0x366a0
    void RemoveParts(const BasicArray<RefPointer<Part> >& parts);
    // RemoveParts(selection). Mac 0x367a0
    void RemoveSelection();
    // Scene::CanConnect for bodies. Mac 0x367c0
    bool CanConnect(const BasicArray<RigidElement*>& bodies, Scene::ConnectResult& result);
    // Scene::Reconnect. Mac 0x367f0
    void Reconnect(const Scene::ConnectResult& result);
    // The parts (expanded to bricks, groups, wires) move to `target` (selection cleared, their
    // wires deselected). Mac 0x36810, Win 0x61c1d9
    void MoveParts(const BasicArray<RefPointer<Part> >& parts, Scene* target);
    // Merges `other` into the scene at `result` (with undo of the moved elements unless the
    // transform is the identity); `insert`: false runs InsertScene instead. Mac 0x370c0
    void MergeScene(Scene* other, const Scene::ConnectResult& result, bool insert);
    // Inserts the bricks of `other` (UndoInsertSelection, flex parts, wires). Mac 0x37250
    void InsertScene(Scene* other, const Scene::ConnectResult& result);
    // Scene::CopyParts. Mac 0x37340
    void CopyParts(const BasicArray<RefPointer<Part> >& parts, Scene* target, Scene::CopyMap& map,
                   bool copyGroups, UInt32 flags);
    // Inserts bricks (BatchInsert) with undo. Mac 0x4d7f0, Win 0x620625
    void InsertBricks(const BasicArray<RefPointer<Brick> >& bricks);
    // Shows every brick; notify slot 22. Mac 0x374a0
    void ShowAllBricks();
    // The removed brick's part references go (unhidden, wire, flex part, selection). Mac 0x4d660
    void OnBrickRemoved(Brick* brick);
    // Transforms. Mac 0x38ef0 / 0x38f10
    void GetTransforms(BasicArray<Pair<PrimitiveElement*, Matrix3D> >& transforms) const;
    void GetChangedTransforms(const BasicArray<Pair<PrimitiveElement*, Matrix3D> >& transforms,
                              BasicArray<Pair<PrimitiveElement*, Matrix3D> >& changed) const;
    // Scene::SetTransforms with an UndoMoveSnapshot (`undo`). Mac 0x38f40, Win 0x6174bf
    void SetTransforms(const BasicArray<Pair<PrimitiveElement*, Matrix3D> >& transforms, bool undo);

    // ---- selection ------------------------------------------------------------------------------
    // Empties the selection (notify slot 14) and notifies slot 15. Mac 0x36a40
    void ClearSelection();
    const BasicArray<RefPointer<Part> >& GetSelection() const { return m_selection; } // Mac 0x3d510, Win 0x61247d
    // The selection-box vertex buffer / mesh of the selected parts (when shown). Mac 0x37780
    void UpdateSelectionMesh();
    void SetShowSelectionBoxes(bool show) { m_showSelectionBoxes = show; }   // Mac 0x4f290

    // ---- wires ----------------------------------------------------------------------------------
    // A wire brick gets a WireData: the next free wire colour (or a random one) and a triangle
    // mesh. Mac 0x34e90, Win 0x61e41c
    void AddWire(Brick* brick);
    // The wire's colour becomes free again. Mac 0x4f420
    void RemoveWire(Brick* brick);
    // The wires of `elements` (their bricks) get `selected`. Mac 0x36fd0, Win 0x61badf
    void SetWiresSelected(const BasicArray<RefPointer<PrimitiveElement> >& elements, bool selected);
    // Every wire. Mac 0x4f220, Win 0x61a038
    void SetAllWiresSelected(bool selected);
    // The endpoint meshes of the selected wires into `queue`. Mac 0x4acd0 (Win not identified;
    // Win 0x61d5cf is LoadTerrain)
    void QueueWireMeshes(RenderQueue* queue);

    // ---- undo / modified --------------------------------------------------------------------------
    UndoManager* GetUndoManager() const { return m_undoManager; }           // Mac 0x37240
    // Mac 0x380b0
    void SetModified(bool modified);
    // m_modified or the undo stack is not empty. Mac 0x43130, Win 0x612cea
    bool IsModified() const;
    // Notify slot 13 with IsModified(). Mac 0x42e80, Win 0x6184ae
    void NotifyUndoState();

    // ---- camera ---------------------------------------------------------------------------------
    // Fits `camera` to the scene's collision volumes (Scene::GetSceneCVPoints, margin 0.025) for
    // a viewport of width x height; an empty scene puts it at (40, 40, 40) looking at the
    // origin. Mac 0x39b60, Win 0x615222
    void FitCamera(Camera* camera, Real32 width, Real32 height);
    // The default view of the scene: FitCamera for the control size + Update, then the grid
    // follows the camera. Mac 0x33da0, Win 0x61769a (+ 0x6137c8)
    void ResetCamera();
    // The camera of a loaded model (StorageScene2 +0x78). Mac 0x352a0, Win 0x618139
    void SetCameraFromStorage(const StorageScene2& storage);
    // Orbit around the target (INVERTEDCAMERAX negates); the grid follows; redraw. Mac 0x426c0
    void RotateCamera(Real32 angle);
    // The same vertically (INVERTEDCAMERAY). Mac 0x427c0
    void RotateCameraVertical(Real32 angle);
    // Camera::Zoom; the grid follows; redraw. Mac 0x428c0
    void ZoomCamera(Real32 amount);
    // The camera targets the box of the element's part (`fit`: distance to fit it). Mac 0x429b0
    void FocusOnElement(PrimitiveElement* element, bool fit);
    Camera*        GetCamera() const { return m_camera; }                   // Mac 0x42650, Win 0x613702
    const Vector3& GetCameraPosition() const;                               // Mac 0x42660
    const Vector3& GetCameraTarget() const;                                 // Mac 0x42680
    const Vector3& GetCameraUp() const;                                     // Mac 0x426a0, Win 0x61371f

    // ---- grid / skybox / post-processing -------------------------------------------------------------
    // Dynamic grid (true) or none. Mac 0x43180, Win 0x61385a
    void SetGridVisible(bool visible);
    // Mac 0x431f0, Win 0x61387d
    void SetSkybox(Skybox* skybox);
    // Skybox::Next. Mac 0x454e0
    void NextSkybox();
    // The active post-processing effect (null: buffering off); only while the application
    // has post-processing. Mac 0x4f2b0
    void SetPostProcessor(PostProcessingEffect* effect);

    // ---- picking (021h) -------------------------------------------------------------------------
    // The view ray through `position` (control coordinates): origin on the near plane,
    // normalised direction, |far - near|. Mac 0x3b750, Win 0x61344d
    void GetPickRay(const Vector2& position, Vector3& origin, Vector3& direction, Real32& length) const;
    // The pick volume of a control rectangle: the camera position and the rays through the four
    // corners at the near plane (min, top right, max, bottom left). Mac 0x3b8f0, Win 0x613556
    // (name inferred)
    void GetPickFrustum(const Rect& rect, Frustum& frustum) const;
    // The nearest element hit by the ray within `length` (shortened to the hit distance):
    // Scene::PickRay (hidden elements skipped) by bounding sphere, the only-selected filter,
    // `filterParts` (when not empty: only elements of these parts), every element of
    // `extraParts` added; then in the order of the spheres' front distance: rigid parts by their
    // bone box and the triangles of the scene manager's import geometry (element space),
    // flexible parts by the triangles of their skinned flex surfaces (world space; the element
    // of the nearest triangle corner's first bone). `surface` (when given) receives the index of
    // the hit geometry / flex surface; with `materialSlot` it is mapped through the primitive's
    // sub-material redirect table. Mac 0x3bb30, Win 0x61f511
    PrimitiveElement* PickElement(const Vector3& origin, const Vector3& direction, Real32& length,
                                  UInt32* surface, bool materialSlot,
                                  const BasicArray<RefPointer<Part> >& filterParts,
                                  const BasicArray<RefPointer<Part> >& extraParts);
    // The same through `position` (GetPickRay); `point` (when given and something was hit)
    // receives the hit point, in the element's space when `local`. Mac 0x3ba30, Win 0x6214ee
    PrimitiveElement* PickElement(const Vector2& position, Vector3* point, bool local, UInt32* surface,
                                  bool materialSlot, const BasicArray<RefPointer<Part> >& filterParts,
                                  const BasicArray<RefPointer<Part> >& extraParts);
    // The connection field points nearest along the ray through `position` (within the pick
    // length; Scene::HideAwareSamePlaceFeaturesPicker). Mac 0x3cbf0, Win 0x614569 (name inferred)
    bool PickFeatures(const Vector2& position, BasicArray<Scene::Feature>& features);
    // Scene::PickFrustum + the only-selected filter. Mac 0x3d4e0, Win 0x61d2c5 (name inferred)
    void PickFrustum(const Frustum& frustum, Scene::PickResult& result);
    // With the only-selected filter on: drops the elements whose part is not in m_selectedParts.
    // Mac 0x3d360, Win 0x61ceb2 (name inferred)
    void FilterOnlySelected(Scene::PickResult& result) const;

    // ---- debug toggles (021h) ----------------------------------------------------------------------
    // Debug key 'm' (`clear` false): toggles the only-selected filter (on: m_selectedParts and
    // their elements from the selection when it is not empty); `clear`: filter off and empty.
    // Rebuilds the render data. Mac 0x3aad0
    void ToggleOnlySelected(bool clear);
    // Debug key 'S': the statistics text control (a RenderControl child, top right) on / off.
    // Mac 0x3adb0
    void ToggleStatisticsControl();
    // Debug key 'A': the version text control on, its next page (UpdateVersionText), off after
    // the last page. Mac 0x3afe0
    void ToggleVersionControl();
    // The text of the version control's page m_field234: 0 the preferences, 1 the
    // LDDWebCommunicator values, the brand URLs and the versions; any later page: back to 0 and
    // false (the control goes). True without a control. Mac 0x3b230, Win (part) 0x613a73
    // (name inferred)
    bool UpdateVersionText();
    // Building-guide mode (BGAutomaticMode): on clears the selection and the only-selected
    // filter; off rebuilds the render data. Mac 0x42d90 / 0x42e60 (names inferred)
    void SetBuildingGuideMode(bool on);
    bool IsBuildingGuideMode() const;
    // The y of the grid plane in use. Mac 0x43160, Win 0x61384f (name inferred)
    Real32 GetGridHeight() const;
    // Camera::SetLookAt + the control's viewport size. Mac 0x42600, Win 0x6136ca (name inferred)
    void SetCameraLookAt(const Vector3& position, const Vector3& target, const Vector3& up);

    // ---- selection (021h) ------------------------------------------------------------------------------
    // Replaces the selection (notify 14 when it became (non-)empty, 15). Mac 0x3d520,
    // Win 0x617806 (name inferred)
    void SetSelection(const BasicArray<RefPointer<Part> >& parts);
    // Adds `part` to the selection or removes it (notify 14 when it became (non-)empty, 15).
    // Mac 0x3dd10, Win 0x617914 (name inferred)
    void ToggleSelection(Part* part);
    // Every part of the scene. Mac 0x3e800, Win 0x617b46 (name inferred)
    void SelectAll();

    // ---- hiding (021h) ---------------------------------------------------------------------------------
    // The bricks with the hidden flag. Mac 0x3fa60
    void GetHiddenBricks(BasicArray<RefPointer<Brick> >& bricks) const;
    // Hides bricks (their parts leave the selection, swap-with-last); notify 22 (true).
    // Mac 0x3ee00, Win 0x617c0a
    void HideBricks(const BasicArray<RefPointer<Brick> >& bricks);
    // Shows bricks; notify 22 (false). Mac 0x3f770, Win 0x61533d (name inferred)
    void ShowBricks(const BasicArray<RefPointer<Brick> >& bricks);

    // ---- groups (021h, with undo) ------------------------------------------------------------------
    // A new group of `parts` (expanded: bricks, wires) below `parent` (UndoAddGroup).
    // Mac 0x3fab0, Win 0x617dd1 (name inferred)
    BrickGroup* CreateGroup(BrickGroup* parent, const BasicArray<RefPointer<Part> >& parts);
    // Dissolves `group` (UndoUnGroup). Mac 0x3fe80, Win 0x617e81 (name inferred)
    void UnGroup(BrickGroup* group);
    // The same after `parts` went back to `groups` (UndoAddGroup::Execute). Mac inlined into
    // 0x54890, Win 0x617ed9 (name inferred)
    void UnGroup(BrickGroup* group, const BasicArray<RefPointer<Part> >& parts,
                 const BasicArray<RefPointer<BrickGroup> >& groups);
    // Puts a dissolved group back (UndoReGroup; Scene::AddGroup). Mac 0x40110, Win 0x613669
    // (name inferred)
    void ReGroup(BrickGroup* group, BrickGroup* parent, const BasicArray<RefPointer<Part> >& parts,
                 const BasicArray<RefPointer<BrickGroup> >& groups);
    // Moves the parts (expanded: bricks, wires) into / out of `group` (UndoAddParts when one
    // moved). Mac 0x401f0 / 0x40400, Win 0x617f40 / 0x61800f (names inferred)
    void AddPartsToGroup(BrickGroup* group, const BasicArray<RefPointer<Part> >& parts);
    void RemovePartsFromGroup(BrickGroup* group, const BasicArray<RefPointer<Part> >& parts);

    // ---- materials / decorations (021h, with undo) ---------------------------------------------------
    // The materials and decorations of one part (RTTI BasicArray<BrickConnectSceneControl::
    // PartDescription>, element 0x20). (member names inferred)
    struct PartDescription {
        PartDescription() {}
        // The part's own arrays. Mac 0x4c9c0
        explicit PartDescription(const Part& part)
            : m_materials(part.GetMaterials()), m_decorations(part.GetDecorations()) {}
        // Part `index` of a palette brick snapshot (GetMaterials / GetDecorations). Mac 0x4cd60
        // (out of line here: PrimitiveElementBrick is only forward declared)
        PartDescription(const PrimitiveElementBrick& brick, UInt32 index);
        BasicArray<UInt32> m_materials;     // +0x00
        BasicArray<UInt32> m_decorations;   // +0x10
    };
    // Applies the descriptions (parts[i] <- descriptions[i]) with one UndoPartDescription of the
    // previous ones; the bricks whose palette brick changed are notified (17); UpdateMissingBricks.
    // Mac 0x41010, Win 0x620251 (name inferred)
    void SetPartDescriptions(const BasicArray<RefPointer<Part> >& parts,
                             const BasicArray<PartDescription>& descriptions);
    // The parts of `brick` get the materials / decorations of the palette brick's part with the
    // same design ID. Mac 0x40800, Win 0x6215bb (name inferred)
    void SetBrickDescription(Brick* brick, const BrickPalette::Brick* paletteBrick);
    // The same from a palette brick snapshot. Mac 0x41dc0, Win 0x6216d5 (name inferred)
    void SetBrickDescription(Brick* brick, const PrimitiveElementBrick* snapshot);
    // Material slot `index` of `part` (when it differs). Mac 0x42120, Win 0x621806 (name inferred)
    void SetPartMaterial(Part* part, UInt32 material, UInt32 index);
    // Every part: material slot 0 = `material`, the other slots 0. Mac 0x423f0, Win 0x6218e6
    // (name inferred)
    void SetPartsMaterial(UInt32 material, const BasicArray<RefPointer<Part> >& parts);
    // SetPartsMaterial of the selection. Mac 0x425d0, Win 0x62199f (name inferred)
    void SetSelectionMaterial(UInt32 material);

    // ---- missing bricks (021h) ------------------------------------------------------------------
    // While shown (and not LDD Extended): per palette brick (LDDBrick::GetPaletteBrick) the number
    // of scene bricks; notify 16. Mac 0x3cf20, Win 0x61e309 (thunk 0x620153)
    void UpdateMissingBricks();
    // Mac 0x4f260, Win 0x6616e2
    void SetShowMissingBricks(bool show);
    // Showing missing bricks and the application is not in the palette mode. Mac 0x4c850
    bool IsShowingMissingBricks() const;
    // The scene count of a palette brick (0 when absent). Mac 0x4f3b0
    UInt32 GetMissingBrickCount(const BrickPalette::Brick* brick) const;

    // ---- mouse / tools ----------------------------------------------------------------------------
    // The tool's cursor delta (+0x260 += 2 * flag - 1); redraw. Mac 0x42a80, Win 0x612484
    void AddPressedButton(bool pressed);
    // The physics world was stepped by someone else than the Scene:
    // the scene-observer slot 5 (OnSceneNotification5) through the private SceneObserver base,
    // as ViewMode::Update does (Mac 0x1cefe0: BCSC +0x1b8 slot 5; Win 0x622ea4: +0x1a8 slot 2).
    // (helper, name inferred)
    void NotifyPhysicsStepped() { static_cast<SceneObserver*>(this)->OnSceneNotification5(); }
    // The cursor of the active tool (021h):
    // Control::SetCursor(cursor, true), then the observers hear about it (with `flag`).
    // Caller: BrickConnectSceneControlTool (Mac 0x1a91c0 / 0x1a9730). Mac 0x42ac0 (name inferred)
    void SetToolCursor(UInt32 cursor, bool flag);

    // ---- clipboard / click-stick helpers (names inferred) ---------------------------------------
    // The element-brick snapshots of `parts` (CreateElementBricks). Caller: CutCommand (Mac
    // 0x26c2b0). Mac 0x37370, Win 0x61e2a9
    void GetElementBricks(const BasicArray<RefPointer<Part> >& parts,
                          BasicArray<ObjectRefPointer<PrimitiveElementBrick> >& bricks);
    // The same, then the bricks of `parts` are removed (RemoveBricks, with undo). Mac 0x37390
    void CutParts(const BasicArray<RefPointer<Part> >& parts,
                  BasicArray<ObjectRefPointer<PrimitiveElementBrick> >& bricks);
    // The bricks the click-stick tool carries (callers ClickStickTool Mac 0x143630, 0x1446f0,
    // 0x145300). Set (always true) Mac 0x46220; cleared Mac 0x462a0 and (the same body) 0x46300;
    // the array Mac 0x462f0.
    bool SetCarriedBricks(const BasicArray<ObjectRefPointer<PrimitiveElementBrick> >& bricks);
    void ClearCarriedBricks();
    void ResetCarriedBricks();
    BasicArray<ObjectRefPointer<PrimitiveElementBrick> >* GetCarriedBricks() { return &m_carriedBricks; }
    // The statistics text shows the triangle counts (+0x3bc); UpdateStatistics. Mac 0x46350,
    // Win 0x618438
    void SetShowStatistics(bool show);

    // ---- storage / statistics / physics ---------------------------------------------------------------
    // The scene, the camera (as StorageScene2::Camera, field of view = distance), the building
    // instructions into `storage` (cleared first). Mac 0x35440
    void SaveToStorage(StorageScene2& storage);
    // fixed (M8, not in the original): the observers (tools) undo their previews on the parts
    // (BrickConnectSceneControlObserver::OnClearPreview) before a save or export reads the scene.
    void ClearToolPreviews();
    // The statistics text of the debug control (+0x22c). Mac 0x38480, Win 0x616132
    void UpdateStatistics();
    // The physics snapshot (PhysicsScene::CreateSnapshot) of the bodies of every hinged element of the scene;
    // `snapshot` is released first and stays null when the scene is empty. Callers: the hinge
    // tool's undo (Mac 0x125c50) and ViewMode (0x1ce0c0). Mac 0x39150 (names inferred)
    void CreatePhysicsSnapshot(PhysicsSnapshot*& snapshot);
    // Puts the snapshot's bodies back (PhysicsScene::RestoreSnapshot) and notifies the scene's
    // observers (slot 5); with `undo` first an UndoMoveSnapshot of the current transforms of the
    // primitive elements of those bodies goes onto the undo stack. Nothing for an empty
    // snapshot. Callers: UndoHingeTool (Mac 0x127f40), ViewMode (0x1cebb0). Mac 0x39480
    void RestorePhysicsSnapshot(PhysicsSnapshot* snapshot, bool undo);
    // A scenery element: the meshes are drawn with the model, the physics object (referenced)
    // goes into the physics scene at `transform` (PhysicsScene slot 25 with `flags`); statistics
    // follow. Caller: LoadTerrain. Mac 0x2f730 (021a; name inferred), Win 0x61d22c
    void AddSceneryElement(PhysicsSceneryElement* object, const Matrix3D& transform, UInt32 flags,
                           const BasicArray<PlacedMesh>& meshes);
    // A scenery element's physics object leaves (physics slot 26, released). Mac 0x38d60
    void RemoveSceneryElement(PhysicsSceneryElement* object);
    // "   Brand - <brand>:" with its upload / check-price URLs (version text). Mac 0x4c880,
    // Win 0x6125c8
    String GetBrandURLText(Brand brand) const;

    // ---- render data ----------------------------------------------------------------------------
    void AddFlexPart(Part* part);                                           // Mac 0x34b80
    void RebuildRenderData();                                               // Mac 0x39df0, Win 0x622104
    // BuildRenderDataForParts of the part list +0x48c and an empty list: the building-guide
    // render data again after RebuildRenderData (UploadModelDialog). Mac 0x488e0, Win 0x61ce6b
    void RebuildPartRenderData();
    // The render data (m_bricks) from `parts` instead of the scene's bodies: one BIBrick per part
    // (no body, identity placement: the parts' own transforms are baked into the PlacedMeshes);
    // meshes of a single-element part that is also in `highlighted` go to the overlay list +0x20.
    // Mac 0x48940, Win 0x61c369 (name inferred)
    void BuildRenderDataForParts(const BasicArray<RefPointer<Part> >& parts,
                                 const BasicArray<RefPointer<Part> >& highlighted);
    // Render mode 0 incremental update: removed bodies' BIBricks go, added ones are built,
    // changed ones rebuilt; flex parts, statistics, the light camera fitted. Mac 0x4bba0
    void UpdateRenderData(const BasicArray<RigidElement*>& added,
                          const BasicArray<RigidElement*>& changed,
                          const BasicArray<RigidElement*>& removed);
    void BuildBodyMeshes(BIBrick& brick, Material* material);               // Mac 0x46560, Win 0x622082
    void FilterPatches(GeometryAccum& accum);                               // Mac 0x4d140, Win 0x619e2e
    void CreateAccumulatedMeshes(GeometryAccum& accum, Material* material, BIBrick& brick,
                                 bool secondary, const Vector3& center);    // Mac 0x4f6a0
    // The level-of-detail regions of a body (LDD64; BrickConnectSceneControl.cpp).
    void BuildLodRegions(BIBrick& brick, Material* material);
    void BuildLodBoxes(BIBrick::LodRegion& region, const BasicArray<PrimitiveElement*>& elements,
                       const BasicArray<Matrix3D>& transforms);
    // The preferences "LevelOfDetail", "LevelOfDetailDistance", "LevelOfDetailBoxes"; rebuilds
    // the render data when the meshes to build changed.
    void ReadLodPreferences(bool rebuild);
    void UpdateFlexParts();                                                 // Mac 0x48320, Win 0x619439
    bool RenderToBitmap(Bitmap& bitmap, Texture* target, bool fitCamera, bool noPostProcessing); // Mac 0x45520
    bool RenderScene(RenderQueue* queue, Bitmap& bitmap, Texture* target, bool fitCamera,
                     bool noPostProcessing);                                // Mac 0x45790

    // ---- accessors (inline in the original) -------------------------------------------------
    BrickConnectSceneManager*   GetSceneManager() const { return m_sceneManager; } // Mac 0x33d20, Win 0x6300b1
    BrickPalette*               GetBrickPalette() const { return BrickPaletteObserver::GetSubject(); } // Mac 0x33e80
    Scene*                      GetScene() const { return m_scene; } // Mac 0x3d350, Win 0x613662
    RenderControlManager*       GetRenderControlManager() const;
    const BasicArray<BIBrick>&  GetBricks() const { return m_bricks; }
    const BasicArray<LDDPart*>& GetFlexParts() const { return m_flexParts; }
    const Light&                GetLight(UInt32 index) const { return m_lights[index].m_light; }
    Camera*                     GetLightCamera(UInt32 index) const { return m_lights[index].m_camera; }
    Grid*                       GetGrid() const { return m_grid; }
    BI*                         GetBuildingInstructions() const { return m_buildingInstructions; }
    bool                        IsDeveloperMode() const { return m_developerMode; }
    void                        SetRenderMode(UInt32 mode) { m_renderMode = mode; }
    UInt32                      GetRenderMode() const { return m_renderMode; }

protected:
    using SceneObserver::OnAttached;
    using PreferencesObserver::OnAttached;
    // Removes the scenery elements, leaves the palette, the preferences, the undo manager and
    // the scene, detaches the observers, Control::OnFinalRelease. Mac 0x44db0 (thunks
    // 0x451d0-0x452b0), Win 0x61c2e9
    virtual void OnFinalRelease();

private:
    BrickConnectSceneControl(const BrickConnectSceneControl&);
    BrickConnectSceneControl& operator=(const BrickConnectSceneControl&);

    // The grid follows the camera and the scene box (+ `extraBox`; Grid slot 3). Out of line
    // Mac 0x39d10, inline at the other call sites. NOTE: skipped while there is no grid (only
    // the HOST-ONLY picture path has none).
    void UpdateGrid(const BoundingBox* extraBox = 0);   // Mac 0x39d10 (and inline copies)
    // The lights and their cameras (part of Initialize). Mac 0x32ed2-0x333c1
    void InitializeLights();
    // The terrain of a height map image (the preferences TerrainHeightMap / TerrainTexture):
    // the grey values - 127 become a physics height field (spacing 0.65, height scale 0.2) in a
    // scenery element, and a lit, textured triangle grid (normals averaged over the adjacent
    // triangles, texture coordinates twice over the map) its mesh; both go to `control`
    // (AddSceneryElement at `transform`, flags 2). False when the file, the codec, the decode,
    // the physics objects or the vertex buffer fail. (021a; a static member here: the original
    // passes the control as an argument) Mac 0x2d5c0, Win 0x61d5cf
    static bool LoadTerrain(const Path& heightMap, Texture* texture, Render* render,
                            BrickConnect* brickConnect, BrickConnectSceneControl* control,
                            const Matrix3D& transform);
    // Invalidate unless the manager suppresses redraws (inline everywhere).
    void Redraw();

    // ---- data (Mac offsets) ----------------------------------------------------------------------
    bool                       m_gridVisible;         // +0x228 1 ('G' toggles dynamic / none grid)
    RefPointer<RenderControl>  m_statisticsControl;   // +0x22c 0 (debug text, 'S', UpdateStatistics)
    RefPointer<RenderControl>  m_versionControl;      // +0x230 0 (debug text, 'A')
    UInt32                     m_field234;            // +0x234 0
    RefPointer<Grid>           m_dynamicGrid;         // +0x238
    RefPointer<Grid>           m_grid;                // +0x23c the grid in use
    RefPointer<Grid>           m_noneGrid;            // +0x240
    RefPointer<Skybox>         m_skybox;              // +0x244 0 (set by the view mode)
    bool                       m_modified;            // +0x248
    bool                       m_loading;             // +0x249 a model is being loaded
    bool                       m_invertedCameraX;     // +0x24a preference INVERTEDCAMERAX
    bool                       m_invertedCameraY;     // +0x24b preference INVERTEDCAMERAY
    BasicArray<RefPointer<Part> > m_loadedParts;      // +0x24c the parts of the model being loaded
    RefPointer<StorageScene2>  m_loadingStorage;      // +0x25c
    Int32                      m_pressedButtons;      // +0x260 (AddPressedButton)
    bool                       m_leftButtonDown;      // +0x264
    bool                       m_zoomToSelection;     // +0x265 preference ZOOMTOSELECTION
    BrickConnectSceneManager*  m_sceneManager;        // +0x268
    // (RTTI BasicArray<ObjectRefPointer<PrimitiveElementBrick>>, element 8 bytes)
    BasicArray<ObjectRefPointer<PrimitiveElementBrick> > m_carriedBricks; // +0x26c (the click-stick tool's)
    UInt32                     m_field27c;            // +0x27c 0
    UndoManager*               m_undoManager;         // +0x280 referenced
    SceneLight                 m_lights[3];           // +0x284 (0x64 each)
    RefPointer<Scene>          m_scene;               // +0x3b0
    bool                       m_developerMode;       // +0x3b4 preference DEVELOPERMODE
    UInt32                     m_cullingFlags;        // +0x3b8 0xf (GeometryAccum flags; keys k q Q K)
    bool                       m_showStatistics;      // +0x3bc 1
    bool                       m_countTriangles;      // +0x3bd 1 ('g')
    bool                       m_showOverlay;         // +0x3be 0 ('P', 0x47a50)
    bool                       m_onlySelected;        // +0x3bf 0 (FilterPatches; 'M' clears)
    bool                       m_field3c0;            // +0x3c0 0 ('p')
    bool                       m_debugFlag3c1;        // +0x3c1 0 ('V', debug info flag 0x400)
    UInt32                     m_renderMode;          // +0x3c4 0 (0: merged meshes; keys h o r v)
    UInt32                     m_debugMode3c8;        // +0x3c8 0 (0x47980; keys 1 2 3 b c)
    Real32                     m_field3cc;            // +0x3cc 1.0
    bool                       m_wireframe;           // +0x3d0 0 ('W')
    RefPointer<Camera>         m_camera;              // +0x3d4 the view camera
    PrimitiveElement*          m_pressedElement;      // +0x3d8 element under the last button-1 press
    bool                       m_showSelectionBoxes;  // +0x3dc 0
    BasicArray<RefPointer<Part> > m_selection;        // +0x3e0
    BasicArray<RefPointer<Part> > m_selectedParts;    // +0x3f0 FilterPatches' part filter
    BasicArray<RefPointer<PrimitiveElement> > m_field400; // +0x400
    Vector2                    m_mousePosition;       // +0x410 last mouse position (press / move)
    BasicMap<RefPointer<const Brick>, RefPointer<WireData> > m_wires;  // +0x418
    BasicArray<Pair<Color, bool> > m_wireColors;      // +0x434 14 colours, used flag
    BasicMap<const BrickPalette::Brick*, UInt32> m_missingBricks; // +0x444 (021h: palette brick -> scene count)
    bool                       m_showMissingBricks;   // +0x460 0
    bool                       m_field461;            // +0x461 0
    RefPointer<BI>             m_buildingInstructions;      // +0x464
    RefPointer<BI>             m_loadedBuildingInstructions; // +0x468
    BasicArray<Selection>      m_selections;          // +0x46c (021h)
    BasicArray<Pair<PrimitiveElement*, Matrix3D> > m_field47c; // +0x47c (021h)
    BasicArray<RefPointer<Part> > m_field48c;         // +0x48c (021h)
    BasicArray<SceneryElement> m_sceneryElements;     // +0x49c
    BasicArray<BIBrick>        m_bricks;              // +0x4ac
    RefPointer<VertexBuffer>   m_selectionBuffer;     // +0x4bc
    RefPointer<Mesh>           m_selectionMesh;       // +0x4c0
    RefPointer<Material>       m_selectionMaterial;   // +0x4c4 the scene manager's silhouette material
    BasicArray<LDDPart*>       m_flexParts;           // +0x4c8
    RefPointer<PrimitiveElement> m_measureFeature1;   // +0x4d8 (measure: the element of the first picked field)
    RefPointer<PrimitiveElement> m_measureFeature2;   // +0x4dc
    Vector3                    m_measurePoint1;       // +0x4e0
    Vector3                    m_measurePoint2;       // +0x4ec
    RefPointer<VertexBuffer>   m_measureBuffer;       // +0x4f8
    RefPointer<Mesh>           m_measureMesh;         // +0x4fc
    RefPointer<PostProcessingEffect> m_postProcessor; // +0x500 the active effect
    RefPointer<Object>         m_field504;            // +0x504 ('t' releases it)
    UInt32                     m_field508;            // +0x508 0
    RefPointer<LDDPostProcessor> m_postProcessorControl; // +0x50c
                                                      // Object +0x510
    // vertex buffers of the body meshes, shared by bodies with equal contents (CreateAccumulatedMeshes)
    class MeshBufferCache;
    MeshBufferCache* m_meshBufferCache;
    // level of detail: on, the squared distances for no studs / boxes (0: never), and the boxes'
    // plain materials by the material they stand for
    bool                       m_lodEnabled;
    bool                       m_lodBoxes;
    Real32                     m_lodReducedDistance2;
    Real32                     m_lodBoxDistance2;
    BasicMap<Material*, RefPointer<Material> > m_lodBoxMaterials;
    // the boxes shown while the camera moves (FastNavigation; BrickConnectSceneControl.cpp)
    class NavigationProxy;
    friend class NavigationProxy;
    NavigationProxy* m_navigation;
    friend struct BrickConnectSceneControlTestAccess;   // (tests/test_bcsc.cpp, diff_bcsc.cpp)
    // The click-stick and move tools move the grid with the carried bricks (UpdateGrid,
    // Mac 0x39d10) and redraw (the inline Redraw).
    friend class ClickStickTool;
    friend class MoveTool;
};

// ---- the undo actions of TU 021 (constructors in 021b / 021c / 021f / 021h; the Execute bodies
// are emitted with the vtables in 021d; the destructors are the implicit inline ones) ----------

// Element transforms before a move / physics step. Mac sizeof 0x20 {vptr, m_control +4,
// m_transforms +8, Object +0x18}; vtable 0x7048a0 {Execute 0x50f10, D1 0x50d70, D0 0x50e20}.
class UndoMoveSnapshot : public UndoAction {
public:
    // Mac 0x39070: the current transforms of `entries`' elements (`fromScene`) or a copy.
    UndoMoveSnapshot(BrickConnectSceneControl* control,
                     const BasicArray<Pair<PrimitiveElement*, Matrix3D> >& entries, bool fromScene);
    // SetTransforms(m_transforms, true). Mac 0x50f10, Win 0x6196ff
    virtual void Execute();
    BrickConnectSceneControl*                       m_control;      // +0x04
    BasicArray<Pair<PrimitiveElement*, Matrix3D> >  m_transforms;   // +0x08
};

// Bricks that were inserted (undo removes them). Mac sizeof 0x20; vtable 0x760950
// {Execute 0x55930, D1 0x55950, D0 0x55a00}.
class UndoInsertSelection : public UndoAction {
public:
    // Mac 0x453c0
    UndoInsertSelection(BrickConnectSceneControl* control, const BasicArray<RefPointer<Brick> >& bricks);
    // RemoveBricks(m_bricks). Mac 0x55930, Win 0x61e1cc
    virtual void Execute();
    BrickConnectSceneControl*        m_control;   // +0x04
    BasicArray<RefPointer<Brick> >   m_bricks;    // +0x08
};

// Bricks that were removed with the groups of their parts (the parts leave their groups).
// Mac sizeof 0x34; vtable 0x760a70 {Execute 0x56300, D1 0x56380, D0 0x563e0}.
class UndoRemoveSelection : public UndoAction {
public:
    // Mac 0x4d370
    UndoRemoveSelection(BrickConnectSceneControl* control, const BasicArray<RefPointer<Brick> >& bricks);
    // InsertBricks(m_bricks), the parts back into their groups. Mac 0x56300, Win 0x62092e
    virtual void Execute();
    BrickConnectSceneControl*                         m_control;   // +0x04
    BasicArray<RefPointer<Brick> >                    m_bricks;    // +0x08
    BasicArray<Pair<Part*, RefPointer<BrickGroup> > > m_groups;    // +0x18
    Scene*                                            m_scene;     // +0x28
};

// A new group (undo: UnGroup after the parts went back to their groups). Mac sizeof 0x34
// {vptr, m_control +4, m_group +8, m_parts +0xc, m_groups +0x1c, Object +0x2c}; vtable 0x7606dc
// {Execute 0x54890, D1 0x54930, D0 0x54990}; Win vftable 0x9cf7a8 {Execute 0x619c40}.
class UndoAddGroup : public UndoAction {
public:
    // The parts and their current groups. Mac 0x3fc60, Win 0x616f92
    UndoAddGroup(BrickConnectSceneControl* control, const BasicArray<RefPointer<Part> >& parts);
    virtual void Execute();                                                 // Mac 0x54890, Win 0x619c40
    BrickConnectSceneControl*            m_control;   // +0x04
    RefPointer<BrickGroup>               m_group;     // +0x08 the new group (set by CreateGroup)
    BasicArray<RefPointer<Part> >        m_parts;     // +0x0c
    BasicArray<RefPointer<BrickGroup> >  m_groups;    // +0x1c the parts' groups before
};

// A dissolved group (undo: ReGroup). Mac sizeof 0x38 {vptr, m_control +4, m_group +8, m_parent
// +0xc, m_parts +0x10, m_groups +0x20, Object +0x30}; vtable 0x76079c {Execute 0x54d00, D1
// 0x54d30, D0 0x54d90}; Win vftable 0x9cf7c4 {Execute 0x6171f6}.
class UndoUnGroup : public UndoAction {
public:
    // The group, its parent, its parts and sub-groups. Mac 0x3ff00, Win 0x61711a
    UndoUnGroup(BrickConnectSceneControl* control, BrickGroup* group);
    virtual void Execute();                                                 // Mac 0x54d00, Win 0x6171f6
    BrickConnectSceneControl*            m_control;   // +0x04
    RefPointer<BrickGroup>               m_group;     // +0x08
    RefPointer<BrickGroup>               m_parent;    // +0x0c
    BasicArray<RefPointer<Part> >        m_parts;     // +0x10
    BasicArray<RefPointer<BrickGroup> >  m_groups;    // +0x20
};

// A group put back (undo: UnGroup). Mac sizeof 0x14 {vptr, m_control +4, m_group +8, Object
// +0xc}; vtable 0x760850 {Execute 0x55320, D1 0x553a0, D0 0x55410}; Win vftable 0x9cf434
// {Execute 0x619c92}. Constructor inline (Mac in 0x40110, Win 0x6132c5).
class UndoReGroup : public UndoAction {
public:
    UndoReGroup(BrickConnectSceneControl* control, BrickGroup* group) : m_control(control), m_group(group) {}
    virtual void Execute();                                                 // Mac 0x55320, Win 0x619c92
    BrickConnectSceneControl* m_control;   // +0x04
    RefPointer<BrickGroup>    m_group;     // +0x08
};

// Parts that moved between groups (undo: back to `m_groups`). Mac sizeof 0x30 {vptr, m_control
// +4, m_parts +8, m_groups +0x18, Object +0x28}; vtable 0x7608a0 {Execute 0x555a0, D1 0x55630,
// D0 0x55690}; Win vftable 0x9cf7e0 {Execute 0x619c9e}.
class UndoAddParts : public UndoAction {
public:
    // Empty (filled by BrickGroup::AddParts / RemoveParts). Mac inline in 0x401f0, Win 0x6172a4
    explicit UndoAddParts(BrickConnectSceneControl* control) : m_control(control) {}
    // The parts and their current groups. Mac 0x40610
    UndoAddParts(BrickConnectSceneControl* control, const BasicArray<RefPointer<Part> >& parts);
    virtual void Execute();                                                 // Mac 0x555a0, Win 0x619c9e
    BrickConnectSceneControl*            m_control;   // +0x04
    BasicArray<RefPointer<Part> >        m_parts;     // +0x08
    BasicArray<RefPointer<BrickGroup> >  m_groups;    // +0x18
};

// Materials / decorations before SetPartDescriptions. Mac sizeof 0x30 {vptr, m_control +4,
// m_parts +8, m_descriptions +0x18, Object +0x28}; vtable 0x7609c8 {Execute 0x55e50, D1 0x55e80,
// D0 0x55ee0}; Win vftable 0x9cf840 {Execute 0x62091d}. Constructor inline (Mac in 0x41010).
class UndoPartDescription : public UndoAction {
public:
    explicit UndoPartDescription(BrickConnectSceneControl* control) : m_control(control) {}
    virtual void Execute();                                                 // Mac 0x55e50, Win 0x62091d
    BrickConnectSceneControl*                              m_control;       // +0x04
    BasicArray<RefPointer<Part> >                          m_parts;         // +0x08
    BasicArray<BrickConnectSceneControl::PartDescription>  m_descriptions;  // +0x18
};

} // namespace LEGO
