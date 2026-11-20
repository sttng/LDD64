// LEGO Digital Designer — restored source
// LEGO::ModelLoaderLDraw / LEGO::ModelSaverLDraw (Mac TU 337 "ModelIOLDraw.cpp",
// Win 0x5a2060-0x5a72ff). See the header for the format and the conversion.
#include "ModelIOLDraw.h"
#include "Path.h"
#include "File.h"
#include "Directory.h"
#include "Platform.h"
#include "Preferences.h"
#include "StringConversion.h"

#include <assert.h>
#include <iostream>

namespace LEGO {

namespace {

const Real32 kLDrawUnitsPerLDDUnit = 25.0f;   // Mac 0x630004
const Real32 kNoMatch = 1.0e9f;               // Mac 0x63cc70: largest accepted assembly distance

// "LDrawMapFile" in the resources directory (both the loader and the saver).
// Returns false with "Could not open assets directory" when there is no such directory.
bool MappingPath(Path& path, String& message)
{
    RefPointer<Directory> resources =
        Platform::GetInstance()->GetSystemDirectory(Platform::kResourceDirectory, true);
    if (resources.IsNull()) {
        message = String("Could not open assets directory");
        return false;
    }
    String file;
    Preferences::GetInstance()->GetValue(String("LDrawMapFile"), file);
    path = Path(file, resources.Get());
    return true;
}

} // namespace

// ============================================================================================
// LdrawMapXMLLoader (the out-of-line destructors were emitted here)
// ============================================================================================

// LdrawMapXMLLoader::~LdrawMapXMLLoader: Mac 0x2faba0 / 0x2fbd50, Win 0x5a4530 (header inline).

// ============================================================================================
// ModelLoaderLDraw
// ============================================================================================

ModelLoaderLDraw::ModelLoaderLDraw()                                         // Mac 0x2f7740
{
    // NOTE: m_unknown80 is not initialised (both builds); fixed: 0 (low).
    m_unknown80 = 0;
}

ModelLoaderLDraw::~ModelLoaderLDraw()                                        // Mac 0x2fbda0, Win 0x5a2060
{
}

bool ModelLoaderLDraw::Load(const AString& content, StorageScene& scene, bool) // Mac 0x2f7c80, Win 0x5a45f0
{
    m_message.Clear();
    scene.Clear();
    m_errors.Clear();
    m_errorMask = 0;
    const char* text = content.GetBuffer();
    return Parse(String(text ? text : ""), scene);
}

bool ModelLoaderLDraw::LoadMapping()                                         // Mac 0x2f9a40, Win 0x5a5f50
{
    Path path;
    if (!MappingPath(path, m_message))
        return false;
    LdrawMapXMLLoader loader;
    if (!loader.Load(path, m_bricks, m_materials, m_transformations, m_assemblies)) {
        m_message = loader.GetError();
        return false;
    }
    return true;
}

namespace {

// Next blank-separated word of `rest` (removed from it), lower case. Win 0x5a5ed0, inlined
// twice on Mac (0x2f9d5c, 0x2f9dd1). (name inferred)
String NextWord(String& rest)
{
    UInt32 position = 0;
    String word = rest.ScanWord(&position);
    rest.Erase(0, position);
    word.ToLower();
    return word;
}

} // namespace

void ModelLoaderLDraw::ClassifyLine(const String& line, UInt32& type, String& rest) // Mac 0x2f9d20, Win 0x5a5a70
{
    rest = line;
    String word = NextWord(rest);
    if (word.EqualsNoCase(String("0"))) {
        word = NextWord(rest);
        type = word.EqualsNoCase(String("clear")) ? kLineClear : kLineOther;
    } else {
        type = word.EqualsNoCase(String("1")) ? kLinePart : kLineOther;
    }
}

bool ModelLoaderLDraw::ParsePartLine(const String& rest, UInt32& color, Matrix3D& transformation,
                                     String& file)                           // Mac 0x2f9f20, Win 0x5a5bc0
{
    BasicArray<String> fields;
    rest.Split((UniChar16)' ', fields);
    for (UInt32 i = 0; i < fields.GetSize();) {
        if (fields[i].IsEmpty())
            fields.Erase(i);
        else
            ++i;
    }
    if (fields.GetSize() < 14) {
        m_message = String("Error while parsing LDraw part");
        return false;
    }
    // x y z, then the 3x3 matrix row by row (a b c / d e f / g h i) into the columns.
    static const UInt32 kTarget[12] = { 12, 13, 14, 0, 1, 2, 4, 5, 6, 8, 9, 10 };
    UInt32 position = 0;
    fields[0].Parse(color, position);
    for (UInt32 i = 0; i < 12; ++i) {
        position = 0;
        fields[i + 1].Parse(transformation.m[kTarget[i]], position);
    }
    transformation.RemoveScaling();
    file = fields[13];
    // NOTE: both builds build this message from field 14 even when there are only 14 fields
    // and drop it. Usually the leading empty field (the blank after "1") was removed, so that
    // slot still holds a copy of the file name; with a tab after the "1" nothing was removed
    // and the slot is uninitialised array storage: the original appends a garbage String and
    // may crash (seen in diff_ldraw). Restored only for a real 15th field.
    if (fields.GetSize() > 14) {
        String ignored("Ignoring: <");
        ignored << fields[14] << "> in part data.";
    }
    return true;
}

void ModelLoaderLDraw::LookupDesignID(const String& file, UInt32& designID) // Win 0x5a6170 (inlined on Mac)
{
    LdrawMapXMLLoader::BrickMap::Iterator it = m_bricks.Find(file);
    if (it != m_bricks.End()) {
        designID = it->second;
        return;
    }
    // "3001.dat" -> 3001 (unchanged when the name does not start with a number)
    UInt32 position = 0;
    file.Parse(designID, position);
}

bool ModelLoaderLDraw::LookupMaterial(UInt32 color, UInt32& material)       // Mac 0x2fa900, Win 0x5a6200
{
    LdrawMapXMLLoader::MaterialMap::Iterator it = m_materials.Find(color);
    if (it == m_materials.End()) {
        String message("Could not find LDraw color: ");
        m_message = message << color;
        return false;
    }
    material = it->second;
    return true;
}

bool ModelLoaderLDraw::ToLXFTransformation(const Matrix3D& ldraw, Matrix3D& lxf,
                                           const String& name)               // Mac 0x2fa9d0, Win 0x5a62f0
{
    Vector3 axis;
    Real32 angle;
    ldraw.GetAxisAngle(axis, angle);
    axis.y = -axis.y;
    axis.z = -axis.z;
    angle = -angle;
    lxf.SetRotation(axis, angle);
    lxf.SetTranslation(ldraw.GetTranslation());
    lxf.m[12] = lxf.m[12] / kLDrawUnitsPerLDDUnit;
    lxf.m[13] = -lxf.m[13] / kLDrawUnitsPerLDDUnit;
    lxf.m[14] = -lxf.m[14] / kLDrawUnitsPerLDDUnit;
    LdrawMapXMLLoader::TransformationMap::Iterator it = m_transformations.Find(name);
    if (it == m_transformations.End()) {
        String message("Could not find transformation for part: ");
        m_message = message << name;
        return false;
    }
    lxf *= it->second;
    return true;
}

bool ModelLoaderLDraw::FillPart(SS1::Part* part, UInt32 color, const Matrix3D& transformation,
                                const String& file, const String& transformationName) // Mac 0x2fa330, Win 0x5a5e70
{
    LookupDesignID(file, part->m_designID);
    UInt32 material;
    if (!LookupMaterial(color, material))
        return false;
    part->m_materials.PushBack(material);
    return ToLXFTransformation(transformation, part->m_transformation, transformationName);
}

bool ModelLoaderLDraw::Parse(const String& text, StorageScene& scene)       // Mac 0x2f7e00, Win 0x5a46b0
{
    if (!LoadMapping()) {
        ModelLoader::Error* error = new ModelLoader::Error(ModelLoader::Error::miscError);
        error->SetDescription(m_message);
        AddError(error);
        return false;
    }

    BasicArray<String> lines;
    text.SplitLines(lines);
    RefPointer<SS1::Rigid> rigid = new SS1::Rigid;
    BasicArray<AssemblyBagItem> bag;   // pieces of assemblies, grouped below

    for (UInt32 i = 0; i < lines.GetSize(); ++i) {
        String rest(lines[i].GetBuffer());
        UInt32 type;
        ClassifyLine(lines[i], type, rest);
        if (type == kLineClear) {
            // NOTE: the parts read so far are dropped with the old rigid (both builds).
            rigid = new SS1::Rigid;
            continue;
        }
        if (type != kLinePart)
            continue;

        Matrix3D transformation;
        String file;
        UInt32 color;
        if (!ParsePartLine(rest, color, transformation, file)) {
            String message("Unparsed LDraw line: ");
            message << i << " - Wrong part format or unknown part: " << lines[i];
            String description(message);
            // NOTE: a part is created and released again without being used (both builds).
            RefPointer<SS1::Part> unused = new SS1::Part;
            ModelLoader::PartError* error = new ModelLoader::PartError(0, ModelLoader::PartError::misc);
            error->SetDescription(description);
            AddError(error);
            continue;
        }

        LdrawMapXMLLoader::AssemblyMap::Iterator found = m_assemblies.Find(file);
        if (found != m_assemblies.End()) {
            // A whole assembly in one line: one part per piece.
            LdrawMapXMLLoader::Assembly assembly = m_assemblies[file];
            // The design id comes from the first assembly (in name order) that lists the first
            // piece. NOTE: an assembly without parts reads a null String here (both builds).
            String first(assembly.m_parts[0].m_ldraw);
            UInt32 designID = 0;
            bool listed = false;
            for (LdrawMapXMLLoader::AssemblyMap::Iterator a = m_assemblies.Begin();
                 a != m_assemblies.End() && !listed; ++a) {
                for (UInt32 p = 0; p < a->second.m_parts.GetSize(); ++p) {
                    if (a->second.m_parts[p].m_ldraw == first) {
                        designID = a->second.m_designID;
                        listed = true;
                        break;
                    }
                }
            }
            RefPointer<SS1::Assembly> brick = new SS1::Assembly(designID);
            for (UInt32 p = 0; p < assembly.m_parts.GetSize(); ++p) {
                const LdrawMapXMLLoader::AssemblyPart& piece = assembly.m_parts[p];
                RefPointer<SS1::Part> part = new SS1::Part;
                if (FillPart(part.Get(), piece.m_color, transformation, piece.m_ldraw, file)) {
                    part->m_transformation *= piece.m_transformation;
                    brick->m_parts.PushBack(part.Get());
                    part->m_assembly = brick;
                    rigid->m_parts.PushBack(part);
                } else {
                    String message("Subpart error: - ");
                    message << piece.m_ldraw << " in " << file;
                }
            }
            continue;
        }

        RefPointer<SS1::Part> part = new SS1::Part;
        if (!FillPart(part.Get(), color, transformation, file, file)) {
            String message("Part error: - ");
            message << file;
            continue;
        }
        // Is the file a piece of some assembly? Then it waits in the bag.
        String name(file);
        UInt32 designID = 0;
        bool piece = false;
        for (LdrawMapXMLLoader::AssemblyMap::Iterator a = m_assemblies.Begin();
             a != m_assemblies.End() && !piece; ++a) {
            for (UInt32 p = 0; p < a->second.m_parts.GetSize(); ++p) {
                if (a->second.m_parts[p].m_ldraw == name) {
                    designID = a->second.m_designID;
                    piece = true;
                    break;
                }
            }
        }
        if (piece)
            bag.PushBack(AssemblyBagItem(part.Get(), designID, String(file)));
        else
            rigid->m_parts.PushBack(part);
    }

    // Group the pieces: per round, try every (bag item, assembly listing it) pair as the anchor,
    // match each piece of the assembly to the nearest unused bag item of that name (distance of
    // the assembly centres the two imply) and keep the complete match with the smallest sum of
    // squared distances. Its items become one assembly; repeat until nothing matches. Items
    // left over are dropped.
    while (!bag.IsEmpty()) {
        const LdrawMapXMLLoader::Assembly* best = 0;
        Real32 bestDistance = kNoMatch;
        BasicArray<SS1::Part*> selected;
        Real32 minimum = kNoMatch;   // NOTE: carried over between searches (see below)

        for (UInt32 b = 0; b < bag.GetSize(); ++b) {
            const AssemblyBagItem& anchor = bag[b];
            for (LdrawMapXMLLoader::AssemblyMap::Iterator a = m_assemblies.Begin();
                 a != m_assemblies.End(); ++a) {
                const LdrawMapXMLLoader::Assembly& assembly = a->second;
                Vector3 center;
                const LdrawMapXMLLoader::AssemblyPart* anchorPiece = 0;
                for (UInt32 p = 0; p < assembly.m_parts.GetSize(); ++p) {
                    if (assembly.m_parts[p].m_ldraw == anchor.m_name) {
                        anchorPiece = &assembly.m_parts[p];
                        break;
                    }
                }
                if (!anchorPiece)
                    continue;
                {
                    Matrix3D inverse;
                    inverse.SetInverse(anchorPiece->m_transformation);
                    center = assembly.m_center * Matrix3D(anchor.m_part->m_transformation, inverse);
                }

                Real32 total = 0.0f;
                bool complete = true;
                for (UInt32 p = 0; p < assembly.m_parts.GetSize() && complete; ++p) {
                    const LdrawMapXMLLoader::AssemblyPart& piece = assembly.m_parts[p];
                    AssemblyBagItem* nearest = 0;
                    for (UInt32 c = 0; c < bag.GetSize(); ++c) {
                        AssemblyBagItem& candidate = bag[c];
                        if (!(piece.m_ldraw == candidate.m_name))
                            continue;
                        Matrix3D inverse;
                        inverse.SetInverse(piece.m_transformation);
                        Vector3 delta = assembly.m_center * Matrix3D(candidate.m_part->m_transformation, inverse);
                        delta = delta - center;
                        Real32 distance = delta.GetLengthSquared();
                        if (candidate.m_used)
                            continue;
                        // NOTE: the first unused candidate is always taken (`minimum` is
                        // not reset per piece; the comparison only matters afterwards).
                        if (!nearest || minimum > distance) {
                            nearest = &candidate;
                            minimum = distance;
                        }
                    }
                    if (!nearest) {
                        complete = false;
                        break;
                    }
                    nearest->m_used = true;
                    total += minimum;
                }
                if (complete && bestDistance > total) {
                    best = &assembly;
                    selected.Clear();
                    for (UInt32 c = 0; c < bag.GetSize(); ++c)
                        if (bag[c].m_used)
                            selected.PushBack(bag[c].m_part.Get());
                    bestDistance = total;
                }
                for (UInt32 c = 0; c < bag.GetSize(); ++c)
                    bag[c].m_used = false;
            }
        }

        if (!best) {
            bag.Clear();
            break;
        }
        RefPointer<SS1::Assembly> brick = new SS1::Assembly(best->m_designID);
        for (UInt32 s = 0; s < selected.GetSize(); ++s) {
            SS1::Part* part = selected[s];
            brick->m_parts.PushBack(part);
            part->m_assembly = brick;
            rigid->m_parts.PushBack(RefPointer<SS1::Part>(part));
            bool found = false;
            UInt32 c = 0;
            for (; c < bag.GetSize(); ++c) {
                if (bag[c].m_part.Get() == part) {
                    found = true;
                    break;
                }
            }
            assert(found);   // ModelIOLDraw.cpp line 231 (Mac)
            bag.Erase(c);
        }
    }

    // Everything goes into one rigid of one model.
    scene.m_models.PushBack(SS1::Model());
    scene.m_models.Back().m_rigids.PushBack(rigid);
    return true;
}

// ============================================================================================
// ModelSaverLDraw
// ============================================================================================

ModelSaverLDraw::~ModelSaverLDraw()                                          // Mac 0x2fc040, Win 0x6037b6
{
}

bool ModelSaverLDraw::LoadMapping()                                          // Mac 0x2fb160, Win 0x5a6f10
{
    Path path;
    String message;   // NOTE: the reason is dropped (no message member in the saver)
    if (!MappingPath(path, message))
        return false;
    LdrawMapXMLLoader loader;
    return loader.Load(path, m_bricks, m_materials, m_transformations, m_assemblies);
}

bool ModelSaverLDraw::GetLDrawName(String& file, UInt32 designID)           // Mac 0x2fb740, Win 0x5a70a0
{
    LdrawMapXMLLoader::BrickNameMap::Iterator it = m_bricks.Find(designID);
    if (it != m_bricks.End()) {
        file = it->second;
        return true;
    }
    String name;
    file = name << designID << ".dat";
    return true;
}

bool ModelSaverLDraw::LookupColor(UInt32 material, UInt32& color)           // Win 0x5a7190 (inlined on Mac)
{
    LdrawMapXMLLoader::MaterialMap::Iterator it = m_materials.Find(material);
    if (it == m_materials.End())
        return false;
    color = it->second;
    return true;
}

bool ModelSaverLDraw::ToLDrawTransformation(Matrix3D& ldraw, const Matrix3D& lxf,
                                            const String& name)              // Mac 0x2fb820, Win 0x5a7200
{
    Matrix3D transformation(lxf);
    LdrawMapXMLLoader::TransformationMap::Iterator it = m_transformations.Find(name);
    if (it == m_transformations.End())
        return false;
    Matrix3D inverse;
    inverse.SetInverse(it->second);
    transformation *= inverse;
    transformation.m[12] *= kLDrawUnitsPerLDDUnit;
    transformation.m[13] *= kLDrawUnitsPerLDDUnit;
    transformation.m[14] *= kLDrawUnitsPerLDDUnit;
    Vector3 axis;
    Real32 angle;
    transformation.GetAxisAngle(axis, angle);
    axis.y = -axis.y;
    axis.z = -axis.z;
    angle = -angle;
    ldraw.SetRotation(axis, angle);
    ldraw.SetTranslation(transformation.GetTranslation());
    ldraw.m[13] = -ldraw.m[13];
    ldraw.m[14] = -ldraw.m[14];
    return true;
}

bool ModelSaverLDraw::ConvertPart(const SS1::Part* part, const Matrix3D& rigid, UInt32& color,
                                  Matrix3D& ldraw, String& file)             // Mac 0x2fb680, Win 0x5a6e70
{
    GetLDrawName(file, part->m_designID);
    if (!part->m_materials.IsEmpty() && !LookupColor(part->m_materials[0], color))
        return false;
    Matrix3D lxf(rigid);
    lxf *= part->m_transformation;
    return ToLDrawTransformation(ldraw, lxf, file);
}

bool ModelSaverLDraw::WritePart(String& out, const Matrix3D& rigid, const SS1::Part* part) // Mac 0x2fb3c0, Win 0x5a6ce0
{
    Matrix3D ldraw;
    String file;
    UInt32 color = 0;   // NOTE: uninitialised in both builds (see ConvertPart)
    if (ConvertPart(part, rigid, color, ldraw, file)) {
        const Real32* m = ldraw.m;
        out << "1" << " " << color << " " << m[12] << " " << m[13] << " " << m[14]
            << " " << m[0] << " " << m[1] << " " << m[2]
            << " " << m[4] << " " << m[5] << " " << m[6]
            << " " << m[8] << " " << m[9] << " " << m[10]
            << " " << file << "\r\n";
    } else {
        String message("Unsaved part: ");
        message << part->m_designID;
    }
    return true;
}

bool ModelSaverLDraw::WriteModel(String& out, const SS1::Model& model)      // Win 0x5a6c70 (inlined on Mac)
{
    for (UInt32 r = 0; r < model.m_rigids.GetSize(); ++r) {
        const SS1::Rigid* rigid = model.m_rigids[r].Get();
        for (UInt32 p = 0; p < rigid->m_parts.GetSize(); ++p)
            if (!WritePart(out, rigid->m_transformation, rigid->m_parts[p].Get()))
                return false;   // (never: WritePart is always true)
    }
    return true;
}

bool ModelSaverLDraw::Save(const Path& path, const StorageScene& scene)    // Mac 0x2fabd0, Win 0x5a64a0
{
    String out;
    if (!LoadMapping())
        return false;
    if (!scene.m_name.IsEmpty())
        out << "0 " << scene.m_name << "\r\n";

    String author, versionMajor, versionMinor;
    for (UInt32 i = 0; i < scene.m_metas.GetSize(); ++i) {
        const StorageScene2::Meta& meta = scene.m_metas[i];
        if (!(meta.m_name == String("Application")))
            continue;
        for (UInt32 a = 0; a < meta.m_attributes.GetSize(); ++a) {
            const StorageScene2::Meta::Attribute& attribute = meta.m_attributes[a];
            if (attribute.m_name == String("name"))
                author = attribute.m_value;
            if (attribute.m_name == String("versionMajor"))
                versionMajor = attribute.m_value;
            if (attribute.m_name == String("versionMinor"))
                versionMinor = attribute.m_value;
        }
    }
    if (!author.IsEmpty()) {
        out << "0 Author: " << author;
        if (!versionMajor.IsEmpty()) {
            out << " " << versionMajor;
            if (!versionMinor.IsEmpty())
                out << "." << versionMinor;
        }
        out << "\r\n";
    }

    for (UInt32 i = 0; i < scene.m_models.GetSize(); ++i)
        if (!WriteModel(out, scene.m_models[i]))
            return false;

    AString text = EncodeString(kTextEncodingUTF8, out);
    RefPointer<File> file = path.OpenFile(OpenMode(false, true), true);
    bool result = false;
    if (!file.IsNull()) {
        if (file->Write(text.GetLength(), text.GetBuffer(), 0)) {
            result = true;
            file->SetSize(text.GetLength());
        }
    }
    return result;
}

} // namespace LEGO
