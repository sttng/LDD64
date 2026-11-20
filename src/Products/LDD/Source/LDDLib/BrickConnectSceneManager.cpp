// LEGO Digital Designer — restored source
// LEGO::BrickConnectSceneManager — see BrickConnectSceneManager.h.
//
// Mac TU 022 (022a 0x5bcd0-0x60880: constructor, Initialize, scene control / tools, lookup and
// creation of vertex buffers, meshes, materials, shaders, decal textures, lazy-load requests;
// 022c 0x60880-0x626d0: Update, the .g -> VertexBuffer upload, the shader-material choice, the
// destructor body; 022b 0x626d0-0x683bb: destructor variants, OnFinalRelease, the inline
// members of the Material family (restored in Render/Materials.cpp), Notifier / BasicMap /
// BasicArray template instances, the TU initialiser). Win 0x609720-0x6123fb (mixed).
#include "BrickConnectSceneManager.h"
#include "LDDApplication.h"
#include "LDDBrickConnect.h"
#include "Application.h"
#include "RenderControlManager.h"
#include "Platform.h"
#include "Preferences.h"
#include "TextLocale.h"
#include "Directory.h"
#include "Path.h"
#include "ReadFile.h"
#include "LEGO/String.h"
#include "BasicString.h"
#include "SystemTime.h"
#include "ObjectFactory.h"
#include "BrickDatabase.h"
#include "BrickGeometry.h"
#include "BrickPhysics.h"
#include "Primitive.h"
#include "SceneGraphUtil.h"
#include "Render.h"
#include "Material.h"
#include "Materials.h"
#include "Mesh.h"
#include "Shader.h"
#include "Texture.h"
#include "VertexBuffer.h"
#include "RoundEdgeDataTextureCreator.h"
#include <string.h>
#include "BrickConnectSceneControl.h"
#include "BrickConnectSceneControlTool.h"

namespace LEGO {

namespace {

// MaterialManual's MaterialColor and the render's MaterialColors have the same 0x44-byte
// layout (the original most likely uses one struct; both names are inferred / RTTI).
MaterialColors ToMaterialColors(const MaterialColor& c)
{
    MaterialColors m;
    m.diffuse = c.diffuse;
    m.ambient = c.ambient;
    m.specular = c.specular;
    m.emissive = c.emission;
    m.shininess = c.shininess;
    return m;
}

// "<face count of the primitive>": the decoration faces, at least the main geometry.
UInt32 GetGeometryFaceCount(const Primitive* primitive)
{
    UInt32 count = primitive->GetDecorationFaceCount();
    return count == 0 ? 1 : count;
}

// The callback of a finished load (inlined on Mac; Win 0x609a3a).
void NotifyLoaded(const BrickConnectSceneManager::LazyLoadData& data)
{
    BrickConnectSceneManager::LazyLoadCallback* callback = data.callback;
    if (callback && !callback->OnLoaded(data))
        callback->OnNotHandled(data);
}

} // namespace

// ===========================================================================================
// LazyLoadData
// ===========================================================================================

// Mac 0x5f770
void BrickConnectSceneManager::LazyLoadData::AddBrick(UInt32 designID)
{
    SceneGraph::PushBackUnique(brickIDs, designID);                     // Mac 0x5f7a0
}

// Mac 0x5fab0
void BrickConnectSceneManager::LazyLoadData::AddMaterials(const BasicArray<UInt32>& ids)
{
    for (BasicArray<UInt32>::ConstIterator it = ids.Begin(); it != ids.End(); ++it)
        if (*it != 0)
            SceneGraph::PushBackUnique(materials, *it);                 // Mac 0x5f7a0
}

// Mac 0x5fb00
void BrickConnectSceneManager::LazyLoadData::AddDecorations(const BasicArray<UInt32>& ids)
{
    for (BasicArray<UInt32>::ConstIterator it = ids.Begin(); it != ids.End(); ++it)
        if (*it != 0)
            SceneGraph::PushBackUnique(decorations, *it);               // Mac 0x5f7a0
}

// ===========================================================================================
// Construction
// ===========================================================================================

// Mac 0x5be80, Win 0x610f17
BrickConnectSceneManager::BrickConnectSceneManager()
    : m_sceneControl(0), m_activeTool(0), m_defaultTool(0), m_field1dc(0), m_lastTicket(0)
{
    m_silhouetteMaterial = new SilhouetteMaterial;                      // Win ctor 0x609720
    m_outlineMaterial = new OutlineMaterial;                            // Win ctor 0x609938
    // NOTE: the original leaves the shader switches to Initialize and never writes [4].
    for (UInt32 i = 0; i < kMaterialSetCount; ++i)
        m_useShaderMaterials[i] = false;

    MaterialColors colors;
    colors.ambient = Color(0.1f, 0.1f, 0.1f, 1.0f);
    colors.specular = Color::Black();
    colors.emissive = Color::Black();
    colors.shininess = 85.0f;
    m_defaultMaterial = new ColoredMaterial(colors);
}

// Mac 0x626d0 / 0x62810 (+ thunks 0x62730, 0x627a0, 0x62880, 0x62900; body 0x61c90),
// Win body 0x6115d7 (ECX = the Object sub-object), complete 0x6117e0, sdd 0x6117f6 (thunk 0x61122f)
BrickConnectSceneManager::~BrickConnectSceneManager()
{
    if (m_activeTool)
        m_activeTool->Observer<BrickConnectSceneControl, BrickConnectSceneControlObserver>::Detach();
    if (m_defaultTool)
        m_defaultTool->Release();
    if (m_activeTool)
        m_activeTool->Release();
    if (m_sceneControl)
        m_sceneControl->Release();
    // The members are destroyed in reverse order (lazy loads … meshes).
}

// Mac 0x5c900, Win 0x60ae2d
bool BrickConnectSceneManager::Initialize(RenderControlManager* manager)
{
    String brickDBName;
    Preferences::GetInstance()->GetValue(String("userBrickDBDirName"), brickDBName);

    RefPointer<Directory> assets =
        Platform::GetInstance()->GetSystemDirectory(Platform::kAssetsDirectory, true);
    if (assets.IsNull())
        return false;
    m_shaderDirectory = assets->GetDirectory(String("Shaders"), false);
    if (m_shaderDirectory.IsNull())
        return false;
    m_roundEdgeShaderDirectory = m_shaderDirectory->GetDirectory(String("RoundEdge"), false);
    if (m_roundEdgeShaderDirectory.IsNull())
        return false;

    RefPointer<Directory> userDirectory = Application::GetInstance()->GetUserDataDirectory();
    if (userDirectory.IsNull())
        return false;
    RefPointer<Directory> databaseDirectory = userDirectory->GetDirectory(brickDBName, false);
    if (databaseDirectory.IsNull())
        return false;

    bool loadAssemblies = true;
    Preferences::GetInstance()->GetValue(String("LoadAssemblies"), loadAssemblies);
    m_database = new BrickDatabase(databaseDirectory);
    if (!m_database->Load(false, loadAssemblies))
        return false;

    RefPointer<Directory> materialNames = databaseDirectory->GetDirectory(String("MaterialNames"), false);
    if (!materialNames.IsNull())
        TextLocale::GetInstance()->AddStrings(materialNames);

    // The physics element settings.
    AString settings;
    ReadFile(Path(String("pes"), databaseDirectory), settings);
    BrickPhysics* physics = CreateBrickPhysics(settings.GetBuffer(), settings.GetLength());
    if (physics == 0)
        return false;
    m_brickConnect = new LDDBrickConnect(physics, m_database, this);   // Win ctor 0x60969d
    physics->Release();

    m_useShaderMaterials[kMaterialSetScene] = LDDApplication::GetInstance()->UseShadersInScene();
    m_useShaderMaterials[kMaterialSetPalette] = LDDApplication::GetInstance()->UseShadersInPalette();
    m_useShaderMaterials[kMaterialSetBackground] = m_useShaderMaterials[kMaterialSetScene];
    m_useShaderMaterials[kMaterialSet3] = m_useShaderMaterials[kMaterialSetScene];

    m_render = manager->GetRender();
    Render* render = m_render;
    m_axesMesh = CreateAxesMesh(render, Color::Red(), Color::Green(), Color::Blue(),
                                new CoordinateSystemMaterial, false);   // Win ctor 0x60984a
    m_boxMesh = CreateBoxMesh(render, Color::Yellow(), m_silhouetteMaterial, false);
    m_selectionBoxMesh = CreateBoxMesh(render, Color::Cyan(), m_silhouetteMaterial, false);
    return !m_axesMesh.IsNull() && !m_boxMesh.IsNull() && !m_selectionBoxMesh.IsNull();
}

// ===========================================================================================
// Scene control and tools
// ===========================================================================================

// Mac 0x5d370
void BrickConnectSceneManager::SetSceneControl(BrickConnectSceneControl* control)
{
    BrickConnectSceneControl* previous = m_sceneControl;
    if (previous == control)
        return;
    m_sceneControl = control;
    control->AddRef();
    if (previous)
        previous->Release();
    if (m_activeTool) {
        m_activeTool->Observer<BrickConnectSceneControl, BrickConnectSceneControlObserver>::Detach();
        m_activeTool->Observer<BrickConnectSceneControl, BrickConnectSceneControlObserver>::Attach(m_sceneControl);
    }
    NotifierT<BrickConnectSceneManager, BrickConnectSceneManagerObserver>::Iterator it(*this);
    while (BrickConnectSceneManagerObserver* observer = it.Next(m_observers))
        observer->OnSceneControlChanged(previous, m_sceneControl);
}

// Mac 0x5d920
void BrickConnectSceneManager::SetActiveTool(BrickConnectSceneControlTool* tool)
{
    BrickConnectSceneControlTool* previous = m_activeTool;
    if (previous == tool)
        return;
    if (previous)
        previous->Observer<BrickConnectSceneControl, BrickConnectSceneControlObserver>::Detach();
    if (m_activeTool != tool) {
        BrickConnectSceneControlTool* old = m_activeTool;
        m_activeTool = tool;
        if (tool)
            tool->AddRef();
        if (old)
            old->Release();
    }
    if (m_activeTool == 0 && m_defaultTool) {
        m_activeTool = m_defaultTool;          // NOTE: plain store + AddRef (no release)
        m_activeTool->AddRef();
    }
    if (m_activeTool && m_sceneControl)
        m_activeTool->Observer<BrickConnectSceneControl, BrickConnectSceneControlObserver>::Attach(m_sceneControl);
    NotifierT<BrickConnectSceneManager, BrickConnectSceneManagerObserver>::Iterator it(*this);
    while (BrickConnectSceneManagerObserver* observer = it.Next(m_observers))
        observer->OnActiveToolChanged(previous, m_activeTool);
}

// Mac 0x5dc80
void BrickConnectSceneManager::SetDefaultTool(BrickConnectSceneControlTool* tool)
{
    if (m_defaultTool != tool) {
        BrickConnectSceneControlTool* old = m_defaultTool;
        m_defaultTool = tool;
        if (tool)
            tool->AddRef();
        if (old)
            old->Release();
    }
    if (m_activeTool == 0 && m_defaultTool)
        SetActiveTool(m_defaultTool);
}


// ===========================================================================================
// Vertex buffers, meshes, flex geometry
// ===========================================================================================

// Mac 0x5dcf0, Win 0x611259
VertexBuffer* BrickConnectSceneManager::GetVertexBuffer(const Primitive* primitive, UInt32 face)
{
    if (primitive->IsFlexible())
        return 0;
    BasicMap<GeometryKey, RefPointer<VertexBuffer> >::Iterator it =
        m_vertexBuffers.Find(GeometryKey(primitive->GetDesignID(), face));
    if (it != m_vertexBuffers.End())
        return it->second;
    return CreateVertexBuffer(primitive, face);
}

// Mac 0x5dd90 (Win: geometry query helper 0x6095ab)
VertexBuffer* BrickConnectSceneManager::CreateVertexBuffer(const Primitive* primitive, UInt32 face)
{
    GeometryKey key(primitive->GetDesignID(), face);
    RefPointer<BrickGeometry> geometry = face == 0
        ? primitive->GetDatabase()->QueryMaterialGeometry(primitive, 0)
        : primitive->GetDatabase()->QueryDecorationFaceGeometry(primitive, face, 0);
    if (geometry.IsNull())
        return 0;
    VertexBuffer* buffer = BuildVertexBuffer(geometry, primitive, face);
    if (buffer == 0)
        return 0;
    m_vertexBuffers[key] = buffer;
    if (face != 0)
        m_decorationVertexBuffers[key] = buffer;
    return buffer;
}

// Mac 0x5df10, Win 0x6112da
Mesh* BrickConnectSceneManager::CreateMesh(const Primitive* primitive, UInt32 face, UInt32 material,
                                           UInt32 materialSet)
{
    if (primitive->IsFlexible())
        return 0;
    VertexBuffer* found;
    BasicMap<GeometryKey, RefPointer<VertexBuffer> >::Iterator it =
        m_vertexBuffers.Find(GeometryKey(primitive->GetDesignID(), face));
    if (it != m_vertexBuffers.End())
        found = it->second;
    else
        found = CreateVertexBuffer(primitive, face);
    if (found == 0)
        return 0;
    RefPointer<VertexBuffer> buffer = found;

    // NOTE: the colour is looked up and not used (both builds).
    MaterialColor color;
    m_database->GetMaterialColor(material, color);

    Material* found2;
    MaterialMap::Iterator mit = m_materials[materialSet].Find(material);
    if (mit != m_materials[materialSet].End())
        found2 = mit->second;
    else
        found2 = CreateMaterial(material, materialSet);
    if (found2 == 0)
        return 0;
    RefPointer<Material> hold = found2;
    return new SingleMesh(found2, buffer);
}

// Mac 0x5e1d0, Win 0x610d9f
VertexBuffer* BrickConnectSceneManager::CreateDecorationVertexBuffer(const Primitive* primitive,
                                                                     UInt32 face)
{
    GeometryKey key(primitive->GetDesignID(), face);
    RefPointer<BrickGeometry> geometry =
        primitive->GetDatabase()->QueryDecorationFaceGeometry(primitive, face, 0);
    if (geometry.IsNull())
        return 0;
    VertexBuffer* buffer = BuildVertexBuffer(geometry, primitive, face);
    if (buffer == 0)
        return 0;
    m_decorationVertexBuffers[key] = buffer;
    if (face != 0)
        m_vertexBuffers[key] = buffer;
    return buffer;
}

// inlined in Mac 0x5e330, Win 0x6113a3
VertexBuffer* BrickConnectSceneManager::GetDecorationVertexBuffer(const Primitive* primitive, UInt32 face)
{
    BasicMap<GeometryKey, RefPointer<VertexBuffer> >::Iterator it =
        m_decorationVertexBuffers.Find(GeometryKey(primitive->GetDesignID(), face));
    if (it != m_decorationVertexBuffers.End())
        return it->second;
    return CreateDecorationVertexBuffer(primitive, face);
}

// Mac 0x5e330, Win 0x61140d
Mesh* BrickConnectSceneManager::CreateDecorationMesh(const Primitive* primitive, UInt32 face,
                                                     UInt32 decoration)
{
    VertexBuffer* found = GetDecorationVertexBuffer(primitive, face);
    if (found == 0)
        return 0;
    RefPointer<VertexBuffer> buffer = found;
    Material* material = GetDecalMaterial(decoration);
    if (material == 0)
        return 0;
    RefPointer<Material> hold = material;
    return new SingleMesh(material, buffer);
}

// Mac 0x5eb70, Win 0x6103eb
const BrickGeometry* BrickConnectSceneManager::GetFlexGeometry(const Primitive* primitive, UInt32 face)
{
    if (!primitive->IsFlexible())
        return 0;
    BasicMap<GeometryKey, RefPointer<const BrickGeometry> >::Iterator it =
        m_flexGeometries.Find(GeometryKey(primitive->GetDesignID(), face));
    if (it != m_flexGeometries.End())
        return it->second;
    return LoadFlexGeometry(primitive, face);
}

// Mac 0x5ec00
const BrickGeometry* BrickConnectSceneManager::LoadFlexGeometry(const Primitive* primitive, UInt32 face)
{
    GeometryKey key(primitive->GetDesignID(), face);
    RefPointer<BrickGeometry> geometry = face == 0
        ? primitive->GetDatabase()->QueryMaterialGeometry(primitive, 0)
        : primitive->GetDatabase()->QueryDecorationFaceGeometry(primitive, face, 0);
    if (geometry.IsNull())
        return 0;
    RefPointer<const BrickGeometry>& slot = m_flexGeometries[key];
    slot = geometry.Get();
    return slot;
}

// Mac 0x60a50, Win 0x610823
VertexBuffer* BrickConnectSceneManager::BuildVertexBuffer(const BrickGeometry* geometry,
                                                          const Primitive* primitive, UInt32 face)
{
    BasicArray<UInt32> texCoordDimensions;
    if (geometry->GetTextureCoordinates())
        texCoordDimensions.PushBack(2);

    const bool roundEdges = LDDApplication::GetInstance()->UseRoundEdges();
    VertexBuffer* buffer;
    if (!roundEdges) {
        VertexBuffer::CreateArgs args;
        args.vertexCount = geometry->GetVertexCount();
        args.indexCount = geometry->GetIndexCount();
        args.frameCount = 0;
        args.dynamic = false;
        args.hasNormals = true;
        args.colorSetCount = 0;
        args.texCoordDimensions = texCoordDimensions;
        Object* object = m_render->GetObjectFactory()->CreateObject(typeid(VertexBuffer), args);
        buffer = object ? dynamic_cast<VertexBuffer*>(object) : 0;
    } else {
        VertexBuffer::CreateArgsRoundEdge args;
        args.vertexCount = geometry->GetVertexCount();
        args.indexCount = geometry->GetIndexCount();
        args.frameCount = 0;
        args.dynamic = false;
        args.hasNormals = true;
        args.colorSetCount = 0;
        args.texCoordDimensions = texCoordDimensions;
        args.roundEdgeDataCount = geometry->GetRoundEdgeDataCount();
        args.roundEdgePointCount = geometry->GetAverageNormalCount();
        Object* object = m_render->GetObjectFactory()->CreateObject(typeid(VertexBuffer), args);
        buffer = object ? dynamic_cast<VertexBuffer*>(object) : 0;
    }
    if (buffer == 0)
        return 0;

    buffer->SetPositions(geometry->GetPositions(), 0, geometry->GetVertexCount(), 0);
    buffer->SetNormals(geometry->GetNormals(), 0, geometry->GetVertexCount());
    if (LDDApplication::GetInstance()->UseRoundEdges()) {
        buffer->SetRoundEdgePoints(geometry->GetAverageNormals(), geometry->GetAverageNormalCount());
        buffer->SetRoundEdgeData((const UInt32*)geometry->GetRoundEdgeData(),
                                 geometry->GetRoundEdgeDataCount());
        buffer->SetRoundEdgePointIndices(geometry->GetAverageNormalIndices(), geometry->GetIndexCount());
        buffer->SetRoundEdgeDataIndices(geometry->GetRoundEdgeIndices(), geometry->GetIndexCount());
    }
    if (geometry->GetTextureCoordinates())
        buffer->SetTexCoords((const Real32*)geometry->GetTextureCoordinates(), 0);
    buffer->SetIndices(geometry->GetIndices(), 0, geometry->GetIndexCount(), 0);

    // The CPU copies of the buffer for the round-edge mesh rebuild (GeometryAccum).
    UInt32 designID = primitive->GetDesignID();
    BasicMap<UInt32, GeometryAccum::ImportPrimitive>::Iterator it = m_importPrimitives.Find(designID);
    GeometryAccum::ImportPrimitive* import;
    if (it != m_importPrimitives.End()) {
        import = &it->second;
    } else {
        import = &m_importPrimitives[designID];                         // Mac 0x661f0
        import->primitive = primitive;
        import->geometries.Clear();                                     // Mac 0x63ee0 (assign of nothing)
        import->geometries.Resize(GetGeometryFaceCount(primitive), GeometryAccum::ImportGeometry());
    }
    GeometryAccum::ImportGeometry& record = import->geometries[face];
    if (record.positions == 0) {
        UInt32 stride;
        record.positions = buffer->GetPositions(stride, 0);
        record.normals = buffer->GetNormals(stride);
        if (LDDApplication::GetInstance()->UseRoundEdges()) {
            record.roundEdgePoints = buffer->GetRoundEdgePoints(stride);
            record.roundEdgeData = buffer->GetRoundEdgeData();
            record.roundEdgePointCount = buffer->GetRoundEdgePointCount();
            record.roundEdgeDataCount = buffer->GetRoundEdgeDataCount();
            record.roundEdgeDataSize = AlignRoundEdgeDataSize(record.roundEdgeDataCount);
            record.roundEdgePointIndices = buffer->GetRoundEdgePointIndices();
            record.roundEdgeDataIndices = buffer->GetRoundEdgeDataIndices();
        }
        if (geometry->GetTextureCoordinates())
            record.texCoords = buffer->GetTexCoords(stride, 0);
        record.vertexCount = buffer->GetVertexCount();
        record.indices = buffer->GetIndices();
        record.indexCount = buffer->GetIndexCount();
        UInt32 cullingSize = geometry->GetCullingDataSize();
        if (cullingSize != 0) {
            record.cullingData = new UInt8[cullingSize];
            record.cullingCount = geometry->GetCullingCount();
            record.ownsCullingData = true;
            memcpy(record.cullingData, geometry->GetCullingData(), cullingSize);
        }
    }
    return buffer;
}

// ===========================================================================================
// Materials
// ===========================================================================================

// Mac 0x5e160, Win 0x610c93
Material* BrickConnectSceneManager::GetMaterial(UInt32 material, UInt32 materialSet)
{
    MaterialMap::Iterator it = m_materials[materialSet].Find(material);
    if (it != m_materials[materialSet].End())
        return it->second;
    return CreateMaterial(material, materialSet);
}

// Mac 0x5eeb0, Win 0x610bb3
Material* BrickConnectSceneManager::CreateMaterial(UInt32 material, UInt32 materialSet)
{
    MaterialColor color;
    if (!m_database->GetMaterialColor(material, color))
        return 0;
    Material* result = 0;
    if (m_useShaderMaterials[materialSet]) {
        MaterialManual::MaterialType type;
        if (!m_database->GetMaterialType(material, type))
            return 0;
        result = CreateShaderMaterial(type, color, materialSet);
    }
    if (result == 0) {
        result = GetColoredMaterial(color);
        if (result == 0)
            return 0;
    }
    m_materials[materialSet][material] = result;
    return result;
}

// Mac 0x615c0
Material* BrickConnectSceneManager::GetColoredMaterial(const MaterialColor& colors)
{
    BasicMap<MaterialColor, RefPointer<Material> >::Iterator it = m_coloredMaterials.Find(colors);
    if (it != m_coloredMaterials.End())
        return it->second;
    Material* material = new ColoredMaterial(ToMaterialColors(colors));
    m_coloredMaterials[colors] = material;                              // Mac 0x676d0
    return material;
}

// Mac 0x614b0, Win 0x610762
Material* BrickConnectSceneManager::CreateShaderMaterial(UInt32 materialType,
                                                         const MaterialColor& colors,
                                                         UInt32 materialSet)
{
    String name;
    if (materialType == MaterialManual::kShinySteel)
        name = "ShinySteel";
    else if (materialType == MaterialManual::kShinyPlastic)
        name = "ShinyPlastic";
    Shader* found = LoadShader(name, materialSet);
    if (found == 0)
        return 0;
    RefPointer<Shader> shader = found;
    return GetShaderMaterial(shader, materialType, colors, materialSet);
}

// Mac 0x61700, Win 0x60fd24
Material* BrickConnectSceneManager::GetShaderMaterial(Shader* shader, UInt32 materialType,
                                                      const MaterialColor& colors,
                                                      UInt32 materialSet)
{
    UniqueShaderMaterialKey key;
    key.materialType = materialType;
    key.colors = colors;
    key.materialSet = materialSet;
    BasicMap<UniqueShaderMaterialKey, RefPointer<Material> >::Iterator it = m_shaderMaterials.Find(key);
    if (it != m_shaderMaterials.End())
        return it->second;

    MaterialColors materialColors = ToMaterialColors(colors);
    Material* material;
    if (!LDDApplication::GetInstance()->UseRoundEdges()) {
        if (materialSet == kMaterialSetColorChange)
            material = new ShadedColorChangeMaterial(m_render, shader, materialColors);
        else
            material = new ShadedMaterial(m_render, shader, materialColors);   // Mac 0x61b30 (C1)
    } else {
        RefPointer<Directory> assets =
            Platform::GetInstance()->GetSystemDirectory(Platform::kAssetsDirectory, true);
        RefPointer<Directory> shaders = assets->GetDirectory(String("Shaders"), false);
        if (materialSet == kMaterialSetColorChange)
            material = new ShadedRoundEdgeColorChangeMaterial(m_render, shader, materialColors, shaders);
        else
            material = new ShadedRoundEdgeMaterial(m_render, shader, materialColors, shaders);
    }
    m_shaderMaterials[key] = material;                                  // Mac 0x67c40
    return material;
}

// Mac 0x5f290, Win 0x60a8c5
Shader* BrickConnectSceneManager::LoadShader(const String& name, UInt32 variant)
{
    Shader::LoadArgs args;
    if (!LDDApplication::GetInstance()->UseRoundEdges()) {
        String file(name);
        file << ".sh";
        args.path = Path(file, m_shaderDirectory);
    } else {
        String file;
        if (variant == kMaterialSetBackground)
            file = "REBG";
        else
            file = "RE";
        file << name << ".sh";
        args.path = Path(file, m_roundEdgeShaderDirectory);
    }
    args.field1C = 0;
    Object* object = m_render->GetObjectFactory()->CreateObject(typeid(Shader), args);
    return object ? dynamic_cast<Shader*>(object) : 0;
}

// Mac 0x5ed00, Win 0x60fc0f
Texture* BrickConnectSceneManager::LoadDecalTexture(UInt32 decoration)
{
    Texture::LoadArgs2D args;
    if (!m_database->GetDecorationPath(decoration, args.path))
        return 0;
    args.create.wrapMode = Texture::kWrapClampToEdge;
    args.create.alphaMode = Texture::kAlphaImage;
    Object* object = m_render->GetObjectFactory()->CreateObject(typeid(Texture), args);
    Texture* texture = object ? dynamic_cast<Texture*>(object) : 0;
    if (texture)
        m_decalTextures[decoration] = texture;                          // Mac 0x67240
    return texture;
}

// inlined in Mac 0x5e4b0 / 0x60460, Win 0x61046c
Texture* BrickConnectSceneManager::GetDecalTexture(UInt32 decoration)
{
    BasicMap<UInt32, RefPointer<Texture> >::Iterator it = m_decalTextures.Find(decoration);
    if (it != m_decalTextures.End())
        return it->second;
    return LoadDecalTexture(decoration);
}

// Mac 0x5e4b0, Win 0x6104c1
Material* BrickConnectSceneManager::GetDecalMaterial(UInt32 decoration)
{
    MaterialMap::Iterator it = m_decalMaterials.Find(decoration);
    if (it != m_decalMaterials.End())
        return it->second;

    Texture* found = GetDecalTexture(decoration);
    if (found == 0)
        return 0;
    RefPointer<Texture> texture = found;

    MaterialColors colors;
    colors.diffuse.r = 1.0f;
    colors.diffuse.g = 1.0f;
    colors.diffuse.b = 1.0f;
    colors.ambient = Color(0.1f, 0.1f, 0.1f, 1.0f);
    colors.specular = Color(1.0f, 1.0f, 1.0f, 1.0f);
    colors.emissive = Color(0.0f, 0.0f, 0.0f, 0.0f);
    colors.shininess = 50.0f;

    Material* material = 0;
    if (LDDApplication::GetInstance()->UseRoundEdges()) {
        Shader* shader = LoadShader(String("Decal"), 0);
        if (shader == 0)
            return 0;
        material = new ShadedRoundEdgeDecalMaterial(m_render, shader, colors, texture);
        shader->SetMaterialColors(colors);
    } else if (LDDApplication::GetInstance()->UseShadersInScene()) {
        Shader* shader = LoadShader(String("Decal"), 0);
        if (shader == 0) {
            material = new DecalMaterial(colors, texture);
        } else {
            material = new ShadedDecalMaterial(m_render, shader, colors, texture);
            shader->SetMaterialColors(colors);
        }
    } else {
        material = new DecalMaterial(colors, texture);
    }
    m_decalMaterials[decoration] = material;                            // Mac 0x64b40
    return material;
}

// Mac 0x5efe0, Win 0x60b38c
Material* BrickConnectSceneManager::CreateWireframeMaterial()
{
    MaterialColors colors;
    colors.diffuse = Color::Black();
    colors.ambient = Color::Black();
    colors.specular = Color::Black();
    colors.emissive = Color::Black();
    colors.shininess = 0.0f;
    if (!LDDApplication::GetInstance()->UseRoundEdges())
        return new WireframeMaterial(colors);                           // Win 0x60944b/0x609500
    Shader* shader = LoadShader(String("Wireframe"), 0);
    if (shader == 0)
        return 0;
    return new ShadedRoundEdgeWireframeMaterial(m_render, shader, colors);
}

// ===========================================================================================
// Lazy loading
// ===========================================================================================

// Mac 0x5fe90, Win 0x60c198
bool BrickConnectSceneManager::IsLoaded(const LazyLoadData& data)
{
    for (BasicArray<UInt32>::ConstIterator id = data.brickIDs.Begin(); id != data.brickIDs.End(); ++id) {
        const BrickDatabase::Brick* brick = m_database->GetBrick(*id);
        if (brick == 0)
            continue;
        for (BasicArray<BrickDatabase::Part>::ConstIterator part = brick->m_parts.Begin();
             part != brick->m_parts.End(); ++part) {
            const Primitive* primitive = m_database->GetPrimitive(part->m_designID);
            UInt32 designID = primitive->GetDesignID();
            UInt32 faces = GetGeometryFaceCount(primitive);
            for (UInt32 face = 0; face < faces; ++face) {
                bool present = primitive->IsFlexible()
                    ? m_flexGeometries.Contains(GeometryKey(designID, face))
                    : m_vertexBuffers.Contains(GeometryKey(designID, face));
                if (!present)
                    return false;
            }
        }
    }
    for (BasicArray<UInt32>::ConstIterator it = data.materials.Begin(); it != data.materials.End(); ++it)
        if (!m_materials[data.materialSet].Contains(*it))
            return false;
    for (BasicArray<UInt32>::ConstIterator it = data.decorations.Begin(); it != data.decorations.End(); ++it)
        if (!m_decalTextures.Contains(*it))
            return false;
    return true;
}

// Mac 0x60460, Win 0x6114bc
// NOTE: an unknown part design id (GetPrimitive null) is dereferenced (both builds).
bool BrickConnectSceneManager::Load(const LazyLoadData& data)
{
    for (BasicArray<UInt32>::ConstIterator id = data.brickIDs.Begin(); id != data.brickIDs.End(); ++id) {
        const BrickDatabase::Brick* brick = m_database->GetBrick(*id);
        if (brick == 0)
            continue;
        for (BasicArray<BrickDatabase::Part>::ConstIterator part = brick->m_parts.Begin();
             part != brick->m_parts.End(); ++part) {
            const Primitive* primitive = m_database->GetPrimitive(part->m_designID);
            UInt32 faces = GetGeometryFaceCount(primitive);
            for (UInt32 face = 0; face < faces; ++face) {
                GeometryKey key(primitive->GetDesignID(), face);
                if (!primitive->IsFlexible()) {
                    BasicMap<GeometryKey, RefPointer<VertexBuffer> >::Iterator it = m_vertexBuffers.Find(key);
                    VertexBuffer* buffer = it != m_vertexBuffers.End() ? it->second.Get()
                                                                       : CreateVertexBuffer(primitive, face);
                    if (buffer == 0)
                        return false;
                } else {
                    BasicMap<GeometryKey, RefPointer<const BrickGeometry> >::Iterator it = m_flexGeometries.Find(key);
                    const BrickGeometry* geometry = it != m_flexGeometries.End() ? it->second.Get()
                                                                                 : LoadFlexGeometry(primitive, face);
                    if (geometry == 0)
                        return false;
                }
            }
        }
    }
    for (BasicArray<UInt32>::ConstIterator it = data.materials.Begin(); it != data.materials.End(); ++it) {
        MaterialMap::Iterator mit = m_materials[data.materialSet].Find(*it);
        Material* material = mit != m_materials[data.materialSet].End() ? mit->second.Get()
                                                                        : CreateMaterial(*it, data.materialSet);
        if (material == 0)
            return false;
    }
    for (BasicArray<UInt32>::ConstIterator it = data.decorations.Begin(); it != data.decorations.End(); ++it) {
        if (GetDecalTexture(*it) == 0)
            return false;
    }
    if (data.callback && !data.callback->OnLoaded(data)) {
        data.callback->OnNotHandled(data);
        return false;
    }
    return true;
}

// Mac 0x5fb50
UInt32 BrickConnectSceneManager::RequestLazyLoad(const LazyLoadData& data)
{
    if (IsLoaded(data)) {
        NotifyLoaded(data);
        return 0;
    }
    if (UpdateObserver::GetSubject() == 0) {
        RefPointer<Updater> updater = MainUpdater::GetInstance();
        UpdateObserver::Attach(updater);
    }
    m_lazyLoads.PushBack(data);                                         // Mac 0x60180
    ++m_lastTicket;
    m_lazyLoads.Back().ticket = m_lastTicket;
    return m_lastTicket;
}

// Mac 0x602c0
void BrickConnectSceneManager::CancelLazyLoad(UInt32 ticket)
{
    for (UInt32 i = 0; i < m_lazyLoads.GetSize(); ++i) {
        if (m_lazyLoads[i].ticket != ticket)
            continue;
        m_lazyLoads.Erase(i);                                           // Mac 0x60350
        if (m_lazyLoads.IsEmpty())
            UpdateObserver::Detach();
        return;
    }
}

// Mac 0x60880 (thunk 0x60a30), Win 0x60d9d6
// NOTE: for up to 3 ms the newest requests are completed (callback, removal) WITHOUT being
// loaded (both builds); then every request whose data happens to be loaded is completed. The
// updater is left when nothing is pending.
void BrickConnectSceneManager::Update()
{
    Int64 deadline = GetMilliseconds() + 3;
    while (GetMilliseconds() < deadline) {
        if (m_lazyLoads.IsEmpty())
            break;
        NotifyLoaded(m_lazyLoads.Back());
        m_lazyLoads.PopBack();
    }
    UInt32 i = 0;
    while (i < m_lazyLoads.GetSize()) {
        if (!IsLoaded(m_lazyLoads[i])) {
            ++i;
            continue;
        }
        NotifyLoaded(m_lazyLoads[i]);
        m_lazyLoads.Erase(i);
    }
    if (m_lazyLoads.IsEmpty())
        UpdateObserver::Detach();
}

} // namespace LEGO
