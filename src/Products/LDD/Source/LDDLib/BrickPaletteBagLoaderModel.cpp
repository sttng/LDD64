// LEGO Digital Designer — restored source
// LEGO::BrickPaletteBagLoaderModel — a model file (LXF / LXFML / LDraw) used as a palette bag:
//   one palette brick per scene brick, each checked against the brick database. Declared in
//   BrickPaletteBagLoaderPAXML.h (TU 057).
//
// TU 440 (Mac 0x3b6500-0x3b73cb, modmap guess "StorageScene2.cpp"; file name inferred:
// BrickPaletteBagLoaderModel.cpp; static initialiser 0x3b7350 = `#include <iostream>`);
// Win 0x64f26e-0x64f94f (InitBag 0x64f26e, CreateParts 0x64f303, LoadBag 0x64f402).
#include "BrickPaletteBagLoaderPAXML.h"
#include "BrickPalette.h"
#include "BrickDatabase.h"
#include "StorageScene2.h"
#include "LDDUtilities.h"
#include "Path.h"
#include "Trace.h"

namespace LEGO {

// Mac 0x3b7280, Win 0x64f26e
void BrickPaletteBagLoaderModel::InitBag(const StorageScene2& scene, BrickPalette::Bag& bag)
{
    bag.m_name = scene.m_name.IsEmpty() ? String("User Palette") : String(scene.m_name);
    bag.m_text = "";
    bag.m_brand = 0;
    bag.m_version = Version();
    bag.m_countable = true;
    bag.m_brandFilter = false;
}

// Mac inlined into LoadBag (0x3b67c5-0x3b6b3b), Win 0x64f303
bool BrickPaletteBagLoaderModel::CreateParts(BrickPalette::Brick& brick, const UInt32* material,
                                             const String& name, const BrickDatabase& database)
{
    brick.m_name = name;
    const BrickDatabase::Brick* databaseBrick = database.GetBrick(brick.m_designID);
    if (!databaseBrick)
        return false;
    for (BasicArray<BrickDatabase::Part>::ConstIterator it = databaseBrick->m_parts.Begin();
         it != databaseBrick->m_parts.End(); ++it) {
        RefPointer<BrickPalette::Part> part = new BrickPalette::Part();
        part->m_designID = it->m_designID;
        part->m_boneTransforms = it->m_boneTransforms;
        // One material per decoration face (at least one); the first is the model brick's
        // material (one-part bricks) or 21.
        UInt32 count = database.GetPrimitive(part->m_designID)->m_decorationFaceCount;
        if (count == 0)
            count = 1;
        part->m_materials.Resize(count, 0);
        part->m_materials[0] = material ? *material : 21;
        brick.m_parts.PushBack(part);
    }
    return !brick.m_parts.IsEmpty();
}

// Mac 0x3b6500, Win 0x64f402
bool BrickPaletteBagLoaderModel::LoadBag(const Path& path, BrickPalette::Bag& bag,
                                         BrickDatabase* database, PaletteNames* names)
{
    bag.Clear();
    RefPointer<StorageScene2> scene = new StorageScene2();
    if (!LoadModel(path, *scene, database, false)) {
        String message("Cannot load palette model to storage scene from path : ");
        Trace(message << path.GetPath(true), true);
        return false;
    }
    InitBag(*scene, bag);
    const String name;   // the palette bricks get no name (Win: CreateParts' argument)
    for (BasicArray<RefPointer<StorageScene2::Brick> >::ConstIterator it = scene->m_bricks.Begin();
         it != scene->m_bricks.End(); ++it) {
        const StorageScene2::Brick* modelBrick = *it;
        RefPointer<BrickPalette::Brick> brick = new BrickPalette::Brick();
        // A one-part brick keeps the material of its first surface; assemblies get 21.
        UInt32 material = 0;
        if (modelBrick->m_parts.GetSize() == 1)
            material = modelBrick->m_parts[0]->m_materials[0];

        if (!brick->Initialize(modelBrick->m_designID, *database)) {
            String message("Cannot create brick: ");
            message << modelBrick->m_designID << " from palette: " << bag.m_name;
            Trace(message, true);
            continue;
        }
        if (!CreateParts(*brick, material != 0 ? &material : 0, name, *database)) {
            String message("Cannot find brick: ");
            message << LDDUtilities::GetBrickDescription(database, brick.Get())
                    << " from palette: " << bag.m_name;
            Trace(message, true);
            continue;
        }

        // The model parts must match the database parts; their materials and decorations must
        // exist (decorations from surface 1 on: entry 0 is the LXFML loader's placeholder).
        // The first error drops the brick.
        bool error = false;
        for (UInt32 i = 0; i < modelBrick->m_parts.GetSize(); ++i) {
            const StorageScene2::Part* modelPart = modelBrick->m_parts[i];
            // NOTE: both builds index the palette parts with the model's part index without a
            // range check (a model brick with more parts than the database one reads past the
            // array); fixed: such a part counts as a part ID error.
            BrickPalette::Part* part = i < brick->m_parts.GetSize() ? brick->m_parts[i].Get() : 0;
            if (!part || part->GetDesignID() != modelPart->m_designID) {
                String message("Part ID error in brick: ");
                message << modelBrick->m_designID << " from palette: " << bag.m_name;
                Trace(message, true);
                error = true;
            }
            if (!error) {
                for (UInt32 surface = 0; surface < modelPart->m_materials.GetSize(); ++surface) {
                    const UInt32 id = modelPart->m_materials[surface];
                    if (id != 0 && !database->HasMaterial(id)) {
                        String message("Material error on surface: ");
                        message << surface << " of part: " << i << " of brick: "
                                << modelBrick->m_designID << " from palette: " << bag.m_name;
                        Trace(message, true);
                        error = true;
                        break;
                    }
                }
            }
            if (!error) {
                for (UInt32 surface = 1; surface < modelPart->m_decorations.GetSize(); ++surface) {
                    const UInt32 id = modelPart->m_decorations[surface];
                    if (id != 0 && !database->HasDecoration(id)) {
                        String message("Decoration error on surface: ");
                        message << surface << " of part: " << i << " of brick: "
                                << modelBrick->m_designID << " from palette: " << bag.m_name;
                        Trace(message, true);
                        error = true;
                        break;
                    }
                }
            }
            if (error)
                break;
            part->m_materials = modelPart->m_materials;
            part->m_decorations = modelPart->m_decorations;
        }
        if (!error)
            bag.AddBrick(brick, 1, *database);
    }
    names->Insert(PaletteName(String(scene->m_name), bag.m_version));
    return true;
}

} // namespace LEGO
