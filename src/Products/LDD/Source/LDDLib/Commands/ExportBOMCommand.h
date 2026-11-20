// LEGO Digital Designer — restored source
// LEGO::ExportBOMCommand — "exportBOM" (LDD.commands.editMode, created by EditMode::Initialize,
//   TU 085): exports the bill of materials of the scene as an Office Open XML workbook
//   (.xlsx; the save panel also offers "Zip file (*.zip)" — the same archive under another
//   extension). Enabled while the scene is not empty.
//
// TU 441 "ExportBOMCommand.cpp" (__FILE__ "/Users/dkjaspja/GitLab/Release/LDDFan4.3.11/Products/
//   LDD/Source/LDDLib/Commands/ExportBOMCommand.cpp", assert in Do line 194): Mac 0x3b73d0-0x3c4d6b
//   (33 functions), Win 0x66efce-0x6749de + 0x6750bb-0x676141.
//
// Writing: no OOXML writer of its own. Do mounts a ZipStorageDirectory "memory/<file name>",
//   saves one XMLDOM per package part (XMLDOMNode::Save) plus one PNG per row (a BricksControl
//   through ModelControl::RenderToBitmap), releases the archive (the zip volume writes its central
//   directory) and Directory::Copy's it to the target. Zip time stamps are 0: reproducible output.
//   Parts: _rels/.rels, [Content_Types].xml, xl/_rels/workbook.xml.rels, xl/workbook.xml,
//   xl/worksheets/sheet1.xml, xl/sharedStrings.xml, xl/worksheets/_rels/sheet1.xml.rels,
//   xl/calcChain.xml, xl/drawings/drawing1.xml, xl/drawings/_rels/drawing1.xml.rels,
//   xl/media/image<n>.png.
//   Sheet: row 1 = "Brick", "Name", "Picture", "Part", "Color code", "Quantity" (shared strings);
//   one row per distinct brick (PrimitiveElementBrick::BrickLess order): A item number
//   (palette brick name), B localized design name, C the picture (a drawing anchor), D design ID,
//   E "<material> - <localized material name>" list, F count; last row "Total:" and
//   F = SUM(F2:F<n+1>).
//
// RTTI: vmi {Command +0 (public), BrickConnectSceneControlObserver +0x28 (private)}, virtual base
//   Object. Mac vtable 0x75906c {D1 0x3c46d0, D0 0x3c4730, OnFinalRelease 0x3c45d0, Do 0x3b75c0,
//   SetEnable 0x372e30, GetEnable 0x3730b0, UpdateEnable 0x3c4530, OnEvent20 0x3c46a0};
//   observer-in 0x759098 {D1 0x3c47a0, D0 0x3c4800, OnFinalRelease 0x3c4660, …, 20: OnEvent20
//   thunk 0x3c46b0}; Object-in 0x75910c {0x3c4870, 0x3c48d0, 0x3c4680, AddRef, Release}. Base
//   object destructor (with VTT) 0x3c4a50.
//   Win: Command-in 0x9d9458 {Do 0x67548a, SetEnable, GetEnable, UpdateEnable 0x670176};
//   observer-in 0x9d9404 (17: OnEvent20 = 0x631956, ICF with ExportCommand::OnEvent19);
//   Object-in 0x9d93f0 {sdd 0x67501c -> 0x6750bb, …}. Constructor Win 0x674f3a, destructor body
//   0x675024.
// Layout (Mac / Win): Command +0x00..+0x28 / +0x00..+0x2c, observer +0x28 (subject +0x2c) /
//   +0x2c, m_sceneControl +0x30 / +0x38, m_sharedStrings +0x34 / +0x3c, m_sharedStringCount
//   +0x44 / +0x4c, Object +0x48 / +0x54.
// The SheetRow_t name is RTTI (BasicArray<ExportBOMCommand::SheetRow_t>); all other names are
// inferred.
#pragma once
#include "LEGO/Types.h"
#include "Object.h"
#include "LEGO/String.h"
#include "BasicArray.h"
#include "Command.h"
#include "BrickConnectSceneControl.h"
#include "BrickConnectSceneControlObserver.h"
#include "XMLDOMNode.h"
#include "Geometry.h"

namespace LEGO {

class Path;
class ModelControl;

class ExportBOMCommand : public Command, private BrickConnectSceneControlObserver {
public:
    // One row of the sheet (RTTI BasicArray<ExportBOMCommand::SheetRow_t>; Mac sizeof 0x50,
    // Win 0x20). Constructor Mac 0x3c4950 (the scalar fields are left uninitialised there),
    // destructor 0x3c49d0 (Win 0x66ed1f), copy 0x3c4c40 (Win 0x66ef41).
    struct SheetRow_t {
        SheetRow_t() : m_designID(0), m_unused(0), m_weight(0.0f), m_count(0) {}

        String m_name;            // +0x00 / +0x00 palette brick name (the item number): column A
        UInt32 m_designID;        // +0x10 / +0x04 column D
        String m_localizedName;   // +0x14 / +0x08 BrickDatabase::GetLocalizedName: column B
        UInt32 m_unused;          // +0x24 / +0x0c never written nor read (copied only)
        Real32 m_weight;          // +0x28 / +0x10 sum of the bone masses (computed, not exported)
        UInt32 m_count;           // +0x2c / +0x14 column F
        String m_materials;       // +0x30 / +0x18 "21 - Bright red, 26 - Black": column E
        String m_image;           // +0x40 / +0x1c "xl/media/image<n>.png"
    };
    typedef BasicArray<SheetRow_t> SheetRows;   // vtable Mac 0x779120 {0x3c0d60, 0x3c4bd0}, Win 0x9d8834

    // Disabled; keeps (and observes) `sceneControl`, then UpdateEnable. Mac 0x3b73d0, Win 0x674f3a
    explicit ExportBOMCommand(BrickConnectSceneControl* sceneControl);
    virtual ~ExportBOMCommand();                                            // Mac 0x3c46d0/0x3c4730, Win sdd 0x6750bb

    // Save panel ("Save BOM to file"), then the workbook (see above). Mac 0x3b75c0, Win 0x67548a
    virtual void Do();
    // Enabled while the scene is not empty (NOTE: m_sceneControl is not checked for null).
    // Mac 0x3c4530, Win 0x670176
    virtual void UpdateEnable();

protected:
    // Observer<BrickConnectSceneControl> part, then NotifierT<Command>'s. Mac 0x3c45d0
    // (thunks 0x3c4660 / 0x3c4680)
    virtual void OnFinalRelease();
    // The scene changed: UpdateEnable. Mac 7 0x3c46a0 (observer thunk 0x3c46b0), Win observer
    // slot 17 0x631956
    virtual void OnEvent20();

    // ---- the package parts (names inferred). Mac: internal functions with the `this` slot
    // left unset by the caller where it is not used; Win: thiscall member functions. ----
    // _rels/.rels. Mac 0x3ba960, Win 0x66f73d
    RefPointer<XMLDOM> CreateRelationships();
    // [Content_Types].xml. Mac 0x3bacc0, Win 0x66efce
    RefPointer<XMLDOM> CreateContentTypes();
    // xl/_rels/workbook.xml.rels. Mac 0x3bbb50, Win 0x66f8f3
    RefPointer<XMLDOM> CreateWorkbookRelationships();
    // xl/workbook.xml. Mac 0x3bc300, Win 0x66fcaf
    RefPointer<XMLDOM> CreateWorkbook();
    // xl/worksheets/sheet1.xml; fills m_sharedStrings / m_sharedStringCount. Mac 0x3bc8c0,
    // Win 0x67251f
    RefPointer<XMLDOM> CreateSheet(SheetRows rows);
    // xl/sharedStrings.xml (count = m_sharedStringCount). Mac 0x3c0dc0, Win 0x670452
    RefPointer<XMLDOM> CreateSharedStrings(BasicArray<String> strings);
    // xl/worksheets/_rels/sheet1.xml.rels. Mac 0x3c11a0, Win 0x66ffc0
    RefPointer<XMLDOM> CreateSheetRelationships();
    // xl/calcChain.xml. Mac 0x3c1500, Win 0x672201
    RefPointer<XMLDOM> CreateCalcChain(SheetRows rows);
    // xl/drawings/drawing1.xml: one picture anchor per row. Mac 0x3c17f0, Win 0x670798
    RefPointer<XMLDOM> CreateDrawing(SheetRows rows);
    // xl/drawings/_rels/drawing1.xml.rels. Mac 0x3c3fe0, Win 0x671f41
    RefPointer<XMLDOM> CreateDrawingRelationships(SheetRows rows);

    // Index of `text` in m_sharedStrings (appended when new); every call with a non-empty text
    // counts one use. NOTE: an empty text is not stored and gives (UInt32)-1 (both builds).
    // Mac inlined (e.g. 0x3bd7ee-0x3bd859), Win 0x672415 (thiscall, String by value)
    UInt32 AddSharedString(String text);

    // Renders `control` into a `size` BGRA bitmap and writes it as PNG to `path`. Mac 0x3ba5b0,
    // Win 0x6701e2
    bool SaveImage(const Path& path, const Vector2& size, ModelControl* control);

    // ---- data (Mac / Win offsets) ----
    RefPointer<BrickConnectSceneControl> m_sceneControl;        // +0x30 / +0x38
    BasicArray<String>                   m_sharedStrings;       // +0x34 / +0x3c (vtable Mac 0x75f768)
    // Uses of shared strings in the sheet (sst count). NOTE: not initialised by the constructor
    // in either build (Do clears it before use); zero here.
    UInt32                               m_sharedStringCount;   // +0x44 / +0x4c
                                                                // Object +0x48 / +0x54
};

} // namespace LEGO
