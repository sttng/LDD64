// LEGO Digital Designer — restored source
// The StorageScene (version 1) based model formats: LEGO::ModelLoaderStorageScene, the base of the
// LXFML 2/3/4 loaders, reads the file into a StorageScene and converts it into the StorageScene2
// the program works with; LEGO::ModelSaverStorageScene, the base of the LXFML 4 and LDraw savers,
// converts a StorageScene2 into a StorageScene and writes that.
//
// Mac TU 335 "ModelIOStorageScene.cpp" (0x2ef9b0-0x2f6e8f),
// Win 0x4d7d30-0x4db280 (+ 0x4b31f0-0x4b3340, 0x5a1e40, 0x601522-0x601556). The object also
// holds the weak copies of ModelLoader::LoadFile/~ModelLoader, the Error/PathError/ContentError
// members (restored in ModelLoader.cpp), the StorageScene destructors (StorageScene.h) and the
// StorageScene2 BIStep/BICamera/BIPart members (StorageScene2.h).
//
// Offsets: Mac / Win. BasicMap is 0x1c bytes on Mac and 0x24 on Win.
#pragma once
#include "LEGO/Types.h"
#include "Object.h"
#include "LEGO/String.h"
#include "BasicString.h"
#include "BasicArray.h"
#include "BasicMap.h"
#include "ModelLoader.h"
#include "StorageScene.h"
#include "StorageScene2.h"

namespace LEGO {

class Path;

// ---------------------------------------------------------------------------------------------
// ModelLoaderStorageScene. sizeof Mac 0x80 / Win 0x98. Abstract.
// Mac vtable 0x74b460 {D1 0x2f3490, D0 0x2f34a0, Object …, Load 0x2efbd0, Load = pure};
// Win vtable 0xa24ff0 {sdd 0x4b3340, Object …, Load 0x4d8560, Load = _purecall}.
// ---------------------------------------------------------------------------------------------
class ModelLoaderStorageScene : public ModelLoader {
public:
    ModelLoaderStorageScene();                   // Win 0x4b31f0; inlined in the derived ctors on Mac
    virtual ~ModelLoaderStorageScene();          // Mac 0x2f5740 (D1 0x2f3490 / D0 0x2f34a0), Win 0x4b32a0 (sdd 0x4b3340)

    // Reads `content` into a new StorageScene with the slot 5 Load and, when that succeeds,
    // converts it into `scene` (Convert). NOTE: `scene` is not cleared here (ModelLoaderAbstract
    // and the BrickDatabase clear it before). Mac 0x2efbd0, Win 0x4d8560.
    virtual bool Load(const AString& content, StorageScene2& scene, bool flags);

    // The format reader (vtable slot 5). NOTE: the original parameter is an integer flag word
    // (the LXFML readers test bit 0 = skip joints and bit 1 = no BIError for unknown cameras);
    // every caller passes the `bool` of ModelLoader::Load, so only bit 0 can be set.
    // (name and parameter name inferred)
    virtual bool Load(const AString& content, StorageScene& scene, bool flags) = 0;

protected:
    typedef StorageScene  SS1;
    typedef StorageScene2 SS2;

    // StorageScene -> StorageScene2. Moves the name (copy), cameras, camera and metas (swap),
    // then one RigidSystem per Model: a Rigid per rigid, a Bone per part (rigid * part
    // transformation), parts of one Flex become one Part with one bone per control point, parts
    // of one Assembly share one Brick (otherwise one Brick per Part); the joints follow the
    // rigids. Building instructions: ConvertSteps. NOTE: the source scene is modified (swapped
    // arrays, matrices normalised in place). Mac 0x2efd40, Win 0x4d8720. (name inferred)
    void Convert(StorageScene& from, StorageScene2& to);
    // Steps -> BIStep events (+ a BICamera before a step with a camera). A Flex appears only
    // once (its first part). Mac 0x2f1410, Win 0x4d94e0. (name inferred)
    void ConvertSteps(BasicArray<RefPointer<SS1::BIStep> >& steps,
                      BasicArray<RefPointer<SS2::BIEvent> >& events);

    BasicMap<SS1::Assembly*, SS2::Brick*> m_assemblyBricks;  // Mac +0x1c / Win +0x1c (name inferred)
    BasicMap<SS1::Flex*, SS2::Part*>      m_flexParts;       // Mac +0x38 / Win +0x40 (name inferred)
    BasicMap<SS1::Part*, SS2::Part*>      m_parts;           // Mac +0x54 / Win +0x64 (name inferred)
    BasicArray<SS1::Flex*>                m_flexes;          // Mac +0x70 / Win +0x88 flexes already
                                                             // in a building step (name inferred)
};

// ---------------------------------------------------------------------------------------------
// ModelSaverStorageScene. sizeof Mac 0x24 / Win 0x2c. Abstract.
// Mac vtable 0x74b494 {D1 0x2f34e0, D0 0x2f3540, Object …, Save 0x2f1d00, Save = pure};
// Win vtable 0x9ce010 {sdd 0x601556, Object …, Save 0x4d98b0, Save = _purecall}.
// ---------------------------------------------------------------------------------------------
class ModelSaverStorageScene : public ModelSaver {
public:
    ModelSaverStorageScene();                    // Win 0x6014ea (thiscall); inlined on Mac
    virtual ~ModelSaverStorageScene();           // Mac 0x2f34e0 (D0 0x2f3540), Win 0x601522 (sdd 0x601556)

    // Converts `scene` into a StorageScene (one Model with one Rigid holding every part; a brick
    // with more than one part becomes an Assembly; bricks with a part that has not exactly one
    // bone are left out) and writes it with the slot 5 Save. Mac 0x2f1d00, Win 0x4d98b0.
    virtual bool Save(const Path& path, const StorageScene2& scene);
    // The format writer (vtable slot 5).
    virtual bool Save(const Path& path, const StorageScene& scene) = 0;

protected:
    typedef StorageScene  SS1;
    typedef StorageScene2 SS2;

    // BIEvents -> steps: a BICamera sets the camera of the next step, a BIPart becomes a part of
    // `parent` (NOTE: a BIPart outside a step dereferences the null parent in both builds), a
    // BIStep a new step. Mac 0x2f2e20, Win 0x4d9f50. (name inferred)
    void ConvertEvents(const BasicArray<RefPointer<SS2::BIEvent> >& events,
                       BasicArray<RefPointer<SS1::BIStep> >& steps, SS1::BIStep* parent);

    // StorageScene2 part -> written StorageScene part. NOTE: never cleared (both builds).
    BasicMap<const SS2::Part*, SS1::Part*> m_parts;   // Mac +0x08 / Win +0x08 (name inferred)
};

} // namespace LEGO
