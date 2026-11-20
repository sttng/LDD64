// LEGO Digital Designer — restored source
// TU 057 "BrickPaletteBagLoaderPAXML.cpp" (Mac 0x1298c0-0x12e78b): the .paxml palette loader and
// writer. See BrickPaletteBagLoaderPAXML.h for the format.
//
// Template instances of the TU restored through the shared templates: the BasicDeque of handler
// pairs (map init Mac 0x12b970, reallocation 0x12baa0, destructors 0x12b890 / 0x12b900, Win sdd
// 0x4e7110), BasicArray<StorageScene2::Meta> (+ ::Attribute) push / copy (0x12c960 helpers),
// BasicArray<RefPointer<BrickPalette::Part>>::PushBack (0x12db50), the PaletteName set insert
// (0x12e500 / 0x12e610), the Less<UInt32> set of Save.
// NOTE (both builds): the "Could not load / parse file", "Cannot Read palette version" messages
// are built and dropped (no trace call; presumably a compiled-out trace macro); the brick errors
// go to TraceVerbose.
#include "BrickPaletteBagLoaderPAXML.h"

#include "LEGO/StdLib.h"
#include "Trace.h"
#include "BasicDeque.h"
#include "StringStream.h"
#include "StringConversion.h"
#include "XMLDOMNode.h"
#include "Directory.h"
#include "File.h"
#include "Path.h"
#include "ReadFile.h"
#include "SafeFileWrite.h"   // (M2) Save
#include "BrickDatabase.h"
#include "Primitive.h"
#include "StorageScene2.h"
#include "Brand.h"
#include "LDDUtilities.h"

#ifndef XML_STATIC   // (the projects define it as well)
#define XML_STATIC   // Expat is linked statically into both programs
#endif
#include "expat.h"

#include <iostream>

namespace LEGO {

namespace {

typedef BrickPalette::Brick PaletteBrick;
typedef BrickPalette::Part  PalettePart;

// The material of new parts when the file / the caller gives none (21 = bright red).
const UInt32 kDefaultMaterial = 21;

// ---------------------------------------------------------------------------------------------
// The Expat user data of one .paxml parse (constructor Mac 0x12b590, Win 0x64e500; destructor
// Mac 0x12e410). Mac sizeof 0x68. (name inferred)
// ---------------------------------------------------------------------------------------------
struct PAXMLContext {
    typedef void (*StartHandler)(void* userData, const char* name, const char** atts);
    typedef void (*EndHandler)(void* userData, const char* name);
    typedef Pair<StartHandler, EndHandler> Handlers;

    PAXMLContext(BrickPalette::Bag* bag, const BrickDatabase* database, XML_Parser* parser)
        : m_bag(bag), m_quantity(0), m_database(database), m_parser(parser),
          // NOTE: m_rootFound (+0x24) is not initialised in the original; it is never read.
          m_rootFound(false), m_versionMajor(0), m_versionMinor(0), m_value30(0), m_value34(0),
          m_partDesignID(0)
    {
    }

    // Installs a handler pair for the children of the current element. Mac 0x12a5e0
    void PushHandlers(StartHandler start, EndHandler end)
    {
        m_handlers.PushBack(Handlers(start, end));
        XML_SetElementHandler(*m_parser, m_handlers.Back().first, m_handlers.Back().second);
    }
    // Back to the pair of the parent element (inlined in every end handler).
    void PopHandlers()
    {
        m_handlers.PopBack();
        XML_SetElementHandler(*m_parser, m_handlers.Back().first, m_handlers.Back().second);
    }
    // The current brick is dropped (inlined: RefPointer reset).
    void DropBrick() { m_brick = 0; }

    BrickPalette::Bag*        m_bag;           // +0x00
    RefPointer<PaletteBrick>         m_brick;         // +0x04 the <Brick> / <Assembly> being read
    UInt32                    m_quantity;      // +0x08 its "quantity"
    const BrickDatabase*      m_database;      // +0x0c
    XML_Parser*               m_parser;        // +0x10 the caller's parser variable
    BasicArray<StorageScene2::Meta> m_metas;   // +0x14 the <Meta> children (not used afterwards)
    bool                      m_rootFound;     // +0x24 <PAXML> seen
    UInt32                    m_versionMajor;  // +0x28
    UInt32                    m_versionMinor;  // +0x2c
    UInt32                    m_value30;       // +0x30 (zeroed, never used)
    UInt32                    m_value34;       // +0x34 (zeroed, never used)
    UInt32                    m_partDesignID;  // +0x38 the current <Assembly><Part designID>
    BasicDeque<Handlers>      m_handlers;      // +0x3c
};

inline PAXMLContext& C(void* userData) { return *static_cast<PAXMLContext*>(userData); }

void OnIgnoredStart(void* userData, const char* name, const char** atts);
void OnIgnoredEnd(void* userData, const char* name);
void OnPAXMLChildStart(void* userData, const char* name, const char** atts);
void OnPAXMLChildEnd(void* userData, const char* name);
void OnBagChildStart(void* userData, const char* name, const char** atts);
void OnBagChildEnd(void* userData, const char* name);
void OnMetaChildStart(void* userData, const char* name, const char** atts);
void OnMetaChildEnd(void* userData, const char* name);
void OnMetaValueChildStart(void* userData, const char* name, const char** atts);
void OnMetaValueChildEnd(void* userData, const char* name);
void OnMetaValueText(void* userData, const char* text, int length);
void OnBrickChildStart(void* userData, const char* name, const char** atts);
void OnBrickChildEnd(void* userData, const char* name);
void OnAssemblyChildStart(void* userData, const char* name, const char** atts);
void OnAssemblyChildEnd(void* userData, const char* name);
void OnPartChildStart(void* userData, const char* name, const char** atts);
void OnPartChildEnd(void* userData, const char* name);

// ---- skipped subtrees ---------------------------------------------------------------------------

// Mac 0x12bce0
void OnIgnoredStart(void* userData, const char*, const char**)
{
    C(userData).PushHandlers(OnIgnoredStart, OnIgnoredEnd);
}

// Mac 0x12bd10
void OnIgnoredEnd(void* userData, const char*)
{
    C(userData).PopHandlers();
}

// ---- document ------------------------------------------------------------------------------------

// <PAXML versionMajor versionMinor>. Mac 0x12a6c0
void OnRootStart(void* userData, const char* name, const char** atts)
{
    PAXMLContext& context = C(userData);
    if (StrCmp(name, "PAXML") != 0) {
        context.PushHandlers(OnIgnoredStart, OnIgnoredEnd);
        return;
    }
    context.m_rootFound = true;
    for (; atts[0]; atts += 2) {
        if (StrCmp(atts[0], "versionMajor") == 0)
            context.m_versionMajor = StrToUInt32(atts[1], 0, 10);
        else if (StrCmp(atts[0], "versionMinor") == 0)
            context.m_versionMinor = StrToUInt32(atts[1], 0, 10);
    }
    if (context.m_versionMajor != 1) {
        XML_StopParser(*context.m_parser, XML_FALSE);
        String error("Cannot Read palette version: ");
        error << context.m_versionMajor << "." << context.m_versionMinor;   // NOTE: dropped
        return;
    }
    context.PushHandlers(OnPAXMLChildStart, OnPAXMLChildEnd);
}

// Mac 0x12a870 (empty)
void OnRootEnd(void*, const char*)
{
}

// Children of <PAXML>: <Bag>. Mac 0x12bbd0
void OnPAXMLChildStart(void* userData, const char* name, const char**)
{
    if (StrCmp(name, "Bag") == 0)
        C(userData).PushHandlers(OnBagChildStart, OnBagChildEnd);
    else
        C(userData).PushHandlers(OnIgnoredStart, OnIgnoredEnd);
}

// Mac 0x12bc30. NOTE: compares with "Bag", not "PAXML" (both builds): </PAXML> pops nothing.
void OnPAXMLChildEnd(void* userData, const char* name)
{
    if (StrCmp(name, "Bag") == 0)
        C(userData).PopHandlers();
}

// ---- <Bag> ---------------------------------------------------------------------------------------

// The parts of `brick` from the database brick: one Part per database part (its bone
// transforms, max(1, decoration faces of the primitive) materials, the first one `*material` or
// 21 without one); the item number becomes the brick's name. False when the database has no
// such brick or it has no parts. Mac 0x12cc10 (database in ECX, brick in EDX)
bool CreateParts(const BrickDatabase* database, PaletteBrick* brick, const UInt32* material,
                 const String& itemNos)
{
    brick->m_name = itemNos;
    const BrickDatabase::Brick* entry = database->GetBrick(brick->m_designID);
    if (!entry)
        return false;
    for (BasicArray<BrickDatabase::Part>::ConstIterator it = entry->m_parts.Begin();
         it != entry->m_parts.End(); ++it) {
        RefPointer<PalettePart> part = new PalettePart();
        part->m_designID = it->m_designID;
        part->m_boneTransforms = it->m_boneTransforms;
        // NOTE: no check for a missing primitive (both builds).
        UInt32 count = database->GetPrimitive(part->m_designID)->m_decorationFaceCount;
        if (count == 0)
            count = 1;
        while (part->m_materials.GetSize() > count)
            part->m_materials.PopBack();
        while (part->m_materials.GetSize() < count)
            part->m_materials.PushBack(0);
        part->m_materials[0] = material ? *material : kDefaultMaterial;
        brick->m_parts.PushBack(part);
    }
    return !brick->m_parts.IsEmpty();
}

// The trace of a brick that is dropped, then the subtree is skipped.
void DropBrick(PAXMLContext& context, const String& message)
{
    TraceVerbose(message, true);
    context.DropBrick();
    context.PushHandlers(OnIgnoredStart, OnIgnoredEnd);
}

// Children of <Bag>: <Meta>, <Brick>, <Assembly>. Mac 0x12bda0
void OnBagChildStart(void* userData, const char* name, const char** atts)
{
    PAXMLContext& context = C(userData);
    if (StrCmp(name, "Meta") == 0) {
        context.PushHandlers(OnMetaChildStart, OnMetaChildEnd);
        return;
    }
    const bool brickElement = StrCmp(name, "Brick") == 0;
    if (!brickElement && StrCmp(name, "Assembly") != 0) {
        context.PushHandlers(OnIgnoredStart, OnIgnoredEnd);
        return;
    }

    context.m_quantity = 0;
    UInt32 designID = 0;
    UInt32 material = IsBrandLDDExtended((Brand)context.m_bag->m_brand) ? kDefaultMaterial : 0;
    String itemNos;
    for (; atts[0]; atts += 2) {
        if (StrCmp(atts[0], "designID") == 0)
            designID = StrToUInt32(atts[1], 0, 10);
        else if (brickElement && StrCmp(atts[0], "materialID") == 0)
            material = StrToUInt32(atts[1], 0, 10);
        else if (StrCmp(atts[0], "quantity") == 0)
            context.m_quantity = StrToUInt32(atts[1], 0, 10);
        else if (StrCmp(atts[0], "ItemNos") == 0)
            itemNos = atts[1];
    }
    context.m_brick = new PaletteBrick();
    if (!context.m_brick->Initialize(designID, *context.m_database)) {
        String message("Cannot create brick: ");
        message << designID << " from palette: " << context.m_bag->m_name;
        DropBrick(context, message);
        return;
    }

    if (brickElement) {
        if (!context.m_database->HasMaterial(material)) {
            String message("Invalid material used: ");
            message << material << " from palette: " << context.m_bag->m_name
                    << " - possibly for brick: " << context.m_brick->m_designID;
            DropBrick(context, message);
        } else if (!CreateParts(context.m_database, context.m_brick, &material, itemNos)) {
            String message("Cannot find brick: ");
            message << LDDUtilities::GetBrickDescription(context.m_database, context.m_brick)
                    << " from palette: " << context.m_bag->m_name;
            DropBrick(context, message);
        } else if (context.m_brick->m_parts.GetSize() == 1) {
            context.PushHandlers(OnBrickChildStart, OnBrickChildEnd);
        } else {
            String message("Brick : ");
            message << designID << " is not single part in palette: " << context.m_bag->m_name
                    << " as described in database.";
            DropBrick(context, message);
        }
        return;
    }

    // <Assembly>: the parts get material 21 (<Part> sets the real ones).
    if (!CreateParts(context.m_database, context.m_brick, 0, itemNos)) {
        String message("Cannot find brick: ");
        message << LDDUtilities::GetBrickDescription(context.m_database, context.m_brick)
                << " from palette: " << context.m_bag->m_name;
        DropBrick(context, message);
    } else if (context.m_brick->m_parts.GetSize() >= 2) {
        context.PushHandlers(OnAssemblyChildStart, OnAssemblyChildEnd);
    } else {
        String message("Brick : ");
        message << designID << " is not multi part in palette: " << context.m_bag->m_name
                << " as described in database.";
        DropBrick(context, message);
    }
}

// Mac 0x12c8b0
void OnBagChildEnd(void* userData, const char* name)
{
    if (StrCmp(name, "Bag") == 0)
        C(userData).PopHandlers();
}

// ---- <Meta> ----------------------------------------------------------------------------------

// A child of <Meta>: kept as a StorageScene2::Meta (name, attributes, text). Mac 0x12c960
void OnMetaChildStart(void* userData, const char* name, const char** atts)
{
    PAXMLContext& context = C(userData);
    StorageScene2::Meta meta;
    meta.m_name = name;
    for (; atts[0]; atts += 2) {
        StorageScene2::Meta::Attribute attribute;
        attribute.m_name = atts[0];
        attribute.m_value = atts[1];
        meta.m_attributes.PushBack(attribute);
    }
    context.m_metas.PushBack(meta);
    context.PushHandlers(OnMetaValueChildStart, OnMetaValueChildEnd);
    XML_SetCharacterDataHandler(*context.m_parser, OnMetaValueText);
}

// Mac 0x12cb60
void OnMetaChildEnd(void* userData, const char* name)
{
    if (StrCmp(name, "Meta") == 0)
        C(userData).PopHandlers();
}

// An element inside a <Meta> child ends the text and is skipped. NOTE: the text handler is not
// reinstalled afterwards (both builds). Mac 0x12da00
void OnMetaValueChildStart(void* userData, const char*, const char**)
{
    PAXMLContext& context = C(userData);
    XML_SetCharacterDataHandler(*context.m_parser, 0);
    context.PushHandlers(OnIgnoredStart, OnIgnoredEnd);
}

// The end of the <Meta> child. Mac 0x12da40
void OnMetaValueChildEnd(void* userData, const char*)
{
    PAXMLContext& context = C(userData);
    XML_SetCharacterDataHandler(*context.m_parser, 0);
    context.PopHandlers();
}

// Mac 0x12dae0
void OnMetaValueText(void* userData, const char* text, int length)
{
    PAXMLContext& context = C(userData);
    context.m_metas.Back().m_text.Append(DecodeString(kTextEncodingUTF8, text, (UInt32)length));
}

// ---- <Brick> ---------------------------------------------------------------------------------

// Children of <Brick>: <SubMaterial surfaceID materialID>, <Decoration surfaceID decorationID>
// on the brick's part. Mac 0x12cf40
void OnBrickChildStart(void* userData, const char* name, const char** atts)
{
    PAXMLContext& context = C(userData);
    if (!context.m_brick)
        return;
    if (StrCmp(name, "SubMaterial") == 0) {
        UInt32 surface = 0;
        UInt32 material = 0;
        for (; atts[0]; atts += 2) {
            if (StrCmp(atts[0], "surfaceID") == 0)
                surface = StrToUInt32(atts[1], 0, 10);
            if (StrCmp(atts[0], "materialID") == 0)
                material = StrToUInt32(atts[1], 0, 10);
        }
        if (!context.m_database->HasMaterial(material)) {
            String message("Invalid material used: ");
            message << material << " from palette: " << context.m_bag->m_name
                    << " - possibly for brick: " << context.m_brick->m_designID;
            DropBrick(context, message);
            return;
        }
        PalettePart* part = context.m_brick->m_parts.Back();
        if (surface < part->m_materials.GetSize()) {
            part->m_materials[surface] = material;
            return;
        }
        String message("Brick: ");
        message << context.m_brick->m_designID << " is missing surface: " << surface
                << " in palette: " << context.m_bag->m_name;
        DropBrick(context, message);
        return;
    }
    if (StrCmp(name, "Decoration") == 0) {
        UInt32 surface = 0;
        UInt32 decoration = 0;
        for (; atts[0]; atts += 2) {
            if (StrCmp(atts[0], "surfaceID") == 0)
                surface = StrToUInt32(atts[1], 0, 10);
            if (StrCmp(atts[0], "decorationID") == 0)
                decoration = StrToUInt32(atts[1], 0, 10);
        }
        const UInt32 designID = context.m_brick->m_designID;
        if (!context.m_database->HasDecoration(decoration) ||
            !context.m_database->IsValidDecorationSurface(designID, surface)) {
            String message("Decoration error on part: ");
            message << designID << " on surface: " << surface << " decoration: " << decoration
                    << " from palette: " << context.m_bag->m_name;
            DropBrick(context, message);
            return;
        }
        PalettePart* part = context.m_brick->m_parts.Back();
        while (part->m_decorations.GetSize() < surface + 1)
            part->m_decorations.PushBack(0);
        part->m_decorations[surface] = decoration;
        return;
    }
    context.PushHandlers(OnIgnoredStart, OnIgnoredEnd);
}

// </Brick>: the brick goes into the bag with its quantity. Mac 0x12d4a0
void OnBrickChildEnd(void* userData, const char* name)
{
    PAXMLContext& context = C(userData);
    if (StrCmp(name, "Brick") != 0)
        return;
    if (context.m_brick) {
        context.m_bag->AddBrick(context.m_brick, context.m_quantity, *context.m_database);
        context.m_brick = 0;
    }
    context.PopHandlers();
}

// ---- <Assembly> ------------------------------------------------------------------------------

// Children of <Assembly>: <Part designID materialID> sets the first material of EVERY part with
// that design ID. Mac 0x12d590
void OnAssemblyChildStart(void* userData, const char* name, const char** atts)
{
    PAXMLContext& context = C(userData);
    if (!context.m_brick)
        return;
    if (StrCmp(name, "Part") != 0) {
        context.PushHandlers(OnIgnoredStart, OnIgnoredEnd);
        return;
    }
    UInt32 designID = 0;
    UInt32 material = 0;
    for (; atts[0]; atts += 2) {
        if (StrCmp(atts[0], "designID") == 0)
            designID = StrToUInt32(atts[1], 0, 10);
        else if (StrCmp(atts[0], "materialID") == 0)
            material = StrToUInt32(atts[1], 0, 10);
    }
    if (!context.m_database->HasMaterial(material)) {
        String message("Invalid material used: ");
        message << material << " from palette: " << context.m_bag->m_name
                << " - possibly for brick: " << context.m_brick->m_designID;
        DropBrick(context, message);
        return;
    }
    bool found = false;
    BasicArray<RefPointer<PalettePart> >& parts = context.m_brick->m_parts;
    for (UInt32 i = 0; i < parts.GetSize(); ++i) {
        if (parts[i]->m_designID == designID) {
            parts[i]->m_materials[0] = material;
            found = true;
        }
    }
    if (!found) {
        String message("Cannot find brick part: ");
        message << designID << " in assembly: " << context.m_brick->m_designID
                << " from palette: " << context.m_bag->m_name;
        DropBrick(context, message);
        return;
    }
    context.m_partDesignID = designID;
    context.PushHandlers(OnPartChildStart, OnPartChildEnd);
}

// </Assembly>. Mac 0x12d910
void OnAssemblyChildEnd(void* userData, const char* name)
{
    PAXMLContext& context = C(userData);
    if (StrCmp(name, "Assembly") != 0)
        return;
    if (context.m_brick) {
        context.m_bag->AddBrick(context.m_brick, context.m_quantity, *context.m_database);
        context.m_brick = 0;
    }
    context.PopHandlers();
}

// Children of <Assembly><Part>: <SubMaterial>, <Decoration> on every part with the <Part>'s
// design ID. Mac 0x12dc90
// NOTE: the "Cannot find brick part" traces of the original print the design ID of the part
// iterator after the loop (one past the end, an out-of-range read in both builds); restored with
// the <Part> design ID.
void OnPartChildStart(void* userData, const char* name, const char** atts)
{
    PAXMLContext& context = C(userData);
    if (!context.m_brick)
        return;
    BasicArray<RefPointer<PalettePart> >& parts = context.m_brick->m_parts;
    if (StrCmp(name, "SubMaterial") == 0) {
        UInt32 surface = 0;
        UInt32 material = 0;
        for (; atts[0]; atts += 2) {
            if (StrCmp(atts[0], "surfaceID") == 0)
                surface = StrToUInt32(atts[1], 0, 10);
            if (StrCmp(atts[0], "materialID") == 0)
                material = StrToUInt32(atts[1], 0, 10);
        }
        if (!context.m_database->HasMaterial(material)) {
            // NOTE: " from palette " without the colon (both builds).
            String message("Invalid material used: ");
            message << material << " from palette " << context.m_bag->m_name
                    << " - possibly for brick: " << context.m_brick->m_designID;
            DropBrick(context, message);
            return;
        }
        bool found = false;
        for (UInt32 i = 0; i < parts.GetSize(); ++i) {
            PalettePart* part = parts[i];
            if (part->m_designID != context.m_partDesignID)
                continue;
            // NOTE: the material surface is checked with the DECORATION surface table, and the
            // store has no range check (both builds); restored with a range check.
            if (!context.m_database->IsValidDecorationSurface(part->m_designID, surface)) {
                String message("Cannot find brick part: ");
                message << part->m_designID << " from palette: " << context.m_bag->m_name;
                DropBrick(context, message);
                return;
            }
            if (surface < part->m_materials.GetSize())
                part->m_materials[surface] = material;
            found = true;
        }
        if (found)
            return;
        String message("Cannot find brick part: ");
        message << context.m_partDesignID << " from palette: " << context.m_bag->m_name;
        DropBrick(context, message);
        return;
    }
    if (StrCmp(name, "Decoration") == 0) {
        UInt32 surface = 0;
        UInt32 decoration = 0;
        for (; atts[0]; atts += 2) {
            if (StrCmp(atts[0], "surfaceID") == 0)
                surface = StrToUInt32(atts[1], 0, 10);
            if (StrCmp(atts[0], "decorationID") == 0)
                decoration = StrToUInt32(atts[1], 0, 10);
        }
        bool found = false;
        for (UInt32 i = 0; i < parts.GetSize(); ++i) {
            PalettePart* part = parts[i];
            if (part->m_designID != context.m_partDesignID)
                continue;
            if (!context.m_database->HasDecoration(decoration) ||
                !context.m_database->IsValidDecorationSurface(part->m_designID, surface)) {
                String message("Decoration error on part: ");
                message << part->m_designID << " on surface: " << surface << " decoration: "
                        << decoration << " from palette: " << context.m_bag->m_name;
                DropBrick(context, message);
                return;
            }
            while (part->m_decorations.GetSize() < surface + 1)
                part->m_decorations.PushBack(0);
            part->m_decorations[surface] = decoration;
            found = true;
        }
        if (found)
            return;
        String message("Cannot find brick part: ");
        message << context.m_partDesignID << " from palette: " << context.m_bag->m_name;
        DropBrick(context, message);
        return;
    }
    context.PushHandlers(OnIgnoredStart, OnIgnoredEnd);
}

// Mac 0x12e360
void OnPartChildEnd(void* userData, const char* name)
{
    if (StrCmp(name, "Part") == 0)
        C(userData).PopHandlers();
}

// ---- Info.baxml ------------------------------------------------------------------------------

// <BAXML versionMajor="1"><Bag name brand version countable brandFilter/></BAXML> of
// `directory`: name and brand are required, a bad version is 0.0.0.0, countable / brandFilter
// default to false; the bag's text is cleared. Mac 0x129fb0, Win 0x64d50a
// NOTE: the Mac copy is called with an unused first argument (the Load's bag).
bool LoadBagInfo(Directory* directory, BrickPalette::Bag& bag)
{
    Path path;
    path.SetElement(directory);
    if (!path.Append(String("Info.baxml")) || !path.Exists())
        return false;
    RefPointer<XMLDOM> document = new XMLDOM();
    if (!document->Parse(path))
        return false;
    UInt32 versionMajor = 0;
    String text;
    if (!document->GetAttribute(String("versionMajor"), text) || !FromString(versionMajor, text))
        return false;
    if (document->GetName() != String("BAXML") || versionMajor != 1)
        return false;
    XMLDOMNode* node = document->FindChild(String("Bag"));
    if (!node)
        return false;
    if (!node->GetAttribute(String("name"), bag.m_name))
        return false;
    bag.m_text = "";
    {
        String value;
        if (!node->GetAttribute(String("brand"), value))
            return false;
        UInt32 position = 0;
        // NOTE: the default 0xffffffff is no Brand value: an unknown brand leaves it in `brand`,
        // which is then read as a Brand (undefined, both builds); fixed: written back only when
        // parsed (the bag is rejected either way) (low).
        Brand brand = kBrandLDD;
        const bool ok = FromString(brand, value, position);
        if (ok)
            bag.m_brand = brand;
        if (!ok)
            return false;
    }
    {
        String value;
        UInt32 position = 0;
        if (!node->GetAttribute(String("version"), value) || !FromString(bag.m_version, value, position))
            bag.m_version = Version(0, 0, 0, 0);
    }
    {
        String value;
        if (!node->GetAttribute(String("countable"), value) || !FromString(bag.m_countable, value))
            bag.m_countable = false;
    }
    {
        String value;
        if (!node->GetAttribute(String("brandFilter"), value) || !FromString(bag.m_brandFilter, value))
            bag.m_brandFilter = false;
    }
    return true;
}

// ---- writing ---------------------------------------------------------------------------------

// One <Brick> (`brick`: quantity and the non-empty item number) or <Part> line of Save, with its
// <SubMaterial> (surfaces 1…) and <Decoration> (surfaces 0…) children for non-zero values.
// Mac 0x12a880, Win 0x4e4490
String WritePart(const PalettePart& part, bool brick, UInt32 quantity, const String& itemNos)
{
    String out;
    bool subMaterials = false;
    for (UInt32 i = 1; i < part.m_materials.GetSize() && !subMaterials; ++i)
        subMaterials = part.m_materials[i] != 0;
    bool decorations = false;
    for (UInt32 i = 0; i < part.m_decorations.GetSize() && !decorations; ++i)
        decorations = part.m_decorations[i] != 0;

    const String indent(brick ? "\t\t" : "\t\t\t");
    const char* const tag = brick ? "Brick" : "Part";
    out << indent << "<" << tag << " " << "designID" << "=\"" << part.GetDesignID() << "\" "
        << "materialID" << "=\"" << part.m_materials[0] << "\"";
    if (brick) {
        out << " " << "quantity" << "=\"" << quantity << "\"";
        // NOTE: the original writes the item number as it is (no String::ToXML, Mac 0x12a880),
        // so one with & < > " ' breaks the file; fixed: it is escaped (M7).
        if (!itemNos.IsEmpty())
            out << " " << "ItemNos" << "=\"" << itemNos.ToXML() << "\"";
    }
    if (!decorations && !subMaterials) {
        out << "/>\r\n";
        return out;
    }
    out << ">\r\n";
    for (UInt32 i = 1; i < part.m_materials.GetSize(); ++i) {
        if (part.m_materials[i] != 0)
            out << indent << "\t<" << "SubMaterial" << " " << "surfaceID" << "=\"" << i << "\" "
                << "materialID" << "=\"" << part.m_materials[i] << "\"/>\r\n";
    }
    for (UInt32 i = 0; i < part.m_decorations.GetSize(); ++i) {
        if (part.m_decorations[i] != 0)
            out << indent << "\t<" << "Decoration" << " " << "surfaceID" << "=\"" << i << "\" "
                << "decorationID" << "=\"" << part.m_decorations[i] << "\"/>\r\n";
    }
    out << indent << "</" << tag << ">\r\n";
    return out;
}

} // namespace

// ================================================================================================
// BrickPaletteBagLoaderPAXML
// ================================================================================================

// Mac 0x1298c0, Win 0x64ee77
bool BrickPaletteBagLoaderPAXML::Load(Directory* directory, BrickPalette::Bag& bag,
                                      const BrickDatabase* database, PaletteNames* names)
{
    bag.Clear();
    if (!LoadBagInfo(directory, bag))
        return false;

    directory->BeginEnumeration();
    for (;;) {
        if (directory->IsEnumerationDone())
            break;
        RefPointer<File> file = directory->OpenCurrent(OpenMode(true, false, true, false), 0);
        if (file && Directory::MatchWildcard(file->GetPath(false), String("*.paxml"))) {
            AString data;
            if (!ReadFile(file.Get(), data)) {
                directory->EndEnumeration();
                String error("BrickPaletteBagLoaderPAXML: Could not load file ");
                error << file->GetPath(true);                           // NOTE: dropped
                return false;
            }
            XML_Parser parser = XML_ParserCreate(0);
            PAXMLContext context(&bag, database, &parser);
            XML_SetUserData(parser, &context);
            context.PushHandlers(OnRootStart, OnRootEnd);
            if (XML_Parse(parser, data.GetBuffer(), (int)data.GetLength(), 1) == XML_STATUS_ERROR) {
                directory->EndEnumeration();
                String error("BrickPaletteBagLoaderPAXML: Could not parse file ");
                error << file->GetPath(true);                           // NOTE: dropped
                XML_ParserFree(parser);
                return false;
            }
            XML_ParserFree(parser);
        } else if (bag.m_brandFilter && file &&
                   file->GetPath(false) == String("MaterialSelector.xml")) {
            bag.m_grid.Load(file.Get(), *database);
        }
        directory->MoveNext();
    }
    directory->EndEnumeration();

    names->Insert(PaletteName(directory->GetPath(false), bag.m_version));
    bag.m_path = Path(directory->GetPath(false), directory->GetParent().Get());
    return true;
}

// Mac 0x12ae20
bool BrickPaletteBagLoaderPAXML::Save(const Path& path, const BrickPalette::Bag& bag)
{
    String out;
    out << "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\" ?>\r\n";
    out << "<" << "PAXML" << " " << "versionMajor" << "=\"1\" " << "versionMinor" << "=\"0\">\r\n";
    out << "\t<" << "Bag" << ">\r\n";
    for (BrickPalette::Bag::BrickCountMap::ConstIterator it = bag.Begin(); it != bag.End(); ++it) {
        RefPointer<const PaletteBrick> brick = it->first;
        const UInt32 count = it->second;
        if (brick->m_parts.GetSize() == 1) {
            out << WritePart(*brick->m_parts[0], true, count, brick->GetName());
            continue;
        }
        out << "\t\t<" << "Assembly" << " " << "designID" << "=\"" << brick->GetDesignID() << "\" "
            << "quantity" << "=\"" << count << "\"";
        // NOTE: the item number is written as it is (Mac 0x12ae20); fixed: escaped (M7).
        if (!brick->GetName().IsEmpty())
            out << " " << "ItemNos" << "=\"" << brick->GetName().ToXML() << "\"";
        out << ">\r\n";
        BasicSet<UInt32> written;
        for (UInt32 i = 0; i < brick->GetPartCount(); ++i) {
            const PalettePart* part = brick->m_parts[i];
            if (written.Insert(part->GetDesignID()))
                out << WritePart(*part, false, 0, String());
        }
        out << "\t\t</" << "Assembly" << ">\r\n";
    }
    out << "\t</" << "Bag" << ">\r\n";
    out << "</" << "PAXML" << ">\r\n";

    AString data = EncodeString(kTextEncodingUTF8, out, 0, String::npos);
    // NOTE: the original rewrites the file in place and ignores the SetSize result: a failed
    //   write leaves a broken .paxml (a cut or partly old file); fixed: written whole with
    //   WriteWholeFile (on the disk "<name>.paxml.tmp", read back, then renamed over the file:
    //   all or nothing), false when anything failed (M2). The only caller,
    //   SavePalettesCommand::Do (a developer command), ignores the result: no localized message
    //   fits, no dialog.
    return WriteWholeFile(path, data.GetBuffer(), data.GetLength());
}

// BrickPaletteBagLoaderModel::LoadBag (TU 440, Mac 0x3b6500): not restored here, see BrickPaletteBagLoaderModel.cpp.

} // namespace LEGO
