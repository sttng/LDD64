// LEGO Digital Designer — restored source
// Skybox.cpp (Mac TU 418, Win 0x5bda90-0x5bfa40). See Skybox.h.
#include "Skybox.h"
#include "ObjectFactory.h"
#include "Materials.h"
#include "RenderQueue.h"
#include "VertexBuffer.h"

#include <typeinfo>

namespace LEGO {

namespace {

const UInt32 kSkyboxQueue = 0x736b7962;   // 'skyb' (Render::GetRenderQueue key)
const UInt32 kFaceCount = 6;

// The face vertices (Mac 0x396be7-0x397133): four corners of each side of the unit cube.
const Real32 kFaceCorners[kFaceCount][4][3] = {
    { { 1.0f, -1.0f, -1.0f}, { 1.0f,  1.0f, -1.0f}, {-1.0f,  1.0f, -1.0f}, {-1.0f, -1.0f, -1.0f} },  // 1: z = -1
    { {-1.0f, -1.0f,  1.0f}, {-1.0f,  1.0f,  1.0f}, { 1.0f,  1.0f,  1.0f}, { 1.0f, -1.0f,  1.0f} },  // 2: z = +1
    { {-1.0f, -1.0f, -1.0f}, {-1.0f,  1.0f, -1.0f}, {-1.0f,  1.0f,  1.0f}, {-1.0f, -1.0f,  1.0f} },  // 3: x = -1
    { { 1.0f, -1.0f,  1.0f}, { 1.0f,  1.0f,  1.0f}, { 1.0f,  1.0f, -1.0f}, { 1.0f, -1.0f, -1.0f} },  // 4: x = +1
    { { 1.0f,  1.0f, -1.0f}, { 1.0f,  1.0f,  1.0f}, {-1.0f,  1.0f,  1.0f}, {-1.0f,  1.0f, -1.0f} },  // 5: y = +1
    { { 1.0f, -1.0f,  1.0f}, { 1.0f, -1.0f, -1.0f}, {-1.0f, -1.0f, -1.0f}, {-1.0f, -1.0f,  1.0f} },  // 6: y = -1
};

} // namespace

// Mac 0x396080, Win 0x5bda90
Skybox::Skybox(Render* render, Directory* directory)
    : m_render(render), m_directory(directory), m_current(0)
{
    m_colors.shininess = 0.0f;   // the four colours: Color() (white)
}

// Mac 0x396470, Win 0x5bdd00
bool Skybox::Initialize()
{
    // NOTE: the original enumerates m_directory without a check, so a missing Graphics/Skyboxes
    // folder (ViewMode::Enter passes the null result of GetDirectory) crashes on entering View
    // mode; fixed: no folder, no skybox (F2).
    if (!m_directory)
        return false;
    DescriptorSet entries;
    m_directory->Enumerate(String("*"), entries);
    for (DescriptorSet::Iterator it = entries.Begin(); it != entries.End(); ++it)
        m_names.PushBack(it->m_name);

    bool created = false;
    for (UInt32 i = 0; i < m_names.GetSize(); ++i) {
        if (Create(i)) {
            created = true;
            break;
        }
    }
    RemoveInvalidNames();
    return created;
}

// Mac 0x3965b0, Win 0x5be7c0
bool Skybox::Create(UInt32 index)
{
    BasicArray<RefPointer<Texture> > textures;
    if (!LoadTextures(textures, m_names[index]))
        return false;

    m_texCoords.PushBack(Vector2(1.0f, 1.0f));
    m_texCoords.PushBack(Vector2(1.0f, 0.0f));
    m_texCoords.PushBack(Vector2(0.0f, 0.0f));
    m_texCoords.PushBack(Vector2(0.0f, 1.0f));
    m_indices.PushBack(0);
    m_indices.PushBack(1);
    m_indices.PushBack(3);
    m_indices.PushBack(2);

    for (UInt32 face = 0; face < kFaceCount; ++face) {
        for (UInt32 corner = 0; corner < 4; ++corner) {
            const Real32* c = kFaceCorners[face][corner];
            m_faces[face].PushBack(Vector3(c[0], c[1], c[2]));
        }
        // NOTE: textures[face] unchecked — LoadTextures only succeeds with exactly six.
        if (!CreateFaceMesh(textures[face], &m_faces[face][0], &m_indices[0], &m_texCoords[0]))
            return false;
    }

    m_colors.ambient = Color::White();
    m_colors.diffuse = Color::White();
    m_colors.specular = Color::White();
    m_colors.emissive = Color::White();
    m_colors.shininess = 0.0f;
    return true;
}

// Mac 0x397690, Win 0x5bf5e0
void Skybox::RemoveInvalidNames()
{
    for (BasicArray<String>::Iterator name = m_invalidNames.Begin(); name != m_invalidNames.End(); ++name) {
        BasicArray<String>::Iterator it = m_names.Begin();
        while (it != m_names.End() && !(*it == *name))
            ++it;
        if (it == m_names.End())
            continue;
        // NOTE: a trace message that is built and dropped in both release builds.
        String message("bogusfolder removed from list: ");
        message << *name;
        m_names.Erase((UInt32)(it - m_names.Begin()));
    }
}

// Mac 0x3977b0, Win 0x5bde70
void Skybox::Draw()
{
    RenderQueue* queue = m_render->GetRenderQueue(kSkyboxQueue);
    Matrix3D camera;
    camera.SetInverse(m_render->GetViewMatrix());
    // the cube is centred on the camera: identity rotation, the camera position
    Matrix3D transform;
    transform.m[12] = camera.m[12];
    transform.m[13] = camera.m[13];
    transform.m[14] = camera.m[14];
    for (BasicArray<RefPointer<SingleMesh> >::Iterator it = m_meshes.Begin(); it != m_meshes.End(); ++it)
        queue->Add(*it, transform, 0.0f, 0, 1.0f);
    m_render->EnableDepthWrite(false);
    queue->Flush(0.02f, true, true);
    queue->Clear();
    m_render->EnableDepthWrite(true);
}

// Mac 0x3978a0, Win 0x5bdf70
void Skybox::Next()
{
    // NOTE: the original divides by the list size, so "Change background" with no valid skybox
    // (an empty or broken Graphics/Skyboxes folder) is an integer division by zero; fixed:
    // nothing to change to (F2).
    if (m_names.GetSize() == 0)
        return;
    UInt32 next = (m_current + 1) % m_names.GetSize();
    if (next == m_current)
        return;
    m_current = next;
    BasicArray<RefPointer<Texture> > textures;
    if (!LoadTextures(textures, m_names[next]))
        return;
    for (UInt32 i = 0; i < m_meshes.GetSize(); ++i)
        m_meshes[i]->SetMaterial(new TexturedSkyboxMaterial(m_colors, textures[i]));
}

// Mac 0x397be0, Win 0x5be0d0
bool Skybox::SetSkybox(const String& name)
{
    for (UInt32 i = 0; i < m_names.GetSize(); ++i) {
        if (!(m_names[i] == name))
            continue;
        if (m_current == i)
            return true;
        m_current = i;
        BasicArray<RefPointer<Texture> > textures;
        if (!LoadTextures(textures, m_names[i]))
            return false;
        for (UInt32 k = 0; k < m_meshes.GetSize(); ++k)
            m_meshes[k]->SetMaterial(new TexturedSkyboxMaterial(m_colors, textures[k]));
        return true;
    }
    return false;
}

// Mac 0x398350, Win 0x5be4e0
bool Skybox::CreateFaceMesh(Texture* texture, const Vector3* positions, const UInt32* indices,
                            const Vector2* texCoords)
{
    RefPointer<Material> material = new TexturedSkyboxMaterial(m_colors, texture);

    VertexBuffer::CreateArgs args;
    args.vertexCount = 4;
    args.indexCount = 4;
    args.frameCount = 0;
    args.dynamic = false;
    args.hasNormals = false;
    args.colorSetCount = 0;
    args.texCoordDimensions.PushBack(2);
    Object* object = m_render->GetObjectFactory()->CreateObject(typeid(VertexBuffer), args);
    RefPointer<VertexBuffer> buffer = object ? dynamic_cast<VertexBuffer*>(object) : 0;
    if (!buffer)
        return false;
    buffer->SetPrimitiveType(4);   // triangle strip
    buffer->SetPositions(positions, 0, 4, 0);
    buffer->SetIndices(indices, 0, 4, 0);
    buffer->SetTexCoords((const Real32*)texCoords, 0);
    m_meshes.PushBack(RefPointer<SingleMesh>(new SingleMesh(material, buffer)));
    return true;
}

// Mac 0x398840, Win 0x5bedf0
bool Skybox::LoadTextures(BasicArray<RefPointer<Texture> >& textures, const String& name)
{
    RefPointer<Directory> directory = m_directory->GetDirectory(name, false);
    if (!directory)
        return false;
    DescriptorSet entries;
    directory->Enumerate(String("*"), entries);
    static const char* const kFaces[kFaceCount] = { "1", "2", "3", "4", "5", "6" };
    for (DescriptorSet::Iterator it = entries.Begin(); it != entries.End(); ++it) {
        String first = it->m_name.Substring(0, 1);
        bool face = false;
        for (UInt32 i = 0; i < kFaceCount && !face; ++i)
            face = first == String(kFaces[i]);
        if (!face)
            continue;
        RefPointer<Texture> texture = LoadTexture(Path(it->m_name, directory));
        if (texture)
            textures.PushBack(texture);
    }
    bool complete = textures.GetSize() == kFaceCount;
    if (!complete)
        m_invalidNames.PushBack(name);
    return complete;
}

// Mac 0x398ee0, Win 0x5bf510
Texture* Skybox::LoadTexture(const Path& path)
{
    Texture::LoadArgs2D args;
    args.path = path;
    args.create.wrapMode = Texture::kWrapClampToEdge;
    Object* object = m_render->GetObjectFactory()->CreateObject(typeid(Texture), args);
    return object ? dynamic_cast<Texture*>(object) : 0;
}

} // namespace LEGO
