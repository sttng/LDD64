// LEGO Digital Designer — restored source
// TU 441 "ExportBOMCommand.cpp": Mac 0x3b73d0-0x3c4d6b, Win 0x66efce-0x6749de + 0x6750bb-0x676141.
// See ExportBOMCommand.h. Weak template code of this TU (BasicArray<SheetRow_t> PushBack Mac
// 0x3ba850 / Win 0x670704, the BasicMap<RefPointer<PrimitiveElementBrick>, UInt32, BrickLess>
// tree code Win 0x6724af-0x674e8e) comes from the container headers. Mac 0x3c4cf0 = <iostream>.
#include "ExportBOMCommand.h"
#include "BrickConnectSceneManager.h"
#include "BricksControl.h"
#include "ModelControl.h"
#include "PrimitiveElementBrick.h"
#include "LDDPart.h"
#include "LDDApplication.h"
#include "Application.h"
#include "Platform.h"
#include "TextLocale.h"
#include "StringStream.h"
#include "BasicMap.h"
#include "Scene.h"
#include "Brick.h"
#include "BrickDatabase.h"
#include "Primitive.h"
#include "BrickPalette.h"
#include "Directory.h"
#include "ZipStorageDirectory.h"
#include "ObjectFactory.h"
#include "Path.h"
#include "File.h"
#include "Bitmap.h"
#include "DataStream.h"
#include "SafeFileWrite.h"
#if defined(_WIN32)
#include "LDDUtilities.h"
#else
#include "MacImageIO.h"
#endif

#include <assert.h>
#include <algorithm>
#include <iostream>
#include <typeinfo>

namespace LEGO {

namespace {

// The pattern every part builder uses (inline in the original: the value goes through
// ToString into a fresh String, Mac 0x1423f0 / 0x142530).
void SetAttribute(XMLDOMNode* node, const char* name, const char* value)
{
    String key(name);
    String text;
    ToString(value, text);
    node->SetAttribute(key, text);
}

void SetAttribute(XMLDOMNode* node, const char* name, UInt32 value)
{
    String key(name);
    String text;
    ToString(value, text);
    node->SetAttribute(key, text);
}

// A new element `name` (referenced by the returned pointer).
RefPointer<XMLDOMNode> NewNode(const char* name)
{
    return RefPointer<XMLDOMNode>(new XMLDOMNode(String(name)));
}

// A child element with text (inline pattern: new node, SetText, AddChild).
void AddTextChild(XMLDOMNode* parent, const char* name, const String& text)
{
    RefPointer<XMLDOMNode> node = NewNode(name);
    node->SetText(text);
    parent->AddChild(node);
}

typedef BasicMap<RefPointer<PrimitiveElementBrick>, UInt32, PrimitiveElementBrick::BrickLess> BrickCountMap;

// Appends `text` unless the array already holds it (Mac inlined 0x3b7e14-0x3b7e28 with std::find
// 0x1809a0; Win 0x6723c8). (name inferred)
void AddUnique(BasicArray<String>& strings, const String& text)
{
    if (std::find(strings.Begin(), strings.End(), text) == strings.End())
        strings.PushBack(text);
}

const char* const kMainNamespace = "http://schemas.openxmlformats.org/spreadsheetml/2006/main";
const char* const kRelationshipsNamespace =
    "http://schemas.openxmlformats.org/officeDocument/2006/relationships";
const char* const kPackageRelationshipsNamespace =
    "http://schemas.openxmlformats.org/package/2006/relationships";
const char* const kDrawingNamespace = "http://schemas.openxmlformats.org/drawingml/2006/main";

// Saves `dom` as the entry `name` of `archive` (recorded in `entries` for the read-back); false
// when the save failed. (not in the original)
bool SavePart(const RefPointer<XMLDOM>& dom, const char* name, Directory* archive,
              BasicArray<String>& entries)
{
    entries.PushBack(String(name));
    return dom->Save(Path(String(name), archive));
}

// The closed archive "memory/<name of target>" is mounted again and each of `entries` must read
// back completely with the size written to it (`sizes`); the archive is then copied to `target`
// all or nothing: on the disk storage to "<name>.tmp" next to it (Directory::Copy, as the
// original copies), read back against the archive and renamed over `target`
// (SafeWriteTarget / CommitSafeWrite); elsewhere copied in place. (not in the original)
bool CopyCheckedArchive(Directory* memory, const Path& target, const BasicArray<String>& entries,
                        const BasicArray<UInt64>& sizes)
{
    const String name(target.GetName());
    {
        BasicArray<String> components;
        components.PushBack(name);
        Directory::OpenArgs openArgs(memory, components, true);
        RefPointer<Directory> archive = ObjectFactory::GetInstance()->Create<ZipStorageDirectory>(openArgs);
        if (archive.IsNull())
            return false;
        for (UInt32 i = 0; i < entries.GetSize(); ++i) {
            if (!ArchiveEntryReadsBack(archive.Get(), entries[i], sizes[i]))
                return false;
        }
    }
    Directory* directory = target.GetParentDirectory();
    if (!directory)
        return false;
    UInt8* data = 0;
    UInt32 size = 0;
    {
        RefPointer<File> source = memory->OpenFile(name, OpenMode(true, false, true, false), false, 0);
        if (source.IsNull())
            return false;
        const UInt64 length = source->GetSize();
        if (length == 0 || length > 0xffffffffu)
            return false;
        size = (UInt32)length;
        data = new UInt8[size];
        UInt64 read = 0;
        if (!source->ReadAt(0, size, data, &read) || read != size) {
            delete[] data;
            return false;
        }
    }
    const Path temporary = SafeWriteTarget(target);
    bool written = memory->Copy(name, *directory, String(temporary.GetName()), true, false);
    if (written) {
        RefPointer<File> copy = temporary.OpenFile(OpenMode(true, false, true, false), false);
        written = !copy.IsNull() && FileHoldsData(copy.Get(), data, size);
    }
    delete[] data;
    return CommitSafeWrite(target, temporary, written);
}

} // namespace

// Mac 0x3b73d0, Win 0x674f3a
ExportBOMCommand::ExportBOMCommand(BrickConnectSceneControl* sceneControl)
    : Command(false, false), m_sceneControl(sceneControl), m_sharedStrings(), m_sharedStringCount(0)
{
    BrickConnectSceneControlObserver::Attach(sceneControl);
    UpdateEnable();
    // NOTE: a trace message that is built and dropped (both builds).
    String message;
    message << "ExportBOMCommand constructor";
}

// Mac 0x3c46d0 (D1) / 0x3c4730 (D0), base object destructor 0x3c4a50; Win 0x675024, sdd 0x6750bb
ExportBOMCommand::~ExportBOMCommand()
{
}

// Mac 0x3c45d0 (thunks 0x3c4660 / 0x3c4680)
void ExportBOMCommand::OnFinalRelease()
{
    BrickConnectSceneControlObserver::OnFinalRelease();
    Command::OnFinalRelease();
}

// Mac 0x3c4530, Win 0x670176
void ExportBOMCommand::UpdateEnable()
{
    {
        // NOTE: built and dropped (both builds).
        String message;
        message << "ExportBOMCommand UpdateCommandEnable; !sceneControl->Empty = "
                << !m_sceneControl->IsSceneEmpty();
    }
    SetEnable(!m_sceneControl->IsSceneEmpty());
}

// Mac 0x3c46a0 (thunk 0x3c46b0), Win 0x631956
void ExportBOMCommand::OnEvent20()
{
    UpdateEnable();
}

// Mac 0x3b75c0, Win 0x67548a
void ExportBOMCommand::Do()
{
    {
        // NOTE: built and dropped (both builds).
        String message;
        message << "ExportBOMCommand oncommanddo";
    }
    Path path(String(), 0);
    RefPointer<Directory> directory = Platform::GetInstance()->GetSystemDirectory(3, true);
    Platform::FileTypeArray types;
    types.PushBack(Platform::FileType(String("Microsoft Office Open XML Workbook (*.xlsx)"), String("xlsx")));
    types.PushBack(Platform::FileType(String("Zip file (*.zip)"), String("zip")));
    UInt32 typeIndex = 0;
    Platform* platform = Platform::GetInstance();
    const bool chosen = platform->GetSavePath(path, directory.Get(),
                                              LDDApplication::GetInstance()->GetDocumentName(), types,
                                              &typeIndex, LDDApplication::GetInstance()->GetWindow(),
                                              String("Save BOM to file"));
    if (!chosen)
        return;
    // NOTE: the extension is not checked: the archive gets the name typed in the panel.

    const Path target(String(path.GetName()), path.GetParentDirectory());
    // The archive is built in memory ("memory/<name>") and copied to the target at the end.
    RefPointer<Directory> memory = Directory::GetRoot()->GetDirectory(String("memory"), false);
    BasicArray<String> components;
    components.PushBack(String(target.GetName()));
    Directory::OpenArgs openArgs(memory.Get(), components, true);
    // ObjectFactory::Create<ZipStorageDirectory> (Win 0x5fbfed: CreateObject(typeid) + dynamic_cast)
    RefPointer<Directory> archive = ObjectFactory::GetInstance()->Create<ZipStorageDirectory>(openArgs);
    // NOTE: the original ignores every write into the archive: the picture writes (SaveImage),
    // the XMLDOMNode::Save results, and the entries reach the archive only when their last file
    // object goes (ZipStorageFile::OnFinalRelease, result dropped) and the zip volume closes. The
    // archive is then copied (Directory::Copy) straight over the target, so an export that
    // failed part way can report success with a broken workbook, and a failed copy leaves a
    // broken file over an existing one (the alert is shown). fixed: every picture and part write
    // counts (the first failure ends the writing); the closed archive is mounted again and every
    // entry must read back with the size written to it; the copy goes to "<name>.tmp" next to
    // the target on the disk storage, is read back and renamed over the target in one step. Any
    // failure shows the original's alert and leaves an existing file as it was.
    bool written = !archive.IsNull();
    BasicArray<String> entries;   // the entries written, for the read-back

    // ---- one row per distinct brick (design, palette brick and materials: BrickLess) ----
    // NOTE: the original reads the parts' materials as they are, the paint bucket's hover
    // preview included; fixed: the previews are undone first.
    m_sceneControl->ClearToolPreviews();
    BrickCountMap counts;
    for (const RefPointer<Brick>* it = m_sceneControl->GetScene()->GetBricks().Begin();
         it != m_sceneControl->GetScene()->GetBricks().End(); ++it) {
        RefPointer<PrimitiveElementBrick> brick =
            new PrimitiveElementBrick(static_cast<const LDDBrick*>(it->Get()));
        if (counts.Find(brick) == counts.End())
            counts[brick] = 0;
        ++counts[brick];
    }

    SheetRows rows;
    m_sharedStrings.Clear();
    m_sharedStringCount = 0;
    UInt32 image = 1;
    for (BrickCountMap::Iterator it = counts.Begin(); it != counts.End(); ++it, ++image) {
        RefPointer<PrimitiveElementBrick> brick = it->first;
        RefPointer<BricksControl> control = new BricksControl(m_sceneControl->GetManager(),
                                                              m_sceneControl->GetSceneManager(), 2);
        control->Create(*brick);

        SheetRow_t row;
        row.m_name = brick->GetBrick()->GetName();
        row.m_designID = brick->GetBrick()->GetDesignID();
        row.m_localizedName =
            m_sceneControl->GetSceneManager()->GetDatabase()->GetLocalizedName(brick->GetBrick()->GetDesignID());
        Real32 weight = 0.0f;
        for (UInt32 i = 0; i < brick->GetBrick()->GetPartCount(); ++i) {
            // NOTE: no null check of the primitive (both builds; palette bricks are validated).
            const Primitive* primitive = m_sceneControl->GetSceneManager()->GetDatabase()->GetPrimitive(
                brick->GetBrick()->m_parts[i]->m_designID);
            const BasicArray<Primitive::Bone>& bones = primitive->GetBones();
            for (const Primitive::Bone* bone = bones.Begin(); bone != bones.End(); ++bone)
                weight += bone->m_mass;
        }
        row.m_weight = weight;

        // "<material> - <localized name>" of every material slot (0 = the part's first), once each
        BasicArray<String> materials;
        for (UInt32 part = 0; part < brick->GetBrick()->m_parts.GetSize(); ++part) {
            const BasicArray<UInt32>& ids = brick->GetMaterials(part);
            for (UInt32 i = 0; i < ids.GetSize(); ++i) {
                const UInt32 material = ids[i] != 0 ? ids[i] : ids[0];
                String text;
                String key("Material");
                key << material;
                text << material << " - " << LocalizedString(key);
                AddUnique(materials, String(text));
            }
        }
        for (const String* m = materials.Begin(); m != materials.End(); ++m) {
            String item;
            if (m != materials.Begin())
                item << ", ";
            item << *m;
            row.m_materials << item;
        }
        row.m_count = it->second;

        String name(String("xl/media/image") << image << String(".png"));
        if (written) {
            entries.PushBack(name);
            written = SaveImage(Path(name, archive.Get()), Vector2(128.0f, 128.0f), control);
        }
        row.m_image = name;
        rows.PushBack(row);
    }

    // ---- the package parts ----
    Directory* zip = archive.Get();
    written = written && SavePart(CreateRelationships(), "_rels/.rels", zip, entries);
    written = written && SavePart(CreateContentTypes(), "[Content_Types].xml", zip, entries);
    written = written && SavePart(CreateWorkbookRelationships(), "xl/_rels/workbook.xml.rels", zip, entries);
    written = written && SavePart(CreateWorkbook(), "xl/workbook.xml", zip, entries);
    written = written && SavePart(CreateSheet(rows), "xl/worksheets/sheet1.xml", zip, entries);
    written = written && SavePart(CreateSharedStrings(m_sharedStrings), "xl/sharedStrings.xml", zip, entries);
    written = written && SavePart(CreateSheetRelationships(), "xl/worksheets/_rels/sheet1.xml.rels", zip, entries);
    written = written && SavePart(CreateCalcChain(rows), "xl/calcChain.xml", zip, entries);
    if (written) {
        RefPointer<XMLDOM> drawingDom = CreateDrawing(rows);
        assert(drawingDom != NULL);   // Mac 0x3b9386 (ExportBOMCommand.cpp line 194, "Do")
        written = SavePart(drawingDom, "xl/drawings/drawing1.xml", zip, entries) &&
                  SavePart(CreateDrawingRelationships(rows), "xl/drawings/_rels/drawing1.xml.rels", zip, entries);
    }
    // the sizes written to the entries, while the archive is mounted
    BasicArray<UInt64> sizes;
    for (UInt32 i = 0; written && i < entries.GetSize(); ++i)
        sizes.PushBack(ArchiveEntrySize(zip, entries[i]));
    zip = 0;
    // The last reference: the zip volume writes its central directory.
    archive = 0;

    if (!written || !CopyCheckedArchive(memory.Get(), target, entries, sizes)) {
        String text = LocalizedString(String("UNABLETOEXPORTMODELTO"));
        text.ReplaceAll(String("%s"), 0, String::npos,
                        Platform::GetInstance()->VFSToNativePath(target.GetPath(true)), 0, String::npos);
        Application::GetInstance()->Alert(2, text, LocalizedString(String("DIALOGWARNING")),
                                          LocalizedString(String("OK")), String(), String());
    }
    memory->Delete(String(target.GetName()), false);
}

// Mac 0x3ba5b0, Win 0x6701e2
bool ExportBOMCommand::SaveImage(const Path& path, const Vector2& size, ModelControl* control)
{
    Bitmap bitmap;
    bitmap.m_width = (UInt32)(Int64)size.x;
    bitmap.m_height = (UInt32)(Int64)size.y;
#if defined(_WIN32)
    bitmap.m_format = kBitmapFormatBGRA32;
    bitmap.m_pitch = (32 >> 3) * bitmap.m_width;          // Win 0x4f5180 (width, 32 bits per pixel)
#else
    bitmap.m_depth = 1;
    bitmap.m_format = kBitmapFormatBGRA32;
    bitmap.m_pitch = bitmap.m_width * 4;
#endif
    bitmap.m_data = new UInt8[bitmap.m_pitch * bitmap.m_height];

    bool result = control->RenderToBitmap(bitmap, 0);
    const Path file(path);
    if (result) {
#if defined(_WIN32)
        RefPointer<DataStreamInterface> png = EncodePNG(bitmap);
#else
        RefPointer<DataStreamInterface> png = EncodePNGWithImageIO(bitmap);
#endif
        // NOTE: the original reports success when the PNG cannot be encoded (nothing written)
        // and ignores the SetSize result; fixed: a failed encoding fails and the picture is
        // written whole and read back (WriteWholeFile; in place inside the archive).
        result = !png.IsNull() && WriteWholeFile(file, png->GetData(), png->GetSize());
    }
    delete[] (UInt8*)bitmap.m_data;
    return result;
}

// Mac inlined, Win 0x672415
// NOTE: the original returns -1 for an empty string (Win 0x672415), so a cell with an empty text
// (a brick without a localized name) refers to shared string 4294967295 and Excel reports the
// workbook as corrupt; fixed: the empty string gets a real entry (<si><t/></si>) like any
// other.
UInt32 ExportBOMCommand::AddSharedString(String text)
{
    const String* it = std::find(m_sharedStrings.Begin(), m_sharedStrings.End(), text);
    ++m_sharedStringCount;
    if (it != m_sharedStrings.End())
        return (UInt32)(it - m_sharedStrings.Begin());
    m_sharedStrings.PushBack(text);
    return m_sharedStrings.GetSize() - 1;
}

// ================================================================================================
// package parts
// ================================================================================================

// Mac 0x3ba960, Win 0x66f73d
RefPointer<XMLDOM> ExportBOMCommand::CreateRelationships()
{
    RefPointer<XMLDOM> dom = new XMLDOM(String("Relationships"));
    SetAttribute(dom, "xmlns", kPackageRelationshipsNamespace);
    RefPointer<XMLDOMNode> relationship = NewNode("Relationship");
    SetAttribute(relationship, "Id", "rId1");
    SetAttribute(relationship, "Type",
                 "http://schemas.openxmlformats.org/officeDocument/2006/relationships/officeDocument");
    SetAttribute(relationship, "Target", "xl/workbook.xml");
    dom->AddChild(relationship);
    return dom;
}

// Mac 0x3bacc0, Win 0x66efce
RefPointer<XMLDOM> ExportBOMCommand::CreateContentTypes()
{
    RefPointer<XMLDOM> dom = new XMLDOM(String("Types"));
    SetAttribute(dom, "xmlns", "http://schemas.openxmlformats.org/package/2006/content-types");

    static const char* const kDefaults[][2] = {
        { "xml", "application/xml" },
        { "rels", "application/vnd.openxmlformats-package.relationships+xml" },
        { "png", "image/png" },
    };
    for (UInt32 i = 0; i < 3; ++i) {
        RefPointer<XMLDOMNode> node = NewNode("Default");
        SetAttribute(node, "Extension", kDefaults[i][0]);
        SetAttribute(node, "ContentType", kDefaults[i][1]);
        dom->AddChild(node);
    }
    static const char* const kOverrides[][2] = {
        { "/xl/workbook.xml", "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet.main+xml" },
        { "/xl/worksheets/sheet1.xml", "application/vnd.openxmlformats-officedocument.spreadsheetml.worksheet+xml" },
        { "/xl/sharedStrings.xml", "application/vnd.openxmlformats-officedocument.spreadsheetml.sharedStrings+xml" },
        { "/xl/calcChain.xml", "application/vnd.openxmlformats-officedocument.spreadsheetml.calcChain+xml" },
        { "/xl/drawings/drawing1.xml", "application/vnd.openxmlformats-officedocument.drawing+xml" },
    };
    for (UInt32 i = 0; i < 5; ++i) {
        RefPointer<XMLDOMNode> node = NewNode("Override");
        SetAttribute(node, "PartName", kOverrides[i][0]);
        SetAttribute(node, "ContentType", kOverrides[i][1]);
        dom->AddChild(node);
    }
    return dom;
}

// Mac 0x3bbb50, Win 0x66f8f3
RefPointer<XMLDOM> ExportBOMCommand::CreateWorkbookRelationships()
{
    RefPointer<XMLDOM> dom = new XMLDOM(String("Relationships"));
    SetAttribute(dom, "xmlns", kPackageRelationshipsNamespace);
    static const char* const kRelationships[][3] = {
        { "rId2", "http://schemas.openxmlformats.org/officeDocument/2006/relationships/sharedStrings",
          "sharedStrings.xml" },
        { "rId1", "http://schemas.openxmlformats.org/officeDocument/2006/relationships/worksheet",
          "worksheets/sheet1.xml" },
        { "rId3", "http://schemas.openxmlformats.org/officeDocument/2006/relationships/calcChain",
          "calcChain.xml" },
    };
    for (UInt32 i = 0; i < 3; ++i) {
        RefPointer<XMLDOMNode> relationship = NewNode("Relationship");
        SetAttribute(relationship, "Id", kRelationships[i][0]);
        SetAttribute(relationship, "Type", kRelationships[i][1]);
        SetAttribute(relationship, "Target", kRelationships[i][2]);
        dom->AddChild(relationship);
    }
    return dom;
}

// Mac 0x3bc300, Win 0x66fcaf
RefPointer<XMLDOM> ExportBOMCommand::CreateWorkbook()
{
    RefPointer<XMLDOM> dom = new XMLDOM(String("workbook"));
    SetAttribute(dom, "xmlns", kMainNamespace);
    SetAttribute(dom, "xmlns:r", kRelationshipsNamespace);
    RefPointer<XMLDOMNode> sheets = NewNode("sheets");
    dom->AddChild(sheets);
    RefPointer<XMLDOMNode> sheet = NewNode("sheet");
    SetAttribute(sheet, "name", "Sheet1");
    SetAttribute(sheet, "sheetId", "1");
    SetAttribute(sheet, "r:id", "rId1");
    sheets->AddChild(sheet);
    RefPointer<XMLDOMNode> calcPr = NewNode("calcPr");
    SetAttribute(calcPr, "calcId", "125725");
    dom->AddChild(calcPr);
    return dom;
}

// Mac 0x3bc8c0, Win 0x67251f
RefPointer<XMLDOM> ExportBOMCommand::CreateSheet(SheetRows rows)
{
    RefPointer<XMLDOM> dom = new XMLDOM(String("worksheet"));
    SetAttribute(dom, "xmlns", kMainNamespace);
    SetAttribute(dom, "xmlns:r", kRelationshipsNamespace);
    SetAttribute(dom, "xmlns:mc", "http://schemas.openxmlformats.org/markup-compatibility/2006");
    SetAttribute(dom, "mc:Ignorable", "x14ac");
    SetAttribute(dom, "xmlns:x14ac", "http://schemas.microsoft.com/office/spreadsheetml/2009/9/ac");

    RefPointer<XMLDOMNode> dimension = NewNode("dimension");
    {
        String ref("A1:F");
        dimension->SetAttribute(String("ref"), ref << rows.GetSize() + 2);
    }
    dom->AddChild(dimension);

    // frozen header row
    RefPointer<XMLDOMNode> sheetViews = NewNode("sheetViews");
    dom->AddChild(sheetViews);
    RefPointer<XMLDOMNode> sheetView = NewNode("sheetView");
    SetAttribute(sheetView, "tabSelected", "1");
    SetAttribute(sheetView, "workbookViewId", "0");
    sheetViews->AddChild(sheetView);
    RefPointer<XMLDOMNode> pane = NewNode("pane");
    SetAttribute(pane, "ySplit", "1");
    SetAttribute(pane, "topLeftCell", "A2");
    SetAttribute(pane, "activePane", "bottomLeft");
    SetAttribute(pane, "state", "frozen");
    sheetView->AddChild(pane);
    RefPointer<XMLDOMNode> selection = NewNode("selection");
    SetAttribute(selection, "pane", "bottomLeft");
    SetAttribute(selection, "activeCell", "A1");
    SetAttribute(selection, "sqref", "A1");
    sheetView->AddChild(selection);

    // column widths of B (name) and E (colour codes)
    RefPointer<XMLDOMNode> cols = NewNode("cols");
    dom->AddChild(cols);
    {
        RefPointer<XMLDOMNode> col = NewNode("col");
        SetAttribute(col, "min", "2");
        SetAttribute(col, "max", "2");
        SetAttribute(col, "width", "32");
        SetAttribute(col, "bestFit", "1");
        cols->AddChild(col);
    }
    {
        RefPointer<XMLDOMNode> col = NewNode("col");
        SetAttribute(col, "min", "5");
        SetAttribute(col, "max", "5");
        SetAttribute(col, "width", "25");
        SetAttribute(col, "bestFit", "1");
        cols->AddChild(col);
    }

    RefPointer<XMLDOMNode> sheetData = NewNode("sheetData");
    dom->AddChild(sheetData);

    // ---- header row ----
    {
        RefPointer<XMLDOMNode> row = NewNode("row");
        {
            String number;
            row->SetAttribute(String("r"), number << (UInt32)1);
        }
        SetAttribute(row, "spans", "1:6");
        sheetData->AddChild(row);
        static const char* const kColumns[] = { "A", "B", "C", "D", "E", "F" };
        static const char* const kTitles[] = { "Brick", "Name", "Picture", "Part", "Color code", "Quantity" };
        for (UInt32 i = 0; i < 6; ++i) {
            RefPointer<XMLDOMNode> cell = NewNode("c");
            {
                String reference(kColumns[i]);
                cell->SetAttribute(String("r"), reference << (UInt32)1);
            }
            SetAttribute(cell, "t", "s");
            row->AddChild(cell);
            RefPointer<XMLDOMNode> value = NewNode("v");
            const UInt32 index = AddSharedString(String(kTitles[i]));
            String text;
            value->SetText(text << index);
            cell->AddChild(value);
        }
    }

    // ---- one row per brick ----
    UInt32 number = 2;
    for (const SheetRow_t* r = rows.Begin(); r != rows.End(); ++r, ++number) {
        RefPointer<XMLDOMNode> row = NewNode("row");
        {
            String text;
            row->SetAttribute(String("r"), text << number);
        }
        SetAttribute(row, "spans", "1:6");
        SetAttribute(row, "ht", "66");
        SetAttribute(row, "customHeight", "1");
        sheetData->AddChild(row);

        // A: item number (no cell without one)
        if (!r->m_name.IsEmpty()) {
            RefPointer<XMLDOMNode> cell = NewNode("c");
            {
                String reference("A");
                cell->SetAttribute(String("r"), reference << number);
            }
            SetAttribute(cell, "t", "s");
            row->AddChild(cell);
            RefPointer<XMLDOMNode> value = NewNode("v");
            const UInt32 index = AddSharedString(String(r->m_name));
            String text;
            value->SetText(text << index);
            cell->AddChild(value);
        }
        // B: localized name
        {
            RefPointer<XMLDOMNode> cell = NewNode("c");
            {
                String reference("B");
                cell->SetAttribute(String("r"), reference << number);
            }
            SetAttribute(cell, "t", "s");
            row->AddChild(cell);
            RefPointer<XMLDOMNode> value = NewNode("v");
            const UInt32 index = AddSharedString(String(r->m_localizedName));
            String text;
            value->SetText(text << index);
            cell->AddChild(value);
        }
        // (C: the picture of the drawing)
        // D: design ID
        {
            RefPointer<XMLDOMNode> cell = NewNode("c");
            {
                String reference("D");
                cell->SetAttribute(String("r"), reference << number);
            }
            row->AddChild(cell);
            RefPointer<XMLDOMNode> value = NewNode("v");
            String text;
            value->SetText(text << r->m_designID);
            cell->AddChild(value);
        }
        // E: colour codes
        {
            RefPointer<XMLDOMNode> cell = NewNode("c");
            {
                String reference("E");
                cell->SetAttribute(String("r"), reference << number);
            }
            SetAttribute(cell, "t", "s");
            row->AddChild(cell);
            RefPointer<XMLDOMNode> value = NewNode("v");
            const UInt32 index = AddSharedString(String(r->m_materials));
            String text;
            value->SetText(text << index);
            cell->AddChild(value);
        }
        // F: quantity
        {
            RefPointer<XMLDOMNode> cell = NewNode("c");
            {
                String reference("F");
                cell->SetAttribute(String("r"), reference << number);
            }
            row->AddChild(cell);
            RefPointer<XMLDOMNode> value = NewNode("v");
            String text;
            value->SetText(text << r->m_count);
            cell->AddChild(value);
        }
    }

    // ---- total ----
    {
        RefPointer<XMLDOMNode> row = NewNode("row");
        {
            String text;
            row->SetAttribute(String("r"), text << number);
        }
        SetAttribute(row, "spans", "1:6");
        sheetData->AddChild(row);
        {
            RefPointer<XMLDOMNode> cell = NewNode("c");
            {
                String reference("A");
                cell->SetAttribute(String("r"), reference << number);
            }
            SetAttribute(cell, "t", "s");
            row->AddChild(cell);
            RefPointer<XMLDOMNode> value = NewNode("v");
            const UInt32 index = AddSharedString(String("Total:"));
            String text;
            value->SetText(text << index);
            cell->AddChild(value);
        }
        {
            RefPointer<XMLDOMNode> cell = NewNode("c");
            {
                String reference("F");
                cell->SetAttribute(String("r"), reference << number);
            }
            row->AddChild(cell);
            RefPointer<XMLDOMNode> formula = NewNode("f");
            String text("SUM(F2:F");
            formula->SetText(text << number - 1 << ")");
            cell->AddChild(formula);
        }
    }

    RefPointer<XMLDOMNode> drawing = NewNode("drawing");
    SetAttribute(drawing, "xmlns:r", kRelationshipsNamespace);
    SetAttribute(drawing, "r:id", "rId1");
    dom->AddChild(drawing);
    return dom;
}

// Mac 0x3c0dc0, Win 0x670452
RefPointer<XMLDOM> ExportBOMCommand::CreateSharedStrings(BasicArray<String> strings)
{
    RefPointer<XMLDOM> dom = new XMLDOM(String("sst"));
    SetAttribute(dom, "xmlns", kMainNamespace);
    SetAttribute(dom, "count", m_sharedStringCount);
    SetAttribute(dom, "uniqueCount", strings.GetSize());
    for (const String* s = strings.Begin(); s != strings.End(); ++s) {
        RefPointer<XMLDOMNode> item = NewNode("si");
        RefPointer<XMLDOMNode> text = NewNode("t");
        text->SetText(*s);
        item->AddChild(text);
        dom->AddChild(item);
    }
    return dom;
}

// Mac 0x3c11a0, Win 0x66ffc0
RefPointer<XMLDOM> ExportBOMCommand::CreateSheetRelationships()
{
    RefPointer<XMLDOM> dom = new XMLDOM(String("Relationships"));
    SetAttribute(dom, "xmlns", kPackageRelationshipsNamespace);
    RefPointer<XMLDOMNode> relationship = NewNode("Relationship");
    SetAttribute(relationship, "Id", "rId1");
    SetAttribute(relationship, "Type", "http://schemas.openxmlformats.org/officeDocument/2006/relationships/drawing");
    SetAttribute(relationship, "Target", "../drawings/drawing1.xml");
    dom->AddChild(relationship);
    return dom;
}

// Mac 0x3c1500, Win 0x672201
RefPointer<XMLDOM> ExportBOMCommand::CreateCalcChain(SheetRows rows)
{
    RefPointer<XMLDOM> dom = new XMLDOM(String("calcChain"));
    SetAttribute(dom, "xmlns", kMainNamespace);
    RefPointer<XMLDOMNode> cell = NewNode("c");
    {
        String reference("F");
        cell->SetAttribute(String("r"), reference << rows.GetSize() + 2);
    }
    SetAttribute(cell, "i", "1");
    dom->AddChild(cell);
    return dom;
}

// Mac 0x3c17f0, Win 0x670798
RefPointer<XMLDOM> ExportBOMCommand::CreateDrawing(SheetRows rows)
{
    RefPointer<XMLDOM> dom = new XMLDOM(String("xdr:wsDr"));
    SetAttribute(dom, "xmlns:a", kDrawingNamespace);
    SetAttribute(dom, "xmlns:r", kRelationshipsNamespace);
    SetAttribute(dom, "xmlns:xdr", "http://schemas.openxmlformats.org/drawingml/2006/spreadsheetDrawing");

    UInt32 index = 1;
    UInt32 y = 1038225;                       // EMU of the picture's top in row `index`
    for (const SheetRow_t* r = rows.Begin(); r != rows.End(); ++r, y += 838200) {
        RefPointer<XMLDOMNode> anchor = NewNode("xdr:twoCellAnchor");
        SetAttribute(anchor, "editAs", "oneCell");
        dom->AddChild(anchor);

        // column C, row `index` (0-based rows: the brick's row)
        RefPointer<XMLDOMNode> from = NewNode("xdr:from");
        anchor->AddChild(from);
        AddTextChild(from, "xdr:col", String("2"));
        AddTextChild(from, "xdr:colOff", String("0"));
        {
            String text;
            AddTextChild(from, "xdr:row", text << index);
        }
        AddTextChild(from, "xdr:rowOff", String("0"));
        RefPointer<XMLDOMNode> to = NewNode("xdr:to");
        anchor->AddChild(to);
        AddTextChild(to, "xdr:col", String("2"));
        AddTextChild(to, "xdr:colOff", String("755650"));
        {
            String text;
            AddTextChild(to, "xdr:row", text << index);
        }
        AddTextChild(to, "xdr:rowOff", String("812800"));

        RefPointer<XMLDOMNode> picture = NewNode("xdr:pic");
        anchor->AddChild(picture);
        RefPointer<XMLDOMNode> nvPicPr = NewNode("xdr:nvPicPr");
        picture->AddChild(nvPicPr);
        RefPointer<XMLDOMNode> cNvPr = NewNode("xdr:cNvPr");
        const UInt32 next = index + 1;
        SetAttribute(cNvPr, "id", next);
        {
            String name;
            name << r->m_name << " Screenshot";
            cNvPr->SetAttribute(String("name"), name);
        }
        {
            String description;
            description << r->m_name;
            cNvPr->SetAttribute(String("descr"), description);
        }
        nvPicPr->AddChild(cNvPr);
        RefPointer<XMLDOMNode> cNvPicPr = NewNode("xdr:cNvPicPr");
        nvPicPr->AddChild(cNvPicPr);
        RefPointer<XMLDOMNode> picLocks = NewNode("a:picLocks");
        SetAttribute(picLocks, "noChangeAspect", "1");
        cNvPicPr->AddChild(picLocks);

        RefPointer<XMLDOMNode> blipFill = NewNode("xdr:blipFill");
        picture->AddChild(blipFill);
        RefPointer<XMLDOMNode> blip = NewNode("a:blip");
        SetAttribute(blip, "xmlns:r", kRelationshipsNamespace);
        SetAttribute(blip, "xmlns:a", kDrawingNamespace);
        {
            String id("image");
            blip->SetAttribute(String("r:embed"), id << index);
        }
        blipFill->AddChild(blip);
        RefPointer<XMLDOMNode> extLst = NewNode("a:extLst");
        blip->AddChild(extLst);
        RefPointer<XMLDOMNode> ext = NewNode("a:ext");
        SetAttribute(ext, "uri", "{28A0092B-C50C-407E-A947-70E740481C1C}");
        extLst->AddChild(ext);
        RefPointer<XMLDOMNode> useLocalDpi = NewNode("a14:useLocalDpi");
        SetAttribute(useLocalDpi, "xmlns:a14", "http://schemas.microsoft.com/office/drawing/2010/main");
        SetAttribute(useLocalDpi, "val", "0");
        ext->AddChild(useLocalDpi);
        RefPointer<XMLDOMNode> stretch = NewNode("a:stretch");
        SetAttribute(stretch, "xmlns:a", kDrawingNamespace);
        blipFill->AddChild(stretch);
        RefPointer<XMLDOMNode> fillRect = NewNode("a:fillRect");
        stretch->AddChild(fillRect);

        RefPointer<XMLDOMNode> spPr = NewNode("xdr:spPr");
        picture->AddChild(spPr);
        RefPointer<XMLDOMNode> xfrm = NewNode("a:xfrm");
        SetAttribute(xfrm, "xmlns:a", kDrawingNamespace);
        spPr->AddChild(xfrm);
        RefPointer<XMLDOMNode> offset = NewNode("a:off");
        SetAttribute(offset, "x", "3448050");
        {
            String text;
            offset->SetAttribute(String("y"), text << y);
        }
        xfrm->AddChild(offset);
        RefPointer<XMLDOMNode> extent = NewNode("a:ext");
        SetAttribute(extent, "cx", "813600");
        SetAttribute(extent, "cy", "813600");
        xfrm->AddChild(extent);
        RefPointer<XMLDOMNode> geometry = NewNode("a:prstGeom");
        SetAttribute(geometry, "xmlns:a", kDrawingNamespace);
        SetAttribute(geometry, "prst", "rect");
        spPr->AddChild(geometry);
        RefPointer<XMLDOMNode> avLst = NewNode("a:avLst");
        geometry->AddChild(avLst);

        RefPointer<XMLDOMNode> clientData = NewNode("xdr:clientData");
        anchor->AddChild(clientData);
        index = next;
    }
    return dom;
}

// Mac 0x3c3fe0, Win 0x671f41
RefPointer<XMLDOM> ExportBOMCommand::CreateDrawingRelationships(SheetRows rows)
{
    RefPointer<XMLDOM> dom = new XMLDOM(String("Relationships"));
    SetAttribute(dom, "xmlns", kPackageRelationshipsNamespace);
    UInt32 index = 1;
    for (const SheetRow_t* r = rows.Begin(); r != rows.End(); ++r, ++index) {
        RefPointer<XMLDOMNode> relationship = NewNode("Relationship");
        {
            String id("rId");
            relationship->SetAttribute(String("Id"), id << index);
        }
        SetAttribute(relationship, "Type", "http://schemas.openxmlformats.org/officeDocument/2006/relationships/image");
        {
            // "xl/media/image<n>.png" -> "../media/image<n>.png"
            String image(r->m_image);
            relationship->SetAttribute(String("Target"), String(image.Replace(0, 2, String(".."))));
        }
        {
            // NOTE: the Id is set a second time (both builds): "image<n>" replaces "rId<n>"
            // (drawing1.xml embeds "image<n>").
            String id("image");
            relationship->SetAttribute(String("Id"), id << index);
        }
        dom->AddChild(relationship);
    }
    return dom;
}

} // namespace LEGO
