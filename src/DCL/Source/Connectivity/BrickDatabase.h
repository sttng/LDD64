// LEGO Digital Designer — restored source
// LEGO::BrickDatabase — the db.lif brick database: colours (MaterialManual), primitives
// (Primitives/*.xml, parsed by LoadXMLOperation on an OperationQueue), the placeable bricks (one
// per primitive outside an assembly + one per Assemblies/*.lxfml), design id aliases,
// decorations (Decorations/*, DecorationMapping.xml), the db version (info.xml) and the geometry
// files (Primitives/Lod<n>/<designID>.g<face>).
// Original: Mac TU 024 "BrickDatabase.cpp" (0x71fc0-0x920c0), Win 0x4b29e0-0x4c0b60; restored as
// MaterialManual.cpp (024a + pieces), BrickDatabase.cpp (024b-d), PrimitiveLoaderXML.cpp (024e)
// and Primitive.h (024f inline code).
// Names from RTTI: BrickDatabase, BrickDatabase::Brick, ::Part, ::AssemblyPartMapping,
// ::LoadXMLOperation; method names come from two error strings
// (QueryDecorationFaceGeometry, QueryMaterialGeometry); every other name is inferred.
#pragma once
#include "LEGO/Types.h"
#include "Object.h"
#include "BasicArray.h"
#include "BasicMap.h"
#include "BasicString.h"
#include "LEGO/String.h"
#include "Geometry.h"
#include "Version.h"
#include "MaterialManual.h"
#include "Primitive.h"
#include "StorageScene2.h"
#include "OperationManager.h"

struct XML_ParserStruct;   // Expat 2.0.1 (XML_Parser)

namespace LEGO {

class Directory;
class Path;
class BrickGeometry;

// RTTI: class LEGO::BrickDatabase : virtual Object.
// Mac sizeof 0xb8: vptr +0 (vtable 0x761c8c {D1 0x889c0, D0 0x88a20}), Object at +0xb0
//   (vtable 0x761cac {virtual thunks 0x88a90 / 0x88b00, Object slots}).
// Win sizeof 0xe0: vbptr +0 (0xa24f70), Object at +0xd8 (vftable 0xa24f60 {sdd 0x4b2c60, …};
//   destructor body 0x4b2a10).
class BrickDatabase : public virtual Object {
public:
    typedef Primitive::AnnotationMap AnnotationMap;

    // One part of a Brick: its primitive and the transformation of every bone (one identity
    // matrix for a rigid primitive). sizeof 0x14 (both). Destructor of the array: Mac 0x89200.
    struct Part {
        Part() : m_designID(0) {}
        UInt32               m_designID;          // +0x00
        BasicArray<Matrix3D> m_boneTransforms;    // +0x04
    };

    // How a part of an assembly is placed relative to another one (<Meta><PartMap part offset
    // relativeTo atIndex relativeToOffset/> of the assembly file). The loader (ParsePartMap)
    // resolves the design ids to indices in the assembly's part list (sorted by design id).
    // sizeof 0x24 (both); push_back Mac 0x83a00.
    struct AssemblyPartMapping {
        UInt32  m_relativeToIndex;    // +0x00 index of the "relativeTo" part (+ "atIndex")
        Vector3 m_relativeToOffset;   // +0x04 "relativeToOffset", in that part's space
        UInt32  m_partIndex;          // +0x10 first candidate index for the "part" design id
        UInt32  m_candidateCount;     // +0x14 parts with that design id not yet mapped
        Vector3 m_offset;             // +0x18 "offset", in the part's own space
    };

    // A brick of the palette: a primitive (one part) or an assembly (Assemblies/*.lxfml).
    // sizeof Mac 0x88 / Win 0x90. Constructor Win 0x4b8f20, destructor Win 0x4b8fd0 / Mac
    // 0x8a320, assignment Mac 0x81a00 / Win 0x4b90d0 (all implicit).
    // NOTE: m_designID is not initialised by the constructor (both builds); fixed: 0 (low).
    struct Brick {
        Brick() : m_designID(0) {}
        UInt32                          m_designID;            // +0x00
        BasicArray<Part>                m_parts;               // +0x04
        Matrix3D                        m_defaultOrientation;  // +0x14 <DefaultOrientation>
        Vector2                         m_defaultCamera;       // +0x54 (lat, lon) <DefaultCamera>
        AnnotationMap                   m_annotations;         // +0x5c (std::map at +0x60)
        BasicArray<AssemblyPartMapping> m_partMappings;        // Mac +0x78 / Win +0x80
    };

    typedef BasicMap<UInt32, Brick>                     BrickMap;          // Mac vtable 0x762088
    typedef BasicMap<UInt32, String>                    DecorationMap;     // Mac vtable 0x769238
    typedef BasicMultimap<Pair<UInt32, UInt32>, UInt32> DecorationMapping; // Mac vtable 0x7620a8
    typedef BasicMap<UInt32, UInt32>                    AliasMap;          // Mac vtable 0x7620c0
    typedef BasicMap<UInt32, RefPointer<Primitive> >    PrimitiveMap;      // Mac vtable 0x7620d8

    // State of the LEGOPrimitive SAX parser (the Expat user data; name inferred). Part of
    // LoadXMLOperation at +0x3c, sizeof 0x68 (both). Handlers in PrimitiveLoaderXML.cpp.
    struct PrimitiveParser {
        // parser states (values from the handlers; names inferred)
        enum State {
            StateDocument      = 0,   // before <LEGOPrimitive> / after </LEGOPrimitive>
            StatePrimitive     = 1,   // inside <LEGOPrimitive>
            StateBounding      = 2,   // <Bounding>
            StateGeometryBounding = 3,// <GeometryBounding>
            StateCollision     = 4,   // <Collision>
            StateConnectivity  = 5,   // <Connectivity>
            StatePaths         = 6,   // <Paths>
            StateFlex          = 7,   // <Flex>
            StateBone          = 8,   // <Flex><Bone>
            StateAnnotations   = 9    // <Annotations>
        };

        Primitive*         m_primitive;            // +0x00
        XML_ParserStruct** m_parser;               // +0x04 &LoadXMLOperation::m_parser
        Matrix3D           m_defaultOrientation;   // +0x08 <DefaultOrientation>
        Vector2            m_defaultCamera;        // +0x48 <DefaultCamera lat lon>, default
                                                   //       (-0.800467, 0.765663)
        bool               m_ok;                   // +0x50 cleared on the first error
        UInt32             m_versionMajor;         // +0x54 <LEGOPrimitive versionMajor>
        UInt32             m_versionMinor;         // +0x58
        Int32              m_bone;                 // +0x5c current <Bone boneId>, -1 outside
        UInt32             m_state;                // +0x60 State
        AString            m_text;                 // +0x64 character data (<Custom2DField>, <Path>)
    };

    class LoadXMLOperation;

    // Reads info.xml (<DB><Bricks version=…/>) and builds the MaterialManual; fills the
    // connection tables (ConnectionFieldPlanar / ConnectionFieldAxle::InitializeTables).
    explicit BrickDatabase(Directory* directory);           // Mac 0x7dae0, Win 0x4b7c10 (thiscall, + most-derived flag)
    virtual ~BrickDatabase();                               // Mac 0x889c0 / 0x88a20, Win 0x4b2a10 / 0x4b2c60

    // Primitives/*.xml (all parsed through an OperationQueue), Assemblies/*.lxfml when
    // `loadAssemblies`, the bricks, then Decorations/ + DecorationMapping.xml unless
    // `noDecorations` — which instead releases the database and primitive directories (no
    // geometry access afterwards). false only when there is no Primitives directory.
    bool Load(bool noDecorations, bool loadAssemblies);     // Mac 0x7e100, Win 0x4b7f00 (thiscall, ret 8)

    // ---- queries ----------------------------------------------------------------------------
    const Primitive* GetPrimitive(UInt32 designID) const;   // Mac 0x81c30, Win 0x4b9180
    const Brick*     GetBrick(UInt32 designID) const;       // Mac 0x81ca0, Win 0x4b91f0
    // Ids of all bricks, ascending.
    void   GetBrickIDs(BasicArray<UInt32>& ids) const;      // Mac 0x82160, Win 0x4b9540
    // Design id an alias (or a design id) stands for; 0 if unknown. (Mac inlined)
    UInt32 ResolveAlias(UInt32 designID) const;             // Win 0x4b95d0
    // "designname_<language preference>" annotation of the brick, else "designname"; empty
    // for an unknown brick.
    String GetLocalizedName(UInt32 designID) const;         // Mac 0x81d00, Win 0x4b9260
    static String GetLocalizedName(const Brick& brick);     // Mac 0x7d670, Win 0x4b7a30

    // MaterialManual forwards (false without a MaterialManual).
    bool GetMaterialColor(UInt32 materialID, MaterialColor& color) const;                    // Mac 0x81d90, Win 0x4b9300
    bool GetMaterialType(UInt32 materialID, MaterialManual::MaterialType& type) const;       // Mac 0x81dc0, Win 0x4b9320
    bool HasMaterial(UInt32 materialID) const;                                               // Mac 0x81e30, Win 0x4b9340
    bool IsMaterialTransparent(UInt32 materialID) const;                                     // Mac 0x81ea0, Win 0x4b9360
    bool IsMaterialObsolete(UInt32 materialID) const;                                        // Mac 0x81f40, Win 0x4b9390

    bool HasDecoration(UInt32 decorationID) const;                                           // Mac 0x81b60, Win 0x4b94c0
    // Path of the decoration image in Decorations/; false if unknown or not loaded.
    bool GetDecorationPath(UInt32 decorationID, Path& path) const;                           // Mac 0x81fb0, Win 0x4b93d0
    // `surface` < the primitive's decoration face count.
    bool IsValidDecorationSurface(UInt32 designID, UInt32 surface) const;                    // Mac 0x820f0, Win 0x4b9510

    // Geometry "Lod<lod>/<designID>.g<face>" / "Lod<lod>/<designID>.g" of the primitives
    // directory: a new BrickGeometry (not referenced; NULL when the Lod directory or the file
    // is missing or does not load).
    BrickGeometry* QueryDecorationFaceGeometry(const Primitive* primitive, UInt32 face, UInt32 lod) const; // Mac 0x82a20, Win 0x4b99f0
    BrickGeometry* QueryMaterialGeometry(const Primitive* primitive, UInt32 lod) const;      // Mac 0x82d10, Win 0x4b9c60

    // Checks a loaded model against the database: aliases replaced by design ids, material
    // and decoration arrays sized to the primitives' face counts, the parts of an assembly
    // put into database order and aligned with its part map. ValidateScene ignores the
    // results.
    void ValidateScene(StorageScene2& scene) const;                                          // Mac 0x82430, Win 0x4b9640
    bool ValidateBrick(StorageScene2::Brick& brick) const;                                   // Mac 0x82470, Win 0x4b9670

    // ---- accessors (inline) -------------------------------------------------------------------
    const BrickMap&          GetBricks() const { return m_bricks; }
    const PrimitiveMap&      GetPrimitives() const { return m_primitives; }
    const AliasMap&          GetAliases() const { return m_aliases; }
    const DecorationMap&     GetDecorations() const { return m_decorations; }
    const DecorationMapping& GetDecorationMapping() const { return m_decorationMapping; }
    const MaterialManual*    GetMaterialManual() const { return m_materials; }
    const Version&           GetVersion() const { return m_version; }
    Directory*               GetDirectory() const { return m_directory; }

private:
    typedef BasicMap<UInt32, Triple<UInt32, UInt32, UInt32> > PartIndexMap;   // Mac vtable 0x762120

    // Assemblies/*.lxfml -> m_bricks; the design ids of all their parts go to `assemblyParts`.
    bool LoadAssemblies(const String& directoryName, BasicSet<UInt32>& assemblyParts);       // Mac 0x7fe10, Win 0x4ba2f0
    // m_aliases[designID] = designID and every "aliases" annotation entry.
    void RegisterAliases(UInt32 designID, const AnnotationMap& annotations);                // Mac 0x7fc10, Win 0x4b97b0
    // One <PartMap> meta element.
    static bool ParsePartMap(const StorageScene2::Meta& meta, PartIndexMap& indices,
                             BasicArray<AssemblyPartMapping>& mappings);                     // Mac 0x83330, Win 0x4bb850
    // Moves the candidate whose `offset` lies within 0.1 of the reference point to
    // parts[partIndex]; false when none does.
    static bool AlignPart(BasicArray<RefPointer<StorageScene2::Part> >& parts, UInt32 relativeToIndex,
                          const Vector3& relativeToOffset, UInt32 partIndex, UInt32 count,
                          const Vector3& offset);                                            // Mac 0x83060, Win 0x4ba190
    // Puts `parts` into the order of the database parts (equal design ids).
    static bool ReorderParts(const BasicArray<Part>& order,
                             BasicArray<RefPointer<StorageScene2::Part> >& parts);           // Mac 0x827a0, Win 0x4b9ee0

    BrickDatabase(const BrickDatabase&);
    BrickDatabase& operator=(const BrickDatabase&);

    BrickMap                   m_bricks;                // Mac +0x04 / Win +0x04
    RefPointer<MaterialManual> m_materials;             // Mac +0x20 / Win +0x28
    DecorationMap              m_decorations;           // Mac +0x24 / Win +0x2c decoration id -> file name
    RefPointer<Directory>      m_directory;             // Mac +0x40 / Win +0x50 db root
    RefPointer<Directory>      m_primitivesDirectory;   // Mac +0x44 / Win +0x54 "Primitives"
    RefPointer<Directory>      m_decorationsDirectory;  // Mac +0x48 / Win +0x58 "Decorations"
    Version                    m_version;               // Mac +0x4c / Win +0x5c info.xml <Bricks version>
    DecorationMapping          m_decorationMapping;     // Mac +0x5c / Win +0x6c (designID, surfaceID) -> decorationID
    AliasMap                   m_aliases;               // Mac +0x78 / Win +0x90 alias -> design id
    PrimitiveMap               m_primitives;            // Mac +0x94 / Win +0xb4
};

// Parses one Primitives/<designID>.xml (the file contents in m_text) on the OperationQueue.
// RTTI: class LEGO::BrickDatabase::LoadXMLOperation : Operation. Mac sizeof 0xb0 (Object at
// +0xa8; vtables 0x705e2c {Execute 0x88610, D1 0x886a0, D0 0x88700}, 0x705e50 {thunks 0x88770,
// 0x887e0, Object slots}); Win sizeof 0xb0 (vftable 0xa24fac {Execute 0x4bf120}, Object vftable
// 0xa24fb4 {sdd 0x4b2ca0}). Implemented in PrimitiveLoaderXML.cpp.
class BrickDatabase::LoadXMLOperation : public Operation {
public:
    // A new Primitive(database, designID) and an Expat parser with the LEGOPrimitive handlers.
    LoadXMLOperation(BrickDatabase* database, UInt32 designID);   // Mac 0x7f810, Win 0x4befb0
    // Mac 0x886a0 (D0 0x88700, thunks 0x88770 / 0x887e0), Win 0x4b2bb0 (sdd 0x4b2ca0).
    // NOTE: the Expat parser is freed by Execute only (an operation that never ran leaks it,
    // both builds); fixed: freed here when Execute did not run (low).
    virtual ~LoadXMLOperation();
    // Parses m_text, frees the parser and the text; m_succeeded = parse ok && no handler error.
    virtual void Execute();                                       // Mac 0x88610, Win 0x4bf120

    const RefPointer<Primitive>& GetPrimitive() const { return m_primitive; }
    const PrimitiveParser&       GetParser() const { return m_context; }
    bool                         Succeeded() const { return m_succeeded; }

    AString               m_text;        // +0x30 file contents (ReadFile)
    RefPointer<Primitive> m_primitive;   // +0x34
    XML_ParserStruct*     m_parser;      // +0x38
    PrimitiveParser       m_context;     // +0x3c
    bool                  m_succeeded;   // +0xa4
};

} // namespace LEGO
