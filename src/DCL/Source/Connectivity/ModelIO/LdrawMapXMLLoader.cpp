// LEGO Digital Designer — restored source
// LEGO::LdrawMapXMLLoader / LDrawLXFContext / LDrawLXFLoadContext / LDrawLXFSaveContext
// (Mac TU 334, Win 0x5b3d70-0x5b5930). See the header for the file format.
#include "LdrawMapXMLLoader.h"
#include "LEGO/StdLib.h"
#include "Path.h"
#include "ReadFile.h"

#ifndef XML_STATIC   // (the projects define it as well)
#define XML_STATIC   // Expat is linked statically into both programs
#endif
#include "expat.h"

#include <iostream>

namespace LEGO {

// ============================================================================================
// LdrawMapXMLLoader
// ============================================================================================

bool LdrawMapXMLLoader::Load(const Path& path, BrickMap& bricks, MaterialMap& materials,
                             TransformationMap& transformations, AssemblyMap& assemblies) // Mac 0x2ebeb0, Win 0x5b5450
{
    bricks.Clear();
    materials.Clear();
    transformations.Clear();
    assemblies.Clear();
    XML_Parser parser = XML_ParserCreate(0);
    LDrawLXFLoadContext context(&parser, bricks, materials, transformations, assemblies);
    return Parse(path, context);
}

bool LdrawMapXMLLoader::Load(const Path& path, BrickNameMap& bricks, MaterialMap& materials,
                             TransformationMap& transformations, AssemblyMap& assemblies) // Mac 0x2ec260, Win 0x5b55c0
{
    bricks.Clear();
    materials.Clear();
    transformations.Clear();
    assemblies.Clear();
    XML_Parser parser = XML_ParserCreate(0);
    LDrawLXFSaveContext context(&parser, bricks, materials, transformations, assemblies);
    return Parse(path, context);
}

bool LdrawMapXMLLoader::Parse(const Path& path, LDrawLXFContext& context)    // Mac 0x2ec0b0, Win 0x5b5780
{
    AString data;
    if (!ReadFile(path, data)) {
        // NOTE: the parser is not freed on this path (both builds).
        String message("Could not read file: ");
        m_error = message << path.GetPath(true);
        return false;
    }
    XML_Parser parser = *context.m_parser;
    XML_SetUserData(parser, &context);
    context.PushHandlers(LDrawLXFContext::OnRootStart, LDrawLXFContext::OnRootEnd);
    const char* text = data.GetBuffer();
    if (!text)
        text = "";
    if (XML_Parse(parser, text, (int)data.GetLength(), 1) != XML_STATUS_ERROR) {
        XML_ParserFree(parser);
        return true;
    }
    m_error = context.m_error;
    XML_ParserFree(parser);
    return false;
}

// ============================================================================================
// LDrawLXFContext
// ============================================================================================

LDrawLXFContext::LDrawLXFContext(XML_ParserStruct** parser)                 // Mac 0x2ec6a0
    // NOTE: the version fields are not initialised in the original (both builds): a mapping
    // root without a version attribute compares stack garbage with 1. Restored as 0 (the file
    // is then rejected, which is what the garbage practically always gives).
    : m_versionMajor(0), m_versionMinor(0), m_parser(parser)
{
}

void LDrawLXFContext::PushHandlers(StartHandler start, EndHandler end)     // Mac 0x2ec460
{
    m_handlers.PushBack(Handlers(start, end));
    XML_SetElementHandler(*m_parser, m_handlers.Back().first, m_handlers.Back().second);
}

void LDrawLXFContext::PopHandlers()                                          // inlined (Mac 0x2ef039…, Win 0x5b4db0…)
{
    m_handlers.PopBack();
    XML_SetElementHandler(*m_parser, m_handlers.Back().first, m_handlers.Back().second);
}

void LDrawLXFContext::Fail(const char* message)                             // inlined
{
    m_error = message;
    XML_StopParser(*m_parser, XML_FALSE);
}

void LDrawLXFContext::OnRootStart(void* userData, const char* name, const char** atts) // Mac 0x2ec540, Win 0x5b4160
{
    LDrawLXFContext* context = static_cast<LDrawLXFContext*>(userData);
    context->m_error = "";
    if (StrCmp(name, "LDrawMapping") != 0) {
        context->PushHandlers(OnIgnoredStart, OnIgnoredEnd);
        return;
    }
    for (; atts[0]; atts += 2) {
        if (StrCmp(atts[0], "versionMajor") == 0)
            context->m_versionMajor = StrToUInt32(atts[1], 0, 10);
        else if (StrCmp(atts[0], "versionMinor") == 0)
            context->m_versionMinor = StrToUInt32(atts[1], 0, 10);
        else if (StrCmp(atts[0], "version") == 0)
            context->m_versionMajor = StrToUInt32(atts[1], 0, 10);
    }
    if (context->m_versionMajor != 1) {
        context->Fail("Invalid XML version");
        return;
    }
    context->PushHandlers(OnMappingStart, OnMappingEnd);
}

void LDrawLXFContext::OnRootEnd(void*, const char*)                          // Mac 0x2ec690, Win 0x459790
{
}

void LDrawLXFContext::OnMappingStart(void* userData, const char* name, const char** atts) // Mac 0x2ee580, Win 0x5b43a0
{
    LDrawLXFContext* context = static_cast<LDrawLXFContext*>(userData);
    if (StrCmp(name, "Material") == 0) {
        UInt32 ldraw = 0xFFFFFFFF;
        UInt32 lego = 0xFFFFFFFF;
        for (; atts[0]; atts += 2) {
            if (StrCmp(atts[0], "ldraw") == 0)
                ldraw = StrToUInt32(atts[1], 0, 10);
            else if (StrCmp(atts[0], "lego") == 0)
                lego = StrToUInt32(atts[1], 0, 10);
        }
        if (ldraw != 0xFFFFFFFF && lego != 0xFFFFFFFF)
            context->AddMaterial(ldraw, lego);
        else
            context->Fail("Invalid color mapping values");
    } else if (StrCmp(name, "Brick") == 0) {
        String ldraw;
        UInt32 lego = 0xFFFFFFFF;
        for (; atts[0]; atts += 2) {
            if (StrCmp(atts[0], "ldraw") == 0)
                ldraw = String(atts[1]);
            else if (StrCmp(atts[0], "lego") == 0)
                lego = StrToUInt32(atts[1], 0, 10);
        }
        if (ldraw != String() && lego != 0xFFFFFFFF)
            context->AddBrick(String(ldraw), lego);
        else
            context->Fail("Invalid partId mapping values");
    } else if (StrCmp(name, "Assembly") == 0) {
        String ldraw;
        UInt32 lego = 0;
        Real32 tx = 0.0f, ty = 0.0f, tz = 0.0f;
        for (; atts[0]; atts += 2) {
            if (StrCmp(atts[0], "ldraw") == 0)
                ldraw = String(atts[1]);
            else if (StrCmp(atts[0], "lego") == 0)
                lego = StrToUInt32(atts[1], 0, 10);
            else if (StrCmp(atts[0], "tx") == 0)
                tx = StrToReal32(atts[1], 0);
            else if (StrCmp(atts[0], "ty") == 0)
                ty = StrToReal32(atts[1], 0);
            else if (StrCmp(atts[0], "tz") == 0)
                tz = StrToReal32(atts[1], 0);
        }
        if (ldraw != String() && lego != 0) {
            context->AddAssembly(String(ldraw), lego, Vector3(tx, ty, tz));
            context->PushHandlers(OnAssemblyStart, OnAssemblyEnd);
        } else {
            context->Fail("Invalid ldrawPart or LEGO assembly id");
        }
    } else if (StrCmp(name, "Transformation") == 0) {
        String ldraw;
        Real32 tx = 0.0f, ty = 0.0f, tz = 0.0f;
        Real32 ax = 0.0f, ay = 1.0f, az = 0.0f, angle = 0.0f;
        for (; atts[0]; atts += 2) {
            if (StrCmp(atts[0], "ldraw") == 0)
                ldraw = String(atts[1]);
            else if (StrCmp(atts[0], "tx") == 0)
                tx = StrToReal32(atts[1], 0);
            else if (StrCmp(atts[0], "ty") == 0)
                ty = StrToReal32(atts[1], 0);
            else if (StrCmp(atts[0], "tz") == 0)
                tz = StrToReal32(atts[1], 0);
            else if (StrCmp(atts[0], "ax") == 0)
                ax = StrToReal32(atts[1], 0);
            else if (StrCmp(atts[0], "ay") == 0)
                ay = StrToReal32(atts[1], 0);
            else if (StrCmp(atts[0], "az") == 0)
                az = StrToReal32(atts[1], 0);
            else if (StrCmp(atts[0], "angle") == 0)
                angle = StrToReal32(atts[1], 0);
        }
        // NOTE: no check: a <Transformation> without "ldraw" is stored under "".
        Matrix3D transformation;
        transformation.SetRotation(Vector3(ax, ay, az), angle);
        transformation.Translate(Vector3(tx, ty, tz));
        context->AddTransformation(String(ldraw), Matrix3D(transformation));
    } else {
        context->PushHandlers(OnIgnoredStart, OnIgnoredEnd);
    }
}

void LDrawLXFContext::OnMappingEnd(void* userData, const char* name)         // Mac 0x2ef010, Win 0x5b4db0
{
    // Only the mapping element itself pops; <Material>, <Brick> and <Transformation> never
    // pushed handlers.
    if (StrCmp(name, "LDrawMapping") == 0)
        static_cast<LDrawLXFContext*>(userData)->PopHandlers();
}

void LDrawLXFContext::OnAssemblyStart(void* userData, const char* name, const char** atts) // Mac 0x2ef180, Win 0x5b4e00
{
    LDrawLXFContext* context = static_cast<LDrawLXFContext*>(userData);
    if (StrCmp(name, "Part") != 0) {
        context->PushHandlers(OnIgnoredStart, OnIgnoredEnd);
        return;
    }
    String ldraw;
    UInt32 color = 0;
    Real32 v[12] = { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };
    static const char* const kNames[12] = {
        "m0", "m1", "m2", "m3", "m4", "m5", "m6", "m7", "m8", "m9", "m10", "m11"
    };
    for (; atts[0]; atts += 2) {
        if (StrCmp(atts[0], "ldraw") == 0) {
            ldraw = String(atts[1]);
        } else if (StrCmp(atts[0], "color") == 0) {
            color = StrToUInt32(atts[1], 0, 10);
        } else {
            for (UInt32 i = 0; i < 12; ++i) {
                if (StrCmp(atts[0], kNames[i]) == 0) {
                    v[i] = StrToReal32(atts[1], 0);
                    break;
                }
            }
        }
    }
    // m0-m2: translation; m3-m11: the 3x3 part, column by column.
    Matrix3D transformation;
    Real32* m = transformation.m;
    m[12] = v[0];  m[13] = v[1];  m[14] = v[2];
    m[0]  = v[3];  m[1]  = v[4];  m[2]  = v[5];
    m[4]  = v[6];  m[5]  = v[7];  m[6]  = v[8];
    m[8]  = v[9];  m[9]  = v[10]; m[10] = v[11];
    context->AddAssemblyPart(String(ldraw), color, Matrix3D(transformation));
}

void LDrawLXFContext::OnAssemblyEnd(void* userData, const char* name)        // Mac 0x2ef860, Win 0x5b5360
{
    if (StrCmp(name, "Assembly") == 0)
        static_cast<LDrawLXFContext*>(userData)->PopHandlers();
}

void LDrawLXFContext::OnIgnoredStart(void* userData, const char*, const char**) // Mac 0x2ef0c0, Win 0x5b53b0
{
    static_cast<LDrawLXFContext*>(userData)->PushHandlers(OnIgnoredStart, OnIgnoredEnd);
}

void LDrawLXFContext::OnIgnoredEnd(void* userData, const char*)              // Mac 0x2ef0f0, Win 0x5b5400
{
    static_cast<LDrawLXFContext*>(userData)->PopHandlers();
}

// ============================================================================================
// LDrawLXFLoadContext
// ============================================================================================

void LDrawLXFLoadContext::AddBrick(const String& ldraw, UInt32 lego)        // Mac 0x2ec920, Win 0x5b3d70
{
    (*m_bricks)[ldraw] = lego;
}

void LDrawLXFLoadContext::AddMaterial(UInt32 ldraw, UInt32 lego)            // Mac 0x2ec950, Win 0x5b3de0
{
    (*m_materials)[ldraw] = lego;
}

void LDrawLXFLoadContext::AddTransformation(const String& ldraw, const Matrix3D& transformation) // Mac 0x2ec9d0, Win 0x5b3e00
{
    (*m_transformations)[ldraw] = transformation;
}

void LDrawLXFLoadContext::AddAssemblyPart(const String& ldraw, UInt32 color, const Matrix3D& transformation) // Mac 0x2eca50, Win 0x5b3fe0
{
    m_assembly->m_parts.PushBack(LdrawMapXMLLoader::AssemblyPart(ldraw, color, transformation));
}

void LDrawLXFLoadContext::AddAssembly(const String& ldraw, UInt32 lego, const Vector3& center) // Mac 0x2ecae0, Win 0x5b4070
{
    // NOTE: a second <Assembly> with the same name replaces the parts of the first.
    LdrawMapXMLLoader::Assembly assembly;
    assembly.m_designID = lego;
    assembly.m_center = center;
    LdrawMapXMLLoader::Assembly& entry = (*m_assemblies)[ldraw];
    if (&entry != &assembly)
        entry.m_parts = assembly.m_parts;
    entry.m_designID = assembly.m_designID;
    entry.m_center = assembly.m_center;
    m_assembly = &(*m_assemblies)[ldraw];
}

// ============================================================================================
// LDrawLXFSaveContext
// ============================================================================================

void LDrawLXFSaveContext::AddBrick(const String& ldraw, UInt32 lego)        // Mac 0x2ee220, Win 0x5b3f50
{
    (*m_bricks)[lego] = ldraw;
}

void LDrawLXFSaveContext::AddMaterial(UInt32 ldraw, UInt32 lego)            // Mac 0x2ee260, Win 0x5b3fc0
{
    (*m_materials)[lego] = ldraw;
}

void LDrawLXFSaveContext::AddTransformation(const String& ldraw, const Matrix3D& transformation) // Mac 0x2ee2e0, Win 0x5b3e00
{
    (*m_transformations)[ldraw] = transformation;
}

void LDrawLXFSaveContext::AddAssemblyPart(const String& ldraw, UInt32 color, const Matrix3D& transformation) // Mac 0x2ee360, Win 0x5b3fe0
{
    m_assembly->m_parts.PushBack(LdrawMapXMLLoader::AssemblyPart(ldraw, color, transformation));
}

void LDrawLXFSaveContext::AddAssembly(const String& ldraw, UInt32 lego, const Vector3& center) // Mac 0x2ee3f0, Win 0x5b4070
{
    LdrawMapXMLLoader::Assembly assembly;
    assembly.m_designID = lego;
    assembly.m_center = center;
    LdrawMapXMLLoader::Assembly& entry = (*m_assemblies)[ldraw];
    if (&entry != &assembly)
        entry.m_parts = assembly.m_parts;
    entry.m_designID = assembly.m_designID;
    entry.m_center = assembly.m_center;
    m_assembly = &(*m_assemblies)[ldraw];
}

} // namespace LEGO
