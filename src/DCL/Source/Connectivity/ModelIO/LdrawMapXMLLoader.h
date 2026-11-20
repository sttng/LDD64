// LEGO Digital Designer — restored source
// The LDraw <-> LEGO mapping file (preference "LDrawMapFile", default "ldraw.xml" in the
// resources directory next to the program — not inside db.lif / Assets.lif):
//   LEGO::LdrawMapXMLLoader — reads the file into the lookup maps of ModelLoaderLDraw (LDraw
//                             names -> LEGO ids) or ModelSaverLDraw (LEGO ids -> LDraw names);
//   LEGO::LDrawLXFContext   — Expat SAX state with a stack of element handler pairs;
//   LEGO::LDrawLXFLoadContext / LDrawLXFSaveContext — store the entries in the load / save maps.
//
// Mac TU 334 (0x2ebeb0-0x2ef9af, `#include <iostream>` initialiser 0x2ef970; file name unknown),
// Win 0x5b3d70-0x5b5930 (+ map templates 0x5a24c0 …).
// The loader's destructors live in TU 337 (ModelIOLDraw.cpp: Mac 0x2faba0 / 0x2fbd50,
// Win sdd 0x5a4530).
//
// File format (element and attribute names compared exactly; numbers with StrToUInt32 /
// StrToReal32, so trailing garbage is ignored and a missing number is 0):
//   <LDrawMapping versionMajor="1" versionMinor="…">          ("version" = versionMajor;
//                                                              major != 1: "Invalid XML version")
//     <Material ldraw="15" lego="1"/>                          LDraw colour -> LEGO material
//     <Brick ldraw="3001.dat" lego="3001"/>                    LDraw file   -> LEGO design id
//     <Transformation ldraw="3001.dat" tx ty tz ax ay az angle/>  LXF origin of the LDraw part:
//                                                              rotation (axis ax/ay/az, default
//                                                              0,1,0; angle in radians), then
//                                                              translation tx/ty/tz (LDD units)
//     <Assembly ldraw="3829.dat" lego="73081" tx ty tz>        one LEGO brick made of several
//       <Part ldraw="3829a.dat" color="4" m0 … m11/>           LDraw parts (m0-2 translation,
//     </Assembly>                                              m3-11 the 3x3 part, by columns)
//   </LDrawMapping>
// Any other element (at any level) is skipped with its whole subtree.
//
// Offsets: Mac (String 16 bytes) / Win (String 4 bytes).
#pragma once
#include "LEGO/Types.h"
#include "Object.h"
#include "LEGO/String.h"
#include "BasicArray.h"
#include "BasicMap.h"
#include "BasicDeque.h"
#include "Geometry.h"

// Expat's parser handle (struct XML_ParserStruct*), without pulling expat.h into every user.
struct XML_ParserStruct;

namespace LEGO {

class Path;

// ---------------------------------------------------------------------------------------------
// LdrawMapXMLLoader. sizeof Mac 0x14 / Win 0x08.
// Mac vtable 0x777210 {D1 0x2faba0, D0 0x2fbd50}; Win vtable 0xa2cb40 {sdd 0x5a4530}.
// Used as a stack object (constructor inlined everywhere).
// ---------------------------------------------------------------------------------------------
class LdrawMapXMLLoader {
public:
    // One LDraw part of an <Assembly>. Mac 0x54 / Win 0x48.
    struct AssemblyPart {
        AssemblyPart() : m_color(0) {}
        AssemblyPart(const String& ldraw, UInt32 color, const Matrix3D& transformation)
            : m_ldraw(ldraw), m_color(color), m_transformation(transformation) {}

        String   m_ldraw;            // +0x00 "ldraw" file name
        UInt32   m_color;            // Mac +0x10 / Win +0x04 "color" (LDraw colour)
        Matrix3D m_transformation;   // Mac +0x14 / Win +0x08 m0-m11, relative to the assembly
    };

    // <Assembly>. Mac 0x20 / Win 0x20.
    struct Assembly {
        Assembly() : m_designID(0) {}

        BasicArray<AssemblyPart> m_parts;      // +0x00
        UInt32                   m_designID;   // +0x10 "lego"
        Vector3                  m_center;     // +0x14 tx/ty/tz: common point of the parts used
                                               // to match separate LDraw parts (assembly space)
    };

    // The four maps filled for reading LDraw files (ModelLoaderLDraw).
    typedef BasicMap<String, UInt32, StringCaseInsensitiveLess<String> >   BrickMap;           // ldraw -> design id
    typedef BasicMap<UInt32, UInt32>                                       MaterialMap;        // key -> value colour/material
    typedef BasicMap<String, Matrix3D, StringCaseInsensitiveLess<String> > TransformationMap;  // ldraw -> LXF offset
    typedef BasicMap<String, Assembly>                                     AssemblyMap;        // ldraw -> assembly
    // For writing (ModelSaverLDraw): design id -> ldraw name.
    typedef BasicMap<UInt32, String>                                       BrickNameMap;

    LdrawMapXMLLoader() {}
    virtual ~LdrawMapXMLLoader() {}                       // Mac 0x2faba0 (D0 0x2fbd50), Win sdd 0x5a4530

    // Empties the four maps and reads `path` into them: bricks[ldraw] = lego,
    // materials[ldraw colour] = lego material. On failure GetError() says why ("Could not read
    // file: <path>", "Invalid XML version", "Invalid color mapping values", …, or nothing for
    // malformed XML); entries read before the error stay in the maps. (name inferred)
    bool Load(const Path& path, BrickMap& bricks, MaterialMap& materials,
              TransformationMap& transformations, AssemblyMap& assemblies);       // Mac 0x2ebeb0, Win 0x5b5450
    // The same for writing: bricks[lego] = ldraw, materials[lego material] = ldraw colour
    // (several LDraw colours for one material: the last one wins). (name inferred)
    bool Load(const Path& path, BrickNameMap& bricks, MaterialMap& materials,
              TransformationMap& transformations, AssemblyMap& assemblies);       // Mac 0x2ec260, Win 0x5b55c0

    const String& GetError() const { return m_error; }   // (accessor, inferred)

private:
    friend class LDrawLXFContext;
    // Reads the file and runs Expat over it with `context`. NOTE: on a read error the parser
    // created by the caller is never freed (both builds). Mac 0x2ec0b0, Win 0x5b5780.
    // (name inferred)
    bool Parse(const Path& path, class LDrawLXFContext& context);

    String m_error;   // +0x04 (name inferred)
};

// ---------------------------------------------------------------------------------------------
// LDrawLXFContext — Expat user data while the mapping file is read. Abstract; the five slots
// store one entry each. NOTE: no virtual destructor (the Mac vtable 0x776e2c has exactly the
// five pure slots; Win 0xa2cd44); the contexts are stack objects.
// Mac layout: +0x04 m_versionMajor, +0x08 m_versionMinor, +0x0c String m_error, +0x1c parser,
// +0x20 BasicDeque (0x2c). Win: +0x0c String, +0x10 parser, +0x14 BasicDeque.
// ---------------------------------------------------------------------------------------------
class LDrawLXFContext {
public:
    typedef void (*StartHandler)(void* userData, const char* name, const char** atts);
    typedef void (*EndHandler)(void* userData, const char* name);
    typedef Pair<StartHandler, EndHandler> Handlers;

    // Mac 0x2ec6a0 (out of line), inlined on Win. `parser` is the caller's handle variable.
    // (name inferred)
    explicit LDrawLXFContext(XML_ParserStruct** parser);

    virtual void AddBrick(const String& ldraw, UInt32 lego) = 0;                          // slot 0
    virtual void AddMaterial(UInt32 ldraw, UInt32 lego) = 0;                              // slot 1
    virtual void AddTransformation(const String& ldraw, const Matrix3D& transformation) = 0; // slot 2
    // A <Part> of the assembly last added. (names of slots 0-4 inferred)
    virtual void AddAssemblyPart(const String& ldraw, UInt32 color, const Matrix3D& transformation) = 0; // slot 3
    virtual void AddAssembly(const String& ldraw, UInt32 lego, const Vector3& center) = 0;  // slot 4

    // Expat handlers (user data = the context). (names inferred)
    static void OnRootStart(void* userData, const char* name, const char** atts);       // Mac 0x2ec540, Win 0x5b4160
    static void OnRootEnd(void* userData, const char* name);                            // Mac 0x2ec690, Win 0x459790 (shared empty body)
    static void OnMappingStart(void* userData, const char* name, const char** atts);    // Mac 0x2ee580, Win 0x5b43a0
    static void OnMappingEnd(void* userData, const char* name);                         // Mac 0x2ef010, Win 0x5b4db0
    static void OnAssemblyStart(void* userData, const char* name, const char** atts);   // Mac 0x2ef180, Win 0x5b4e00
    static void OnAssemblyEnd(void* userData, const char* name);                        // Mac 0x2ef860, Win 0x5b5360
    static void OnIgnoredStart(void* userData, const char* name, const char** atts);    // Mac 0x2ef0c0, Win 0x5b53b0
    static void OnIgnoredEnd(void* userData, const char* name);                         // Mac 0x2ef0f0, Win 0x5b5400

protected:
    ~LDrawLXFContext() {}

private:
    friend class LdrawMapXMLLoader;

    // Installs a handler pair (kept on the stack until the matching end element).
    // Mac 0x2ec460; Win inlined (0x4e7850 push + 0x4e7800 back). (name inferred)
    void PushHandlers(StartHandler start, EndHandler end);
    // Drops the current pair and reinstalls the previous one (inlined in the end handlers).
    // (name inferred)
    void PopHandlers();
    // Stops the parser with `message` (inlined). (name inferred)
    void Fail(const char* message);

    UInt32               m_versionMajor;   // +0x04 (name inferred)
    UInt32               m_versionMinor;   // +0x08 (name inferred)
    String               m_error;          // +0x0c (name inferred)
    XML_ParserStruct**   m_parser;         // Mac +0x1c / Win +0x10
    BasicDeque<Handlers> m_handlers;       // Mac +0x20 / Win +0x14
};

// For ModelLoaderLDraw. Mac vtable 0x776df8 {0x2ec920, 0x2ec950, 0x2ec9d0, 0x2eca50, 0x2ecae0},
// Win 0xa2cd5c {0x5b3d70, 0x5b3de0, 0x5b3e00, 0x5b3fe0, 0x5b4070}. sizeof Mac 0x60.
class LDrawLXFLoadContext : public LDrawLXFContext {
public:
    LDrawLXFLoadContext(XML_ParserStruct** parser, LdrawMapXMLLoader::BrickMap& bricks,
                        LdrawMapXMLLoader::MaterialMap& materials,
                        LdrawMapXMLLoader::TransformationMap& transformations,
                        LdrawMapXMLLoader::AssemblyMap& assemblies)
        : LDrawLXFContext(parser), m_bricks(&bricks), m_materials(&materials),
          m_transformations(&transformations), m_assemblies(&assemblies), m_assembly(0) {}
    ~LDrawLXFLoadContext() {}

    virtual void AddBrick(const String& ldraw, UInt32 lego);                              // Mac 0x2ec920, Win 0x5b3d70
    virtual void AddMaterial(UInt32 ldraw, UInt32 lego);                                  // Mac 0x2ec950, Win 0x5b3de0
    virtual void AddTransformation(const String& ldraw, const Matrix3D& transformation);  // Mac 0x2ec9d0, Win 0x5b3e00
    virtual void AddAssemblyPart(const String& ldraw, UInt32 color, const Matrix3D& transformation); // Mac 0x2eca50, Win 0x5b3fe0
    virtual void AddAssembly(const String& ldraw, UInt32 lego, const Vector3& center);    // Mac 0x2ecae0, Win 0x5b4070

private:
    LdrawMapXMLLoader::BrickMap*          m_bricks;            // Mac +0x4c
    LdrawMapXMLLoader::MaterialMap*       m_materials;         // Mac +0x50
    LdrawMapXMLLoader::TransformationMap* m_transformations;   // Mac +0x54
    LdrawMapXMLLoader::AssemblyMap*       m_assemblies;        // Mac +0x58
    LdrawMapXMLLoader::Assembly*          m_assembly;          // Mac +0x5c current <Assembly>
};

// For ModelSaverLDraw. Mac vtable 0x776e48 {0x2ee220, 0x2ee260, 0x2ee2e0, 0x2ee360, 0x2ee3f0},
// Win 0xa2cd74 {0x5b3f50, 0x5b3fc0, 0x5b3e00, 0x5b3fe0, 0x5b4070} (slots 2-4 share the load
// context's bodies on Win). sizeof Mac 0x60.
class LDrawLXFSaveContext : public LDrawLXFContext {
public:
    LDrawLXFSaveContext(XML_ParserStruct** parser, LdrawMapXMLLoader::BrickNameMap& bricks,
                        LdrawMapXMLLoader::MaterialMap& materials,
                        LdrawMapXMLLoader::TransformationMap& transformations,
                        LdrawMapXMLLoader::AssemblyMap& assemblies)
        : LDrawLXFContext(parser), m_bricks(&bricks), m_materials(&materials),
          m_transformations(&transformations), m_assemblies(&assemblies), m_assembly(0) {}
    ~LDrawLXFSaveContext() {}

    virtual void AddBrick(const String& ldraw, UInt32 lego);                              // Mac 0x2ee220, Win 0x5b3f50
    virtual void AddMaterial(UInt32 ldraw, UInt32 lego);                                  // Mac 0x2ee260, Win 0x5b3fc0
    virtual void AddTransformation(const String& ldraw, const Matrix3D& transformation);  // Mac 0x2ee2e0, Win 0x5b3e00
    virtual void AddAssemblyPart(const String& ldraw, UInt32 color, const Matrix3D& transformation); // Mac 0x2ee360, Win 0x5b3fe0
    virtual void AddAssembly(const String& ldraw, UInt32 lego, const Vector3& center);    // Mac 0x2ee3f0, Win 0x5b4070

private:
    LdrawMapXMLLoader::BrickNameMap*      m_bricks;            // Mac +0x4c
    LdrawMapXMLLoader::MaterialMap*       m_materials;         // Mac +0x50
    LdrawMapXMLLoader::TransformationMap* m_transformations;   // Mac +0x54
    LdrawMapXMLLoader::AssemblyMap*       m_assemblies;        // Mac +0x58
    LdrawMapXMLLoader::Assembly*          m_assembly;          // Mac +0x5c
};

} // namespace LEGO
