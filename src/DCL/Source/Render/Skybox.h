// LEGO Digital Designer — restored source
// LEGO::Skybox — the background cube's six textured faces. Each sub-directory of
// "Graphics/Skyboxes" (Desert, Mountain, Ocean, Space) is one skybox; files starting '1'..'6'
// are the faces. TU 418 (Mac 0x396080-0x39928f, Win 0x5bda90-0x5bfa40). Only user: LDDSkybox
// (TU 417), created by ViewMode (Mac 115 vf44).
//
// Vtables: Mac 0x757cdc {Initialize 0x396470, Draw 0x3977b0, Next 0x3978a0, SetSkybox 0x397be0,
//          D1 0x398190, D0 0x398260, LoadTextures 0x398840} (+ Object-in-Skybox 0x757d10);
//          Win 0xa2d044 {0x5bdd00, 0x5bde70, 0x5bdf70, 0x5be0d0, 0x5bedf0} — MSVC puts the
//          (virtual Object) destructor into the Object vtable 0xa2d05c {sdd 0x5bdcc0, …}, so
//          LoadTextures is Win slot 4.
// Layout: Mac {vptr, members +0x04.., Object +0x104} (sizeof 0x10c);
//         Win {vptr, vbptr, members +0x08.., Object +0x108} (sizeof 0x110).
// Class name from RTTI; method and member names inferred.
#pragma once
#include "LEGO/Types.h"
#include "Object.h"
#include "LEGO/String.h"
#include "BasicArray.h"
#include "Geometry.h"
#include "Render.h"
#include "Mesh.h"
#include "Texture.h"
#include "Path.h"
#include "Directory.h"

namespace LEGO {

class Skybox : public virtual Object {
public:
    // Mac 0x396080 (C2, VTT), Win 0x5bda90 (thiscall(render, directory, mostDerived), ret 0xc).
    // `render` is not referenced; `directory` is (RefPointer).
    Skybox(Render* render, Directory* directory);

    // Collects the sub-directory names and builds the cube from the first one whose faces load;
    // then drops the names whose faces failed to load. false when no skybox could be built.
    // NOTE: the current index (m_current) stays 0 even when a later skybox was the one built.
    virtual bool Initialize();                                        // 0 Mac 0x396470, Win 0x5bdd00
    // Draws the cube around the camera position (the translation of the inverse view matrix)
    // through its own render queue 'skyb' with depth writes off, and flushes it at once.
    virtual void Draw();                                              // 1 Mac 0x3977b0, Win 0x5bde70
    // Switches to the next skybox (cyclic) and re-textures the faces.
    // NOTE: divides by the number of names (no skybox: division by zero in both builds).
    virtual void Next();                                              // 2 Mac 0x3978a0, Win 0x5bdf70
    // Switches to the skybox `name` (exact, case-sensitive match). true when it is already the
    // current one or its faces loaded; false for an unknown name or failed faces.
    virtual bool SetSkybox(const String& name);                       // 3 Mac 0x397be0, Win 0x5be0d0
    virtual ~Skybox() {}                                              // Mac 0x398190/0x398260, Win 0x5be350 (sdd 0x5bdcc0)
    // Loads the six face textures of skybox `name` into `textures` (appended). false (and
    // `name` remembered as invalid) unless exactly six face files loaded.
    virtual bool LoadTextures(BasicArray<RefPointer<Texture> >& textures,
                              const String& name);                    // 6 Mac 0x398840, Win 0x5bedf0 (slot 4)

    UInt32 GetSkyboxCount() const { return m_names.GetSize(); }
    const String& GetSkyboxName(UInt32 index) const { return m_names[index]; }
    UInt32 GetCurrentIndex() const { return m_current; }

protected:
    // Loads skybox `index` and builds the six face meshes (appended to m_meshes).
    bool Create(UInt32 index);                                        // Mac 0x3965b0, Win 0x5be7c0
    // One face: a 4-vertex triangle strip with the texture coordinates and indices of the
    // members, drawn with a TexturedSkyboxMaterial of `texture`.
    bool CreateFaceMesh(Texture* texture, const Vector3* positions, const UInt32* indices,
                        const Vector2* texCoords);                    // Mac 0x398350, Win 0x5be4e0
    // An image file through the render's texture factory (clamped, no mipmaps).
    Texture* LoadTexture(const Path& path);                           // Mac 0x398ee0, Win 0x5bf510
    // Removes every name of m_invalidNames from m_names.
    void RemoveInvalidNames();                                        // Mac 0x397690, Win 0x5bf5e0

    Render*                          m_render;          // Mac +0x04, Win +0x08 (no reference)
    RefPointer<Directory>            m_directory;       // Mac +0x08, Win +0x0c
    BasicArray<String>               m_names;           // Mac +0x0c, Win +0x10 one per sub-directory
    BasicArray<String>               m_invalidNames;    // Mac +0x1c, Win +0x20 faces failed to load
    BasicArray<RefPointer<SingleMesh> > m_meshes;       // Mac +0x2c, Win +0x30 the faces
    UInt32                           m_current;         // Mac +0x3c, Win +0x40 index into m_names
    // NOTE: Create appends to the geometry arrays below every time it runs (they grow by one
    // face set per Create); the faces only read the first four entries.
    BasicArray<Vector2>              m_texCoords;       // Mac +0x40, Win +0x44
    BasicArray<UInt32>               m_indices;         // Mac +0x50, Win +0x54
    BasicArray<Vector3>              m_faces[6];        // Mac +0x60..+0xbf, Win +0x64..+0xc3
    MaterialColors                   m_colors;          // Mac +0xc0, Win +0xc4 (all white, set by Create)
};

} // namespace LEGO
