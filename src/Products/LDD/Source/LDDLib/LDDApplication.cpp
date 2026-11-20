// LEGO Digital Designer — restored source
// LEGO::LDDApplication — sub-TUs 082a (constructor, assets, fonts, round-edge shaders, start-up
// checks; Mac 0x16ee30-0x173ff0) and 082b (documents, alerts, recent models, brand, update;
// Mac 0x173ff0-0x17b230) of TU 082 "LDDApplication.cpp", and 082c (palettes,
// modules, library groups, render switches, version check, destructor, OnFinalRelease; Mac
// 0x17b230-0x18126b, at the end of this file).
// See LDDApplication.h for the layout. Win twins: constructor 0x5ffb95, LoadAssets 0x602179,
// fonts 0x5fe125, start-up checks 0x601ad7, the rest per function.
//
// Template instances of the TU restored through the shared templates: BasicArray<Pair<String,
// String>>::PushBack (Mac 0x16ee30) and its destructor (0x16f010), BasicArray<ObjectRefPointer<
// Font>> destructor (0x16efb0), BitmapMask::LoadArgs destructor (0x173d40), the Notifier
// iteration of every notification (inlined: 0x174310, 0x1780a0, 0x178370, 0x178690, 0x17aa40).
// The small helpers of 082c this code uses (std::find over the recent models 0x1809a0, the
// <Meta> helpers 0x17f830 / 0x17b110 / 0x180d40 / 0x180f60 / 0x180ae0) are file-local below.
#include "LDDApplication.h"

#include "Platform.h"
#include "Preferences.h"
#include "StringStream.h"
#include "TextLocale.h"
#include "Directory.h"
#include "ObjectFactory.h"
#include "Version.h"
#include "Window.h"
#include "Menu.h"
#include "Font.h"
#include "Color.h"
#include "Render.h"
#include "Shader.h"
#include "BitmapMask.h"
#include "SimpleSound.h"
#include "GraphicsCardInfo.h"
#include "RenderControlManager.h"
#include "ButtonSoundAugmentation.h"
#include "Command.h"
#include "PostProcessor.h"
#include "BrickGeometry.h"
#include "BrickDatabase.h"
#include "ModelLoader.h"
#include "ModelIOLXFML5.h"
#include "ModelIOLXFML4.h"
#include "ModelIOLDraw.h"
#include "LuaPlus.h"
#include "LuaScriptManager.h"
#include "LuaBindings.h"
#include "LuaCommandAdapter.h"
#include "BrickPalette.h"
#include "BrickConnectSceneControl.h"
#include "BrickConnectSceneManager.h"
#include "ModelLoaderHandler.h"
#include "BI.h"
#include "LDDAlertBox.h"
#include "ProgressBarDialog.h"
#include "PreferencesControl.h"
#include "GenerateBuildingGuideCommand.h"
#include "LDDUtilities.h"
#include "Trace.h"
#include "ReadFile.h"
#include "Primitive.h"
#include "OperationManager.h"
#include "ModelLoaderAbstract.h"
#include "BrickPaletteBagLoaderPAXML.h"
#include "PaletteNames.h"

// SaveModel's disk replacement (M1: Directory::ReplaceFile) and the archive read-back (M2).
#include "SafeFileWrite.h"

namespace LEGO {

namespace {

// ---- small helpers -------------------------------------------------------------------------

// Preferences value `key` into `value` when present (the inlined GetInstance / GetValue /
// FromString sequence of the original).
template <class T>
void ReadPreference(const char* key, T& value)
{
    RefPointer<Preferences> preferences = Preferences::GetInstance();
    String text;
    if (preferences->GetValue(String(key), text))
        FromString(value, text);
}

template <class T>
void WritePreference(const char* key, const T& value)
{
    RefPointer<Preferences> preferences = Preferences::GetInstance();
    String text;
    ToString(value, text);
    preferences->SetValue(String(key), text);
}

// std::find over the recent models (Mac 0x1809a0, String::operator==) followed by the erase of
// that one element (the shift loop inlined at every caller).
void EraseFirst(BasicArray<String>& strings, const String& value)
{
    for (UInt32 i = 0; i < strings.GetSize(); ++i) {
        if (strings[i] == value) {
            strings.Erase(i);
            return;
        }
    }
}

// "a#b#c" (Mac 0x178690 / 0x17a720).
// NOTE: the original joins the paths with '#' and LoadAssets splits the value at every '#', so
// a path containing '#' comes back as several broken entries (dropped as missing files).
// fixed: LDDApplication::EncodeRecentModels: a '#' before every entry, '%' and '#' inside an
// entry as "%25" / "%23"; a VFS path never starts with '#', so DecodeRecentModels still reads
// the old "a#b#c" values of existing preference files (M9).
String JoinRecentModels(const BasicArray<String>& models)
{
    return LDDApplication::EncodeRecentModels(models);
}

// ---- SaveModel helpers (M1, M2; not in the original): SafeFileWrite.h ----------------------
// Directory::DoSupportsReplaceFile / ReplaceFile (the disk storage flushes a file and renames it
// over another in one step) and ArchiveEntrySize / ArchiveEntryReadsBack (the archive read-back).

// <Meta> helpers (082c, Mac 0x17b110 / 0x180d40 / 0x180f60 / 0x17f830).
void AddMetaAttribute(StorageScene2::Meta& meta, const String& name, const String& value)
{
    StorageScene2::Meta::Attribute attribute;
    attribute.m_name = name;
    attribute.m_value = value;
    meta.m_attributes.PushBack(attribute);
}

template <class T>
void AddMetaAttributeValue(StorageScene2::Meta& meta, const String& name, const T& value)
{
    String text;
    ToString(value, text);
    AddMetaAttribute(meta, name, text);
}

// Removes the first <Meta> child named `name` (Mac 0x17f830).
void RemoveMeta(StorageScene2& storage, const String& name)
{
    for (UInt32 i = 0; i < storage.m_metas.GetSize(); ++i) {
        if (storage.m_metas[i].m_name == name) {
            storage.m_metas.Erase(i);
            return;
        }
    }
}

void ReplaceMeta(StorageScene2& storage, const StorageScene2::Meta& meta)
{
    RemoveMeta(storage, meta.m_name);
    storage.m_metas.PushBack(meta);
}

} // namespace

// ================================================================================================
// 082a: constructor, assets
// ================================================================================================

// Mac 0x16f0a0 (C1), Win 0x5ffb95
LDDApplication::LDDApplication(const String& name, BasicArray<String>& arguments,
                               const Preferences::ValueMap& defaults)
    : Application(name, arguments, defaults),
      m_untitledCount(0),
      m_online(false),
      m_extendedTooltip(false),
      m_importing(false),
      m_ready(false),
      m_brand(kBrandLDD),
      m_useShadersInScene(false),
      m_useShadersInPalette(false),
      m_roundEdgeShadersLoaded(false),
      m_useRoundEdges(false),
      m_shadersSupported(false),
      m_flag184(false),
      m_flag185(false),
      m_checkBuildingGuide(false),
      m_data187(0)
{
    // NOTE: +0x10d (m_sortByIdOnly) and +0x17c (m_postProcessing) are not initialised (both
    // builds); LoadAssets sets them. Fixed: false (low).
    m_data170[0] = m_data170[1] = m_data170[2] = 0;
    m_sortByIdOnly = false;
    m_postProcessing = false;
#if defined(_WIN32) || defined(LDD_LINUX)
    // Win only: a document named by the preference "Open" (command line) is opened after the
    // start-up (Mac: -[LEGOApplicationDelegate application:openFile:]). LDD64 Linux likewise
    // (main.mm turns a file argument into "-Open <file>").
    {
        String file;
        Preferences::GetInstance()->GetValue(String("Open"), file);
        if (!file.IsEmpty())
            m_documentToOpen = Platform::GetInstance()->NativeToVFSPath(file);
    }
#endif
    m_brickPalette = new BrickPalette();

    m_modelFileTypes.PushBack(FileType(String("LXF-Files (*.lxf)"), String("lxf")));
    bool developerMode = false;
    ReadPreference("DeveloperMode", developerMode);
    if (developerMode) {
        m_modelFileTypes.PushBack(FileType(String("LXFML-Files (*.lxfml)"), String("lxfml")));
        m_modelFileTypes.PushBack(FileType(String("LDraw-Files (*.ldr)"), String("ldr")));
    }
    m_exchangeFileTypes.PushBack(FileType(String("LXF-Files (*.lxf)"), String("lxf")));
    m_exchangeFileTypes.PushBack(FileType(String("LXFML-Files (*.lxfml)"), String("lxfml")));
    m_exchangeFileTypes.PushBack(FileType(String("LDraw-Files (*.ldr)"), String("ldr")));
    m_exchangeFileTypes.PushBack(FileType(String("LXFML4-Files (*.lxfml)"), String("lxfml")));
}

// Mac 0x1701c0, Win 0x5fbcd9 (→ Application::GetInstance 0x4226b0)
LDDApplication* LDDApplication::GetInstance()
{
    return static_cast<LDDApplication*>(Application::GetInstance());
}

// Mac 0x1701d0, Win 0x602179
bool LDDApplication::LoadAssets(Window* window, RenderControlManager* manager,
                                GraphicsCardInfo* cardInfo)
{
    m_window = window;
    m_graphicsCardInfo = cardInfo;
    if (!LoadFonts(manager->GetObjectFactory()))
        return false;

    // ---- render switches
    m_shadersSupported = manager->GetRender()->SupportsShaders();
    ReadPreference("UseShadersInScene", m_useShadersInScene);
    m_useShadersInScene = m_useShadersInScene ? m_shadersSupported : false;
    WritePreference("UseShadersInScene", m_useShadersInScene);
    ReadPreference("UseShadersInPalette", m_useShadersInPalette);
    m_useShadersInPalette = m_useShadersInPalette ? m_shadersSupported : false;
    WritePreference("UseShadersInPalette", m_useShadersInPalette);
    if (m_useShadersInScene && m_useShadersInPalette)
        manager->GetRender()->SetShaderMaterialColors(true);
    if (!m_shadersSupported) {
        m_useRoundEdges = false;
    } else {
        ReadPreference("UseRoundEdges", m_useRoundEdges);
        LoadRoundEdgeShaders(manager->GetRender());
        bool roundEdges = false;
        if (m_roundEdgeShadersLoaded && m_useRoundEdges && m_useShadersInPalette &&
            m_useShadersInScene)
            roundEdges = manager->GetRender()->SupportsFloatTextures();
        m_useRoundEdges = roundEdges;
    }
    WritePreference("UseRoundEdges", m_useRoundEdges);
    BrickGeometry::SetLoadShaderData(m_useRoundEdges);

    // ---- post-processing
    m_postProcessor = new PostProcessor(manager->GetRender());
    if (!m_shadersSupported) {
        m_postProcessing = false;
    } else {
        if (!m_postProcessor->Initialize(m_useRoundEdges))
            return false;
        if (GetPostProcessingEffects().IsEmpty()) {
            m_postProcessing = false;
        } else {
            m_postProcessing = m_useShadersInScene;
            if (m_postProcessing) {
                // NOTE: read through the application instance, kept only in the preferences.
                UInt32 selected = GetInstance()->m_postProcessor->GetEffects().GetSize();
                ReadPreference("SelectedPostProcessingEffect", selected);
                WritePreference("SelectedPostProcessingEffect", selected);
            }
        }
    }

    // ---- dialogs
    m_preferencesControl = new PreferencesControl(manager);
    if (!m_preferencesControl->Initialize())
        return false;
    m_alertBox = new LDDAlertBox(manager);
    if (!m_alertBox->Initialize())
        return false;
    m_progressDialog = new ProgressBarDialog(manager);
    if (!m_progressDialog->Initialize())
        return false;

    // ---- recent models ("a#b#c"; "none" when there are none)
    {
        String text;
        Preferences::GetInstance()->GetValue(String("RecentModels"), text);
        BasicArray<String> models;
        // NOTE: a '#' inside a path splits it (see JoinRecentModels); fixed (M9).
        DecodeRecentModels(text, models);
        if (&models != &m_recentModels)
            m_recentModels = models;
        EraseFirst(m_recentModels, String("none"));
    }

    // ---- picking mask
    {
        BitmapMask::LoadArgs args = BitmapMask::LoadArgs(Path(String(), 0));
        args.m_path.SetElement(Platform::GetInstance()->GetSystemDirectory(Platform::kAssetsDirectory, true).Get());
        args.m_path.Append(String("Graphics/General/masker.bmp"));
        m_mask = ObjectFactory::GetInstance()->Create<BitmapMask>(args);
        if (!m_mask)
            return false;
    }

    // ---- sounds, preferences
    m_pauseSound = LDDUtilities::GetInstance()->CreateSound(String("IA_pause.mp3"));
    m_clickSound = LDDUtilities::GetInstance()->CreateSound(String("IA_mouseclick.mp3"));
    PreferencesObserver::Attach(Preferences::GetInstance().Get());
    ReadPreference("SortByIdOnly", m_sortByIdOnly);

    // ---- "LEGO Creations/Models" in the documents
    RefPointer<Directory> documents =
        Platform::GetInstance()->GetSystemDirectory(Platform::kDocumentDirectory, true);
    if (documents) {
        if (!documents->DirectoryExists(String("LEGO Creations"), 0)) {
            RefPointer<Directory> creations = documents->GetDirectory(String("LEGO Creations"), true);
            RefPointer<Directory> models = documents->GetDirectory(String("LEGO Creations/Models"), true);
        } else if (!documents->DirectoryExists(String("LEGO Creations/Models"), 0)) {
            RefPointer<Directory> models = documents->GetDirectory(String("LEGO Creations/Models"), true);
        }
    }

    if (!DoStartupChecks())
        return false;

    // ---- the version that ran last
    Version version;
    if (GetVersion(version)) {
        RefPointer<Preferences> preferences = Preferences::GetInstance();
        String text;
        ToString(version, text);
        preferences->SetValue(String("ApplicationVersion"), text);
        Preferences::GetInstance()->RemoveValue(String("ApplicationVersionMajor"));
        Preferences::GetInstance()->RemoveValue(String("ApplicationVersionMinor"));
        Preferences::GetInstance()->RemoveValue(String("ApplicationVersionRevision"));
        Preferences::GetInstance()->RemoveValue(String("ApplicationVersionBuild"));
    }
    return true;
}

// Mac 0x171bb0, Win 0x5fe125: the font table of GetFont (index: system type, size, colour).
bool LDDApplication::LoadFonts(ObjectFactory* factory)
{
    static const struct { UInt32 type; UInt32 size; UInt32 color; } kFonts[] = {
        // color: 0 black, 1 white, 2 (0.8, 0.24, 0.24), 3 red, 4 gray
        { Font::kToolTipsFont,      0, 0 },   //  0
        { Font::kBoldSystemFont,   14, 0 },   //  1
        { Font::kTitleBarFont,      0, 0 },   //  2
        { Font::kLabelFont,         0, 0 },   //  3
        { Font::kLabelFont,         0, 1 },   //  4
        { Font::kLabelFont,         0, 2 },   //  5
        { Font::kMessageFont,       0, 0 },   //  6
        { Font::kMessageFont,      11, 0 },   //  7
        { Font::kMessageFont,       9, 0 },   //  8
        { Font::kControlContentFont, 0, 0 },  //  9
        { Font::kSystemFont,        0, 0 },   // 10
        { Font::kBoldSystemFont,    0, 0 },   // 11
        { Font::kBoldSystemFont,   11, 0 },   // 12
        { Font::kBoldSystemFont,   24, 0 },   // 13
        { Font::kSystemFont,       16, 0 },   // 14
        { Font::kToolTipsFont,      0, 3 },   // 15
        { Font::kBoldSystemFont,   48, 0 },   // 16
        { Font::kBoldSystemFont,   11, 0 },   // 17
        { Font::kControlContentFont, 16, 0 }, // 18
        { Font::kMessageFont,      11, 4 }    // 19
    };
    // NOTE: the original writes the twenty blocks out (one argument block reused).
    Font::SystemTypeFontArgs args;
    ObjectRefPointer<Font> font;
    for (UInt32 i = 0; i < sizeof(kFonts) / sizeof(kFonts[0]); ++i) {
        switch (kFonts[i].color) {
        case 0:  args.color = Color::Black(); break;
        case 1:  args.color = Color::White(); break;
        case 2:  args.color = Color(0.8f, 0.24f, 0.24f, 1.0f); break;
        case 3:  args.color = Color::Red(); break;
        default: args.color = Color::Gray(); break;
        }
        args.type = kFonts[i].type;
        args.size = kFonts[i].size;
        font = factory->Create<Font>(args);
        if (!font)
            return false;
        m_fonts.PushBack(font);
    }
    return true;
}

// Mac 0x172d20, Win 0x601fc8
void LDDApplication::LoadRoundEdgeShaders(Render* render)
{
    RefPointer<Directory> assets =
        Platform::GetInstance()->GetSystemDirectory(Platform::kAssetsDirectory, true);
    if (!assets)
        return;
    RefPointer<Directory> shaders = assets->GetDirectory(String("Shaders/RoundEdge"), false);
    if (!shaders)
        return;
    DescriptorSet entries;
    shaders->Enumerate(String("*.sh"), entries);
    for (DescriptorSet::Iterator it = entries.Begin(); it != entries.End(); ++it) {
        Shader::LoadArgs args;
        args.path.Assign(Path(it->m_name, shaders.Get()));
        RefPointer<Shader> shader = render->GetObjectFactory()->Create<Shader>(args);
        if (!shader) {
            m_roundEdgeShadersLoaded = false;
            return;
        }
    }
    m_roundEdgeShadersLoaded = true;
}

// Mac 0x173150, Win 0x5fbd6b
bool LDDApplication::AreRoundEdgeShadersLoaded() const
{
    return m_roundEdgeShadersLoaded;
}

// Mac 0x173160, Win 0x5fccae
const BasicArray<RefPointer<PostProcessingEffect> >& LDDApplication::GetPostProcessingEffects() const
{
    return m_postProcessor->GetEffects();
}

// Mac 0x173430, Win 0x601ad7
bool LDDApplication::DoStartupChecks()
{
    if (!Preferences::GetInstance()->GetBool(String("DOSTARTUPCHECKS"), true))
        return true;
    // "Cleanup" is true until a start-up finished the checks (removed at the end).
    const bool cleanup = Preferences::GetInstance()->GetBool(String("Cleanup"), true);
    WritePreference("Cleanup", true);
    Preferences::GetInstance()->Save();
    const bool newer = IsNewerVersion();

    RefPointer<Directory> user = GetUserDataDirectory();
    if (!user)
        return false;
    String databaseName;
    Preferences::GetInstance()->GetValue(String("userBrickDBDirName"), databaseName);

    RefPointer<Directory> database;
    if (!cleanup && !newer)
        database = user->GetDirectory(databaseName, false);
    if (cleanup || newer || !database) {
        database = 0;
        // The user database and the user palettes are rebuilt by the start mode.
        String palettesName;
        Preferences::GetInstance()->GetValue(String("userPalettesDirName"), palettesName);
        DescriptorSet entries;
        user->Enumerate(String("*"), entries);
        for (DescriptorSet::Iterator it = entries.Begin(); it != entries.End(); ++it) {
            String name(it->m_name);
            bool remove = true;
            if (name.CompareNoCase(0, 2, String("db")) != 0 &&
                name.CompareNoCase(0, 8, String("palettes")) != 0) {
                if (!databaseName.IsEmpty() &&
                    name.CompareNoCase(0, databaseName.GetLength(), databaseName) == 0) {
                    remove = true;
                } else if (!palettesName.IsEmpty()) {
                    remove = name.CompareNoCase(0, palettesName.GetLength(), palettesName) == 0;
                } else {
                    remove = false;
                }
            }
            if (remove)
                user->Delete(name, true);
        }
    }
    Preferences::GetInstance()->RemoveValue(String("Cleanup"));
    Preferences::GetInstance()->Save();
    return true;
}

// Mac 0x173db0
void LDDApplication::SetBrickDatabase(BrickDatabase* database)
{
    m_database = database;
}

// Mac 0x173e00, Win 0x5fc29c
Window* LDDApplication::GetWindow() const
{
    return m_window.Get();
}

// Mac 0x173e10
void LDDApplication::ReleaseResources()
{
    m_window = 0;
    m_preferencesControl = 0;
    m_alertBox = 0;
    m_progressDialog = 0;
    m_brickPalette = 0;
    m_loaderHandler = 0;
    m_storage = 0;
    m_mask = 0;
    m_pauseSound = 0;
    m_clickSound = 0;
    m_editCommand = 0;
    BrickConnectSceneControlObserver::Detach();
}

// ================================================================================================
// 082b: Application overrides
// ================================================================================================

// Mac 0x173ff0, Win 0x5fc2a3
bool LDDApplication::IsIdle()
{
    if (IsModal())
        return false;
    return !m_window || !m_window->IsModal();
}

void LDDApplication::OnEvent7() {}                                         // Mac 0x174030
void LDDApplication::OnEvent8() {}                                         // Mac 0x174040

// Mac 0x174050, Win 0x5fc2cb
bool LDDApplication::CanQuit()
{
    if (IsModal() || (m_window && m_window->IsModal()))
        return false;
    return QuerySaveChanges();
}

// Mac 0x174090, Win 0x5fc2fe: the window frame for the next start.
void LDDApplication::OnQuit()
{
    if (!m_window)
        return;
    m_window->SetMenu(new Menu());
    const WindowStyle& style = m_window->GetStyle();
    String origin;
    String size;
    ToString(style.position, origin);
    ToString(style.size, size);
    Preferences::GetInstance()->SetValue(String("mainWindowOrigin"), origin);
    Preferences::GetInstance()->SetValue(String("mainWindowSize"), size);
    Preferences::GetInstance()->Save();
}

// Mac 0x1742e0, Win 0x6036d6: the open panel.
bool LDDApplication::NewDocument()
{
    if (!CanChangeDocument())
        return false;
    return OpenModel(0);
}

// Mac 0x174a80, Win 0x6036f8
bool LDDApplication::OpenDocumentFile(const Path& path)
{
    if (m_editCommand)
        m_editCommand->Do();
    if (!CanChangeDocument())
        return false;
    return OpenModel(&path);
}

// Mac 0x174ac0, Win 0x5fc47f
bool LDDApplication::SaveDocument()
{
    BrickConnectSceneControl* control = BrickConnectSceneControlObserver::GetSubject();
    if (!control || (!control->IsModified() && !m_flag185))
        return false;
    if (m_editCommand)
        m_editCommand->Do();
    control->GetSceneManager()->SetActiveTool(0);
    if (RefPointer<Directory>(m_documentPath.GetParentDirectory()))
        return SaveDocumentTo(m_documentPath);
    return SaveDocumentAs();
}

// Mac 0x174b60, Win 0x60435e
bool LDDApplication::SaveDocumentAs()
{
    Path path(String(), 0);
    BrickConnectSceneControl* control = BrickConnectSceneControlObserver::GetSubject();
    if (control)
        control->GetSceneManager()->SetActiveTool(0);
    UInt32 typeIndex = 0;
    Platform* platform = Platform::GetInstance();
    RefPointer<Directory> directory = m_documentPath.GetParentDirectory();
    if (!directory)
        directory = GetUserModelDirectory();
    const bool chosen = platform->GetSavePath(path, directory.Get(), String(m_documentPath.GetName()),
                                              m_modelFileTypes, &typeIndex, m_window.Get(), String(""));
    if (!chosen) {
        if (control)
            control->RebuildRenderData();
        return false;
    }
    UInt32 format = 0;
    if (!PrepareSavePath(path, typeIndex, m_modelFileTypes, format))
        return false;
    return SaveModelAs(path, format);
}

// Mac 0x1753d0, Win 0x6044ff
bool LDDApplication::SaveDocumentTo(const Path& path)
{
    return SaveModelAs(path, kSaveFormatAny);
}

// Mac 0x1753f0, Win 0x5fc506
bool LDDApplication::QuerySaveChanges()
{
    BrickConnectSceneControl* control = BrickConnectSceneControlObserver::GetSubject();
    if (!control)
        return true;
    const bool untitled = !m_documentPath.GetParentDirectory() && m_documentPath.GetName().IsEmpty();
    if (!control->IsModified()) {
        if (!m_flag185)
            return true;
        if (!untitled)
            return SaveDocument();
    } else if (m_flag185 && !untitled) {
        return SaveDocument();
    }

    LocalizedString question;
    if (untitled) {
        question.Load(String("SAVEMODEL"));
    } else {
        question.Load(String("SAVECHANGESTO"));
        question.ReplaceAll(String("%s"), 0, String::npos, String(m_documentPath.GetName()), 0,
                            String::npos);
    }
    // message = CHANGESWILLBELOST, title (the alert box's heading) = the question (Win 0x5fc6c0)
    const UInt32 answer = Alert(3, LocalizedString(String("CHANGESWILLBELOST")), question,
                                LocalizedString(String("SAVE")), LocalizedString(String("CANCEL")),
                                LocalizedString(String("DONTSAVE")));
    if (answer == 0)
        return SaveDocument();
    return answer == 2;
}

// Mac 0x1757c0, Win 0x600fb9
bool LDDApplication::CloseDocument()
{
    BrickConnectSceneControl* control = BrickConnectSceneControlObserver::GetSubject();
    if (!control || !QuerySaveChanges())
        return false;
    control->ClearScene();
    SetDocumentPath(Path(String(), 0));
    return true;
}

// Mac 0x177c80, Win 0x5fc802
UInt32 LDDApplication::Alert(UInt32 style, const String& message, const String& title,
                             const String& button1, const String& button2, const String& button3)
{
    if (m_alertBox)
        return m_alertBox->Run(style, message, title, button1, button2, button3, 0, String());
    return Application::Alert(style, message, title, button1, button2, button3);
}

// Mac 0x177ed0, Win 0x5fc938: runs the Lua command "ldd.commands.about".
void LDDApplication::ShowAboutBox()
{
    if (IsModal() || (m_window && m_window->IsModal()))
        return;
    RefPointer<LuaScriptManager> scripts = LuaScriptManager::GetInstance();
    LuaPlus::LuaObject globals = scripts->GetLuaState()->GetGlobals();
    LuaPlus::LuaObject about = globals.Lookup("ldd.commands.about");
    Command* command = 0;
    if (BindableLuaObject* bound = GetBoundObject(about)) {
        if (LuaCommandAdapter* adapter = dynamic_cast<LuaCommandAdapter*>(bound))
            command = adapter->GetAdaptedObject();
        else
            command = dynamic_cast<Command*>(bound);
    }
    if (command)
        command->Do();
}

// Mac 0x178050, Win 0x5fc9de
void LDDApplication::ShowPreferences()
{
    if (IsModal() || (m_window && m_window->IsModal()))
        return;
    if (m_preferencesControl)
        m_preferencesControl->ShowModal();
}

// Mac 0x176560, Win 0x5fdfab
bool LDDApplication::OpenDocument(const Path& path, StorageScene2* storage)
{
    if (!CanChangeDocument())
        return false;
    if (BrickConnectSceneControlObserver::GetSubject()) {
        m_pendingPath.Assign(path);
        LoadDocument(storage);
    }
    return true;
}

// Mac 0x1765c0, Win 0x60029d
void LDDApplication::LoadDocument(StorageScene2* storage)
{
    m_importing = false;
    BrickConnectSceneControl* control = BrickConnectSceneControlObserver::GetSubject();
    control->ClearScene();
    Brand brand;
    if (LDDUtilities::GetModelBrand(storage, brand))
        SetBrand(brand);
    m_loaderHandler = control ? static_cast<ModelLoaderHandler*>(control) : 0;
    m_loaderHandler->OnLoadStarted(storage);
    UpdateObserver::Attach(MainUpdater::GetInstance().Get());
    LocalizedString message(String("LOADINGMODEL"));
    m_progressDialog->SetProgress(0.0f, 100.0f);
    m_progressDialog->SetMessage(message);
}

// Mac 0x177ba0, Win 0x5fc763
String LDDApplication::GetDocumentName() const
{
    if (!RefPointer<Directory>(m_documentPath.GetParentDirectory()))
        return String("");
    String name(m_documentPath.GetName());
    return name.Substring(0, name.FindLastOf('.', String::npos));
}

// Mac 0x177c70, Win 0x5fbcde
const Path& LDDApplication::GetDocumentPath() const
{
    return m_documentPath;
}

// Mac 0x178670 (empty)
void LDDApplication::OnAttached(BrickConnectSceneControl* /*control*/) {}

// Mac 0x178c20 (thunk 0x178c50), Win 0x5fca19
void LDDApplication::OnEvent13(bool flag)
{
    if (m_window)
        m_window->SetModified(flag);
}

// Mac 0x178c80 (thunk 0x179710), Win 0x600461: model loading progress.
void LDDApplication::Update()
{
    if (m_progressDialog->IsCancelled() && !m_loaderHandler->IsCancelled())
        m_loaderHandler->OnLoadCancelled();
    if (m_loaderHandler->IsLoading()) {
        m_progressDialog->SetProgress((Real32)m_loaderHandler->GetProgress(), 100.0f);
        return;
    }

    UpdateObserver::Detach();
    m_progressDialog->EndModal();
    m_storage = 0;
    m_loaderHandler->OnLoadFinished();
    BrickConnectSceneControl* control = BrickConnectSceneControlObserver::GetSubject();

    if (!m_loaderHandler->IsCancelled()) {
        if (m_loaderHandler->Succeeded()) {
            if (!m_importing &&
                (m_pendingPath.GetParentDirectory() || !m_pendingPath.GetName().IsEmpty())) {
                SetDocumentPath(m_pendingPath);
                AddRecentModel(m_pendingPath);
                m_pendingPath.Assign(Path(String(), 0));
            } else {
                control->SetModified(true);
            }
        }
        if (m_pauseSound)
            m_pauseSound->Play();
        if (m_loaderHandler->GetMissingBrickCount() != 0 ||
            m_loaderHandler->GetUnplaceableBrickCount() != 0) {
            String text;
            if (m_loaderHandler->GetMissingBrickCount() != 0) {
                LocalizedString missing;
                if (m_loaderHandler->GetMissingBrickCount() == 1) {
                    missing.Load(String("MISSINGBRICKONLOAD"));
                } else {
                    missing.Load(String("MISSINGBRICKSONLOAD"));
                    String count;
                    count << m_loaderHandler->GetMissingBrickCount();
                    missing.ReplaceAll(String("%s"), 0, String::npos, count, 0, String::npos);
                }
                text = text << missing;
            }
            if (m_loaderHandler->GetUnplaceableBrickCount() != 0) {
                if (m_loaderHandler->GetMissingBrickCount() != 0)
                    text = text << " " << LocalizedString(String("AND")) << " ";
                LocalizedString unplaceable;
                if (m_loaderHandler->GetUnplaceableBrickCount() == 1) {
                    unplaceable.Load(String("UNPLACEABLEBRICKONLOAD"));
                } else {
                    unplaceable.Load(String("UNPLACEABLEBRICKSONLOAD"));
                    String count;
                    count << m_loaderHandler->GetUnplaceableBrickCount();
                    unplaceable.ReplaceAll(String("%s"), 0, String::npos, count, 0, String::npos);
                }
                text = text << unplaceable;
            }
            Alert(1, text, LocalizedString(String("DIALOGWARNING")), LocalizedString(String("OK")),
                  String(), String());
        }
    } else if (!m_importing) {
        SetDocumentPath(Path(String(), 0));
        m_pendingPath.Assign(Path(String(), 0));
    }
    control->SetKeyboardFocus(true);
}

// Mac 0x179730 (thunk 0x179860), Win 0x5fca28
void LDDApplication::OnPreferenceChanged(const String& key)
{
    if (key.CompareNoCase(String("PLAYSOUND")) == 0)
        m_pauseSound = LDDUtilities::GetInstance()->CreateSound(String("IA_pause.mp3"));
}

// ================================================================================================
// 082b: documents
// ================================================================================================

// Mac 0x174310
bool LDDApplication::CanChangeDocument()
{
    {
        NotifierT<LDDApplication, LDDApplicationObserver>::Iterator it(*this);
        while (LDDApplicationObserver* observer = it.Next(m_observers)) {
            if (!observer->OnQuery10())
                return false;
        }
    }
    if (!m_ready || IsModal())
        return false;
    return m_window && !m_window->IsModal();
}

// Mac 0x1745d0, Win 0x602d21
bool LDDApplication::OpenModel(const Path* path)
{
    if (!QuerySaveChanges())
        return false;
    Path model(String(), 0);
    if (path) {
        model.Assign(*path);
    } else {
        Platform* platform = Platform::GetInstance();
        RefPointer<Directory> directory = GetUserModelDirectory();
        const bool chosen = platform->GetOpenPath(model, directory.Get(), String(), m_modelFileTypes,
                                                  m_window.Get(), String(""));
        directory = 0;
        if (!chosen)
            return false;
        StoreUserModelDirectory(model);
    }
    if (!BrickConnectSceneControlObserver::GetSubject() || m_storage)
        return true;
    bool oldVersion;
    if (LoadModelFile(model, oldVersion)) {
        m_pendingPath.Assign(model);
        LoadDocument(m_storage.Get());
        return true;
    }
    if (oldVersion) {
        const UInt32 answer = Alert(1, LocalizedString(String("LOADOFOLDMODELS")),
                                    LocalizedString(String("DIALOGWARNING")),
                                    LocalizedString(String("YES")), LocalizedString(String("NO")),
                                    String());
        if (answer == 0)
            return OpenModelDirectly(model);
    }
    return false;
}

// Mac 0x174e90, Win 0x5fcb90
RefPointer<Directory> LDDApplication::GetUserModelDirectory()
{
    String path;
    Preferences::GetInstance()->GetValue(String("UserModelDirectory"), path);
    return Directory::GetRoot()->GetDirectory(path, false);
}

// Mac 0x174fd0, Win 0x5fcfc9
bool LDDApplication::PrepareSavePath(Path& path, UInt32 typeIndex, const FileTypeArray& types,
                                     UInt32& format)
{
    String name(path.GetName());
    const UInt32 npos = String::npos;
    if (name.Substring(0, name.ReverseFind('.', npos)).IsEmpty()) {
        LocalizedString message(String("UNABLETOSAVETOEMPTYFILENAME"));
        Platform::ShowMessage(message, 0, 0);
        return false;
    }
    if (types.IsEmpty()) {
        format = kSaveFormatAny;
    } else {
        String extension;
        if (name.FindLastOf('.', npos) != npos)
            extension = name.Substring(name.FindLastOf('.', npos) + 1, npos);
        if (extension != types[typeIndex].second) {
            String fixed(name);
            path.SetName(fixed << "." << types[typeIndex].second);
        }
        format = typeIndex;
    }
    return true;
}

// Mac 0x175260, Win 0x60426c
bool LDDApplication::SaveModelAs(const Path& path, UInt32 format)
{
    BrickConnectSceneControl* control = BrickConnectSceneControlObserver::GetSubject();
    if (!control)
        return false;
    if (!SaveModel(control, path, LocalizedString(String("UNABLETOSAVEMODELTO")),
                   LocalizedString(String("DIALOGWARNING")), format, true, 0))
        return false;
    control->SetModified(false);
    SetDocumentPath(path);
    AddRecentModel(path);
    StoreUserModelDirectory(path);
    return true;
}

// Mac 0x1758d0, Win 0x600368
void LDDApplication::SetDocumentPath(const Path& path)
{
    m_documentPath.Assign(path);
    if (!m_documentPath.GetParentDirectory() && m_documentPath.GetName().IsEmpty()) {
        m_metas.Clear();
        LocalizedString untitled(String("Untitled"));
        String number;
        if (m_untitledCount != 0)
            number << m_untitledCount;
        untitled.ReplaceAll(String("%s"), 0, String::npos, number, 0, String::npos);
        m_documentPath.SetName(untitled);
        ++m_untitledCount;
    }
    m_window->SetRepresentedFile(m_documentPath);
}

// Mac 0x175aa0, Win 0x602d00 (Lua "Import")
void LDDApplication::Import(const Path& path, ModelLoaderHandler* handler)
{
    m_loaderHandler = handler;
    ImportFile(path);
}

// Mac 0x175af0, Win 0x601717
bool LDDApplication::ImportFile(const Path& path)
{
    if (!CanChangeDocument() || !LoadModelStorage(path))
        return false;
    m_pendingPath.Assign(Path(String(), 0));
    StartImport(m_storage.Get());
    return true;
}

// Mac 0x175c20
bool LDDApplication::ImportWithPanel(ModelLoaderHandler* handler)
{
    m_loaderHandler = handler;
    return ImportWithPanel();
}

// Mac 0x175c70, Win 0x602f99
bool LDDApplication::ImportWithPanel()
{
    Path path(String(), 0);
    Platform* platform = Platform::GetInstance();
    RefPointer<Directory> directory = GetUserModelDirectory();
    const bool chosen = platform->GetOpenPath(path, directory.Get(), String(), m_exchangeFileTypes,
                                              m_window.Get(), LocalizedString(String("IMPORT")));
    directory = 0;
    if (!chosen)
        return false;
    return ImportFile(path);
}

// Mac 0x175e80, Win 0x601025
bool LDDApplication::ImportStorage(StorageScene2* storage, ModelLoaderHandler* handler)
{
    if (!CanChangeDocument())
        return false;
    m_loaderHandler = handler;
    m_pendingPath.Assign(Path(String(), 0));
    StartImport(storage);
    return true;
}

// Mac 0x175fd0, Win 0x6009b5
void LDDApplication::StartImport(StorageScene2* storage)
{
    m_importing = true;
    m_loaderHandler->OnLoadStarted(storage);
    UpdateObserver::Attach(MainUpdater::GetInstance().Get());
    LocalizedString message(String("IMPORTINGMODEL"));
    m_progressDialog->SetProgress(0.0f, 100.0f);
    m_progressDialog->SetMessage(message);
}

// Mac 0x176100, Win 0x6044ff
bool LDDApplication::Export()
{
    Path path(String(), 0);
    UInt32 typeIndex = 0;
    Platform* platform = Platform::GetInstance();
    RefPointer<Directory> directory = m_documentPath.GetParentDirectory();
    if (!directory)
        directory = GetUserModelDirectory();
    const bool chosen = platform->GetSavePath(path, directory.Get(), String(m_documentPath.GetName()),
                                              m_exchangeFileTypes, &typeIndex, m_window.Get(),
                                              LocalizedString(String("EXPORT")));
    directory = 0;
    if (!chosen)
        return false;
    UInt32 format = 0;
    if (!PrepareSavePath(path, typeIndex, m_exchangeFileTypes, format))
        return false;
    return ExportTo(path, format);
}

// Mac 0x176430, Win 0x604153
bool LDDApplication::ExportTo(const Path& path, UInt32 format)
{
    BrickConnectSceneControl* control = BrickConnectSceneControlObserver::GetSubject();
    if (!control)
        return false;
    return SaveModel(control, path, LocalizedString(String("UNABLETOEXPORTMODELTO")),
                     LocalizedString(String("DIALOGWARNING")), format, true, 0);
}

// Mac 0x176770
bool LDDApplication::SaveModel(BrickConnectSceneControl* control, const Path& path,
                               const String& errorMessage, const String& errorTitle, bool thumbnail,
                               ModalControl* progress)
{
    return SaveModel(control, path, errorMessage, errorTitle, kSaveFormatAny, thumbnail, progress);
}

// Mac 0x1767a0, Win 0x603830
bool LDDApplication::SaveModel(BrickConnectSceneControl* control, const Path& path,
                               const String& errorMessage, const String& errorTitle, UInt32 format,
                               bool thumbnail, ModalControl* progress)
{
    String name(path.GetName());
    String message(errorMessage);
    message.ReplaceAll(String("%s"), 0, String::npos,
                       Platform::GetInstance()->VFSToNativePath(path.GetPath(true)), 0, String::npos);

    if (m_checkBuildingGuide && !control->GetBuildingInstructions()->HasSteps()) {
        // "Save without a building guide?" — OK generates one first.
        Application* application = Application::GetInstance();
        if (application->Alert(0, LocalizedString(String("NOBUILDINGGUIDE")),
                               LocalizedString(String("NOBUILDINGGUIDETITLE")),
                               LocalizedString(String("OK")), LocalizedString(String("CANCEL")),
                               String()) != 0)
            return false;
        m_buildingGuideCommand->Do();
        if (!m_buildingGuideCommand->WasGenerated())
            return false;
    }

    RefPointer<StorageScene2> storage = new StorageScene2();
    control->SaveToStorage(*storage);
    storage->m_name = name.Substring(0, name.ReverseFind('.', String::npos));
    FillMeta(*storage);

    // NOTE: the original writes straight into `path`. An existing .lxf is opened and its entries
    // are rewritten in place (data behind them moved), a plain file is overwritten and then cut
    // to size: a crash or a full disk part way leaves a broken model (M1). The archive entries
    // reach the file only when their last file object goes (ZipStorageFile::OnFinalRelease),
    // which drops the result, and the thumbnail and the truncation results are not looked at
    // either, so a failed write reports success (M2).
    // fixed: on disk the model is written to a new "<name>.tmp" next to `path`; a .lxf is then
    // mounted again and its entries must read back completely with the sizes written; the file
    // is flushed to the disk and renamed over `path` in one step (MoveFileEx / rename). Any
    // failure deletes the temporary file, leaves `path` as it was and shows the save error
    // (the original's alert). Outside the disk storage (Upload's "memory" directory) the file is
    // still written in place, with the read-back check (M1, M2).
    const bool archived = name.CompareNoCase(name.GetLength() - 4, 4, String(".lxf")) == 0;
    const String fileName(archived ? String("IMAGE100.LXFML") : name);
    const String pictureName("IMAGE100.PNG");
    RefPointer<ModelSaver> saver;
    if (fileName.CompareNoCase(fileName.GetLength() - 6, 6, String(".lxfml")) == 0) {
        if (format == kSaveFormatLXFML4)
            saver = new ModelSaverLXFML4();
        else
            saver = new ModelSaverLXFML5();
    } else if (fileName.CompareNoCase(fileName.GetLength() - 4, 4, String(".ldr")) == 0) {
        saver = new ModelSaverLDraw();
    } else {
        return false;
    }

    RefPointer<Directory> folder = path.GetParentDirectory();
    const bool replace = !folder.IsNull() && folder->DoSupportsReplaceFile();
    String writeName(name);
    if (replace)
        writeName << ".tmp";
    const Path target(writeName, folder.Get());     // `path` itself when not replacing
    bool result = true;
    if (replace && target.Exists()) {
        target.Delete(false);                       // left behind by an earlier failed save
        result = !target.Exists();
    }
    UInt64 modelSize = 0;
    UInt64 pictureSize = 0;
    if (result && archived) {
        BasicArray<String> components;
        components.PushBack(writeName);
        Directory::OpenArgs args(folder.Get(), components, true);
        RefPointer<Directory> archive = ObjectFactory::GetInstance()->Create<Directory>(args);
        result = !archive.IsNull();
        if (result) {
            if (thumbnail) {
                BasicArray<RefPointer<Brick> > hidden;
                control->GetHiddenBricks(hidden);
                control->ShowAllBricks();
                Path picture(pictureName, archive.Get());
                SaveScreenshot(control, picture, Vector2(128.0f, 128.0f), 0, true, 0, true);
                control->HideBricks(hidden);
            }
            result = saver->Save(Path(fileName, archive.Get()), *storage);
            modelSize = ArchiveEntrySize(archive.Get(), fileName);
            pictureSize = ArchiveEntrySize(archive.Get(), pictureName);
        }
    } else if (result) {
        result = saver->Save(target, *storage);
    }
    if (result && archived) {
        // the archive is closed: mount it again as the loader does and read the entries back
        // (a thumbnail that was not rendered is not required)
        RefPointer<Directory> archive = target.GetDirectory(false);
        result = !archive.IsNull() && ArchiveEntryReadsBack(archive.Get(), fileName, modelSize) &&
                 (pictureSize == 0 || ArchiveEntryReadsBack(archive.Get(), pictureName, pictureSize));
    }
    if (result && replace)
        result = folder->ReplaceFile(writeName, name);   // flush + rename in one step
    if (!result) {
        if (replace)
            target.Delete(false);
        if (progress)
            progress->EndModal();
        Alert(2, message, errorTitle, LocalizedString(String("OK")), String(), String());
    }
    return result;
}

// Mac 0x177d50
UInt32 LDDApplication::AlertWithCheckBox(UInt32 style, const String& key, const String& checkBoxText,
                                         const String& message, const String& title,
                                         const String& button1, const String& button2,
                                         const String& button3)
{
    if (!Preferences::GetInstance()->GetBool(key, true))
        return 0;
    bool dontShowAgain = false;
    const UInt32 answer = m_alertBox->Run(style, message, title, button1, button2, button3,
                                          &dontShowAgain, checkBoxText);
    if (dontShowAgain) {
        dontShowAgain = false;
        String text;
        ToString(dontShowAgain, text);
        Preferences::GetInstance()->SetValue(key, text);
    }
    return answer;
}

// Mac 0x178690, Win 0x5feeaa
void LDDApplication::AddRecentModel(const Path& path)
{
    if (!path.Exists())
        return;
    EraseFirst(m_recentModels, path.GetPath(true));
    m_recentModels.Insert(m_recentModels.Begin(), 1, path.GetPath(true));
    if (m_recentModels.GetSize() > 10)
        m_recentModels.Resize(10, String());
    Preferences::GetInstance()->SetValue(String("RecentModels"), JoinRecentModels(m_recentModels));

    NotifierT<LDDApplication, LDDApplicationObserver>::Iterator it(*this);
    while (LDDApplicationObserver* observer = it.Next(m_observers))
        observer->OnRecentModelsChanged(path);
}

// Mac 0x17a720, Win 0x5fdfdd
void LDDApplication::RemoveRecentModels(const BasicArray<String>& models)
{
    for (UInt32 i = 0; i < models.GetSize(); ++i)
        EraseFirst(m_recentModels, models[i]);
    if (m_recentModels.IsEmpty())
        Preferences::GetInstance()->SetValue(String("RecentModels"), String("none"));
    else
        Preferences::GetInstance()->SetValue(String("RecentModels"), JoinRecentModels(m_recentModels));
}

// Mac 0x178660, Win 0x5fbd25
const BasicArray<String>& LDDApplication::GetRecentModels() const
{
    return m_recentModels;
}

// Fix M9 (not in the original): "#a#b#c" with '%' / '#' inside an entry as "%25" / "%23".
String LDDApplication::EncodeRecentModels(const BasicArray<String>& models)
{
    String text;
    for (UInt32 i = 0; i < models.GetSize(); ++i) {
        const String& model = models[i];
        text << "#";
        for (UInt32 c = 0; c < model.GetLength(); ++c) {
            const UniChar16 ch = model.GetAt(c);
            if (ch == '%')
                text << "%25";
            else if (ch == '#')
                text << "%23";
            else
                text.Append(ch);
        }
    }
    return text;
}

// Fix M9: reads EncodeRecentModels' format and, without the leading '#', the original "a#b#c"
// (a VFS path never starts with '#').
void LDDApplication::DecodeRecentModels(const String& text, BasicArray<String>& models)
{
    models.Clear();
    if (!text.StartsWith((UniChar16)'#')) {
        text.Split('#', models);
        return;
    }
    BasicArray<String> parts;
    text.Split('#', parts);
    for (UInt32 i = 1; i < parts.GetSize(); ++i) {   // parts[0]: the empty text before the first '#'
        const String& part = parts[i];
        String model;
        for (UInt32 c = 0; c < part.GetLength(); ++c) {
            const UniChar16 ch = part.GetAt(c);
            if (ch == '%' && c + 2 < part.GetLength() && part.GetAt(c + 1) == '2' &&
                (part.GetAt(c + 2) == '5' || part.GetAt(c + 2) == '3')) {
                model.Append((UniChar16)(part.GetAt(c + 2) == '5' ? '%' : '#'));
                c += 2;
            } else {
                model.Append(ch);
            }
        }
        models.PushBack(model);
    }
}

// Mac 0x179880, Win 0x5fcabe
void LDDApplication::StoreUserModelDirectory(const Path& path)
{
    if (!RefPointer<Directory>(path.GetParentDirectory()))
        return;
    RefPointer<Directory> directory = path.GetParentDirectory();
    Preferences::GetInstance()->SetValue(String("UserModelDirectory"), directory->GetPath(true));
}

// Mac 0x1799c0, Win 0x601796
bool LDDApplication::OpenModelDirectly(const Path& path)
{
    BrickConnectSceneControl* control = BrickConnectSceneControlObserver::GetSubject();
    if (!control)
        return true;
    if (!LoadModelStorage(path))
        return false;
    control->ClearScene();
    SetDocumentPath(Path(String(), 0));
    m_pendingPath.Assign(Path(String(), 0));
    LoadDocument(m_storage.Get());
    return true;
}

// Mac 0x179bb0, Win 0x601217
bool LDDApplication::LoadModelFile(const Path& path, bool& oldVersion)
{
    oldVersion = false;
    m_storage = new StorageScene2();
    Path file(String(), 0);
    RefPointer<Directory> archive = path.GetDirectory(false);
    if (!archive)
        file.Assign(path);
    else
        file.Assign(Path(String("IMAGE100.LXFML"), archive.Get()));

    RefPointer<ModelLoaderLXFML5> loader = new ModelLoaderLXFML5(m_database.Get());
    if (!loader->LoadFile(file, *m_storage, true)) {
        oldVersion = (loader->GetErrorMask() & ModelLoader::Error::versionError) != 0;
        if (!oldVersion)
            ShowModelLoadError(*loader, path, *m_storage);
        if (!CheckLoaderErrors(*loader, *m_storage)) {
            m_storage = 0;
            return false;
        }
    }
    if (&m_storage->m_metas != &m_metas)
        m_metas = m_storage->m_metas;
    BrickConnectSceneControlObserver::GetSubject()->GetSceneManager()->GetDatabase()->ValidateScene(*m_storage);
    return true;
}

// Mac 0x17a0c0
bool LDDApplication::LoadModelStorage(const Path& path)
{
    BrickDatabase* database =
        BrickConnectSceneControlObserver::GetSubject()->GetSceneManager()->GetDatabase();
    m_storage = new StorageScene2();
    if (!LoadModel(path, *m_storage, database, true)) {
        m_storage = 0;
        return false;
    }
    return true;
}

// Mac 0x17ad00, Win 0x600aeb
void LDDApplication::FillMeta(StorageScene2& storage)
{
    if (&storage.m_metas != &m_metas)
        storage.m_metas = m_metas;

    StorageScene2::Meta application;
    application.m_name = String("Application");
    AddMetaAttribute(application, String("name"), GetName());
    Version version;
    GetVersion(version);
    AddMetaAttributeValue(application, String("versionMajor"), version.m_major);
    AddMetaAttributeValue(application, String("versionMinor"), version.m_minor);
    ReplaceMeta(storage, application);

    StorageScene2::Meta brand;
    brand.m_name = String("Brand");
    AddMetaAttributeValue(brand, String("name"), m_brand);
    ReplaceMeta(storage, brand);

    StorageScene2::Meta brickSet;
    brickSet.m_name = String("BrickSet");
    AddMetaAttributeValue(brickSet, String("version"),
        BrickConnectSceneControlObserver::GetSubject()->GetSceneManager()->GetDatabase()->GetVersion());
    ReplaceMeta(storage, brickSet);
}

// ================================================================================================
// 082b: small accessors and state
// ================================================================================================

// Mac 0x1780a0
void LDDApplication::SetExtendedTooltip(bool enabled)
{
    if (enabled == m_extendedTooltip)
        return;
    m_extendedTooltip = enabled;
    NotifierT<LDDApplication, LDDApplicationObserver>::Iterator it(*this);
    while (LDDApplicationObserver* observer = it.Next(m_observers))
        observer->OnExtendedTooltipChanged(m_extendedTooltip);
}

// Mac 0x178340
bool LDDApplication::IsExtendedTooltip() const
{
    return m_extendedTooltip;
}

// Mac 0x178350, Win 0x5fca12
BrickPalette* LDDApplication::GetBrickPalette() const
{
    return m_brickPalette.Get();
}

// Mac 0x178360, Win 0x5fbcec
bool LDDApplication::IsOnline() const
{
    return m_online;
}

// Mac 0x178370, Win 0x5fd9f7
void LDDApplication::SetOnline(bool online)
{
    if (m_online == online)
        return;
    m_online = online;
    NotifierT<LDDApplication, LDDApplicationObserver>::Iterator it(*this);
    while (LDDApplicationObserver* observer = it.Next(m_observers))
        observer->OnOnlineStatusChanged(m_online);
}

// Mac 0x178610
bool LDDApplication::IsValidBrand(UInt32 brand) const
{
    return brand < 3;
}

// Mac 0x178620, Win 0x5fbd0b
bool LDDApplication::IsBrandLDDExtended() const
{
    return LEGO::IsBrandLDDExtended(m_brand);
}

// Mac 0x178640
UInt32 LDDApplication::GetBrandBrickSet() const
{
    return LEGO::GetBrandBrickSet(m_brand);
}

// Mac 0x17a2d0, Win 0x5fcc7a (no range check)
Font* LDDApplication::GetFont(UInt32 index) const
{
    return m_fonts[index].Get();
}

// Mac 0x17a2f0, Win 0x5fcc8a
BitmapMask* LDDApplication::GetMask() const
{
    return m_mask.Get();
}

// Mac 0x17a300, Win 0x600a54
void LDDApplication::AddButtonSound(Control* control)
{
    if (!m_clickSound)
        return;
    RefPointer<ButtonSoundAugmentation> augmentation = new ButtonSoundAugmentation(m_clickSound.Get());
    augmentation->Attach(control, String("bspa"));
}

// Mac 0x17a6e0
SimpleSound* LDDApplication::GetClickSound() const
{
    return m_clickSound.Get();
}

// Mac 0x17a6f0
void LDDApplication::SetReady(bool ready)
{
    m_ready = ready;
}

// Mac 0x17a710
bool LDDApplication::ReturnTrue17a710() const
{
    return true;
}

// Mac 0x17a9a0, Win 0x5fcc98 (Lua "SetEditCommand")
void LDDApplication::SetEditCommand(Command* command)
{
    m_editCommand = command;
}

// Mac 0x17a9f0, Win 0x5fcca3
void LDDApplication::SetBuildingGuideCommand(GenerateBuildingGuideCommand* command)
{
    m_buildingGuideCommand = command;
}

// Mac 0x17aa40, Win 0x5fdae6 (Lua "SetBrand")
void LDDApplication::SetBrand(Brand brand)
{
    if ((UInt32)brand > 2 || m_brand == brand)
        return;
    m_brand = brand;
    NotifierT<LDDApplication, LDDApplicationObserver>::Iterator it(*this);
    while (LDDApplicationObserver* observer = it.Next(m_observers))
        observer->OnBrandChanged(m_brand);
}

// Mac 0x17acf0, Win 0x5fbd3c (Lua "GetBrand")
Brand LDDApplication::GetBrand() const
{
    return m_brand;
}

// ================================================================================================
// 082c: palettes, modules, library groups, render switches, version check,
// destructor. Mac 0x17b230-0x18126b. Template instances restored through the shared templates:
// BasicArray<RefPointer<LoadModuleOperation>> push / destructor (0x17ecf0, 0x17f2b0, 0x181170),
// BasicArray<RefPointer<StorageScene2>>::PushBack (0x17ee30), BasicArray<ObjectRefPointer<Font>>
// push (0x17e510, LoadFonts), the Notifier iterations and destructors (0x17e410, 0x17f320-
// 0x17f710), BasicMap destructors of the model savers' maps (0x180320-0x1805c0), the <Meta>
// vector assignment (0x180ae0, 0x180d40-0x181060), the LdrawMapXMLLoader arrays (0x180610-
// 0x180750), BitmapMask::LoadArgs D0 (0x17f7b0). The palette loader classes' vtables
// (BrickPaletteBagLoaderPAXML.h) are emitted here.
// NOTE (both builds): the "Loading palette from path", "Could not load palette", "Could not
// parse module", "Could not load / parse library group", "Could not find" and "Unable to open
// user palette directory" messages are built and dropped (no trace call).
// ================================================================================================

// Loads one palette folder's bricks on a worker thread (BrickPaletteBagLoaderPAXML).
// RTTI: si Operation. Mac vtables 0x76907c {Execute 0x17fb80, D1 0x17fbe0, D0 0x17fc40},
// Object-in 0x7690a0 {thunks 0x17fcb0, 0x17fd10, Object slots}; Win 0x9cd5f4 {0x5fc232}, 0x9cd5e0
// {sdd 0x5fc272}. Mac sizeof 0x4c (Object +0x44). (member names inferred)
class LoadPaletteBricksOperation : public Operation {
public:
    // Mac 0x17eb30, Win 0x5fc1c3
    LoadPaletteBricksOperation(BrickDatabase* database, Directory* directory, PaletteNames* names)
        : m_bag(new BrickPalette::Bag()), m_ok(false), m_database(database), m_names(names),
          m_directory(directory)
    {
    }
    virtual ~LoadPaletteBricksOperation() {}                                // Mac 0x17fbe0 / 0x17fc40 (body 0x17fdb0), Win sdd 0x5fc272

    // Mac 0x17fb80, Win 0x5fc232
    virtual void Execute()
    {
        BrickPaletteBagLoaderPAXML loader;
        m_ok = loader.Load(m_directory, *m_bag, m_database, m_names);
    }

    RefPointer<BrickPalette::Bag> m_bag;        // +0x30
    bool                          m_ok;         // +0x34
    BrickDatabase*                m_database;   // +0x38
    PaletteNames*                 m_names;      // +0x3c
    RefPointer<Directory>         m_directory;  // +0x40
};

// Reads one module / library group model (an LXFML file already read into m_data) into a
// StorageScene2 on a worker thread. RTTI: si Operation. Mac vtables 0x76914c {Execute 0x17fe50,
// D1 0x1800a0, D0 0x180100}, Object-in 0x769170 {0x180170, 0x1801d0, Object slots}; Win 0x9cdb70
// {0x601612}, 0x9cdb5c {sdd 0x5fd96e}. Mac sizeof 0x58 (Object +0x50). (member names inferred)
class LoadModuleOperation : public Operation {
public:
    // Mac 0x17ec20, Win 0x5fd642
    LoadModuleOperation(BrickDatabase* database, const String& path)
        : m_path(path), m_ok(false), m_database(database)
    {
    }
    virtual ~LoadModuleOperation() {}                                       // Mac 0x1800a0 / 0x180100 (body 0x180250), Win sdd 0x5fd96e

    // ModelLoaderAbstract (all LXFML versions / LDraw); a scene with load errors is kept unless
    // CheckLoaderErrors rejects it; the database validates the scene. Mac 0x17fe50, Win 0x601612
    virtual void Execute()
    {
        RefPointer<ModelLoaderAbstract> loader = new ModelLoaderAbstract(m_database);
        m_storage = new StorageScene2();
        if (!loader->Load(m_data, *m_storage, true) && !CheckLoaderErrors(*loader, *m_storage)) {
            m_ok = false;
            return;
        }
        m_database->ValidateScene(*m_storage);
        m_ok = true;
    }

    AString                   m_data;       // +0x30 the file content
    String                    m_path;       // +0x34 its full VFS path
    RefPointer<StorageScene2> m_storage;    // +0x44
    bool                      m_ok;         // +0x48
    BrickDatabase*            m_database;   // +0x4c
};

namespace {

// Every database brick above 999 as a palette brick in `bag` (count 1, unnamed, every part in
// material 21 with max(1, decoration faces) materials). The inlined body of Mac 0x17ef50 (whose
// last statement sets +0x187: done by the callers). (name inferred)
void FillExtendedBag(BrickDatabase* database, BrickPalette::Bag& bag)
{
    BasicArray<UInt32> ids;
    database->GetBrickIDs(ids);
    for (BasicArray<UInt32>::ConstIterator id = ids.Begin(); id != ids.End(); ++id) {
        if (*id <= 999)
            continue;
        RefPointer<BrickPalette::Brick> brick = new BrickPalette::Brick();
        brick->Initialize(*id, *database);
        brick->m_name = String();
        const BrickDatabase::Brick* entry = database->GetBrick(brick->m_designID);
        for (BasicArray<BrickDatabase::Part>::ConstIterator it = entry->m_parts.Begin();
             it != entry->m_parts.End(); ++it) {
            RefPointer<BrickPalette::Part> part = new BrickPalette::Part();
            part->m_designID = it->m_designID;
            part->m_boneTransforms = it->m_boneTransforms;
            UInt32 count = database->GetPrimitive(part->m_designID)->m_decorationFaceCount;
            if (count == 0)
                count = 1;
            while (part->m_materials.GetSize() > count)
                part->m_materials.PopBack();
            while (part->m_materials.GetSize() < count)
                part->m_materials.PushBack(0);
            part->m_materials[0] = 21;
            brick->m_parts.PushBack(part);
        }
        bag.AddBrick(brick, 1, *database);
    }
}

} // namespace

// Mac 0x17b230, Win 0x60309c
void LDDApplication::LoadPalettes()
{
    {
        RefPointer<Preferences> preferences = Preferences::GetInstance();
        String text;
        if (preferences->GetValue(String("SORTBYIDONLY"), text)) {
            bool off = false;
            if (FromString(off, text))
                BrickPalette::SetMainGroupsOff(off);
        }
    }
    String palettePath;
    String userPalettesPath;
    Preferences::GetInstance()->GetValue(String("PalettePath"), palettePath);
    Preferences::GetInstance()->GetValue(String("UserPalettesPath"), userPalettesPath);

    PaletteNames names;
    m_brickPalette->BeginUpdate();
    if (!palettePath.IsEmpty()) {
        String message("Loading palette from path: ");
        message << palettePath;                                             // NOTE: dropped
        RefPointer<Directory> root = Directory::GetRoot();
        RefPointer<Directory> directory =
            root->GetDirectory(Platform::GetInstance()->NativeToVFSPath(palettePath), false);
        if (directory)
            LoadPaletteDirectory(directory, &names);
    } else {
        String palettesName;
        Preferences::GetInstance()->GetValue(String("userPalettesDirName"), palettesName);
        RefPointer<Directory> directory = GetUserDataDirectory()->GetDirectory(palettesName, false);
        if (directory) {
            LoadPaletteDirectories(directory, &names);
        } else {
            String message("Unable to open palettes directory: ");
            TraceVerbose(message << GetUserDataDirectory()->GetPath(true) << "/" << palettesName, true);
        }
        AddExtendedPalette();
    }

    if (!userPalettesPath.IsEmpty()) {
        RefPointer<Directory> directory = GetUserDataDirectory()->GetDirectory(userPalettesPath, false);
        if (!directory) {
            RefPointer<Directory> root = Directory::GetRoot();
            directory = root->GetDirectory(Platform::GetInstance()->NativeToVFSPath(userPalettesPath), false);
        }
        if (!directory) {
            String message("Unable to open user palettes directory: ");
            TraceVerbose(message << GetUserDataDirectory()->GetPath(true) << "/" << userPalettesPath, true);
        } else {
            LoadPaletteDirectories(directory, &names);
        }
    }

    RefPointer<Directory> groups = GetUserDataDirectory()->GetDirectory(String("LibraryGroups"), false);
    if (groups) {
        DescriptorSet entries;
        groups->Enumerate(String("*"), entries);
        for (DescriptorSet::ConstIterator it = entries.Begin(); it != entries.End(); ++it) {
            Descriptor entry(*it);
            RefPointer<Directory> group = groups->GetDirectory(entry.m_name, false);
            if (group)
                LoadLibraryGroup(group);
        }
    }
    m_brickPalette->EndUpdate();

    {
        RefPointer<Preferences> preferences = Preferences::GetInstance();
        String value;
        ToString(names, value);
        preferences->SetValue(String("PaletteNames"), value);
    }
    Preferences::GetInstance()->Save();
}

// Mac 0x17bf10, Win 0x5ff03d
bool LDDApplication::LoadPaletteDirectory(Directory* directory, PaletteNames* names)
{
    RefPointer<OperationQueue> queue = new OperationQueue();
    RefPointer<Directory> modules = directory->GetDirectory(String("Modules"), false);
    RefPointer<LoadPaletteBricksOperation> bricks =
        new LoadPaletteBricksOperation(m_database, directory, names);
    queue->Add(bricks);

    BasicArray<RefPointer<LoadModuleOperation> > moduleOperations;
    if (modules) {
        modules->BeginEnumeration();
        for (;;) {
            if (modules->IsEnumerationDone()) {
                modules->EndEnumeration();
                break;
            }
            RefPointer<File> file = modules->OpenCurrent(OpenMode(true, false, true, false), 0);
            if (file && Directory::MatchWildcard(file->GetPath(false), String("*.lxfml"))) {
                RefPointer<LoadModuleOperation> module =
                    new LoadModuleOperation(m_database, file->GetPath(true));
                if (!ReadFile(file.Get(), module->m_data)) {
                    String message("Could not load module: ");
                    Trace(message << file->GetPath(true), true);
                    modules->EndEnumeration();
                    return false;   // NOTE: without waiting (the queue's destructor waits)
                }
                moduleOperations.PushBack(module);
                queue->Add(module);
            }
            modules->MoveNext();
        }
    }
    queue->Wait();

    if (!bricks->m_ok) {
        String message("Could not load palette: ");
        message << directory->GetPath(false);                              // NOTE: dropped
        return false;
    }
    BrickPalette::Bag& bag = *bricks->m_bag;
    for (UInt32 i = 0; i < moduleOperations.GetSize(); ++i) {
        LoadModuleOperation* module = moduleOperations[i];
        if (module->m_ok) {
            bag.m_models.PushBack(module->m_storage);
        } else {
            String message("Could not parse module: ");
            message << module->m_path;                                      // NOTE: dropped
        }
    }
    if (LEGO::GetBrandBrickSet((Brand)bag.m_brand) == (UInt32)kBrickSetLDDExtended && bag.IsEmpty()) {
        FillExtendedBag(m_database, bag);
        m_data187 = 1;
    }
    m_brickPalette->AddBag(bag);
    return true;
}

// Mac 0x17c650, Win 0x601857
void LDDApplication::LoadPaletteDirectories(Directory* directory, PaletteNames* names)
{
    DescriptorSet entries;
    directory->Enumerate(String("*"), entries);
    for (DescriptorSet::ConstIterator it = entries.Begin(); it != entries.End(); ++it) {
        Descriptor entry(*it);
        const String& name = entry.m_name;
        if (Directory::MatchWildcard(name, String("*.lxf")) ||
            Directory::MatchWildcard(name, String("*.lxfml")) ||
            Directory::MatchWildcard(name, String("*.ldr"))) {
            LoadModelPalette(Path(name, directory), names);
            continue;
        }
        RefPointer<Directory> palette = directory->GetDirectory(name, false);
        if (palette) {
            LoadPaletteDirectory(palette, names);
        } else {
            String message("Unable to open user palette directory: ");
            message << directory->GetPath(true) << "/" << name;            // NOTE: dropped
        }
    }
}

// Mac 0x17cac0, Win 0x5ff4eb
void LDDApplication::AddExtendedPalette()
{
    if (m_data187)
        return;
    RefPointer<BrickPalette::Bag> bag = new BrickPalette::Bag();
    bag->m_name = "LDDExtended";
    bag->m_brand = kBrandLDDExtended;
    bag->m_version = Version(1, 0, 0, 0);
    bag->m_countable = false;
    bag->m_brandFilter = true;
    FillExtendedBag(m_database, *bag);
    m_data187 = 1;
    m_brickPalette->AddBag(*bag);
}

// Mac 0x17cb90, Win 0x5fe883
void LDDApplication::LoadLibraryGroup(Directory* directory)
{
    BasicArray<RefPointer<LoadModuleOperation> > operations;
    RefPointer<OperationQueue> queue = new OperationQueue();
    directory->BeginEnumeration();
    while (!directory->IsEnumerationDone()) {
        RefPointer<File> file = directory->OpenCurrent(OpenMode(true, false, true, false), 0);
        if (file && Directory::MatchWildcard(file->GetPath(false), String("*.lxfml"))) {
            RefPointer<LoadModuleOperation> operation =
                new LoadModuleOperation(m_database, file->GetPath(true));
            if (ReadFile(file.Get(), operation->m_data)) {
                operations.PushBack(operation);
                queue->Add(operation);
            } else {
                String message("Could not load library group: ");
                message << file->GetPath(true);                             // NOTE: dropped
            }
        }
        directory->MoveNext();
    }
    directory->EndEnumeration();
    queue->Wait();

    for (UInt32 i = 0; i < operations.GetSize(); ++i) {
        LoadModuleOperation* operation = operations[i];
        if (operation->m_ok) {
            Path path(String(), 0);
            if (!path.Append(operation->m_path)) {
                String message("Could not find: ");
                message << operation->m_path;                               // NOTE: dropped
            }
            m_brickPalette->AddModel(operation->m_storage, path);
        } else {
            String message("Could not parse library group: ");
            message << operation->m_path;                                   // NOTE: dropped
        }
    }
}

// Mac 0x17d170, Win 0x5fcece
bool LDDApplication::LoadModelPalette(const Path& path, PaletteNames* names)
{
    RefPointer<BrickPalette::Bag> bag = new BrickPalette::Bag();
    BrickPaletteBagLoaderModel loader;
    if (loader.LoadBag(path, *bag, m_database, names)) {
        m_brickPalette->AddBag(*bag);
        return true;
    }
    String message("Could not load palette: ");
    message << path.GetPath(true);                                          // NOTE: dropped
    return false;
}

// Mac 0x17d2a0, Win 0x5fbd43
bool LDDApplication::UseShadersInScene() const
{
    return m_useShadersInScene && m_shadersSupported;
}

// Mac 0x17d2c0, Win 0x5fbd57
bool LDDApplication::UseShadersInPalette() const
{
    return m_useShadersInPalette && m_shadersSupported;
}

// Mac 0x17d2e0, Win 0x5fbd72
bool LDDApplication::UseRoundEdges() const
{
    return m_useRoundEdges;
}

// Mac 0x17d300, Win 0x5fccb9
const BasicArray<RefPointer<PostProcessingEffect> >& LDDApplication::GetPostProcessingExtraEffects() const
{
    return m_postProcessor->GetExtraEffects();
}

// Mac 0x17d320, Win 0x5fbd80
void LDDApplication::SetUnsavedChanges(bool unsaved)
{
    m_flag185 = unsaved;
}

// Mac 0x17d340, Win 0x5fbd8d
void LDDApplication::SetCheckBuildingGuide(bool check)
{
    m_checkBuildingGuide = check;
}

// Mac 0x17dd60 (D1; base-object body 0x17d360, thunks 0x17ddc0-0x17df80) / 0x17dff0 (D0,
// thunks 0x17e060-0x17e260), Win sdd 0x60110f: the members and bases only.
LDDApplication::~LDDApplication()
{
}

// Mac 0x17e2e0 (thunks 0x17e470, 0x17e490, 0x17e4b0, 0x17e4d0, 0x17e4f0), Win 0x600f8d: the
// secondary bases leave their subjects (preferences, observers of this notifier, the updater,
// the scene control). NOTE: Application has no OnFinalRelease of its own.
void LDDApplication::OnFinalRelease()
{
    PreferencesObserver::OnFinalRelease();
    Notifier<LDDApplicationObserver>::OnFinalRelease();
    UpdateObserver::OnFinalRelease();
    BrickConnectSceneControlObserver::OnFinalRelease();
}

// Mac 0x17e670, Win 0x5fccc4
// NOTE: "ApplicationVersionRevision" goes to the third component (Version::m_build) and
// "ApplicationVersionBuild" to the fourth (both builds).
bool LDDApplication::IsNewerVersion()
{
    Version stored;
    bool major = false, minor = false, revision = false, build = false;
    {
        String text;
        if (Preferences::GetInstance()->GetValue(String("ApplicationVersionMajor"), text))
            major = FromString(stored.m_major, text);
    }
    {
        String text;
        if (Preferences::GetInstance()->GetValue(String("ApplicationVersionMinor"), text))
            minor = FromString(stored.m_minor, text);
    }
    {
        String text;
        if (Preferences::GetInstance()->GetValue(String("ApplicationVersionRevision"), text))
            revision = FromString(stored.m_build, text);
    }
    {
        String text;
        if (Preferences::GetInstance()->GetValue(String("ApplicationVersionBuild"), text))
            build = FromString(stored.m_revision, text);
    }
    bool version = false;
    {
        String text;
        if (Preferences::GetInstance()->GetValue(String("ApplicationVersion"), text)) {
            UInt32 position = 0;
            version = FromString(stored, text, position);
        }
    }
    if (!(major && minor && revision && build) && !version)
        return true;
    Version current;
    if (!GetVersion(current))
        return false;
    return current > stored;
}

} // namespace LEGO
