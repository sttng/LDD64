// LEGO Digital Designer — restored source
// LDDUtilities (Mac TU 063, Win 0x635e9f-0x639696) — the model part: model files and their
// load errors, the model brand, brick descriptions, palette bricks of stored bricks, lazy loads,
// element-brick snapshots and part boxes. See LDDUtilities.h (one object in the original).
#include "LDDUtilities.h"
#include "ModelLoader.h"
#include "ModelIOLXFML5.h"
#include "ModelLoaderAbstract.h"
#include "BrickDatabase.h"
#include "Primitive.h"
#include "Scene.h"
#include "Brick.h"
#include "Part.h"
#include "PrimitiveElement.h"
#include "PrimitiveElementBrick.h"
#include "LDDPart.h"
#include "LDDApplication.h"
#include "TextLocale.h"
#include "Path.h"
#include "Directory.h"

namespace LEGO {

// ===========================================================================================
// Model files
// ===========================================================================================

// Mac inlined into 0x1496c0, Win 0x636ead (thiscall(loader, text))
void AppendLoaderErrors(const ModelLoader& loader, String& text)
{
    text << "Loader error:\n";
    const BasicArray<RefPointer<ModelLoader::Error> >& errors = loader.GetErrors();
    for (UInt32 i = 0; i < errors.GetSize(); ++i) {
        text << "    ";
        errors[i]->ToString(text);
        text << "\n";
    }
}

// Mac 0x1496c0 (Win: inlined into the callers, 0x636ead out of line)
bool CheckLoaderErrors(const ModelLoader& loader, StorageScene2& scene)
{
    String text;
    AppendLoaderErrors(loader, text);
    // NOTE: the text is not used (both builds).
    const UInt32 mask = loader.GetErrorMask();
    if ((mask & (ModelLoader::Error::pathError | ModelLoader::Error::versionError |
                 ModelLoader::Error::parserError | ModelLoader::Error::miscError)) != 0)
        return false;
    if ((mask & ModelLoader::Error::biError) != 0)
        scene.m_buildingInstructions.Clear();
    return true;
}

// One alert of ShowModelLoadError (inlined 8 times).
static void ShowLoadFileError(const char* key)
{
    LDDApplication* application = LDDApplication::GetInstance();
    const String messageKey(key);
    LocalizedString message(messageKey);
    LocalizedString title((String("UNABLETOLOADFILE")));
    LocalizedString button((String("OK")));
    application->Alert(1, message, title, button, String(), String());
}

// Mac 0x1497a0, Win 0x635f5a
void ShowModelLoadError(const ModelLoader& loader, const Path& /*path*/, StorageScene2& /*scene*/)
{
    if (loader.GetErrorMask() & ModelLoader::Error::pathError)
        ShowLoadFileError("PATH_LOADFILEERROR");
    if (loader.GetErrorMask() & ModelLoader::Error::versionError)
        ShowLoadFileError("VERSION_LOADFILEERROR");
    if (loader.GetErrorMask() & ModelLoader::Error::parserError)
        ShowLoadFileError("PARSER_LOADFILEERROR");
    if (loader.GetErrorMask() & ModelLoader::Error::miscError)
        ShowLoadFileError("MISC_LOADFILEERROR");
    if (loader.GetErrorMask() & ModelLoader::Error::biError)
        ShowLoadFileError("BI_LOADFILEERROR");
    if (loader.GetErrorMask() & ModelLoader::Error::partError)
        ShowLoadFileError("PART_LOADFILEERROR");
    if (loader.GetErrorMask() & ModelLoader::Error::jointError)
        ShowLoadFileError("JOINT_LOADFILEERROR");
    if (loader.GetErrorMask() & ModelLoader::Error::cameraError)
        ShowLoadFileError("CAMERA_LOADFILEERROR");
}

// Mac 0x14a680, Win 0x639696
bool LoadModel(const Path& path, StorageScene2& scene, BrickDatabase* database, bool showErrors)
{
    Path file(String(), 0);
    RefPointer<Directory> archive = path.GetDirectory(false);
    if (!archive)
        file.Assign(path);
    else
        file.Assign(Path(String("IMAGE100.LXFML"), archive.Get()));

    // Both originals create the format cascade (Win ctor call 0x639745, Mac 0x14a78c).
    RefPointer<ModelLoaderAbstract> loader = new ModelLoaderAbstract(database);
    if (!loader->LoadFile(file, scene, true)) {
        if (showErrors)
            ShowModelLoadError(*loader, path, scene);
        if (!CheckLoaderErrors(*loader, scene))
            return false;
    }
    database->ValidateScene(scene);
    return true;
}

// Mac 0x148400, Win 0x636f08
bool LDDUtilities::GetModelBrand(const StorageScene2* model, Brand& brand)
{
    for (UInt32 i = 0; i < model->m_metas.GetSize(); ++i) {
        const StorageScene2::Meta& meta = model->m_metas[i];
        if (!(meta.m_name == String("Brand")))
            continue;
        for (UInt32 j = 0; j < meta.m_attributes.GetSize(); ++j) {
            if (meta.m_attributes[j].m_name == String("name")) {
                UInt32 position = 0;
                return FromString(brand, meta.m_attributes[j].m_value, position);
            }
        }
    }
    return false;
}

// ===========================================================================================
// Brick descriptions
// ===========================================================================================

// "<localized name>\r\n" and "Part: <id> (<name>) " / "Assembly: …", the main group, "\r\n".
// Mac 0x14af10 (database in a register; indent constant-folded to 0), Win 0x6368d9
// (cdecl(designID, assembly, name, indent, mainGroupID, mainGroup), ESI = out, ECX = database)
static void AppendBrickHeader(String& out, const BrickDatabase* database, UInt32 designID,
                              bool assembly, const String& name, UInt32 indent,
                              UInt32 mainGroupID, UInt32 mainGroup)
{
    String localizedName = database->GetLocalizedName(designID);
    out.Append(indent, ' ');
    out << localizedName << "\r\n";
    out.Append(indent, ' ');
    out << (assembly ? "Assembly: " : "Part: ");
    out << designID << " (" << name << ") ";
    if (mainGroupID != 0) {
        out << "\r\nMaingroup: " << mainGroupID;
        if (mainGroup != 0)
            out << " (" << mainGroup << ")";
    }
    out << "\r\n";
}

// "Part: <id>" (when given; the rest is indented 2 more), "Material: <id> - <localized
// "Material<id>">" per material (0 = the first one), "Deco: <index>, <id>" per decoration.
// Mac 0x14b0a0 (partDesignID / materials in ECX / EDX), Win 0x636b81 (ECX = materials)
static void AppendPartLines(String& out, UInt32 partDesignID, const BasicArray<UInt32>& materials,
                            const BasicArray<UInt32>& decorations, UInt32 indent)
{
    if (partDesignID != 0) {
        out.Append(indent, ' ');
        out << "Part: " << partDesignID << "\r\n";
        indent += 2;
    }
    for (UInt32 i = 0; i < materials.GetSize(); ++i) {
        out.Append(indent, ' ');
        UInt32 material = materials[i];
        if (material == 0)
            material = materials[0];
        String key("Material");
        key << material;
        LocalizedString name(key);
        out << "Material: " << material << " - ";
        out << static_cast<const String&>(name) << "\r\n";
    }
    for (UInt32 i = 0; i < decorations.GetSize(); ++i) {
        if (decorations[i] != 0) {
            out.Append(indent, ' ');
            out << "Deco: " << i << ", " << decorations[i] << "\r\n";
        }
    }
}

// Mac 0x14adf0, Win 0x637264
String LDDUtilities::GetBrickDescription(const BrickDatabase* database, const BrickPalette::Brick* brick)
{
    const bool assembly = brick->m_parts.GetSize() > 1;
    String description;
    AppendBrickHeader(description, database, brick->m_designID, assembly, brick->m_name, 0,
                      brick->GetMainGroupID(), brick->GetMainGroup());
    for (UInt32 i = 0; i < brick->m_parts.GetSize(); ++i) {
        const BrickPalette::Part* part = brick->m_parts[i];
        AppendPartLines(description, assembly ? part->m_designID : 0, part->m_materials,
                        part->m_decorations, assembly ? 2 : 0);
    }
    return description;
}

// Mac 0x14b430, Win 0x6373be
String LDDUtilities::GetBrickDescription(const BrickDatabase* database, const StorageScene2::Brick* brick)
{
    const bool assembly = brick->m_parts.GetSize() > 1;
    String description;
    AppendBrickHeader(description, database, brick->m_designID, assembly, brick->m_itemNos, 0, 0, 0);
    for (UInt32 i = 0; i < brick->m_parts.GetSize(); ++i) {
        const StorageScene2::Part* part = brick->m_parts[i];
        AppendPartLines(description, assembly ? part->m_designID : 0, part->m_materials,
                        part->m_decorations, assembly ? 2 : 0);
    }
    return description;
}

// Mac 0x14b320 (Win inlined into SelectionToolTooltipAugmentation 0x667256)
String LDDUtilities::GetBrickDescription(const BrickDatabase* database, const PrimitiveElement* element)
{
    const Brick* brick = element->GetPart()->GetBrick();
    const UInt32 designID = brick->GetDesignID();
    const UInt32 partCount = brick->GetPartCount();
    String description;
    AppendBrickHeader(description, database, designID, partCount > 1, brick->GetName(), 0, 0, 0);
    if (partCount < 2) {
        const Part* part = element->GetPart();
        AppendPartLines(description, 0, part->GetMaterials(), part->GetDecorations(), 0);
    } else {
        const BasicArray<RefPointer<Part> >& parts = brick->GetParts();
        for (UInt32 i = 0; i < parts.GetSize(); ++i)
            AppendPartLines(description, parts[i]->GetDesignID(), parts[i]->GetMaterials(),
                            parts[i]->GetDecorations(), 2);
    }
    return description;
}

// ===========================================================================================
// Palette bricks of stored bricks
// ===========================================================================================

// One design of the database brick: its primitive, how often the palette brick has it and
// how often the database brick has it. (RTTI name ValidateBrickAccumulator; members inferred)
struct ValidateBrickAccumulator {
    ValidateBrickAccumulator() : primitive(0), count(0), expected(1) {}
    const Primitive* primitive;   // +0x00
    UInt32           count;       // +0x04
    UInt32           expected;    // +0x08
};

// Mac 0x148b30
bool ValidatePaletteBrick(const BrickPalette::Brick& brick, BrickDatabase* database)
{
    const BrickDatabase::Brick* databaseBrick = database->GetBrick(brick.m_designID);
    if (databaseBrick == 0)
        return false;
    if (brick.m_parts.GetSize() != databaseBrick->m_parts.GetSize())
        return false;

    BasicMap<UInt32, ValidateBrickAccumulator> designs;
    for (UInt32 i = 0; i < databaseBrick->m_parts.GetSize(); ++i) {
        const UInt32 designID = databaseBrick->m_parts[i].m_designID;
        BasicMap<UInt32, ValidateBrickAccumulator>::Iterator it = designs.Find(designID);
        if (it != designs.End()) {
            ++it->second.expected;
            continue;
        }
        ValidateBrickAccumulator& entry = designs[designID];
        entry = ValidateBrickAccumulator();
        entry.primitive = database->GetPrimitive(designID);
        if (entry.primitive == 0)
            return false;
    }
    for (UInt32 i = 0; i < brick.m_parts.GetSize(); ++i) {
        const BrickPalette::Part* part = brick.m_parts[i];
        BasicMap<UInt32, ValidateBrickAccumulator>::Iterator it = designs.Find(part->m_designID);
        if (it == designs.End())
            return false;
        ++it->second.count;
        const Primitive* primitive = it->second.primitive;
        if (part->m_boneTransforms.GetSize() != primitive->GetBoneCount())
            return false;
        const UInt32 faces = primitive->m_decorationFaceCount;
        if (part->m_materials.GetSize() != (faces != 0 ? faces : 1) ||
            faces < part->m_decorations.GetSize())
            return false;
    }
    for (BasicMap<UInt32, ValidateBrickAccumulator>::ConstIterator it = designs.Begin();
         it != designs.End(); ++it) {
        if (it->second.count != it->second.expected)
            return false;
    }
    return true;
}

// Mac 0x148830
RefPointer<BrickPalette::Brick> CreatePaletteBrick(const StorageScene2::Brick& stored, BrickDatabase* database)
{
    RefPointer<BrickPalette::Brick> brick = new BrickPalette::Brick;
    if (!brick->Initialize(stored.m_designID, *database))
        return RefPointer<BrickPalette::Brick>();
    brick->m_name = stored.m_itemNos;
    for (UInt32 i = 0; i < stored.m_parts.GetSize(); ++i) {
        const StorageScene2::Part* source = stored.m_parts[i];
        brick->m_parts.PushBack(RefPointer<BrickPalette::Part>(new BrickPalette::Part));
        BrickPalette::Part* part = brick->m_parts.Back();
        part->m_designID = source->m_designID;
        part->m_materials = source->m_materials;
        part->m_decorations = source->m_decorations;
        part->m_boneTransforms.Reserve(source->m_bones.GetSize());
        for (UInt32 j = 0; j < source->m_bones.GetSize(); ++j)
            part->m_boneTransforms.PushBack(source->m_bones[j]->m_transformation);
    }
    if (!ValidatePaletteBrick(*brick, database))
        return RefPointer<BrickPalette::Brick>();
    return brick;
}

// Mac 0x148e70
bool CreatePaletteBricks(StorageScene2& model, BrickDatabase* database,
                         BasicArray<RefPointer<const BrickPalette::Brick> >& bricks, bool removeInvalid)
{
    UInt32 i = 0;
    while (i < model.m_bricks.GetSize()) {
        RefPointer<BrickPalette::Brick> brick = CreatePaletteBrick(*model.m_bricks[i], database);
        if (brick) {
            bricks.PushBack(RefPointer<const BrickPalette::Brick>(brick.Get()));
            ++i;
        } else if (removeInvalid) {
            model.m_bricks.Erase(i);
        } else {
            ++i;
        }
    }
    return model.m_bricks.GetSize() == bricks.GetSize();
}

// Mac 0x148530
void CreateElementBricks(const BasicArray<RefPointer<Part> >& parts,
                         BasicArray<ObjectRefPointer<PrimitiveElementBrick> >& bricks)
{
    BasicArray<RefPointer<Brick> > sceneBricks;
    Scene::GetBricks(parts, sceneBricks);
    for (UInt32 i = 0; i < sceneBricks.GetSize(); ++i) {
        PrimitiveElementBrick* brick =
            new PrimitiveElementBrick(static_cast<const LDDBrick*>(sceneBricks[i].Get()));
        bricks.PushBack(ObjectRefPointer<PrimitiveElementBrick>(brick));
    }
}

// Mac 0x149210, Win 0x636faf
void AddLazyLoads(const Scene& scene, BrickConnectSceneManager::LazyLoadData& data)
{
    const BasicArray<RefPointer<Brick> >& bricks = scene.GetBricks();
    for (UInt32 i = 0; i < bricks.GetSize(); ++i) {
        const Brick* brick = bricks[i];
        data.AddBrick(brick->GetDesignID());
        const BasicArray<RefPointer<Part> >& parts = brick->GetParts();
        for (UInt32 j = 0; j < parts.GetSize(); ++j) {
            data.AddMaterials(parts[j]->GetMaterials());
            data.AddDecorations(parts[j]->GetDecorations());
        }
    }
}

// ===========================================================================================
// Part boxes
// ===========================================================================================

// Mac 0x14b530, Win 0x637457
void GetPartBox(const Part* part, Matrix3D& transform, BoundingBox& box)
{
    if (part->GetElementCount() < 2) {
        part->GetElement(0)->GetTransform(transform);
        box = part->GetPrimitive()->GetBoundingBox();
        return;
    }
    box.Reset();
    for (UInt32 i = 0; i < part->GetElementCount(); ++i) {
        const PrimitiveElement* element = part->GetElement(i);
        Matrix3D elementTransform;
        element->GetTransform(elementTransform);
        BoundingBox bone = element->GetPrimitive()->GetBone(element->GetBoneIndex()).m_bounding * elementTransform;
        box.Union(bone);
    }
    transform.SetIdentity();
}

// Mac 0x14b650, Win 0x6369c5
void GetBoxTransform(const BoundingBox& box, Matrix3D& transform, bool grow)
{
    transform.Translate(Rotate(box.m_min, transform));
    Real32 padding = 0.0f;
    if (grow) {
        transform.Translate(Rotate(Vector3(-0.025f, -0.025f, -0.025f), transform));
        padding = 0.05f;
    }
    Matrix3D scaling;
    const Real32 width = box.GetWidth() + padding;
    const Real32 height = box.GetHeight() + padding;
    scaling.SetScaling(width, height, padding + box.GetDepth());
    transform.Multiply(scaling);
}

// Mac 0x14b600, Win 0x6374f5
void GetPartBoxTransform(const Part* part, Matrix3D& transform, bool flag)
{
    BoundingBox box(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
    GetPartBox(part, transform, box);
    GetBoxTransform(box, transform, flag);
}

// Mac 0x14b780, Win 0x637542
void GetElementBoxTransform(const PrimitiveElement* element, Matrix3D& transform, bool grow)
{
    element->GetTransform(transform);
    GetBoxTransform(element->GetPrimitive()->GetBone(element->GetBoneIndex()).m_bounding, transform, grow);
}

} // namespace LEGO
