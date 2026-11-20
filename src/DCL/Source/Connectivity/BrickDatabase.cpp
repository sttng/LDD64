// LEGO Digital Designer — restored source
// LEGO::BrickDatabase (see BrickDatabase.h). Part of the original BrickDatabase.cpp (Mac TU
// 024): sub-TU 024b (constructor 0x7dae0, Load 0x7e100, localized names 0x7d670, aliases
// 0x7fc10, the info.xml version attribute 0x7d900), 024c (assemblies 0x7fe10) and 024d (query
// API, geometry access, scene validation, part map 0x81b60-0x83b50); Win 0x4b77e0-0x4bbe20.
// The LEGOPrimitive parser and LoadXMLOperation are in PrimitiveLoaderXML.cpp, MaterialManual
// in MaterialManual.cpp.
#include "BrickDatabase.h"
#include "ConnectionFieldPlanar.h"
#include "ConnectionFieldAxle.h"
#include "BrickGeometry.h"
#include "ModelLoaderAbstract.h"
#include "ModelIOLXFML5.h"
#include "ModelIOLXFML4.h"
#include "Directory.h"
#include "File.h"
#include "Path.h"
#include "ReadFile.h"
#include "XMLDOMNode.h"
#include "XMLDOMAttribute.h"
#include "StringStream.h"
#include "Preferences.h"
#include "LEGO/StdLib.h"
#include "Trace.h"

#include <iostream>

namespace LEGO {

namespace {

// The original builds these messages and drops them (the reporting is compiled out in both
// builds; same helper as MaterialManual.cpp).
inline void DebugMessage(const String&) {}

// AString == C string the way the inlined comparisons do it (StrNCmp over the shorter length,
// then the lengths). Same helper as MaterialManual.cpp.
bool EqualsAString(const AString& a, const char* b)
{
    UInt32 bLength = StrLen(b);
    UInt32 aLength = a.GetLength();
    if (aLength == 0)
        return bLength == 0;
    UInt32 n = aLength < bLength ? aLength : bLength;
    return StrNCmp(a.GetBuffer(), b, n) == 0 && aLength == bLength;
}

// Mode 5 of the original calls (read, share read).
const OpenMode& ReadShared()
{
    static const OpenMode mode(true, false, true, false);
    return mode;
}

} // namespace

// ---------------------------------------------------------------------------------------------
// sub-TU 024b
// ---------------------------------------------------------------------------------------------

// Mac 0x7dae0, Win 0x4b7c10
BrickDatabase::BrickDatabase(Directory* directory)
    : m_directory(directory)
{
    ConnectionFieldPlanar::InitializeTables();
    ConnectionFieldAxle::InitializeTables();

    RefPointer<File> info = m_directory->OpenFile(String("info.xml"), ReadShared(), false, 0);
    if (!info.IsNull()) {
        RefPointer<XMLDOMAString> xml = new XMLDOMAString;
        if (xml->Parse(info, 0)) {
            RefPointer<XMLDOMNodeAString> bricks = xml->FindChild(AString("Bricks"));
            if (!bricks.IsNull())
                GetXMLAttribute(*bricks, AString("version"), m_version);     // Mac 0x7d900, Win 0x4c7200
        }
    }
    m_materials = new MaterialManual(directory);
}

// Mac 0x889c0 (D0 0x88a20, virtual thunks 0x88a90 / 0x88b00; members 0x91e70), Win 0x4b2a10
// (sdd 0x4b2c60). Implicit in the original.
BrickDatabase::~BrickDatabase()
{
}

// Mac 0x7e100, Win 0x4b7f00
bool BrickDatabase::Load(bool noDecorations, bool loadAssemblies)
{
    m_primitivesDirectory = m_directory->GetDirectory(String("Primitives"), false);
    if (m_primitivesDirectory.IsNull()) {
        Trace(String("BrickDatabase: Could not open Primitives directory."), true);
        return false;
    }

    BasicSet<UInt32> assemblyParts;
    BasicArray<RefPointer<LoadXMLOperation> > operations;      // Mac vtable 0x7620f0
    RefPointer<OperationQueue> queue = new OperationQueue;

    // every Primitives/<designID>.xml is parsed by an operation on the queue
    m_primitivesDirectory->BeginEnumeration();
    while (!m_primitivesDirectory->IsEnumerationDone()) {
        RefPointer<File> file = m_primitivesDirectory->OpenCurrent(ReadShared(), 0);
        if (!file.IsNull() && Directory::MatchWildcard(file->GetPath(false), String("*.xml"))) {
            // NOTE: the original does not reset the id between files (Mac keeps the previous
            // value when the name does not start with a number, Win stores 0).
            UInt32 designID = 0;
            UInt32 position = 0;
            file->GetPath(false).Parse(designID, position);
            RefPointer<LoadXMLOperation> operation = new LoadXMLOperation(this, designID);
            if (!ReadFile(file, operation->m_text)) {
                String message("BrickDatabase: Could not load file for Primitive ");
                message << designID;
                Trace(message, true);
            } else {
                operations.PushBack(operation);                  // Mac 0x7f9f0
                queue->Add(operation);
            }
        }
        m_primitivesDirectory->MoveNext();
    }
    m_primitivesDirectory->EndEnumeration();
    queue->Wait();

    for (UInt32 i = 0; i < operations.GetSize();) {
        const LoadXMLOperation& operation = *operations[i];
        if (operation.m_succeeded) {
            const RefPointer<Primitive>& primitive = operation.m_primitive;
            m_primitives[primitive->m_designID] = primitive;
            RegisterAliases(primitive->m_designID, primitive->m_annotations);
            ++i;
        } else {
            String message("BrickDatabase: Could not parse Primitive ");
            message << operation.m_primitive->m_designID;
            Trace(message, true);
            operations.Erase(i);                                 // Mac 0x7fb30
        }
    }

    if (loadAssemblies)
        LoadAssemblies(String("Assemblies"), assemblyParts);

    // a brick for every primitive that is not a part of an assembly
    for (UInt32 i = 0; i < operations.GetSize(); ++i) {
        const LoadXMLOperation& operation = *operations[i];
        const Primitive& primitive = *operation.m_primitive;
        const UInt32 designID = primitive.m_designID;
        if (assemblyParts.Contains(designID))
            continue;
        Brick brick;
        brick.m_designID = designID;
        brick.m_defaultCamera = operation.m_context.m_defaultCamera;
        brick.m_defaultOrientation = operation.m_context.m_defaultOrientation;
        brick.m_annotations = primitive.m_annotations;
        Part part;
        part.m_designID = designID;
        if (primitive.m_bones.GetSize() < 2) {
            part.m_boneTransforms.PushBack(Matrix3D());
        } else {
            part.m_boneTransforms.Reserve(primitive.m_bones.GetSize());
            for (UInt32 b = 0; b < primitive.m_bones.GetSize(); ++b)
                part.m_boneTransforms.PushBack(primitive.m_bones[b].m_transform);   // Mac 0x452d0
        }
        brick.m_parts.PushBack(part);                            // Mac 0x81790
        m_bricks[designID] = brick;                              // Mac 0x8d290 + 0x81a00
    }

    if (!noDecorations) {
        m_decorationsDirectory = m_directory->GetDirectory(String("Decorations"), false);
        if (!m_decorationsDirectory.IsNull()) {
            // Decorations/<decorationID>.png|jpg|bmp
            DescriptorSet entries;
            m_decorationsDirectory->Enumerate(String("*"), entries);
            for (DescriptorSet::ConstIterator it = entries.Begin(); it != entries.End(); ++it) {
                const String& name = it->m_name;
                if (Directory::MatchWildcard(name, String("*.bmp")) ||
                    Directory::MatchWildcard(name, String("*.jpg")) ||
                    Directory::MatchWildcard(name, String("*.png"))) {
                    // NOTE: the id is not reset between entries in the original (see above)
                    UInt32 decorationID = 0;
                    UInt32 position = 0;
                    name.Parse(decorationID, position);
                    m_decorations[decorationID] = name;          // Mac 0x8da30
                }
            }

            // <DecorationMapping><Mapping decorationID designID surfaceID/>…
            RefPointer<File> mapping = m_directory->OpenFile(String("DecorationMapping.xml"), ReadShared(), false, 0);
            if (!mapping.IsNull()) {
                RefPointer<XMLDOMAString> xml = new XMLDOMAString;
                if (xml->Parse(mapping, 0)) {
                    const XMLDOMNodeAString::NodeArray& children = xml->GetChildren();
                    for (XMLDOMNodeAString::NodeArray::ConstIterator it = children.Begin(); it != children.End(); ++it) {
                        const XMLDOMNodeAString& node = **it;
                        if (!EqualsAString(node.GetName(), "Mapping"))
                            continue;
                        UInt32 decorationID;
                        if (!GetXMLAttribute(node, AString("decorationID"), decorationID)) {
                            String message;
                            message << "Missing  decorationID tag in file: " << mapping->GetPath(true);
                            DebugMessage(message);
                            continue;
                        }
                        if (!m_decorations.Contains(decorationID)) {
                            String message;
                            message << "BrickDatabase does not contain decorationID: " << decorationID;
                            DebugMessage(message);
                            continue;
                        }
                        UInt32 designID;
                        if (!GetXMLAttribute(node, AString("designID"), designID)) {
                            String message;
                            message << "Missing  designID tag in file: " << mapping->GetPath(true);
                            DebugMessage(message);
                            continue;
                        }
                        UInt32 surfaceID;
                        if (!GetXMLAttribute(node, AString("surfaceID"), surfaceID)) {
                            String message;
                            message << "Missing  surfaceID tag in file: " << mapping->GetPath(true);
                            DebugMessage(message);
                            continue;
                        }
                        m_decorationMapping.Insert(Pair<UInt32, UInt32>(designID, surfaceID), decorationID);
                    }
                }
            }
        }
    } else {
        m_directory = 0;
        m_primitivesDirectory = 0;
    }
    return true;
}

// Mac 0x7d670, Win 0x4b7a30
String BrickDatabase::GetLocalizedName(const Brick& brick)
{
    String name;
    String language;
    bool found = false;
    RefPointer<Preferences> preferences = Preferences::GetInstance();
    if (preferences->GetValue(String("language"), language)) {
        String key;
        key << "designname_" << language;
        AnnotationMap::ConstIterator it = brick.m_annotations.Find(key);
        if (it != brick.m_annotations.End())
            found = FromString(name, it->second);
    }
    preferences = 0;
    if (!found) {
        AnnotationMap::ConstIterator it = brick.m_annotations.Find(String("designname"));
        if (it != brick.m_annotations.End())
            FromString(name, it->second);
    }
    return name;
}

// m_aliases[designID] = designID, then every number of every "aliases" annotation
// ("3001;6210;…": parsed at each position after the previous number). Mac 0x7fc10, Win 0x4b97b0
// NOTE: Mac leaves the alias variable unchanged when a parse fails (it is not initialised
// before the first one), Win stores 0; restored: 0.
void BrickDatabase::RegisterAliases(UInt32 designID, const AnnotationMap& annotations)
{
    m_aliases[designID] = designID;                              // Mac 0x8ed90
    for (AnnotationMap::ConstIterator it = annotations.Begin(); it != annotations.End(); ++it) {
        if (!(it->first == String("aliases")))
            continue;
        const String& value = it->second;
        const UInt32 length = value.GetLength();
        for (UInt32 position = 0; position < length; ++position) {
            UInt32 alias = 0;
            value.Parse(alias, position);
            if (alias != 0)
                m_aliases[alias] = designID;
        }
    }
}

// ---------------------------------------------------------------------------------------------
// sub-TU 024c
// ---------------------------------------------------------------------------------------------

// Assemblies/<designID>.lxfml: loaded with the LXFML 5 loader (then LXFML 4) into a
// StorageScene2; its parts are sorted by design id (stable), aligned with the <PartMap> meta
// entries and become the parts of one Brick. <Meta><Annotation …/> and the annotations of the
// part primitives (except "aliases", without overwriting) become the brick's annotations.
// Mac 0x7fe10, Win 0x4ba2f0
bool BrickDatabase::LoadAssemblies(const String& directoryName, BasicSet<UInt32>& assemblyParts)
{
    RefPointer<Directory> directory = m_directory->GetDirectory(directoryName, false);
    if (directory.IsNull())
        return true;

    directory->BeginEnumeration();
    while (!directory->IsEnumerationDone()) {
        RefPointer<File> file = directory->OpenCurrent(ReadShared(), 0);
        if (!file.IsNull() && Directory::MatchWildcard(file->GetPath(false), String("*.lxfml"))) {
            RefPointer<StorageScene2> scene = new StorageScene2();
            Brick brick;
            // NOTE: not reset in the original (see Load)
            brick.m_designID = 0;
            UInt32 position = 0;
            file->GetPath(false).Parse(brick.m_designID, position);
            AString text;
            if (!ReadFile(file, text) || text.IsEmpty()) {
                String message("BrickDatabase: Brick \"");
                message << file->GetPath(true) << "\" could not be loaded. Could not open file";
                Trace(message, true);
            } else {
                // NOTE: the LXFML 5 loader gets no database (both builds), so it does not look up
                // flex bones of the parts.
                RefPointer<ModelLoader> loader = new ModelLoaderLXFML5(0);
                bool loaded = loader->Load(text, *scene, false);
                if (!loaded) {
                    scene->Clear();
                    loader = new ModelLoaderLXFML4;
                    loaded = loader->Load(text, *scene, false);
                }
                if (!loaded) {
                    String message("BrickDatabase: Brick \"");
                    message << file->GetPath(true) << "\" could not be loaded.";
                    Trace(message, true);
                } else do {
                    Vector2 camera(-0.800467014f, 0.76566267f);   // Mac 0xbf4ceb68, 0x3f440278
                    // all parts of the model, sorted by design id
                    RefPointer<StorageScene2::Brick> assembly = new StorageScene2::Brick(brick.m_designID);
                    BasicArray<RefPointer<StorageScene2::Part> >& parts = assembly->m_parts;
                    for (UInt32 b = 0; b < scene->m_bricks.GetSize(); ++b) {
                        StorageScene2::Brick& sceneBrick = *scene->m_bricks[b];
                        for (UInt32 p = 0; p < sceneBrick.m_parts.GetSize(); ++p) {
                            RefPointer<StorageScene2::Part> current = sceneBrick.m_parts[p];
                            current->m_brick = assembly;
                            for (UInt32 i = 0; i < parts.GetSize(); ++i) {
                                if (current->m_designID < parts[i]->m_designID && parts[i] != current) {
                                    RefPointer<StorageScene2::Part> swapped = parts[i];
                                    parts[i] = current;
                                    current = swapped;
                                }
                            }
                            parts.PushBack(current);             // Mac 0x83210
                        }
                    }
                    if (parts.IsEmpty()) {
                        String message("BrickDatabase: Assembly \"");
                        message << brick.m_designID << "\" does not contain any parts.";
                        TraceVerbose(message, true);
                        break;
                    }

                    // first index and count of every design id in the sorted list
                    PartIndexMap indices;
                    for (UInt32 i = 0; i < parts.GetSize(); ++i) {
                        Triple<UInt32, UInt32, UInt32>& entry = indices[parts[i]->m_designID];   // Mac 0x8f4b0
                        const UInt32 first = i - entry.third;
                        entry.first = first;
                        entry.second = first;
                        entry.third = entry.third + 1;
                    }

                    for (UInt32 m = 0; m < scene->m_metas.GetSize(); ++m) {
                        const StorageScene2::Meta& meta = scene->m_metas[m];
                        if (meta.m_name == String("Annotation")) {
                            for (UInt32 a = 0; a < meta.m_attributes.GetSize(); ++a)
                                brick.m_annotations[meta.m_attributes[a].m_name] = meta.m_attributes[a].m_value;
                        }
                        if (meta.m_name == String("DefaultCamera")) {
                            for (UInt32 a = 0; a < meta.m_attributes.GetSize(); ++a) {
                                const StorageScene2::Meta::Attribute& attribute = meta.m_attributes[a];
                                UInt32 at = 0;
                                if (attribute.m_name == String("lat"))
                                    attribute.m_value.Parse(camera.x, at);
                                else if (attribute.m_name == String("lon"))
                                    attribute.m_value.Parse(camera.y, at);
                            }
                        }
                        if (meta.m_name == String("PartMap"))
                            ParsePartMap(meta, indices, brick.m_partMappings);
                    }
                    brick.m_defaultCamera = camera;

                    bool aligned = true;
                    for (UInt32 m = 0; m < brick.m_partMappings.GetSize(); ++m) {
                        const AssemblyPartMapping& mapping = brick.m_partMappings[m];
                        if (!AlignPart(parts, mapping.m_relativeToIndex, mapping.m_relativeToOffset,
                                       mapping.m_partIndex, mapping.m_candidateCount, mapping.m_offset)) {
                            String message("BrickDatabase: Assembly \"");
                            message << brick.m_designID << "\" could not be aligned.";
                            TraceVerbose(message, true);
                            aligned = false;
                            break;
                        }
                    }
                    if (!aligned)
                        break;

                    brick.m_parts.Reserve(parts.GetSize());     // Mac 0x83850
                    bool complete = true;
                    for (UInt32 i = 0; i < parts.GetSize(); ++i) {
                        const StorageScene2::Part& scenePart = *parts[i];
                        PrimitiveMap::ConstIterator found = m_primitives.Find(scenePart.m_designID);
                        if (found == m_primitives.End() || found->second.IsNull()) {
                            String message("BrickDatabase: Assembly \"");
                            message << brick.m_designID << "\" is missing part \"" << scenePart.m_designID << "\".";
                            TraceVerbose(message, true);
                            complete = false;
                            break;
                        }
                        brick.m_parts.PushBack(Part());          // Mac 0x81790
                        Part& part = brick.m_parts.Back();
                        part.m_designID = scenePart.m_designID;
                        part.m_boneTransforms.Reserve(scenePart.m_bones.GetSize());
                        for (UInt32 b = 0; b < scenePart.m_bones.GetSize(); ++b)
                            part.m_boneTransforms.PushBack(scenePart.m_bones[b]->m_transformation);
                        const AnnotationMap& annotations = found->second->m_annotations;
                        for (AnnotationMap::ConstIterator it = annotations.Begin(); it != annotations.End(); ++it) {
                            if (it->first == String("aliases"))
                                continue;
                            if (!brick.m_annotations.Contains(it->first))
                                brick.m_annotations[it->first] = it->second;
                        }
                    }
                    if (!complete)
                        break;

                    m_bricks[brick.m_designID] = brick;
                    RegisterAliases(brick.m_designID, brick.m_annotations);
                    for (UInt32 i = 0; i < brick.m_parts.GetSize(); ++i)
                        assemblyParts.Insert(brick.m_parts[i].m_designID);   // Mac 0x8cbd0
                } while (false);
            }
        }
        directory->MoveNext();
    }
    directory->EndEnumeration();
    return true;
}

// ---------------------------------------------------------------------------------------------
// sub-TU 024d
// ---------------------------------------------------------------------------------------------

// Mac 0x81b60, Win 0x4b94c0
bool BrickDatabase::HasDecoration(UInt32 decorationID) const
{
    return m_decorations.Contains(decorationID);
}

// Mac 0x81c30, Win 0x4b9180
const Primitive* BrickDatabase::GetPrimitive(UInt32 designID) const
{
    PrimitiveMap::ConstIterator it = m_primitives.Find(designID);
    if (it == m_primitives.End())
        return 0;
    return it->second;
}

// Mac 0x81ca0, Win 0x4b91f0
const BrickDatabase::Brick* BrickDatabase::GetBrick(UInt32 designID) const
{
    BrickMap::ConstIterator it = m_bricks.Find(designID);
    if (it == m_bricks.End())
        return 0;
    return &it->second;
}

// Mac 0x81d00, Win 0x4b9260
String BrickDatabase::GetLocalizedName(UInt32 designID) const
{
    const Brick* brick = GetBrick(designID);
    if (!brick)
        return String();
    return GetLocalizedName(*brick);
}

// Mac 0x81d90, Win 0x4b9300
bool BrickDatabase::GetMaterialColor(UInt32 materialID, MaterialColor& color) const
{
    if (m_materials.IsNull())
        return false;
    return m_materials->GetMaterialColor(materialID, color);
}

// Mac 0x81dc0, Win 0x4b9320
bool BrickDatabase::GetMaterialType(UInt32 materialID, MaterialManual::MaterialType& type) const
{
    if (m_materials.IsNull())
        return false;
    return m_materials->GetMaterialType(materialID, type);
}

// Mac 0x81e30, Win 0x4b9340
bool BrickDatabase::HasMaterial(UInt32 materialID) const
{
    if (m_materials.IsNull())
        return false;
    return m_materials->HasMaterial(materialID);
}

// Mac 0x81ea0, Win 0x4b9360
bool BrickDatabase::IsMaterialTransparent(UInt32 materialID) const
{
    if (m_materials.IsNull())
        return false;
    return m_materials->IsTransparent(materialID);
}

// Mac 0x81f40, Win 0x4b9390
bool BrickDatabase::IsMaterialObsolete(UInt32 materialID) const
{
    if (m_materials.IsNull())
        return false;
    return m_materials->IsObsolete(materialID);
}

// Mac 0x81fb0, Win 0x4b93d0
bool BrickDatabase::GetDecorationPath(UInt32 decorationID, Path& path) const
{
    if (m_decorationsDirectory.IsNull())
        return false;
    DecorationMap::ConstIterator it = m_decorations.Find(decorationID);
    if (it == m_decorations.End())
        return false;
    path.Assign(Path(it->second, m_decorationsDirectory));
    return true;
}

// Mac 0x820f0, Win 0x4b9510
bool BrickDatabase::IsValidDecorationSurface(UInt32 designID, UInt32 surface) const
{
    const Primitive* primitive = GetPrimitive(designID);
    return primitive && surface < primitive->m_decorationFaceCount;
}

// Mac 0x82160, Win 0x4b9540
void BrickDatabase::GetBrickIDs(BasicArray<UInt32>& ids) const
{
    ids.Clear();
    ids.Reserve(m_bricks.GetSize());
    for (BrickMap::ConstIterator it = m_bricks.Begin(); it != m_bricks.End(); ++it)
        ids.PushBack(it->first);
}

// Win 0x4b95d0 (EAX = this); inlined on Mac (0x82470)
UInt32 BrickDatabase::ResolveAlias(UInt32 designID) const
{
    AliasMap::ConstIterator it = m_aliases.Find(designID);
    if (it == m_aliases.End())
        return 0;
    return it->second;
}

// Mac 0x82430, Win 0x4b9640
void BrickDatabase::ValidateScene(StorageScene2& scene) const
{
    for (UInt32 i = 0; i < scene.m_bricks.GetSize(); ++i)
        ValidateBrick(*scene.m_bricks[i]);
}

// Mac 0x82470, Win 0x4b9670
bool BrickDatabase::ValidateBrick(StorageScene2::Brick& brick) const
{
    BrickMap::ConstIterator found = m_bricks.Find(ResolveAlias(brick.m_designID));
    if (found == m_bricks.End())
        return false;
    const Brick& reference = found->second;
    brick.m_designID = reference.m_designID;
    for (UInt32 i = 0; i < brick.m_parts.GetSize(); ++i) {
        StorageScene2::Part& part = *brick.m_parts[i];
        const Primitive* primitive = GetPrimitive(ResolveAlias(part.m_designID));
        if (!primitive)
            return false;
        part.m_designID = primitive->m_designID;
        const UInt32 faces = primitive->m_decorationFaceCount;
        part.m_materials.Resize(faces == 0 ? 1 : faces, 0);      // Mac 0x8ddc0
        if (faces < part.m_decorations.GetSize())
            part.m_decorations.Resize(faces);
    }
    if (brick.m_parts.GetSize() < 2)
        return true;
    if (brick.m_parts.GetSize() != reference.m_parts.GetSize())
        return false;
    if (!ReorderParts(reference.m_parts, brick.m_parts))
        return false;
    for (UInt32 m = 0; m < reference.m_partMappings.GetSize(); ++m) {
        const AssemblyPartMapping& mapping = reference.m_partMappings[m];
        if (!AlignPart(brick.m_parts, mapping.m_relativeToIndex, mapping.m_relativeToOffset,
                       mapping.m_partIndex, mapping.m_candidateCount, mapping.m_offset))
            return false;
    }
    return true;
}

// Mac 0x827a0, Win 0x4b9ee0
bool BrickDatabase::ReorderParts(const BasicArray<Part>& order,
                                 BasicArray<RefPointer<StorageScene2::Part> >& parts)
{
    const UInt32 count = order.GetSize();
    if (count != parts.GetSize())
        return false;
    BasicArray<RefPointer<StorageScene2::Part> > sorted;
    if (count != 0)
        sorted.Resize(count, RefPointer<StorageScene2::Part>());   // Mac 0x8f050
    for (UInt32 p = 0; p < parts.GetSize(); ++p) {
        StorageScene2::Part* part = parts[p];
        UInt32 i = 0;
        for (; i < count; ++i) {
            if (part->m_designID == order[i].m_designID && sorted[i].IsNull()) {
                sorted[i] = part;
                break;
            }
        }
        if (i == count)
            return false;
    }
    parts = sorted;
    return true;
}

// Geometry of one decoration surface: "Lod<lod>/<designID>.g<face>". Mac 0x82a20, Win 0x4b99f0
// NOTE: when the file does not exist, both builds build the "failed to open" message from
// DirectoryElement::GetPath of the NULL file (Win 0x4b9b05: xor ecx, ecx; call 0x4255e0) and
// crash (access violation; the restoration's diff tests). Restored: the message uses the
// path string, so a missing face file returns NULL.
BrickGeometry* BrickDatabase::QueryDecorationFaceGeometry(const Primitive* primitive, UInt32 face,
                                                          UInt32 lod) const
{
    if (m_primitivesDirectory.IsNull())
        return 0;
    String lodDirectory("Lod");
    lodDirectory << lod;
    if (!m_primitivesDirectory->DirectoryExists(lodDirectory, 0))
        return 0;
    String path("Lod");
    path << lod << "/" << primitive->m_designID << ".g" << face;
    RefPointer<File> file = m_primitivesDirectory->OpenFile(path, ReadShared(), false, 0);
    if (file.IsNull()) {
        String message("BrickDatabase::QueryDecorationFaceGeometry: Failed to open '");
        message << path << "'.";
        DebugMessage(message);
        return 0;
    }
    BrickGeometry* geometry = new BrickGeometry;
    if (!geometry->Read(file)) {
        String message("BrickDatabase::QueryDecorationFaceGeometry: Failed to load '");
        message << path << "'.";
        DebugMessage(message);
        geometry->Release();
        return 0;
    }
    return geometry;
}

// Geometry of the whole primitive: "Lod<lod>/<designID>.g". Mac 0x82d10, Win 0x4b9c60
BrickGeometry* BrickDatabase::QueryMaterialGeometry(const Primitive* primitive, UInt32 lod) const
{
    if (m_primitivesDirectory.IsNull())
        return 0;
    String lodDirectory("Lod");
    lodDirectory << lod;
    if (!m_primitivesDirectory->DirectoryExists(lodDirectory, 0))
        return 0;
    String path("Lod");
    path << lod << "/" << primitive->m_designID << ".g";
    RefPointer<File> file = m_primitivesDirectory->OpenFile(path, ReadShared(), false, 0);
    if (file.IsNull()) {
        String message("BrickDatabase::QueryMaterialGeometry: Failed to open '");
        message << "Lod" << lod << "/" << primitive->m_designID << ".g" << "'.";
        DebugMessage(message);
        return 0;
    }
    BrickGeometry* geometry = new BrickGeometry;
    if (!geometry->Read(file)) {
        String message("BrickDatabase::QueryMaterialGeometry: Failed to load '");
        message << path << "'.";
        DebugMessage(message);
        geometry->Release();
        return 0;
    }
    return geometry;
}

// Mac 0x83060, Win 0x4ba190
bool BrickDatabase::AlignPart(BasicArray<RefPointer<StorageScene2::Part> >& parts, UInt32 relativeToIndex,
                              const Vector3& relativeToOffset, UInt32 partIndex, UInt32 count,
                              const Vector3& offset)
{
    // NOTE: the part indices of the mapping and the first bone of each part are used without
    // checks (both builds): an assembly in a model file whose part has no <Bone> dereferences
    // null; fixed: such an assembly does not align.
    if (relativeToIndex >= parts.GetSize() || parts[relativeToIndex]->m_bones.IsEmpty() ||
        partIndex > parts.GetSize() || count > parts.GetSize() - partIndex)
        return false;
    for (UInt32 i = 0; i < count; ++i)
        if (parts[partIndex + i]->m_bones.IsEmpty())
            return false;
    const Vector3 target = relativeToOffset * parts[relativeToIndex]->m_bones[0]->m_transformation;
    for (UInt32 i = 0; i < count; ++i) {
        const Vector3 position = offset * parts[partIndex + i]->m_bones[0]->m_transformation;
        if (Distance(target, position) < 0.1f) {
            if (i != 0) {
                RefPointer<StorageScene2::Part> swapped = parts[partIndex];
                parts[partIndex] = parts[partIndex + i];
                parts[partIndex + i] = swapped;
            }
            return true;
        }
    }
    return false;
}

// <PartMap part offset relativeTo atIndex relativeToOffset/> -> one AssemblyPartMapping: the
// next unmapped part with the "part" design id (indices[part].second++, .third--), placed
// relative to the atIndex-th part with the "relativeTo" design id. Mac 0x83330, Win 0x4bb850
bool BrickDatabase::ParsePartMap(const StorageScene2::Meta& meta, PartIndexMap& indices,
                                 BasicArray<AssemblyPartMapping>& mappings)
{
    Vector3 relativeToOffset(0.0f, 0.0f, 0.0f);
    Vector3 offset(0.0f, 0.0f, 0.0f);
    UInt32 part = 0;
    UInt32 relativeTo = 0;
    UInt32 atIndex = 0;
    for (UInt32 a = 0; a < meta.m_attributes.GetSize(); ++a) {
        const StorageScene2::Meta::Attribute& attribute = meta.m_attributes[a];
        if (attribute.m_name == String("part"))
            FromString(part, attribute.m_value);
        if (attribute.m_name == String("offset")) {
            UInt32 position = 0;
            FromString(offset, attribute.m_value, position);
        }
        if (attribute.m_name == String("relativeTo"))
            FromString(relativeTo, attribute.m_value);
        if (attribute.m_name == String("atIndex"))
            FromString(atIndex, attribute.m_value);
        if (attribute.m_name == String("relativeToOffset")) {
            UInt32 position = 0;
            FromString(relativeToOffset, attribute.m_value, position);
        }
    }
    if (!indices.Contains(part) || !indices.Contains(relativeTo))
        return false;
    Triple<UInt32, UInt32, UInt32>& entry = indices[part];
    if (entry.third == 0)
        return false;
    AssemblyPartMapping mapping;
    mapping.m_partIndex = entry.second;
    entry.second = entry.second + 1;
    mapping.m_candidateCount = entry.third;
    entry.third = entry.third - 1;
    mapping.m_relativeToIndex = atIndex + indices[relativeTo].first;
    mapping.m_relativeToOffset = relativeToOffset;
    mapping.m_offset = offset;
    mappings.PushBack(mapping);                                 // Mac 0x83a00
    return true;
}

} // namespace LEGO
