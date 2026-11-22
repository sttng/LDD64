// LEGO Digital Designer — restored source
// LEGO::LDDApplication — the LDD application object (Application subclass): assets (fonts, the
//   picking mask, sounds, render switches, post-processor, dialogs), the document (open / save /
//   import / export / close through the brick connect scene control and the model loaders,
//   recent models, "Untitled n" names), the brand, alerts through LDD's alert box, about box /
//   preferences; it observes the scene control, the main updater (model loading progress) and
//   the preferences, and tells its LDDApplicationObservers.
// LEGO::LDDApplicationObserver — Observer<LDDApplication, LDDApplicationObserver>.
//
// TU 082 "LDDApplication.cpp" (Mac 0x16ee30-0x18126b; Win ≈ 0x5fbcaa-0x6044ff). Sub-TUs:
//   082a (Mac 0x16ee30-0x173ff0) constructor, LoadAssets, fonts, round-edge shaders, start-up
//        checks; 082b (Mac 0x173ff0-0x17b230) documents, alerts, recent models, brand, update;
//   082c (Mac 0x17b230-0x18126b) palettes / modules / library groups, render switches, version
//        check, destructor, OnFinalRelease.
//
// Mac layout (constructor 0x16f0a0, operator new 0x190 in AppMain 0x13ade6): Application
//   +0x00..+0x58, BrickConnectSceneControlObserver +0x58 (scene control +0x5c), UpdateObserver
//   +0x60 (+0x64), Notifier<LDDApplicationObserver> +0x68 (observers +0x6c, iterations +0x7c),
//   PreferencesObserver +0x8c (+0x90), members +0x94..+0x188, Object +0x188 (Object-in vtable
//   0x71455c). Win (constructor 0x5ffb95): members from +0x98, Object +0x16c (vtable 0x9cdde4);
//   flag offsets +0x153..+0x157 (= Mac - 0x20).
// Virtual slots: Mac 0x7143ec (36 entries), Win 0x9cde74 (30 entries; Win = Mac - 1 for 2..30):
//   Application overrides 5, 7-17, 24-26; new 27-30; Mac-only primary entries 31-35 for the
//   overriders of secondary-base virtuals (OnFinalRelease, BrickConnectSceneControlObserver
//   slots 3 and 13, UpdateObserver::Update, PreferencesObserver::OnPreferenceChanged).
// NOTE: Application's inferred slot names do not all fit LDD's overrides: slot 11 ("NewDocument")
// asks for a file to open (open panel), slot 12 ("OpenDocumentFile") the same after taking a
// reference to the edit command; the names are kept (Application.h).
// LDDApplicationObserver names OnRecentModelsChanged, OnBrandChanged, OnExtendedTooltipChanged,
// OnOnlineStatusChanged are the Lua callbacks of LuaLDDApplicationObserver (TU 398); the Lua
// binding (TU 397) names SetBrand, GetBrand, SetEditCommand, Import. Every other name is
// inferred.
#pragma once
#include "LEGO/Types.h"
#include "Object.h"
#include "ObjectRefPointer.h"
#include "LEGO/String.h"
#include "BasicArray.h"
#include "BasicMap.h"
#include "Notifier.h"
#include "Updater.h"
#include "Preferences.h"
#include "Path.h"
#include "StorageScene2.h"
#include "Application.h"
#include "BrickConnectSceneControlObserver.h"
#include "Brand.h"

namespace LEGO {

class LDDApplication;
class Font;            // (GetFont)
class BrickPalette;
class Window;
class RenderControlManager;
class ObjectFactory;
class Render;
class GraphicsCardInfo;
class PreferencesControl;
class LDDAlertBox;
class ProgressBarDialog;
class ModelLoaderHandler;
class BrickDatabase;
class BitmapMask;
class SimpleSound;
class Command;
class GenerateBuildingGuideCommand;
class PostProcessor;
class PostProcessingEffect;
class Control;
class ModalControl;
class BrickConnectSceneControl;
class Directory;
class PaletteNames;

// Mac vtable 0x71624c {D1 0x14f730, D0 0x14f760, OnFinalRelease, OnAttached, OnDetached,
// handlers 0x14f820-0x14f870}; Win 0x9d0514 (8 entries: OnAttached, OnDetached, 5 × 0x65a12a
// (ret 4), 0x5fbd39 (returns true)).
class LDDApplicationObserver : public Observer<LDDApplication, LDDApplicationObserver> {
public:
    LDDApplicationObserver() {}
    virtual ~LDDApplicationObserver() {}                                     // Mac 0x14f730/0x14f760

    // (Mac slot / Win slot)
    // A model was opened / saved under `path` (AddRecentModel, Mac 0x178690). (Lua name; the Lua
    // observer passes path.GetPath(true))
    virtual void OnRecentModelsChanged(const Path& /*path*/) {}             // 5/2
    virtual void OnBrandChanged(Brand /*brand*/) {}                          // 6/3  (Lua name; SetBrand)
    virtual void OnEvent7(UInt32 /*arg*/) {}                                 // 7/4  (name unknown; no caller in TU 082)
    virtual void OnExtendedTooltipChanged(bool /*enabled*/) {}               // 8/5  (Lua name)
    virtual void OnOnlineStatusChanged(bool /*online*/) {}                   // 9/6  (Lua name)
    // Every observer must agree before the document is replaced (CanChangeDocument).
    virtual bool OnQuery10() { return true; }                                // 10/7 (name unknown)
};

class LDDApplication : public Application,
                       public BrickConnectSceneControlObserver,
                       public UpdateObserver,
                       public Notifier<LDDApplicationObserver>,
                       public PreferencesObserver {
public:
    // One file type of the open / save / import / export panels (Platform::FileType).
    typedef Pair<String, String>  FileType;
    typedef BasicArray<FileType>  FileTypeArray;

    // Save formats: the index of the chosen type in the export types (+0x120). (names inferred)
    enum SaveFormat {
        kSaveFormatLXF    = 0,
        kSaveFormatLXFML  = 1,
        kSaveFormatLDraw  = 2,
        kSaveFormatLXFML4 = 3,
        kSaveFormatAny    = 4     // SaveDocumentTo / 0x176770: by extension, LXFML 5
    };

    // Mac 0x16f0a0 (C1), Win 0x5ffb95: the arguments of Application's constructor; the file
    // types, the brick palette, Win: Preferences "Open" → the document to open.
    LDDApplication(const String& name, BasicArray<String>& arguments,
                   const Preferences::ValueMap& defaults);
    virtual ~LDDApplication();                                              // Mac 0x17dd60/0x17dff0, Win sdd 0x60110f (082c)

    // Application::GetInstance (Mac 0x26f0) cast to LDDApplication.
    static LDDApplication* GetInstance();                                   // Mac 0x1701c0, Win 0x4226b0 (Application's)

    // ---- Application overrides (Mac slot / Win slot) ----
    virtual bool IsIdle();                                                  // 5/4   Mac 0x173ff0, Win 0x5fc2a3
    virtual void OnEvent7();                                                // 7/6   Mac 0x174030 (empty)
    virtual void OnEvent8();                                                // 8/7   Mac 0x174040 (empty)
    virtual bool CanQuit();                                                 // 9/8   Mac 0x174050, Win 0x5fc2cb
    virtual void OnQuit();                                                  // 10/9  Mac 0x174090, Win 0x5fc2fe
    virtual bool NewDocument();                                             // 11/10 Mac 0x1742e0, Win 0x6036d6
    virtual bool OpenDocumentFile(const Path& path);                        // 12/11 Mac 0x174a80, Win 0x6036f8
    virtual bool SaveDocument();                                            // 13/12 Mac 0x174ac0, Win 0x5fc47f
    virtual bool SaveDocumentAs();                                          // 14/13 Mac 0x174b60, Win 0x60435e
    virtual bool SaveDocumentTo(const Path& path);                          // 15/14 Mac 0x1753d0, Win 0x6044ff
    // "SAVECHANGESTO" / "CHANGESWILLBELOST" / SAVE / DONTSAVE / CANCEL.
    virtual bool QuerySaveChanges();                                        // 16/15 Mac 0x1753f0, Win 0x5fc506
    virtual bool CloseDocument();                                           // 17/16 Mac 0x1757c0, Win 0x600fb9
    virtual UInt32 Alert(UInt32 style, const String& message, const String& title,
                         const String& button1, const String& button2, const String& button3); // 24/23 Mac 0x177c80, Win 0x5fc802
    // "ldd.commands.about"
    virtual void ShowAboutBox();                                            // 25/24 Mac 0x177ed0, Win 0x5fc938
    virtual void ShowPreferences();                                         // 26/25 Mac 0x178050, Win 0x5fc9de

    // ---- new ----
    // Keeps `path` as the document to be (+0xf0) and LoadDocument(storage) when the document
    // may be replaced (true without a scene control).
    virtual bool OpenDocument(const Path& path, StorageScene2* storage);    // 27/26 Mac 0x176560, Win 0x5fdfab
    // Clears the scene and hands `storage` to the loader handler (the scene control) with the
    // "LOADINGMODEL" progress dialog; Update finishes it.
    virtual void LoadDocument(StorageScene2* storage);                      // 28/27 Mac 0x1765c0, Win 0x60029d
    // The document file name without extension ("" without a document directory).
    virtual String GetDocumentName() const;                                 // 29/28 Mac 0x177ba0, Win 0x5fc763
    virtual const Path& GetDocumentPath() const;                            // 30/29 Mac 0x177c70, Win 0x5fbcde

protected:
    // ---- overriders of secondary-base virtuals (Mac-only primary entries 31-35) ----
    virtual void OnFinalRelease();                                          // 31 Mac 0x17e2e0 (thunks 0x17e470 …), Win 0x600f8d (082c)
    virtual void OnAttached(BrickConnectSceneControl* control);             // 32 Mac 0x178670 (empty; thunk 0x178680)
    using UpdateObserver::OnAttached;          // (the other bases' overloads stay visible)
    using PreferencesObserver::OnAttached;
    // Scene control slot 13: forwarded to the window (Window slot 29). (name unknown)
    virtual void OnEvent13(bool flag);                                      // 33 Mac 0x178c20 (thunk 0x178c50), Win 0x5fca19
public:
    // Model loading progress; when the load finished: document path, recent models, the
    // "MISSINGBRICKONLOAD" … alerts.
    virtual void Update();                                                  // 34 Mac 0x178c80 (thunk 0x179710), Win 0x600461
    // "PLAYSOUND": reloads the pause sound.
    virtual void OnPreferenceChanged(const String& key);                    // 35 Mac 0x179730 (thunk 0x179860), Win 0x5fca28

    // ---- start-up / shut-down (082a) ----
    // The assets of the main window: fonts, render switches (UseShadersInScene /
    // UseShadersInPalette / UseRoundEdges), round-edge shaders, post-processor, the preference /
    // alert / progress dialogs, recent models, the picking mask, sounds, "LEGO Creations"
    // folders, start-up checks, "ApplicationVersion". False when anything essential fails
    // (AppMain then reports "Cannot load LDD assets"). Mac 0x1701d0, Win 0x602179
    bool LoadAssets(Window* window, RenderControlManager* manager, GraphicsCardInfo* cardInfo);
    // The 20 system-type fonts of GetFont. Mac 0x171bb0, Win 0x5fe125
    bool LoadFonts(ObjectFactory* factory);
    // Loads every "Shaders/RoundEdge/*.sh" of the assets with `render`; sets +0x175. Mac 0x172d20,
    // Win 0x601fc8
    void LoadRoundEdgeShaders(Render* render);
    bool AreRoundEdgeShadersLoaded() const;                                 // Mac 0x173150, Win 0x5fbd6b
    const BasicArray<RefPointer<PostProcessingEffect> >& GetPostProcessingEffects() const; // Mac 0x173160, Win 0x5fccae
    // m_postProcessing (+0x17c): post-processing is available (BrickConnectSceneControl::
    // SetPostProcessor Mac 0x4f2b0). Mac 0x17d2f0 (name inferred)
    bool IsPostProcessingEnabled() const { return m_postProcessing; }
    // "DOSTARTUPCHECKS": an interrupted start or a broken database ("Cleanup"), or a database that
    // cannot be opened, deletes the user brick database / palettes. False without a user data
    // directory. Mac 0x173430, Win 0x601ad7
    bool DoStartupChecks();
    void SetBrickDatabase(BrickDatabase* database);                         // Mac 0x173db0
    Window* GetWindow() const;                                              // Mac 0x173e00, Win 0x5fc29c
    // Drops the window, dialogs, palette, loader, scene, mask, sounds, edit command and leaves the
    // scene control (end of AppMain). Mac 0x173e10
    void ReleaseResources();

    // One of the application's fonts (entry `index` of +0x134).
    // DropDownControl uses font 3. Mac 0x17a2d0, Win 0x5fcc7a
    Font* GetFont(UInt32 index) const;
    BitmapMask* GetMask() const;                                            // Mac 0x17a2f0, Win 0x5fcc8a
    SimpleSound* GetClickSound() const;                                     // Mac 0x17a6e0
    // A ButtonSoundAugmentation "bspa" with the click sound on `control`. Mac 0x17a300, Win 0x600a54
    void AddButtonSound(Control* control);
    void SetReady(bool ready);                                              // Mac 0x17a6f0 (+0x160)
    bool ReturnTrue17a710() const;                                          // Mac 0x17a710 (returns true; name unknown)

    // ---- document (082b) ----
    // All observers agree (slot 10), the application is ready (+0x160), not modal and the window
    // has no modal control. Mac 0x174310
    bool CanChangeDocument();
    // `path`, or the open panel (user model directory, model file types) without one; asks
    // about old models ("LOADOFOLDMODELS"). Mac 0x1745d0, Win 0x602d21
    bool OpenModel(const Path* path);
    // Preferences "UserModelDirectory" below the root. Mac 0x174e90, Win 0x5fcb90
    static RefPointer<Directory> GetUserModelDirectory();
    // Fixes the extension of `path` for the chosen type of `types` (`typeIndex`) and returns the
    // save format; false (alert) for an empty name. Mac 0x174fd0, Win 0x5fcfc9
    bool PrepareSavePath(Path& path, UInt32 typeIndex, const FileTypeArray& types, UInt32& format);
    // Saves the scene to `path` ("UNABLETOSAVEMODELTO"); on success the document path, the recent
    // models and the user model directory follow. Mac 0x175260, Win 0x60426c
    bool SaveModelAs(const Path& path, UInt32 format);
    // Sets the document path ("Untitled n" without a name) and the window's file. Mac 0x1758d0, Win 0x600368
    void SetDocumentPath(const Path& path);
    // (Lua "Import") Mac 0x175aa0, Win 0x602d00
    void Import(const Path& path, ModelLoaderHandler* handler);
    bool ImportFile(const Path& path);                                      // Mac 0x175af0, Win 0x601717
    bool ImportWithPanel(ModelLoaderHandler* handler);                      // Mac 0x175c20
    bool ImportWithPanel();                                                 // Mac 0x175c70, Win 0x602f99 ("IMPORT")
    bool ImportStorage(StorageScene2* storage, ModelLoaderHandler* handler); // Mac 0x175e80, Win 0x601025
    void StartImport(StorageScene2* storage);                               // Mac 0x175fd0, Win 0x6009b5 ("IMPORTINGMODEL")
    bool Export();                                                          // Mac 0x176100, Win 0x6044ff ("EXPORT")
    bool ExportTo(const Path& path, UInt32 format);                         // Mac 0x176430, Win 0x604153
    // SaveModel with format kSaveFormatAny. Mac 0x176770
    bool SaveModel(BrickConnectSceneControl* control, const Path& path, const String& errorMessage,
                   const String& errorTitle, bool thumbnail, ModalControl* progress);
    // Writes the scene of `control` to `path`: .lxf (zip with IMAGE100.LXFML and, with
    // `thumbnail`, IMAGE100.PNG), .lxfml (LXFML 5, format 3: LXFML 4) or .ldr; errorMessage's
    // "%s" becomes the native path. Mac 0x1767a0, Win 0x603830
    bool SaveModel(BrickConnectSceneControl* control, const Path& path, const String& errorMessage,
                   const String& errorTitle, UInt32 format, bool thumbnail, ModalControl* progress);
    // Alert with a "do not show again" check box stored in the bool preference `key` (only
    // shown while the preference is true). Mac 0x177d50
    UInt32 AlertWithCheckBox(UInt32 style, const String& key, const String& checkBoxText,
                             const String& message, const String& title, const String& button1,
                             const String& button2, const String& button3);
    // Moves `path` to the front of the recent models (at most 10, preference "RecentModels",
    // OnRecentModelsChanged). Mac 0x178690, Win 0x5feeaa
    void AddRecentModel(const Path& path);
    void RemoveRecentModels(const BasicArray<String>& models);              // Mac 0x17a720, Win 0x5fdfdd
    const BasicArray<String>& GetRecentModels() const;                      // Mac 0x178660, Win 0x5fbd25
    // The "RecentModels" preference value of `models` ('#' before every entry, '%' and '#' in
    // an entry as "%25" / "%23") and back; a value without the leading '#' is the original's
    // "a#b#c". Fix, not in the original (see JoinRecentModels in LDDApplication.cpp).
    static String EncodeRecentModels(const BasicArray<String>& models);
    static void   DecodeRecentModels(const String& text, BasicArray<String>& models);
    // Preferences "UserModelDirectory" = the directory of `path`. Mac 0x179880, Win 0x5fcabe
    void StoreUserModelDirectory(const Path& path);
    // Loads `path` without the loader handler (clears the scene, LoadDocument). Mac 0x1799c0, Win 0x601796
    bool OpenModelDirectly(const Path& path);
    // Reads `path` (an .lxf: its IMAGE100.LXFML) into a new StorageScene2 (+0xc4) with the LXFML 5
    // loader; `oldVersion` = version error. Mac 0x179bb0, Win 0x601217
    bool LoadModelFile(const Path& path, bool& oldVersion);
    bool LoadModelStorage(const Path& path);                                // Mac 0x17a0c0
    // The application / brand / brick set meta data of a saved model. Mac 0x17ad00, Win 0x600aeb
    void FillMeta(StorageScene2& storage);

    // ---- state (082b) ----
    void SetExtendedTooltip(bool enabled);                                  // Mac 0x1780a0
    bool IsExtendedTooltip() const;                                         // Mac 0x178340
    bool IsOnline() const;                                                  // Mac 0x178360, Win 0x5fbcec
    void SetOnline(bool online);                                            // Mac 0x178370, Win 0x5fd9f7
    BrickPalette* GetBrickPalette() const;                                  // Mac 0x178350, Win 0x5fca12
    // (Lua "SetBrand") ignores unknown brands; OnBrandChanged. Mac 0x17aa40, Win 0x5fdae6
    void SetBrand(Brand brand);
    Brand GetBrand() const;                                                 // Mac 0x17acf0, Win 0x5fbd3c
    bool IsValidBrand(UInt32 brand) const;                                  // Mac 0x178610
    bool IsBrandLDDExtended() const;                                        // Mac 0x178620, Win 0x5fbd0b
    UInt32 GetBrandBrickSet() const;                                        // Mac 0x178640
    // (Lua "SetEditCommand") Mac 0x17a9a0, Win 0x5fcc98
    void SetEditCommand(Command* command);
    void SetBuildingGuideCommand(GenerateBuildingGuideCommand* command);    // Mac 0x17a9f0, Win 0x5fcca3

    // ---- 082c ----
    // Fills the brick palette at start-up (AppMain, Mac 0x13ad50): "SORTBYIDONLY" (main groups
    // off), the palettes of "PalettePath" (one palette folder, native path) or else of the user
    // data directory's "userPalettesDirName" folder, the LDD Extended palette, the user palettes
    // of "UserPalettesPath" (user data relative, else native), the "LibraryGroups" models, then
    // the preference "PaletteNames" (saved). Mac 0x17b230, Win 0x60309c (name inferred)
    void LoadPalettes();
    // The second effect list of the post-processor (PostProcessor +0x24, Mac 0x361580); used by
    // LDDPostProcessor's controls (Mac 0x361cc5). Mac 0x17d300, Win 0x5fccb9 (name inferred)
    const BasicArray<RefPointer<PostProcessingEffect> >& GetPostProcessingExtraEffects() const;
    // +0x185 / +0x186 (UploadModelCommand, Mac 0x278810). Mac 0x17d320, Win 0x5fbd80 /
    // Mac 0x17d340, Win 0x5fbd8d (names inferred)
    void SetUnsavedChanges(bool unsaved);
    void SetCheckBuildingGuide(bool check);

    // ---- render switches (082c) ----
    // The render switches (bytes the start-up reads from the preferences, Mac 0x17029b ff.)
    // AND +0x177 (shaders supported by the render, set at 0x17027b).
    bool UseShadersInScene() const;                                         // Mac 0x17d2a0, Win 0x5fbd43
    bool UseShadersInPalette() const;                                       // Mac 0x17d2c0, Win 0x5fbd57
    bool UseRoundEdges() const;                                             // Mac 0x17d2e0, Win 0x5fbd72

protected:
    // ---- palettes (082c; names inferred) ----
    // One palette folder: its "*.paxml" bag (LoadPaletteBricksOperation) and its "Modules/*.lxfml"
    // models (LoadModuleOperation), all on an OperationQueue; the parsed modules become the bag's
    // models, an empty LDD Extended bag gets every database brick; the bag goes to the brick
    // palette. False when the bag did not load or a module file could not be read.
    // Mac 0x17bf10, Win 0x5ff03d
    bool LoadPaletteDirectory(Directory* directory, PaletteNames* names);
    // Every entry of `directory`: model files (*.lxf, *.lxfml, *.ldr) as palettes, folders /
    // archives through LoadPaletteDirectory. Mac 0x17c650, Win 0x601857
    void LoadPaletteDirectories(Directory* directory, PaletteNames* names);
    // A model file as a palette bag (BrickPaletteBagLoaderModel). Mac 0x17d170, Win 0x5fcece
    bool LoadModelPalette(const Path& path, PaletteNames* names);
    // The "LDDExtended" bag (brand LDD Extended, version 1, brand filter) with every database
    // brick, once (+0x187). Mac 0x17cac0, Win 0x5ff4eb
    void AddExtendedPalette();
    // The "*.lxfml" models of a library group folder as palette models. Mac 0x17cb90, Win 0x5fe883
    void LoadLibraryGroup(Directory* directory);

protected:
    // ---- data (Mac offsets; constructor 0x16f0a0; Win from the constructor 0x5ffb95) ----
    RefPointer<Window>                  m_window;            // +0x094 / Win +0x98 (LoadAssets)
    RefPointer<PreferencesControl>      m_preferencesControl; // +0x098 (LoadAssets)
    RefPointer<LDDAlertBox>             m_alertBox;          // +0x09c (LoadAssets)
    ObjectRefPointer<ProgressBarDialog> m_progressDialog;    // +0x0a0 (+ its Object +0x0a4)
    RefPointer<BrickPalette>            m_brickPalette;      // +0x0a8 (constructor)
    RefPointer<ModelLoaderHandler>      m_loaderHandler;     // +0x0ac
    BasicArray<String>                  m_recentModels;      // +0x0b0 (at most 10)
    RefPointer<BrickDatabase>           m_database;          // +0x0c0 (SetBrickDatabase)
    RefPointer<StorageScene2>           m_storage;           // +0x0c4 the model being loaded
    BasicArray<StorageScene2::Meta>     m_metas;             // +0x0c8 of the loaded model
    Path                                m_documentPath;      // +0x0d8
    Path                                m_pendingPath;       // +0x0f0 (OpenDocument)
    UInt32                              m_untitledCount;     // +0x108 "Untitled n"
    bool                                m_online;            // +0x10c
    bool                                m_sortByIdOnly;      // +0x10d "SortByIdOnly" (not initialised)
    bool                                m_extendedTooltip;   // +0x10e
    FileTypeArray                       m_modelFileTypes;    // +0x110 open / save: LXF (+ LXFML, LDraw in DeveloperMode)
    FileTypeArray                       m_exchangeFileTypes; // +0x120 import / export: LXF, LXFML, LDraw, LXFML4
    bool                                m_importing;         // +0x130
    BasicArray<ObjectRefPointer<Font> > m_fonts;             // +0x134
    RefPointer<BitmapMask>              m_mask;              // +0x144 Graphics/General/masker.bmp
    RefPointer<SimpleSound>             m_pauseSound;        // +0x148 IA_pause.mp3
    String                              m_value14c;          // +0x14c (not used in 082a/b)
    RefPointer<SimpleSound>             m_clickSound;        // +0x15c IA_mouseclick.mp3
    bool                                m_ready;             // +0x160
    RefPointer<Command>                 m_editCommand;       // +0x164
    RefPointer<GenerateBuildingGuideCommand> m_buildingGuideCommand; // +0x168
    Brand                               m_brand;             // +0x16c
    UInt8                               m_data170[3];        // +0x170 (zero; not used in 082a/b)
    bool                                m_useShadersInScene;   // +0x173 "UseShadersInScene"
    bool                                m_useShadersInPalette; // +0x174 "UseShadersInPalette"
    bool                                m_roundEdgeShadersLoaded; // +0x175
    bool                                m_useRoundEdges;       // +0x176 "UseRoundEdges"
    bool                                m_shadersSupported;    // +0x177 (render capability)
    RefPointer<PostProcessor>           m_postProcessor;     // +0x178
    bool                                m_postProcessing;    // +0x17c
    RefPointer<GraphicsCardInfo>        m_graphicsCardInfo;  // +0x180
    bool                                m_flag184;           // +0x184 (not used in 082a/b)
    bool                                m_flag185;           // +0x185 SaveDocument / QuerySaveChanges: unsaved even when unmodified
    bool                                m_checkBuildingGuide; // +0x186 (SaveModel)
    UInt8                               m_data187;           // +0x187 the LDD Extended palette exists (AddExtendedPalette)
                                                             // Object +0x188
};

} // namespace LEGO
