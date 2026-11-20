// LEGO Digital Designer — restored source
// LEGO::BrickConnectSceneManager — the scene -> render glue: owns the brick database and the
// LDDBrickConnect; creates and caches the render materials per brick material (ColoredMaterial or
// shader materials, decal materials / textures), the primitives' vertex buffers (plain or
// round-edge) and flex geometries, loaded lazily (LazyLoadData, pumped by the main updater).
// Tracks the active BrickConnectSceneControl and its active tool.
// LEGO::BrickConnectSceneManagerObserver, LEGO::BrickConnectSceneManager::LazyLoadCallback.
//
// TU "BrickConnectSceneManager.cpp" (Mac TU 022 0x5bcd0-0x683bb, split 022a 0x5bcd0-0x60880,
// 022c 0x60880-0x626d0, 022b 0x626d0-0x683bb; Win 0x609720-0x6123fb mixed with other TUs).
// Directory LDDLib: it reads the LDDApplication render switches, creates the LDDBrickConnect
// (TU 351) and is created by the start-up (TU 060); its Win code sits in the LDDLib block (0x5c0000-,
// between the start-up 0x5c841a and ViewMode 0x622ea4), far from DCL Connectivity.
//
// RTTI: class LEGO::BrickConnectSceneManager : Notifier<BrickConnectSceneManagerObserver>,
//   UpdateObserver.
//   Mac sizeof 0x1fc (start-up 0x13ad50: operator new(0x1fc), ctor 0x5be80): vptr +0 (vtable
//       0x704fac {D1 0x626d0, D0 0x62810, OnFinalRelease 0x62e50, Update 0x60880}), observers
//       +0x04, iterators +0x14, UpdateObserver +0x24 (vtable 0x704fc8 {thunks 0x62730, 0x62880,
//       0x62ee0, OnAttached 0x1e380, OnDetached 0x1e390, Update thunk 0x60a30}; subject +0x28),
//       members +0x2c-+0x1f0, Object at +0x1f4 (vtable 0x704ff8). VTT 0x705010; destructor body
//       0x61c90.
//   Win: vfptr +0, vbptr +4, members from +0x08; the BasicMaps are 0x24 bytes (Mac 0x1c), so
//       the offsets drift (Win = Mac + 4 up to +0x5c; the five material maps 5 x 0x24 from
//       Win +0x84; flags +0x13c; shader directories +0x144 / +0x148; lazy loads +0x24c).
//       Constructor 0x610f17 (thiscall + most-derived flag; Win sizeof 0x26c, start-up 0x5cc8dd),
//       destructor 0x6115d7 / 0x6117e0 (sdd 0x6117f6), OnFinalRelease 0x611237, Update 0x60d9d6.
// Class names from RTTI (also LazyLoadData, LazyLoadCallback, UniqueShaderMaterialKey); every
// member and method name is inferred unless noted.
#pragma once
#include "LEGO/Types.h"
#include "Object.h"
#include "Notifier.h"
#include "Updater.h"
#include "BasicArray.h"
#include "BasicMap.h"
#include "Color.h"
#include "Render.h"
#include "MaterialManual.h"
#include "GeometryAccum.h"

namespace LEGO {

class BrickConnectSceneManager;
class BrickConnectSceneControl;
class RenderControlManager;
class BrickConnectSceneControlTool;
class BrickConnect;
class BrickDatabase;
class BrickGeometry;
class Primitive;
class Directory;
class Material;
class Mesh;
class Shader;
class Texture;
class VertexBuffer;

// Mac vtable 0x73944c {D1 0x193600, D0 0x193630, OnFinalRelease 0x193680, OnAttached 0x1936d0,
// OnDetached 0x1936e0, 0x1936f0, 0x193700} (emitted in another TU; the notifications are empty).
class BrickConnectSceneManagerObserver
    : public Observer<BrickConnectSceneManager, BrickConnectSceneManagerObserver> {
public:
    virtual ~BrickConnectSceneManagerObserver() {}
    // slot 5: SetSceneControl (Mac 0x5d370) — previous and new control.
    virtual void OnSceneControlChanged(BrickConnectSceneControl* /*previous*/,
                                       BrickConnectSceneControl* /*current*/) {}   // Mac 0x1936f0
    // slot 6: SetActiveTool (Mac 0x5d920) — previous and new tool.
    virtual void OnActiveToolChanged(BrickConnectSceneControlTool* /*previous*/,
                                     BrickConnectSceneControlTool* /*current*/) {} // Mac 0x193700
};

// Ordering of the colour-keyed material caches (body of Less<MaterialColor>, Mac 0x67630):
// diffuse, ambient, specular, emission by Color::operator!= / operator<, then shininess.
inline bool operator<(const MaterialColor& a, const MaterialColor& b)
{
    if (a.diffuse != b.diffuse)
        return a.diffuse < b.diffuse;
    if (a.ambient != b.ambient)
        return a.ambient < b.ambient;
    if (a.specular != b.specular)
        return a.specular < b.specular;
    if (a.emission != b.emission)
        return a.emission < b.emission;
    return a.shininess < b.shininess;
}

class BrickConnectSceneManager : public Notifier<BrickConnectSceneManagerObserver>,
                                 public UpdateObserver {
public:
    // The five material sets (index of m_materials; names inferred from the switches and the
    // shader variants): 0 scene, 1 palette, 2 scene with the "REBG" round-edge shaders,
    // 3 like 0, 4 colour change (ShadedColorChange / ShadedRoundEdgeColorChange materials).
    enum MaterialSet {
        kMaterialSetScene = 0, kMaterialSetPalette = 1, kMaterialSetBackground = 2,
        kMaterialSet3 = 3, kMaterialSetColorChange = 4, kMaterialSetCount = 5
    };

    class LazyLoadCallback;

    // What a control needs before it can draw: the geometry of these bricks, these materials
    // (of one material set) and these decorations. 0x3c bytes (both builds). Copy constructor
    // Mac 0x64f30; BasicArray<LazyLoadData> vtable Mac 0x761560.
    struct LazyLoadData {
        LazyLoadData() : callback(0), materialSet(0), ticket(0) {}
        // Adds the brick once (std::find + push_back). (name inferred)
        void AddBrick(UInt32 designID);                                 // Mac 0x5f770
        // Adds every non-zero id once (std::find + push_back). (names inferred)
        void AddMaterials(const BasicArray<UInt32>& ids);               // Mac 0x5fab0
        void AddDecorations(const BasicArray<UInt32>& ids);             // Mac 0x5fb00

        BasicArray<UInt32> brickIDs;      // +0x00 BrickDatabase brick (design) ids
        BasicArray<UInt32> materials;     // +0x10
        BasicArray<UInt32> decorations;   // +0x20
        LazyLoadCallback*  callback;      // +0x30
        UInt32             materialSet;   // +0x34
        UInt32             ticket;        // +0x38 RequestLazyLoad's result
    };

    // Told when a lazy load is complete. Plain class (no Object base), not owned.
    // Mac vtable 0x76e3c8 {D1 0x14e960, D0 0x14e970, 0x14e980, 0x14e990} (inline, emitted in
    // LDDUtilities); implemented by ModelControl.
    class LazyLoadCallback {
    public:
        virtual ~LazyLoadCallback() {}
        // Returns false to get OnNotHandled called. (name inferred)
        virtual bool OnLoaded(const LazyLoadData& /*data*/) { return true; }   // Mac 0x14e980
        virtual void OnNotHandled(const LazyLoadData& /*data*/) {}             // Mac 0x14e990
    };

    // Key of the shader-material cache (Mac node: type +0x10, colours +0x14, set +0x58).
    struct UniqueShaderMaterialKey {
        UniqueShaderMaterialKey() : materialType(0), materialSet(0) {}
        bool operator<(const UniqueShaderMaterialKey& o) const   // inlined in Mac 0x61700
        {
            if (materialSet != o.materialSet)
                return (Int32)materialSet < (Int32)o.materialSet;
            if (materialType != o.materialType)
                return (Int32)materialType < (Int32)o.materialType;
            return colors < o.colors;
        }
        UInt32        materialType;   // +0x00 MaterialManual::MaterialType
        MaterialColor colors;         // +0x04
        UInt32        materialSet;    // +0x48
    };

    typedef Pair<UInt32, UInt32> GeometryKey;   // (design id, geometry face)

    BrickConnectSceneManager();                                         // Mac 0x5be80, Win 0x610f17 (thiscall + most-derived flag)
    virtual ~BrickConnectSceneManager();                                // Mac 0x626d0/0x62810 (body 0x61c90), Win 0x6115d7 (body) / 0x6117e0, sdd 0x6117f6

    // Opens the shader directories (assets "Shaders", "Shaders/RoundEdge"), the user brick
    // database (user data directory / preference "userBrickDBDirName"; assemblies unless the
    // preference "LoadAssemblies" is false; "MaterialNames" added to the TextLocale), the
    // physics ("pes") and the LDDBrickConnect, reads the shader switches and creates the axes
    // and the two box meshes (the render comes from the render control manager). false when one
    // of them fails.
    bool Initialize(RenderControlManager* manager);                                    // Mac 0x5c900, Win 0x60ae2d

    // ---- scene control / tools ----------------------------------------------------------------
    void SetSceneControl(BrickConnectSceneControl* control);            // Mac 0x5d370
    void SetActiveTool(BrickConnectSceneControlTool* tool);             // Mac 0x5d920
    void SetDefaultTool(BrickConnectSceneControlTool* tool);            // Mac 0x5dc80
    BrickConnectSceneControl*     GetSceneControl() const { return m_sceneControl; }
    BrickConnectSceneControlTool* GetActiveTool() const { return m_activeTool; }
    BrickConnectSceneControlTool* GetDefaultTool() const { return m_defaultTool; }

    // ---- geometry ---------------------------------------------------------------------------
    // Vertex buffer of geometry `face` (0 = "<id>.g", n = "<id>.g<n>") of a rigid primitive,
    // loaded on first use; null for flexible primitives or when the file is missing.
    VertexBuffer* GetVertexBuffer(const Primitive* primitive, UInt32 face);        // Mac 0x5dcf0, Win 0x611259
    // New SingleMesh (not referenced) of that vertex buffer with GetMaterial(material, set).
    Mesh* CreateMesh(const Primitive* primitive, UInt32 face, UInt32 material,
                     UInt32 materialSet);                                          // Mac 0x5df10, Win 0x6112da
    // The same for the decoration geometry of `face` and GetDecalMaterial(decoration).
    Mesh* CreateDecorationMesh(const Primitive* primitive, UInt32 face, UInt32 decoration); // Mac 0x5e330, Win 0x61140d
    // Decoration-geometry vertex buffer of `face` (QueryDecorationFaceGeometry), loaded on first
    // use. (Mac: inlined into 0x5e330; Win out of line)
    VertexBuffer* GetDecorationVertexBuffer(const Primitive* primitive, UInt32 face);       // Win 0x6113a3
    // .g data of a flexible primitive (null for rigid ones), loaded on first use.
    const BrickGeometry* GetFlexGeometry(const Primitive* primitive, UInt32 face); // Mac 0x5eb70, Win 0x6103eb

    // ---- materials ------------------------------------------------------------------------
    // Material of brick material `material` in set `materialSet`, created on first use (null
    // for an unknown material). Not referenced for the caller.
    Material* GetMaterial(UInt32 material, UInt32 materialSet);        // Mac 0x5e160, Win 0x610c93
    // Material of decoration `decoration` (its image from db Decorations/), created on first use.
    Material* GetDecalMaterial(UInt32 decoration);                      // Mac 0x5e4b0, Win 0x6104c1
    // A new black wireframe material (not cached, not referenced).
    Material* CreateWireframeMaterial();                                // Mac 0x5efe0, Win 0x60b38c
    // The shader material of a MaterialType (ShinySteel for 2, ShinyPlastic for 0, else the
    // unnamed shader) for `colors`; cached.
    Material* CreateShaderMaterial(UInt32 materialType, const MaterialColor& colors,
                                   UInt32 materialSet);                 // Mac 0x614b0, Win 0x610762
    // Shader descriptor "<name>.sh" of the shader directory, or "RE<name>.sh" / "REBG<name>.sh"
    // (variant 2) of the round-edge directory when round edges are on. Not referenced.
    Shader* LoadShader(const String& name, UInt32 variant);            // Mac 0x5f290, Win 0x60a8c5

    // ---- lazy loading ---------------------------------------------------------------------
    // Everything of `data` loaded? (name inferred)
    bool IsLoaded(const LazyLoadData& data);                            // Mac 0x5fe90, Win 0x60c198
    // Loads everything of `data` now; true when all of it exists. Calls the callback.
    bool Load(const LazyLoadData& data);                                // Mac 0x60460, Win 0x6114bc
    // Queues `data` (calls back at once and returns 0 when it is already loaded); returns the
    // ticket for CancelLazyLoad. Registers with the main updater while loads are pending.
    UInt32 RequestLazyLoad(const LazyLoadData& data);                   // Mac 0x5fb50
    void   CancelLazyLoad(UInt32 ticket);                               // Mac 0x602c0

    // UpdateObserver: the pending loads (see the NOTE in the .cpp).
    virtual void Update();                                              // Mac 0x60880 (thunk 0x60a30), Win 0x60d9d6

    // ---- accessors (inline; used by BrickDraw, LDDPart, the controls) ----------------------
    BrickDatabase* GetDatabase() const          { return m_database; }
    BrickConnect*  GetBrickConnect() const      { return m_brickConnect; }
    Render*        GetRender() const            { return m_render; }
    Mesh*          GetAxesMesh() const          { return m_axesMesh; }
    Mesh*          GetBoxMesh() const           { return m_boxMesh; }
    Mesh*          GetSelectionBoxMesh() const  { return m_selectionBoxMesh; }
    Material*      GetSilhouetteMaterial() const { return m_silhouetteMaterial; }
    Material*      GetDefaultMaterial() const   { return m_defaultMaterial; }
    Material*      GetOutlineMaterial() const   { return m_outlineMaterial; }
    bool           UsesShaderMaterials(UInt32 materialSet) const { return m_useShaderMaterials[materialSet]; }
    const BasicMap<UInt32, GeometryAccum::ImportPrimitive>& GetImportPrimitives() const { return m_importPrimitives; }
    // (used by the BrickConnectSceneControl mesh rebuild, Mac 021e/021g, TU 053)
    // The import records for GeometryAccum::AddElement (which marks them while merging).
    GeometryAccum::ImportPrimitiveMap& GetImportPrimitives() { return m_importPrimitives; }
    // A new stamp for GeometryAccum::WritePatch (++m_field1dc). Mac 0x11a800 (TU 053, called
    // with the map: m_field1dc follows it), Win inlined.
    UInt32 NextImportStamp() { return ++m_field1dc; }

protected:
    // Leaves the updater, then detaches the observers.
    virtual void OnFinalRelease()                                       // Mac 0x62e50 (thunks 0x62ee0, 0x62f00), Win 0x611237
    {
        UpdateObserver::OnFinalRelease();
        Notifier<BrickConnectSceneManagerObserver>::OnFinalRelease();
    }

private:
    // (not in the original) tests set the database without Initialize (tests/test_templates_groups.cpp)
    friend struct BrickConnectSceneManagerTestAccess;

    // Creates (and caches) the vertex buffer of geometry `face`: "<id>.g" for face 0, else
    // the decoration geometry; also registered as decoration buffer for face != 0.
    VertexBuffer* CreateVertexBuffer(const Primitive* primitive, UInt32 face);              // Mac 0x5dd90
    // Same from the decoration geometry (always QueryDecorationFaceGeometry).
    VertexBuffer* CreateDecorationVertexBuffer(const Primitive* primitive, UInt32 face);    // Mac 0x5e1d0, Win 0x610d9f
    const BrickGeometry* LoadFlexGeometry(const Primitive* primitive, UInt32 face);         // Mac 0x5ec00
    // Cached decal texture, else LoadDecalTexture. (inline; Win out of line 0x61046c)
    Texture* GetDecalTexture(UInt32 decoration);                                            // Win 0x61046c
    // .g data -> VertexBuffer(::CreateArgs / CreateArgsRoundEdge) through the render's factory,
    // and the primitive's GeometryAccum::ImportGeometry record.
    VertexBuffer* BuildVertexBuffer(const BrickGeometry* geometry, const Primitive* primitive,
                                    UInt32 face);                                           // Mac 0x60a50, Win 0x610823
    Material* CreateMaterial(UInt32 material, UInt32 materialSet);      // Mac 0x5eeb0, Win 0x610bb3
    Material* GetColoredMaterial(const MaterialColor& colors);          // Mac 0x615c0
    Material* GetShaderMaterial(Shader* shader, UInt32 materialType, const MaterialColor& colors,
                                UInt32 materialSet);                    // Mac 0x61700, Win 0x60fd24
    Texture* LoadDecalTexture(UInt32 decoration);                       // Mac 0x5ed00, Win 0x60fc0f

    BrickConnectSceneManager(const BrickConnectSceneManager&);
    BrickConnectSceneManager& operator=(const BrickConnectSceneManager&);

    typedef BasicMap<UInt32, RefPointer<Material> > MaterialMap;        // Mac vtable 0x7614e8

    // ---- data (Mac offsets) -----------------------------------------------------------------
    RefPointer<Mesh>          m_axesMesh;              // +0x2c red/green/blue axes, CoordinateSystemMaterial
    RefPointer<Mesh>          m_boxMesh;               // +0x30 yellow unit box
    RefPointer<Mesh>          m_selectionBoxMesh;      // +0x34 cyan unit box
    RefPointer<BrickConnect>  m_brickConnect;          // +0x38 LDDBrickConnect
    RefPointer<BrickDatabase> m_database;              // +0x3c
    RefPointer<Render>        m_render;                // +0x40
    RefPointer<Material>      m_silhouetteMaterial;    // +0x44 SilhouetteMaterial (also the box meshes')
    RefPointer<Material>      m_defaultMaterial;       // +0x48 ColoredMaterial (white, ambient 0.1, black specular/emission, 85)
    RefPointer<Material>      m_material4c;            // +0x4c never set in this TU
    RefPointer<Material>      m_outlineMaterial;       // +0x50 OutlineMaterial
    BrickConnectSceneControl*     m_sceneControl;      // +0x54 referenced (through its VisualDebugger AddRef)
    BrickConnectSceneControlTool* m_activeTool;        // +0x58 referenced; observes m_sceneControl
    BrickConnectSceneControlTool* m_defaultTool;       // +0x5c referenced
    BasicMap<UInt32, RefPointer<Texture> > m_decalTextures;          // +0x60 decoration id -> texture
    MaterialMap               m_materials[kMaterialSetCount];        // +0x7c (5 x 0x1c) material id -> material
    // Whether a set uses shader materials: [0] UseShadersInScene, [1] UseShadersInPalette,
    // [2] = [3] = [0]. NOTE: [4] (+0x10c) is never written in either build (read by
    // CreateMaterial for set 4); the restored constructor clears it.
    bool                      m_useShaderMaterials[kMaterialSetCount]; // +0x108
    RefPointer<Directory>     m_shaderDirectory;                     // +0x110 assets "Shaders"
    RefPointer<Directory>     m_roundEdgeShaderDirectory;            // +0x114 "Shaders/RoundEdge"
    BasicMap<MaterialColor, RefPointer<Material> > m_coloredMaterials;          // +0x118
    BasicMap<UniqueShaderMaterialKey, RefPointer<Material> > m_shaderMaterials; // +0x134
    MaterialMap               m_decalMaterials;                      // +0x150 decoration id -> material
    BasicMap<GeometryKey, RefPointer<VertexBuffer> > m_vertexBuffers;           // +0x16c
    BasicMap<GeometryKey, RefPointer<VertexBuffer> > m_decorationVertexBuffers; // +0x188
    BasicMap<GeometryKey, RefPointer<const BrickGeometry> > m_flexGeometries;   // +0x1a4
    BasicMap<UInt32, GeometryAccum::ImportPrimitive> m_importPrimitives;        // +0x1c0 design id ->
    UInt32                    m_field1dc;                            // +0x1dc cleared, not used here
    BasicArray<LazyLoadData>  m_lazyLoads;                           // +0x1e0
    UInt32                    m_lastTicket;                          // +0x1f0
};

} // namespace LEGO
