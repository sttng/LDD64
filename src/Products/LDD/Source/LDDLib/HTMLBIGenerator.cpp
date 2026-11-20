// LEGO Digital Designer — restored source
// TU 113 "HTMLBIGenerator.cpp": Mac 0x1c06a0-0x1c829b, Win 0x68dd8c-0x6910f7 + 0x675447-0x675489
// (+ helpers 0x68da7e-0x68dbd7). See HTMLBIGenerator.h.
// Template code of this TU: the BasicMap<RefPointer<
// PrimitiveElementBrick>, UInt32, BrickLess> and BasicMap<String, UInt32> trees (Mac 0x1c06a0,
// 0x1c7700, 0x1c7750, 0x1c7c90 operator[], 0x1c7dc0, 0x1c8010, 0x1c80f0, 0x1c8170, 0x1c81c0),
// BasicArray destructors (0x1c06d0, 0x1c45e0, 0x1c7680, 0x1c77b0-0x1c7840, 0x1c7c60),
// BasicArray::InsertRange<PrimitiveElement*> (0x1c69f0), PushBack<RefPointer<…>> (0x1c5d90),
// NotifierT<HTMLBIGenerator, HTMLBIGeneratorObserver> (0x1c6020-0x1c6470, RemoveObserver
// 0x1c7870 + 0x1c7a60) and the thunks 0x1c66c0-0x1c69d0 come from the headers. The static
// initialiser 0x1c8220 is <iostream> plus the 24-byte header static of LuaLDDApplication.cpp's
// note (0, 0, 0, 0x1000000, 0, 0) — not code.
#include "HTMLBIGenerator.h"
#include "BGMode.h"
#include "BGApplication.h"
#include "BGScene.h"
#include "BIGenerator.h"
#include "BIBrick.h"
#include "BrickConnectSceneControl.h"
#include "BrickConnectSceneManager.h"
#include "BricksControl.h"
#include "ModelControl.h"
#include "ProgressBarDialog.h"
#include "RenderControlManager.h"
#include "LDDApplication.h"
#include "LDDUtilities.h"
#include "Application.h"
#include "Platform.h"
#include "TextLocale.h"
#include "StringConversion.h"
#include "BrickPalette.h"
#include "BrickDatabase.h"
#include "PrimitiveElement.h"
#include "Part.h"
#include "Scene.h"
#include "RenderQueue.h"
#include "SimpleSound.h"
#include "Directory.h"
#include "ReadFile.h"
#include "File.h"
#include "Path.h"
#include "Bitmap.h"
#include "DataStream.h"
#include "SafeFileWrite.h"   // SaveImage / WritePage
#if !defined(_WIN32)
#include "MacImageIO.h"
#endif

#include <iostream>

namespace LEGO {

// Mac 0x1c0740, Win 0x68ed28
HTMLBIGenerator::HTMLBIGenerator(BrickConnectSceneControl* sceneControl, RenderControlManager* manager,
                                 BrickConnectSceneManager* sceneManager, BGMode* mode)
    : m_sceneControl(sceneControl), m_mode(mode), m_manager(manager), m_sceneManager(sceneManager),
      m_stepCount(0), m_partCount(0), m_renderTarget(0), m_step(0)
{
    PreferencesObserver::Attach(Preferences::GetInstance().Get());
    m_pauseSound = LDDUtilities::GetInstance()->CreateSound(String("IA_pause.mp3"));
    m_renderQueue = new RenderQueue(m_manager->GetRender(), 2000);
}

// Mac 0x1c6510 / 0x1c6570, Win sdd 0x68f8f5
HTMLBIGenerator::~HTMLBIGenerator()
{
}

// Mac 0x1c65e0
void HTMLBIGenerator::OnFinalRelease()
{
    PreferencesObserver::OnFinalRelease();
    Notifier<HTMLBIGeneratorObserver>::OnFinalRelease();
    UpdateObserver::OnFinalRelease();
}

// Mac 0x1c5ed0 (thunk 0x1c6000), Win 0x68dd8c
void HTMLBIGenerator::OnPreferenceChanged(const String& key)
{
    if (key.EqualsNoCase(String("PLAYSOUND")))
        m_pauseSound = LDDUtilities::GetInstance()->CreateSound(String("IA_pause.mp3"));
}

// Mac inlined, Win 0x68da7e
void HTMLBIGenerator::ReplaceTag(String& target, const String& tag, const String& value)
{
    target.ReplaceAll(tag, 0, String::npos, value, 0, String::npos);
}

// fixed (not in the original): `text` as HTML text / attribute content: & < > " escaped as
// String::ToXML does, ' as &#39; (&apos; is not HTML 4); fixed: # as &#35;, so the inserted
// text cannot hold a tag literal that a later ReplaceTag over the whole page would expand.
static String EscapeHTML(const String& text)
{
    String escaped(text.ToXML());
    escaped.ReplaceAll(String("#"), String("&#35;"));   // (before &#39; is added)
    escaped.ReplaceAll(String("&apos;"), String("&#39;"));
    return escaped;
}

// fixed (not in the original): `path` (a relative folder name) as a URL path inside an HTML
// attribute or a quoted CSS url(): the ASCII characters that a URL would read as an escape, a
// query or a fragment (% ? #), that end or escape the quoted value (" ' \), that are markup
// (& < >) and the control characters are percent-encoded (%XX); everything else is kept as it is
// (so an ordinary name gives the same page as before), non-ASCII too (the page is UTF-8, browsers
// encode it themselves). The result holds no '#', so it cannot hold a tag literal either.
static String EncodeURLPath(const String& path)
{
    static const char kHex[] = "0123456789ABCDEF";
    String encoded;
    for (UInt32 i = 0; i < path.GetLength(); ++i) {
        const UniChar16 c = path.GetAt(i);
        if (c < 0x20 || c == 0x7f || c == '%' || c == '?' || c == '#' || c == '"' || c == '\'' ||
            c == '\\' || c == '&' || c == '<' || c == '>') {
            const char escape[4] = { '%', kHex[(c >> 4) & 0xf], kHex[c & 0xf], 0 };
            encoded << escape;
        } else {
            encoded.Append(c);
        }
    }
    return encoded;
}

// Mac inlined, Win 0x68db94
void HTMLBIGenerator::DeleteOutput()
{
    if (m_directory->Exists(m_imageDirectoryName))
        m_directory->Delete(m_imageDirectoryName, false);
    if (m_directory->Exists(m_fileName))
        m_directory->Delete(m_fileName, false);
}

// Mac inlined (Update 0x1c31e4 / 0x1c36c3 / 0x1c3e0f), Win 0x68e81d
void HTMLBIGenerator::Stop()
{
    if (!IsRunning())
        return;
    m_progressDialog->EndModal();
    m_progressDialog = 0;
    UpdateObserver::Detach();
}

// Mac 0x1c2dc0, Win 0x68defa
String HTMLBIGenerator::LoadTemplate(const String& name)
{
    RefPointer<Directory> assets = Platform::GetInstance()->GetSystemDirectory(Platform::kAssetsDirectory, true);
    AString data;
    if (!ReadFile(assets.Get(), String("Templates/") << name, data))
        return String();
    return DecodeString(kTextEncodingUTF8, data.GetBuffer(), data.GetLength());
}

// Mac 0x1c2f20, Win 0x68f35d
String HTMLBIGenerator::BuildPaletteList()
{
    String list;
    // NOTE: an unused local BasicMap<String, UInt32> (constructed and destroyed, both builds).
    BasicMap<String, UInt32> unused;
    BrickPalette* palette = m_sceneControl->GetBrickPalette();
    BrickPalette::BagArray::ConstIterator it = palette->GetBagsBegin();
    while (it != palette->GetBagsEnd()) {
        list << m_paletteTemplate;
        ReplaceTag(list, String("#PaletteCount#"), String("1"));
        // NOTE: the original inserts the palette name as it is (Win 0x68f35d); fixed: escaped.
        ReplaceTag(list, String("#PaletteName#"), EscapeHTML((*it)->m_name));
        ++it;
    }
    return list;
}

// Mac 0x1c0d80, Win 0x68f917
bool HTMLBIGenerator::Generate(Window* window)
{
    if (IsRunning())
        return false;

    m_stepBricks.Clear();
    m_step = 0;
    m_directory = 0;
    m_imageDirectory = 0;
    m_brickImages.Clear();

    // NOTE: taken and never used (both builds).
    RefPointer<Directory> documents = Platform::GetInstance()->GetSystemDirectory(Platform::kDocumentDirectory, true);

    String userDirectory;
    Preferences::GetInstance()->GetValue(String("UserBIDirectory"), userDirectory);
    m_directory = Directory::GetRoot()->GetDirectory(userDirectory, false);

    LocalizedString chooseMessage(String("FINDDIRECTORYFORHTMLSAVE"));
    Path chosen(String(), 0);
    if (!Platform::GetInstance()->ChooseFolder(chosen, chooseMessage, m_directory.Get(), window))
        return false;

    m_directory = chosen.GetDirectory(true);
    if (!m_directory) {
        LocalizedString message(String("NODIRFORHTMLSAVE"));
        LocalizedString title(String("ERRORTITLE"));
        LocalizedString ok(String("OK"));
        Application::GetInstance()->Alert(2, message, title, ok, String(), String());
        return false;
    }

    Preferences::GetInstance()->SetValue(String("UserBIDirectory"), m_directory->GetPath(true));

    if (!m_directory->GetAttributes().m_writable) {
        LocalizedString message(String("DIRNONWRITEABLE"));
        LocalizedString title(String("ERRORTITLE"));
        LocalizedString ok(String("OK"));
        Application::GetInstance()->Alert(2, message, title, ok, String(), String());
        return false;
    }

    m_modelName = LDDApplication::GetInstance()->GetDocumentName();
    if (m_modelName == String())
        m_modelName = "No Name";
    m_imageDirectoryName = String("Building Instructions [") << m_modelName << "]-images";
    m_fileName = String("Building Instructions [") << m_modelName << "].html";

    if (m_directory->DirectoryExists(m_imageDirectoryName, 0) || m_directory->FileExists(m_fileName)) {
        LocalizedString message(String("OVERWRITEHTMLDIALOGUE"));
        LocalizedString title(String("DIALOGWARNING"));
        LocalizedString yes(String("YES"));
        LocalizedString no(String("NO"));
        if (Application::GetInstance()->Alert(1, message, title, yes, no, String()) == 1)
            return false;
    }
    DeleteOutput();

    m_imageDirectory = m_directory->GetDirectory(m_imageDirectoryName, true);
    if (!m_imageDirectory) {
        LocalizedString message(String("NODIRFORHTMLPICTURESAVE"));
        LocalizedString title(String("ERRORTITLE"));
        LocalizedString ok(String("OK"));
        Application::GetInstance()->Alert(2, message, title, ok, String(), String());
        return false;
    }

    RefPointer<Directory> templates = Platform::GetInstance()->GetSystemDirectory(Platform::kAssetsDirectory, true);
    templates = templates->GetDirectory(String("Templates"), false);
    // NOTE: the original ignores the result of the copy of the template pictures (Win 0x68f917),
    // so a folder that takes no files gives a page without its header and logo; fixed: a failed
    // copy (of an Images folder the assets have) ends the generation with the picture folder's
    // error.
    if (templates && !templates->Copy(String("Images"), *m_imageDirectory, String("Images"), true, false) &&
        templates->DirectoryExists(String("Images"), 0)) {
        LocalizedString message(String("NODIRFORHTMLPICTURESAVE"));
        LocalizedString title(String("ERRORTITLE"));
        LocalizedString ok(String("OK"));
        Application::GetInstance()->Alert(2, message, title, ok, String(), String());
        return false;
    }

    m_stepCount = m_mode->GetApplication()->GetStepCount();
    if (m_stepCount == 0) {
        LocalizedString message(String("MISSINGMODEL"));
        LocalizedString title(String("ERRORTITLE"));
        LocalizedString ok(String("OK"));
        Application::GetInstance()->Alert(2, message, title, ok, String(), String());
        return false;
    }

    m_partCount = 0;
    m_mode->Rewind();
    ++m_step;

    m_page = LoadTemplate(String("BITemplate.html"));
    m_stepTemplate = LoadTemplate(String("BIStepTemplate.html"));
    m_paletteTemplate = LoadTemplate(String("BIPaletteListTemplate.html"));
    m_partTemplate = LoadTemplate(String("PartListTemplate.html"));
    m_elementTemplate = LoadTemplate(String("ElementListTemplate.html"));

    // NOTE: the original inserts the model name as it is (Win 0x68f917), so a name with & or <
    // breaks the page (or adds markup to it); fixed: escaped, # too (tags are replaced
    // one after another over the whole page, so a name holding e.g. "#ElementList#" was expanded).
    // (#ImageDir# is not entity-escaped: it is also used inside the template's <style>, where
    // entities are not decoded; it is percent-encoded instead, see Update.)
    ReplaceTag(m_page, String("#ModelName#"), EscapeHTML(LDDApplication::GetInstance()->GetDocumentName()));
    ReplaceTag(m_page, String("#ModelNameOrNoName#"), EscapeHTML(m_modelName));
    ReplaceTag(m_page, String("#PaletteList#"), BuildPaletteList());
    ReplaceTag(m_page, String("#OverviewImageSize#"),
               String("height=\"") << (UInt32)kOverviewImageHeight << "\" width=\""
                                    << (UInt32)kOverviewImageWidth << "\"");
    ReplaceTag(m_page, String("#StepImageWidth#"), String() << (UInt32)kStepImageWidth);
    ReplaceTag(m_page, String("#StepImageWidth2#"), String() << (UInt32)kPageWidth);
    ReplaceTag(m_stepTemplate, String("#StepImageSize#"),
               String("height=\"") << (UInt32)kStepImageHeight << "\" width=\""
                                    << (UInt32)kStepImageWidth << "\"");
    ReplaceTag(m_partTemplate, String("#BrickImageSize#"),
               String("height=\"") << (UInt32)kBrickImageHeight << "\" width=\""
                                    << (UInt32)kBrickImageWidth << "\"");

    UpdateObserver::Attach(MainUpdater::GetInstance().Get());

    m_progressDialog = new ProgressBarDialog(m_manager);
    if (!m_progressDialog->Initialize())
        return false;
    LocalizedString message(String("GENERATEHTMLBI"));
    m_progressDialog->SetProgress(0.0f, (Real32)m_stepCount);
    m_progressDialog->SetMessage(message);
    return true;
}

// The two step blocks of Update (Mac 0x1c32cc-0x1c3695 and 0x1c3c7c-0x1c413f)
void HTMLBIGenerator::TakeStepBricks(bool first)
{
    BasicArray<BIBrick*> bricks;
    m_mode->NextStep(true, false, bricks, true);
    if (!first)
        ++m_step;

    BasicArray<PrimitiveElement*> elements;
    for (BasicArray<BIBrick*>::Iterator it = bricks.Begin(); it != bricks.End(); ++it)
        if ((*it)->GetType() == 1)
            elements.InsertRange(elements.End(), (*it)->GetElements().Begin(), (*it)->GetElements().End());

    BasicArray<RefPointer<Part> > parts;
    for (BasicArray<PrimitiveElement*>::Iterator it = elements.Begin(); it != elements.End(); ++it)
        parts.PushBack(RefPointer<Part>((*it)->GetPart()));

    BasicArray<ObjectRefPointer<PrimitiveElementBrick> > elementBricks;
    CreateElementBricks(parts, elementBricks);

    m_stepBricks.Clear();
    if (first)
        m_allBricks.Clear();
    for (UInt32 i = 0; i < elementBricks.GetSize(); ++i) {
        RefPointer<PrimitiveElementBrick> brick(elementBricks[i].Get());
        brick->SetTransform(Matrix3D::Identity());
        m_stepBricks[brick] = m_stepBricks[brick] + 1;
        m_allBricks[brick] = m_allBricks[brick] + 1;
    }
}

// Mac 0x1c3140, Win 0x6908a9
void HTMLBIGenerator::Update()
{
    if (m_progressDialog->IsCancelled()) {
        DeleteOutput();
        Stop();
        return;
    }

    m_progressDialog->SetProgress((Real32)m_mode->GetApplication()->GetCurrentStep() + 1.0f, (Real32)m_stepCount);

    if (m_step == 1)
        TakeStepBricks(true);

    String step = BuildStep();
    if (step.IsEmpty()) {
        Stop();
        LocalizedString message(String("CANNOTGENERATEHTMLBI"));
        LocalizedString title(String("ERRORTITLE"));
        LocalizedString ok(String("OK"));
        Application::GetInstance()->Alert(2, message, title, ok, String(), String());
        return;
    }

    step << "#BISteps#";
    ReplaceTag(m_page, String("#BISteps#"), step);

    BGApplication* application = m_mode->GetApplication();
    if (!application->IsAtEnd()) {
        TakeStepBricks(false);
        return;
    }

    const String overview(String("Step") << m_step << ".png");
    ReplaceTag(m_page, String("#OverviewImage#"), overview);
    ReplaceTag(m_page, String("#BISteps#"), String(""));
    // NOTE: the original inserts the image folder name as it is (Win 0x6908a9), so a model name
    // with # % ? loses the images (read as fragment / query / escape), ' ends the CSS url() and "
    // ends the src attribute (markup injection); fixed: percent-encoded (the folder keeps its
    // name).
    ReplaceTag(m_page, String("#ImageDir#"), EncodeURLPath(m_imageDirectory->GetPath(false)));
    ReplaceTag(m_page, String("#ElementList#"), BuildElementList());
    ReplaceTag(m_page, String("#MaxStepNumber#"), String() << m_step);
    if (m_mode->GetApplication()->HasGenerators()) {
        Scene* scene = m_mode->GetApplication()->GetRootGenerator()->GetScene()->GetModelScene();
        ReplaceTag(m_page, String("#BrickCount#"), String() << scene->GetBricks().GetSize());
    } else {
        ReplaceTag(m_page, String("#BrickCount#"), String());
    }
    // NOTE: the original ignores WritePage's result (Win 0x6908a9): a page that could not be
    // written ends the generation as a success; fixed: the error dialog of the target folder
    // (WritePage writes the page all or nothing).
    if (!WritePage()) {
        Stop();
        LocalizedString message(String("NODIRFORHTMLPICTURESAVE"));
        LocalizedString title(String("ERRORTITLE"));
        LocalizedString ok(String("OK"));
        Application::GetInstance()->Alert(2, message, title, ok, String(), String());
        return;
    }
    Stop();
    if (m_pauseSound)
        m_pauseSound->Play();
}

// Mac 0x1c4610, Win 0x68f18b
String HTMLBIGenerator::BuildStep()
{
    String page(m_stepTemplate);
    ReplaceTag(page, String("#StepNumber#"), String() << m_step);
    const String image(String("Step") << m_step << ".png");
    if (!SaveStepImage(image, Vector2((Real32)kStepImageWidth, (Real32)kStepImageHeight)))
        return String();
    // NOTE: the original ignores a brick picture that could not be saved (Win 0x68f18b: the part
    // list gets an empty image name, GetBrickImage); fixed: the step fails as for its own picture
    // (Update's CANNOTGENERATEHTMLBI). The pictures are made here in BuildPartList's order, so
    // BuildPartList and BuildElementList find them all made.
    for (BrickCountMap::Iterator it = m_stepBricks.Begin(); it != m_stepBricks.End(); ++it)
        if (GetBrickImage(it->first.Get()).IsEmpty())
            return String();
    String partList = BuildPartList();
    ReplaceTag(page, String("#PartList#"), partList);
    ReplaceTag(page, String("#StepImage#"), image);
    return page;
}

// Mac 0x1c51e0, Win 0x68eb5d
String HTMLBIGenerator::BuildPartList()
{
    String list;
    for (BrickCountMap::Iterator it = m_stepBricks.Begin(); it != m_stepBricks.End(); ++it) {
        const String image = GetBrickImage(it->first.Get());
        list << m_partTemplate;
        // (over the whole list: the earlier items have no tags left)
        ReplaceTag(list, String("#BrickImage#"), image);
        ReplaceTag(list, String("#BrickAmount#"), String() << it->second);
        m_partCount += it->second;
    }
    return list;
}

// Mac 0x1c48d0, Win 0x68f646
String HTMLBIGenerator::BuildElementList()
{
    String list;
    for (BrickCountMap::Iterator it = m_allBricks.Begin(); it != m_allBricks.End(); ++it) {
        String item(m_elementTemplate);
        ReplaceTag(item, String("#BrickCount#"), String() << it->second);
        // NOTE: the image folder name as it is (Win 0x68f646); fixed: percent-encoded (see Update).
        ReplaceTag(item, String("#ImageDir#"), EncodeURLPath(m_imageDirectory->GetPath(false)));
        ReplaceTag(item, String("#BrickImageSize#"),
                   String("height=\"") << (UInt32)kBrickImageHeight << "\" width=\""
                                        << (UInt32)kBrickImageWidth << "\"");
        ReplaceTag(item, String("#BrickImage#"), GetBrickImage(it->first.Get()));
        // NOTE: the item number and the description are inserted as they are (Win 0x68f646);
        // fixed: escaped.
        ReplaceTag(item, String("#BrickNumber#"), EscapeHTML(it->first->GetBrick()->GetName()));
        ReplaceTag(item, String("#BrickName#"), EscapeHTML(GetBrickName(it->first.Get())));
        list << item;
    }
    return list;
}

// Mac 0x1c53f0, Win 0x68e975
String HTMLBIGenerator::GetBrickImage(PrimitiveElementBrick* brick)
{
    UInt32 index = 0;
    const RefPointer<const PrimitiveElementBrick>* it = m_brickImages.Begin();
    for (; it != m_brickImages.End(); ++it, ++index)
        if (**it == *brick)
            break;
    const String name(String("Brick") << index << ".png");
    if (it != m_brickImages.End())
        return name;

    m_brickImages.PushBack(RefPointer<const PrimitiveElementBrick>(brick));
    RefPointer<BricksControl> control = new BricksControl(m_manager, m_sceneManager, 2);
    if (control->Create(*brick)) {
        if (!SaveImage(name, Vector2((Real32)kBrickImageWidth, (Real32)kBrickImageHeight), control.Get()))
            return String();
    }
    return name;
}

// Mac 0x1c56e0, Win 0x68f466
String HTMLBIGenerator::GetBrickName(const PrimitiveElementBrick* brick)
{
    String name(m_sceneManager->GetDatabase()->GetLocalizedName(brick->GetBrick()->GetDesignID()) << " - ");
    BasicSet<UInt32> materials;
    for (UInt32 part = 0; part < brick->GetBrick()->m_parts.GetSize(); ++part) {
        const BasicArray<UInt32> ids(brick->GetMaterials(part));
        for (UInt32 i = 0; i < ids.GetSize(); ++i)
            if (ids[i] != 0)
                materials.Insert(ids[i]);
    }
    for (BasicSet<UInt32>::Iterator it = materials.Begin(); it != materials.End(); ++it) {
        if (it != materials.Begin())
            name << ",";
        name << LocalizedString(String("Material") << *it);
    }
    return name;
}

// Mac 0x1c5080, Win 0x68dbd7
bool HTMLBIGenerator::SaveStepImage(const String& name, const Vector2& size)
{
    const Path path(name, m_imageDirectory.Get());
    m_renderQueue->Clear();
    m_mode->QueueParts(m_renderQueue.Get());
    return SaveScreenshot(m_sceneControl.Get(), path, size, m_renderTarget, true, m_renderQueue.Get(), false);
}

// Mac 0x1c5ae0 (Win inlined in GetBrickImage); the shape of ExportBOMCommand::SaveImage
bool HTMLBIGenerator::SaveImage(const String& name, const Vector2& size, ModelControl* control)
{
    const Path file(name, m_imageDirectory.Get());
    Bitmap bitmap;
    bitmap.m_width = (UInt32)(Int64)size.x;
    bitmap.m_height = (UInt32)(Int64)size.y;
#if defined(_WIN32)
    bitmap.m_format = kBitmapFormatBGRA32;
    bitmap.m_pitch = (32 >> 3) * bitmap.m_width;
#else
    bitmap.m_depth = 1;
    bitmap.m_format = kBitmapFormatBGRA32;
    bitmap.m_pitch = bitmap.m_width * 4;
#endif
    bitmap.m_data = new UInt8[bitmap.m_pitch * bitmap.m_height];

    bool result = control->RenderToBitmap(bitmap, 0);
    if (result) {
#if defined(_WIN32)
        RefPointer<DataStreamInterface> png = EncodePNG(bitmap);
#else
        RefPointer<DataStreamInterface> png = EncodePNGWithImageIO(bitmap);
#endif
        // NOTE: the original ignores SetSize and reports success when the picture cannot be
        // encoded (Win 0x68e975, inlined); fixed: WriteWholeFile (all or nothing, read back), and
        // no picture is a failure.
        result = !png.IsNull() && WriteWholeFile(file, png->GetData(), (UInt32)png->GetSize());
    }
    delete[] (UInt8*)bitmap.m_data;
    return result;
}

// Mac 0x1c4db0, Win 0x68dfcd
bool HTMLBIGenerator::WritePage()
{
    // NOTE: the original ignores the results of SetSize and Write (Win 0x68dfcd), so a failed
    // write (a full disk) leaves a cut page and is opened as a success; fixed: written all or
    // nothing (WriteWholeFile: "<name>.tmp", read back, renamed over the page), false on any
    // failure, and only a complete page is opened.
    const Path path(m_fileName, m_directory.Get());
    const AString text = EncodeString(kTextEncodingUTF8, m_page, 0, String::npos);
    if (!WriteWholeFile(path, text.GetBuffer(), text.GetLength()))
        return false;
    Platform::GetInstance()->OpenDocument(path);
    return true;
}

} // namespace LEGO
