// LEGO Digital Designer — restored source
// TU 060 "LDDMain.cpp" (file name inferred): sub-TU 060a Mac 0x138d40-0x13fad0 (preference
// defaults, main window, AppMain, the Lua functions GetDeveloperMode / UserHasAccessToBrand and
// the inline constructors AppMain uses), sub-TU 060b Mac 0x13fad0-0x1423d0 (LDDErrorHandler, the
// graphics-failure handler, ExitCommand, BackgroundControlObserver; BitmapMaskFactory is in
// Base/BitmapMask.cpp). Win: AppMain 0x5cbc41 and neighbours 0x5c7b1e-0x5cd662; the Win-only
// single instance 0x5c7ba0 / 0x5c85bb.
// Not code here: template / inline instances emitted into the object (Notifier/Observer copies
// of UndoManager and Command, BasicArray destructors, LuaObject::SetObject 0x142210 / 0x13f8b0,
// the StringStream.h helpers 0x1423d0-0x1427f0, CreateLuaControlTable<LuaControl> 0x13f790).
#include "LDDMain.h"
#include "Main.h"

#include "LDDApplication.h"
#include "LDDWindow.h"
#include "LDDWindowControl.h"
#include "LDDCursor.h"
#include "LDDUtilities.h"
#include "LDDWebCommunicator.h"
#include "Brand.h"
#include "BrickConnectSceneManager.h"
#include "BrickConnectSceneControl.h"
#include "BrickConnectSceneControlObserver.h"
#include "BrickReorientationControl.h"
#include "BrickModeGlobalObject.h"
#include "BrickPalette.h"
#include "StandardTextTooltipAugmentation.h"
#include "ScrollContainerControl.h"
#include "Mode.h"
#include "StartMode.h"
#include "EditMode.h"
#include "ViewMode.h"
#include "BGAutomaticMode.h"
#include "ModeNavigationCommand.h"
#include "BGModeCommand.h"
#include "UploadModelCommand.h"
#include "ScreenShotCommand.h"
#include "PrintCommand.h"
#include "StartupDelayedCommand.h"
#include "SaveTraceStringCommand.h"

#include "LuaBindings.h"
#include "LuaScriptManager.h"
#include "LuaPreferences.h"
#include "LuaPreferencesObserver.h"
#include "LuaCommand.h"
#include "LuaCommandAdapter.h"
#include "LuaControlManager.h"
#include "LuaControlManagerObserver.h"
#include "LuaLayout.h"
#include "LuaWindow.h"
#include "LuaControl.h"
#include "LuaButtonControl.h"
#include "LuaToggleControl.h"
#include "LuaMatrixLayoutControl.h"
#include "LuaArrayLayoutControl.h"
#include "LuaWindowControl.h"
#include "LuaContainerControl.h"
#include "LuaSliderControl.h"
#include "LuaTextInputControl.h"
#include "LuaControlAugmentation.h"
#include "LuaDragAugmentation.h"
#include "LuaSaveRectAugmentation.h"
#include "LuaButtonSoundAugmentation.h"
#include "LuaFrameAugmentation.h"
#include "LuaPopupAugmentation.h"
#include "LuaTooltipAugmentation.h"
#include "LuaStandardTextTooltipAugmentation.h"
#include "LuaExtendedTextTooltipAugmentation.h"
#include "LuaSlidingHideAugmentation.h"
#include "LuaRadioGroup.h"
#include "LuaSceneControl.h"
#include "LuaBrickConnectSceneManagerObserver.h"
#include "LuaBrickDatabase.h"
#include "LuaTool.h"
#include "LuaHingeTool.h"
#include "LuaMode.h"
#include "LuaBrickMode.h"
#include "LuaBrickPaletteControl.h"
#include "LuaLDDApplication.h"
#include "LuaLDDApplicationObserver.h"
#include "LuaLDDWebCommunicator.h"
#include "LuaToolButtonControl.h"
#include "LuaConsole.h"

#include "Application.h"
#include "Platform.h"
#include "Preferences.h"
#include "StringStream.h"
#include "TextLocale.h"
#include "Trace.h"
#include "Thread.h"
#include "UUID.h"
#include "ProfileTimer.h"
#include "ObjectRegistry.h"
#include "ObjectFactory.h"
#include "Updater.h"
#include "SystemUpdater.h"
#include "UndoManager.h"
#include "Directory.h"
#include "LiffStorageFactory.h"
#include "BitmapCodecFactoryHelper.h"
#include "BitmapMask.h"
#include "ImageLoader.h"
#include "TooltipManager.h"
#include "RenderControlManager.h"
#include "OpenGLWindow.h"
#include "OpenGLRender.h"
#include "OpenGLGraphicsCardInfo.h"
#include "OpenGLCrashReport.h"
#include "Path.h"
#include "Texture.h"
#include "Scene.h"

#if defined(_WIN32)
#include <windows.h>
#include "Win32Unicode.h"
#endif

#include <stdlib.h>
#include <typeinfo>

namespace LEGO {

namespace {

// The ErrorCode layout of Error.h: severity 1 (error), the LDD facility, `value`.
ErrorCode MakeErrorCode(UInt8 severity, UInt64 facility, UInt16 value)   // Win 0x418350 (shared)
{
    return ((UInt64)severity << 56) | ((facility & 0xFFFFFFFFFFULL) << 16) | value;
}

// The start-up error of AppMain: `value` with the localized text of `key`.
void ReportSetupError(UInt16 value, const char* key)
{
    String text(key);
    LocalizedString message(text);
    ReportError(MakeLDDError(value), message);
}

// GetValue + FromString(bool) (the original's template GetValue<bool>; Mac inlined, helper
// 0x142710; Win 0x41c700 / 0x419110).
bool GetPreferenceBool(const char* key, bool& value)
{
    RefPointer<Preferences> preferences = Preferences::GetInstance();
    return preferences->GetValue(String(key), value);
}

// Written once by AppMain and never read in either build (Mac 0x77ae68, Win 0xaeec80).
// NOTE: kept as in the original.
BrickConnectSceneControl* s_sceneControl = 0;

// Lua: ldd.GetDeveloperMode() -> the "developerMode" preference. Mac 0x13efe0 (Win in 0x5d0fe0
// registration)
int LuaGetDeveloperMode(lua_State* L)
{
    LuaPlus::LuaState* state = LuaPlus::LuaState::CastState(L);
    RefPointer<Preferences> preferences = Preferences::GetInstance();
    state->PushBoolean(preferences->GetBool(String("developerMode"), false));
    return 1;
}

// Lua: ldd.UserHasAccessToBrand(name) -> the brand is known and valid for the application.
// Mac 0x13f0c0
int LuaUserHasAccessToBrand(lua_State* L)
{
    LuaPlus::LuaState* state = LuaPlus::LuaState::CastState(L);
    Brand brand = (Brand)3;
    String name = CheckLuaString(state, 1);
    UInt32 position = 0;
    if (FromString(brand, name, position))
        state->PushBoolean(LDDApplication::GetInstance()->IsValidBrand(brand));
    else
        state->PushBoolean(false);
    return 1;
}

#if defined(_WIN32)
// Window::GetVirtualDesktopRect is Mac-only (TU 020); the Win build computes the same rectangle
// inline in GetMainWindowStyle (0x5c872f): the virtual screen, else the primary full screen.
Rect GetVirtualDesktopRect()
{
    Rect rect(Vector2((Real32)GetSystemMetrics(SM_XVIRTUALSCREEN),
                      (Real32)GetSystemMetrics(SM_YVIRTUALSCREEN)), Vector2());
    Vector2 size((Real32)GetSystemMetrics(SM_CXVIRTUALSCREEN),
                 (Real32)GetSystemMetrics(SM_CYVIRTUALSCREEN));
    if (size.x == 0.0f) {
        size = Vector2((Real32)GetSystemMetrics(SM_CXFULLSCREEN),
                       (Real32)GetSystemMetrics(SM_CYFULLSCREEN));
        if (size.x == 0.0f)
            return rect;
    }
    rect.SetSize(size);
    return rect;
}

// EnumWindows state of ActivateOtherInstance (Win 0x5c7c56 / 0x512c18).
struct FindWindowData {
    String name;
    HWND   window;
};

// The first "DCLWindowClass" window whose title contains the name. Win 0x5c7ba0
BOOL CALLBACK FindApplicationWindow(HWND window, LPARAM parameter)
{
    FindWindowData* data = (FindWindowData*)parameter;
    String className;
    PlatformGetClassName(window, className);
    if (className == String("DCLWindowClass")) {
        String title;
        PlatformGetWindowText(window, title);
        if (title.Find(data->name, 0) != String::npos) {
            data->window = window;
            return FALSE;
        }
    }
    return TRUE;
}
#endif

} // namespace

// Win 0x5c7b1e
ErrorCode MakeLDDError(UInt16 value)
{
    return MakeErrorCode(1, kLDDErrorFacility, value);
}

// =============================================================================================
// Preference defaults, main window
// =============================================================================================

// Mac 0x138d40, Win 0x5ca35e
void SetDefaultPreferences(Preferences::ValueMap& defaults)
{
    {
        String size;
        ToString(Vector2(1024.0f, 768.0f), size);
        defaults[String("mainWindowSize")] = size;
    }
    {
        String size;
        ToString(Vector2(1024.0f, 768.0f), size);
        defaults[String("mainWindowMinSize")] = size;
    }
    {
        String origin;
        ToString(Vector2(0.0f, 0.0f), origin);
        defaults[String("mainWindowOrigin")] = origin;
    }
    defaults[String("language")] = "en";
    defaults[String("initialiseDesignerURL")] =
        "http://ldd.lego.com/service/integration.asmx/initialiseDesigner";
    defaults[String("userPalettesDirName")] = "Palettes";
    defaults[String("userBrickDBDirName")] = "db";
    defaults[String("ShowStartHelp")] = "1";
    defaults[String("repeatPaste")] = "0";
    defaults[String("ShowBrickReorientation")] = "1";
    defaults[String("SortByIdOnly")] = "0";
    defaults[String("PlaySound")] = "1";
    defaults[String("SHOWBRICKCOUNTS")] = "1";
    defaults[String("UseShadersInScene")] = "1";
    defaults[String("UseShadersInPalette")] = "1";
    defaults[String("SHOWTOOLTIPS")] = "1";
    defaults[String("AllowMultipleLDDInstances")] = "0";
    defaults[String("ThemeAccess")] = "";
    defaults[String("SubstitutionTransparency")] = "0.5";
    defaults[String("ShowCompatibilityModeWarningAtStartup")] = "1";
    defaults[String("ShowHideSlideDuration")] = "500";
    defaults[String("UseRoundEdges")] = "0";
    defaults[String("SelectedPostProcessingEffect")] = "0";
    defaults[String("LDrawMapFile")] = "ldraw.xml";
    defaults[String("SHOWINFOFIELD")] = "1";
    defaults[String("INVERTEDCAMERAX")] = "0";
    defaults[String("INVERTEDCAMERAY")] = "0";
    defaults[String("LevelOfDetail")] = "0";
    defaults[String("LevelOfDetailDistance")] = "500";
    defaults[String("LevelOfDetailBoxes")] = "0";
    defaults[String("Cleanup")] = "0";
    RefPointer<Directory> documents = Platform::GetInstance()->GetSystemDirectory(3, true);
    if (documents) {
        RefPointer<Directory> models = documents->GetDirectory(String("LEGO Creations/Models"), true);
        if (models)
            defaults[String("UserModelDirectory")] = models->GetPath(true);
        RefPointer<Directory> instructions =
            documents->GetDirectory(String("LEGO Creations/Building Instructions"), true);
        if (instructions)
            defaults[String("UserBIDirectory")] = instructions->GetPath(true);
    }
    defaults[String("DoServerCall")] = "1";
    defaults[String("DoStartupChecks")] = "1";
    defaults[String("DeveloperMode")] = "0";
    defaults[String("ShowExtendedBrickToolTip")] = "0";
    defaults[String("IgnorePatching")] = "0";
    defaults[String("ForcePatching")] = "0";
    defaults[String("BRICKTOOLTIP")] = "0";
    defaults[String("ZoomToSelection")] = "0";
    defaults[String("LoadAssemblies")] = "1";
    defaults[String("LoadMostRecentModel")] = "0";
    defaults[String("QAMode")] = "0";
    defaults[String("PalettePath")] = "";
    defaults[String("UserPalettesPath")] = "UserPalettes";
    defaults[String("verbose")] = "0";
}

// Mac 0x139e20, Win 0x5c872f
void GetMainWindowStyle(WindowStyle& style)
{
    Vector2 size;
    Vector2 minimumSize;
    Vector2 origin;
    {
        RefPointer<Preferences> preferences = Preferences::GetInstance();
        String value;
        if (preferences->GetValue(String("mainWindowMinSize"), value)) {
            UInt32 position = 0;
            FromString(minimumSize, value, position);
        }
    }
    {
        RefPointer<Preferences> preferences = Preferences::GetInstance();
        String value;
        if (preferences->GetValue(String("mainWindowSize"), value)) {
            UInt32 position = 0;
            FromString(size, value, position);
        }
    }
    if (size.x <= minimumSize.x)
        size.x = minimumSize.x;
    if (size.y <= minimumSize.y)
        size.y = minimumSize.y;
    {
        RefPointer<Preferences> preferences = Preferences::GetInstance();
        String value;
        if (preferences->GetValue(String("mainWindowOrigin"), value)) {
            UInt32 position = 0;
            FromString(origin, value, position);
        }
    }
    Rect desktop(origin, Vector2());
#if defined(_WIN32)
    desktop = GetVirtualDesktopRect();
#else
    desktop = Window::GetVirtualDesktopRect();
#endif
    Real32 x = desktop.m_max.x - size.x;
    if (origin.x <= x)
        x = origin.x;
    if (x <= desktop.m_min.x)
        x = desktop.m_min.x;
    origin.x = x;
    Real32 y = desktop.m_max.y - size.y;
    if (origin.y <= y)
        y = origin.y;
    if (y <= desktop.m_min.y)
        y = desktop.m_min.y;
    origin.y = y;
    style.title = "LEGO Digital Designer";
    style.position = origin;
    style.size = size;
    style.minimumSize = minimumSize;
    style.hidden = false;
#if defined(_WIN32)
    style.icon = 101;
#endif
}

// Mac 0x13a1c0, Win 0x5cb675
bool CreateMainWindow(RefPointer<Window>& window, RefPointer<OpenGLRender>& render,
                      RefPointer<RenderControlManager>& controlManager)
{
    UInt32 level = 0;
    {
        RefPointer<Preferences> preferences = Preferences::GetInstance();
        String value;
        if (preferences->GetValue(String("CompatibilityModeLevel"), value))
            FromString(level, value);
    }
    window = new LDDWindow(true, level);
    WindowStyle style;
    GetMainWindowStyle(style);
    if (!window->Create(style)) {
        // the stored geometry did not work: the default one
        style.position = Vector2(0.0f, 0.0f);
        style.size = Vector2(1024.0f, 768.0f);
        style.minimumSize = style.size;
        if (!window->Create(style)) {
            ReportSetupError(0, "Could not create window");
            return false;
        }
    }
    Render::CreateArgs args;
    args.maxRenderEntries = 2000;
    args.flags = 0;
    OpenGLWindow* glWindow = static_cast<OpenGLWindow*>(window.Get());
    render = new OpenGLRender(glWindow->GetLevel(), window);
    if (!render->Initialize(args)) {
        ReportSetupError(1, "Requires OpenGL version 1.1 or above");
        return false;
    }
    ImageLoader::GetInstance()->SetRender(render);
    Path descriptions;
    RefPointer<Directory> assets = Platform::GetInstance()->GetSystemDirectory(9, true);
    if (!assets) {
        ReportSetupError(0x11, "Could not open Assets directory");
        return false;
    }
    descriptions.SetElement(assets);
    descriptions.Append(String("Graphics/imagedescriptions.xml"));
    if (!ImageLoader::GetInstance()->Load(descriptions)) {
        ReportSetupError(2, "Could not load image descriptions");
        return false;
    }
    controlManager = new RenderControlManager(window, render);
    if (!controlManager->Initialize()) {
        ReportSetupError(5, "Could not create window");
        return false;
    }
    Control* background = controlManager->GetLayer(1);
    background->SetInputFlags(background->GetInputFlags() | 3);
    background->SetHitTestable(true);
    RefPointer<TooltipManager> tooltips = new TooltipManager(controlManager);
    RegisterBitmapCodecFactories();
    ObjectFactory::GetInstance()->RegisterFactory(typeid(BitmapMask), new BitmapMaskFactory);
    return true;
}

// =============================================================================================
// AppMain
// =============================================================================================

} // namespace LEGO

using namespace LEGO;

#if !defined(LDD_STUB_APPMAIN_SMOKE)   // (not original: defined only by the restoration's
                                       // test builds)
// Mac 0x13ad50, Win 0x5cbc41
int AppMain(BasicArray<String>& arguments)
{
    String applicationName("LEGO Digital Designer");
    SetTraceFlags(GetTraceFlags() | 2);
    RegisterStorageFactories();
    Preferences::ValueMap defaults;
    SetDefaultPreferences(defaults);
    RefPointer<LDDApplication> application = new LDDApplication(applicationName, arguments, defaults);
    {
        RefPointer<Preferences> preferences = Preferences::GetInstance();
        preferences->RemoveValue(String("UserBrickVersion"));
    }
#if defined(_WIN32)
    {
        bool largeStackDump = false;
        GetPreferenceBool("CreateLargeStackDump", largeStackDump);
        SetCrashDumpOptions(0, &applicationName, &largeStackDump);
    }
#endif
    {
        RefPointer<Preferences> preferences = Preferences::GetInstance();
        ProfileTimer::SetReportEnabled(preferences->GetBool(String("verbose"), false));
    }
    {
        RefPointer<Preferences> preferences = Preferences::GetInstance();
        ProfileTimer::SetSamplingEnabled(preferences->GetBool(String("verbose"), false));
    }
    bool verbose = false;
    if (!GetPreferenceBool("verbose", verbose))
        return 3;
    if (verbose)
        SetTraceFlags(GetTraceFlags() | 4);
    {
        RefPointer<Directory> assets = Platform::GetInstance()->GetSystemDirectory(9, true);
        if (!assets) {
            String title("Cannot load LDD assets");
            String message("Try reinstall LDD.\nSee the read me file for help, or go to\n"
                           "http://ldd.lego.com/support/");
            Platform::ShowMessage(message, &title, 0);
            return 3;
        }
    }
#if defined(_WIN32)
    {
        RefPointer<Directory> program = Platform::GetInstance()->GetSystemDirectory(0, true);
        if (program)
            PlatformSetCurrentDirectory(Platform::GetInstance()->VFSToNativePath(program->GetPath(true)));
    }
#endif
    // NOTE: the handler is a local and stays registered after AppMain returns (both builds).
    LDDErrorHandler errorHandler;
    GetErrorRoot().AddHandler(&errorHandler);
    GetErrorRoot().EnableTrace();
    bool allowMultipleInstances = false;
    bool allowMultipleFound = GetPreferenceBool("AllowMultipleLDDInstances", allowMultipleInstances);
#if defined(_WIN32)
    {
        bool otherInstance = ActivateOtherInstance(applicationName);
        if (allowMultipleFound ? (otherInstance && !allowMultipleInstances) : otherInstance)
            return 0;
    }
#else
    (void)allowMultipleFound;
#endif
    {
        LEGO::UUID applicationID;
        bool valid = false;
        {
            RefPointer<Preferences> preferences = Preferences::GetInstance();
            String value;
            if (preferences->GetValue(String("ApplicationID"), value)) {
                UInt32 position = 0;
                valid = FromString(applicationID, value, position);
            }
        }
        if (!valid) {
            applicationID.Generate();
            {
                RefPointer<Preferences> preferences = Preferences::GetInstance();
                String value;
                ToString(applicationID, value);
                preferences->SetValue(String("ApplicationID"), value);
            }
            RefPointer<Preferences> preferences = Preferences::GetInstance();
            preferences->Save();
        }
    }

    RefPointer<LEGO::Window> window;
    RefPointer<OpenGLRender> render;
    RefPointer<RenderControlManager> controlManager;
    if (!CreateMainWindow(window, render, controlManager))
        return 3;
    RefPointer<GraphicsCardInfo> cardInfo = new OpenGLGraphicsCardInfo(render);
    if (!application->LoadAssets(window, controlManager, cardInfo)) {
        ReportSetupError(6, "SETUPERRORGENERIC");
        return 3;
    }

    // ---- Lua: the bindings (in the original's order) and the scripts ----
    RefPointer<LuaScriptManager> scriptManager = new LuaScriptManager(0, 0, true);
    LuaPlus::LuaState* state = scriptManager->GetLuaState();
    RegisterLuaBaseLibrary(state);
    RegisterLuaCommand(state);
    RegisterLuaCommandAdapter(state);
    RegisterLuaPreferencesObserver(state);
    RegisterLuaControlManager(state);
    RegisterLuaControlManagerObserver(state, controlManager);
    RegisterLuaLayout(state, scriptManager->GetLayouts());
    RegisterLuaWindow(state);
    RegisterLuaControl(state, controlManager);
    RegisterLuaButtonControl(state, controlManager);
    RegisterLuaToggleControl(state, controlManager);
    RegisterLuaMatrixLayoutControl(state, controlManager);
    RegisterLuaArrayLayoutControl(state, controlManager);
    RegisterLuaWindowControl(state, controlManager);
    RegisterLuaContainerControl(state, controlManager);
    RegisterLuaSliderControl(state, controlManager);
    RegisterLuaTextInputControl(state, controlManager, scriptManager->GetLayouts());
    RegisterLuaControlAugmentation(state);
    RegisterLuaDragAugmentation(state);
    RegisterLuaSaveRectAugmentation(state);
    RegisterLuaButtonSoundAugmentation(state, application->GetClickSound());
    RegisterLuaFrameAugmentation(state);
    RegisterLuaPopupAugmentation(state);
    RegisterLuaTooltipAugmentation(state);
    RegisterLuaStandardTextTooltipAugmentation(state);
    RegisterLuaExtendedTextTooltipAugmentation(state);
    RegisterLuaSlidingHideAugmentation(state);
    RegisterLuaRadioGroup(state);
    RegisterLuaSceneControl(state);
    RegisterLuaSceneManager(state);
    RegisterLuaBrickDatabase(state);
    RegisterLuaTool(state);
    RegisterLuaHingeTool(state);
    RegisterLuaMode(state);
    RegisterLuaBrickMode(state);
    RegisterLuaBrickPaletteControl(state);
    RegisterLuaLDDApplication(state);
    RegisterLuaLDDApplicationObserver(state);
    RegisterLuaLDDWebCommunicator(state);
    RefPointer<Object> loadRequest = scriptManager->Load(String("Scripts/LDD.xml"), 0, false, false);
    if (!loadRequest) {
        ReportSetupError(3, "SETUPLUAGENERIC");
        return 3;
    }

    // ---- cursor, brick reorientation ----
    LDDCursor::RegisterCursorNames();
    RefPointer<LDDCursor> cursor = new LDDCursor(controlManager);
    if (!cursor->Initialize()) {
        ReportSetupError(7, "SETUPERRORGENERIC");
        return 3;
    }
    if (!cursor->Attach()) {
        ReportSetupError(7, "SETUPERRORGENERIC");
        return 3;
    }
    controlManager->ChangeCursor(cursor);
    controlManager->SetCursorVisible(true);
    RefPointer<BrickReorientationControl> reorientation =
        new BrickReorientationControl(controlManager, cursor);
    if (!reorientation->Create(controlManager->GetLayer(2))) {
        ReportSetupError(7, "SETUPERRORGENERIC");
        return 3;
    }
    window->SetModalState(true);

    // ---- StartMode: splash, brick installation, start-up web calls ----
    bool doStartupChecks;
    {
        RefPointer<Preferences> preferences = Preferences::GetInstance();
        doStartupChecks = preferences->GetBool(String("DoStartupChecks"), true);
    }
    RefPointer<StartMode> startMode;
    if (doStartupChecks) {
        startMode = new StartMode(controlManager, window);
        if (!startMode->Create(controlManager->GetLayer(1), 0)) {
            ReportSetupError(9, "SETUPERRORGENERIC");
            return 3;
        }
        startMode->Enter();
        window->SetHidden(false);
        application->RunLoop();
        startMode->ReleaseResources();
        Thread::ReleaseObjects();
        if (startMode->ShouldQuit()) {
            startMode->Exit();
            return 0;
        }
    }

    // ---- the scene ----
    RefPointer<BrickConnectSceneManager> sceneManager = new BrickConnectSceneManager;
    if (!sceneManager->Initialize(controlManager)) {
        ReportSetupError(8, "SETUPERRORGENERIC");
        return 3;
    }
    application->SetBrickDatabase(sceneManager->GetDatabase());
    BrickPalette::LoadMainGroups(*sceneManager->GetDatabase());
    RefPointer<UndoManager> undoManager = new UndoManager;
    RefPointer<BrickConnectSceneControl> sceneControl =
        new BrickConnectSceneControl(controlManager, sceneManager, undoManager,
                                     application->GetBrickPalette());
    application->BrickConnectSceneControlObserver::Attach(sceneControl);
    sceneControl->Initialize(0);
    sceneControl->SetCursor(5, false);
    sceneControl->SetAxisMargins(1, 0.0f, 0.0f, 0);
    sceneManager->SetSceneControl(sceneControl);
    RefPointer<BackgroundControlObserver> backgroundObserver =
        new BackgroundControlObserver(sceneControl);
    backgroundObserver->Attach(controlManager->GetLayer(1));
    RefPointer<Texture> editModeTexture =
        LoadApplicationTexture(String("Graphics/EditScreen/EditMode_Final.png"), render);
    if (!editModeTexture) {
        ReportSetupError(0xd, "Could not create control, try restarting LDD");
        return 3;
    }
    {
        LEGO::Rect frame(374.0f, 27.0f, 381.0f, 34.0f);
        StandardTextTooltipAugmentation::InstallTooltipControl(controlManager, editModeTexture,
            &frame, Vector3(2.0f, 3.0f, 2.0f), Vector3(2.0f, 3.0f, 2.0f), true);
    }
    editModeTexture = 0;
    s_sceneControl = sceneControl;
    application->LoadPalettes();

    // ---- modes ----
    RefPointer<EditMode> editMode = new EditMode(controlManager, window, sceneControl);
    RefPointer<BGAutomaticMode> bgAutomaticMode = new BGAutomaticMode(controlManager, window, sceneControl);
    RefPointer<ViewMode> viewMode = new ViewMode(controlManager, window, sceneControl);
    LuaPlus::LuaObject globals = state->GetGlobals();
    RegisterLuaToolButtonControl(state, controlManager, sceneManager);
    RegisterLuaSceneManagerObserver(state, sceneManager);

    bool failed = false;
    RefPointer<LuaConsole> console;
    RefPointer<LDDWindowControl> consoleWindow;
    {
        // ---- the Lua namespace "LDD" (tables) and "ldd" (objects, commands) ----
        LuaPlus::LuaObject ldd = globals.CreateTable("LDD");
        LuaPlus::LuaObject tools = ldd.CreateTable("tools");
        LuaPlus::LuaObject commands = ldd.CreateTable("commands");
        LuaPlus::LuaObject controls = ldd.CreateTable("controls");
        LuaPlus::LuaObject commonCommands = commands.CreateTable("common");
        LuaPlus::LuaObject editModeCommands = commands.CreateTable("editMode");
        LuaPlus::LuaObject viewModeCommands = commands.CreateTable("viewMode");
        LuaPlus::LuaObject bgModeCommands = commands.CreateTable("bgAutomaticMode");
        LuaPlus::LuaObject commonControls = controls.CreateTable("common");
        LuaPlus::LuaObject editModeControls = controls.CreateTable("editMode");
        LuaPlus::LuaObject viewModeControls = controls.CreateTable("viewMode");
        LuaPlus::LuaObject bgModeControls = controls.CreateTable("bgAutomaticMode");
        ldd.Register("GetDeveloperMode", LuaGetDeveloperMode, 0);
        ldd.Register("UserHasAccessToBrand", LuaUserHasAccessToBrand, 0);
        {
            LuaPlus::LuaObject object = LuaBrickDatabase::CreateLuaObject(sceneManager->GetDatabase(), state);
            ldd.SetObject("brickDatabase", object);
        }
        {
            LuaPlus::LuaObject object = LuaSceneManager::CreateLuaObject(sceneManager, state);
            ldd.SetObject("sceneManager", object);
        }
        {
            LuaPlus::LuaObject object = LuaSceneControl::CreateLuaObject(sceneControl, state);
            ldd.SetObject("sceneControl", object);
        }
        {
            LuaPlus::LuaObject object = CreateLuaWindow(window, state);
            ldd.SetObject("window", object);
        }
        {
            LuaPlus::LuaObject object = CreateLuaControlManager(controlManager, state);
            ldd.SetObject("controlManager", object);
        }
        {
            LuaPlus::LuaObject object = LuaLDDApplication::CreateLuaObject(application, state);
            ldd.SetObject("application", object);
        }
        {
            LDDWebCommunicator* communicator = LDDWebCommunicator::GetInstance();
            LuaPlus::LuaObject object = LuaLDDWebCommunicator::CreateLuaObject(communicator, state);
            ldd.SetObject("webCommunicator", object);
        }

        // ---- the common commands ----
        RefPointer<ModeNavigationCommand> editModeCommand = new ModeNavigationCommand(editMode);
        RefPointer<ModeNavigationCommand> viewModeCommand = new ModeNavigationCommand(viewMode);
        RefPointer<BGModeCommand> bgModeCommand = new BGModeCommand(bgAutomaticMode, sceneControl);
        RefPointer<UploadModelCommand> uploadCommand = new UploadModelCommand(sceneControl, editModeCommand);
        {
            LuaPlus::LuaObject object = LuaCommandAdapter::CreateLuaObject(editModeCommand, state);
            commonCommands.SetObject("editMode", object);
        }
        {
            LuaPlus::LuaObject object = LuaCommandAdapter::CreateLuaObject(viewModeCommand, state);
            commonCommands.SetObject("viewMode", object);
        }
        {
            LuaPlus::LuaObject object = LuaCommandAdapter::CreateLuaObject(bgModeCommand, state);
            commonCommands.SetObject("bgAutomaticMode", object);
        }
        {
            LuaPlus::LuaObject object = LuaCommandAdapter::CreateLuaObject(new PrintCommand(window, sceneControl), state);
            commonCommands.SetObject("print", object);
        }
        {
            LuaPlus::LuaObject object = LuaCommandAdapter::CreateLuaObject(new ExitCommand, state);
            commonCommands.SetObject("quit", object);
        }
        {
            LuaPlus::LuaObject object = LuaCommandAdapter::CreateLuaObject(uploadCommand, state);
            commonCommands.SetObject("uploadToFactory", object);
        }
        {
            LuaPlus::LuaObject object =
                LuaCommandAdapter::CreateLuaObject(new ScreenShotCommand(sceneControl), state);
            commonCommands.SetObject("screenShot", object);
        }
        {
            LuaPlus::LuaObject object =
                LuaCommandAdapter::CreateLuaObject(new SaveTraceStringCommand(window), state);
            commonCommands.SetObject("saveTraceLog", object);
        }

        // ---- the Lua console (a hidden LDD window; toggled by the Lua command consoleToggle) ----
        RefPointer<ScrollContainerControl> consoleContainer = new ScrollContainerControl(controlManager);
        if (!consoleContainer->Create(ScrollContainerControl::kVertical, true, 0)) {
            ReportSetupError(4, "SETUPLUAGENERIC");
            failed = true;
        } else {
            console = new LuaConsole(controlManager, consoleContainer, state);
            if (!console->Initialize()) {
                ReportSetupError(4, "SETUPLUAGENERIC");
                failed = true;
            }
        }
        if (!failed) {
            consoleWindow = new LDDWindowControl(controlManager);
            consoleWindow->Create(true, false);
            consoleWindow->SetTitle(String("Lua console"));
            consoleWindow->SetContent(console);
            consoleWindow->SetContentSize(Vector2(console->GetMinimumSize()));
            consoleWindow->Setup(0, 0, 6, LEGO::Rect(40.0f, 10.0f, 400.0f, 200.0f), 0, false);
            RefPointer<LuaControl> consoleBinding;
            {
                LuaPlus::LuaObject object =
                    CreateLuaControlTable<LuaControl>(consoleWindow, state, "Control", consoleBinding);
                globals.SetObject("console", object);
            }
            state->DoString("consoleToggle = Command:New(true)\n"
                            "function consoleToggle:Do()\n"
                            "\tconsole:SetVisible( not console:GetVisible() )\n"
                            "end\n");

            // ---- wait for the scripts, then ldd:Create() ----
            while (scriptManager->IsLoading(loadRequest)) {
                MainUpdater::GetInstance()->Update();
                SystemUpdater::GetInstance()->Update();
            }
            if (scriptManager->HasFailed(loadRequest)) {
                ReportSetupError(3, "SETUPLUAGENERIC");
                failed = true;
            } else {
                loadRequest = 0;
                LuaPlus::LuaObject lddObject = globals.GetByName("ldd");
                if (!CallLuaMethod(lddObject, "Create", false, 1)) {
                    ReportSetupError(4, "SETUPLUAGENERIC");
                    failed = true;
                } else {
                    LuaPlus::LuaObject result(state, -1);
                    state->Pop(1);
                    if (!LuaToBoolean(result)) {
                        ReportSetupError(4, "SETUPLUAGENERIC");
                        failed = true;
                    } else if (!BrickModeGlobalObject::Create(controlManager, sceneControl)) {
                        failed = true;
                    } else {
                        BrickModeGlobalObject::GetSceneHolder()->SetMinimumSize(Vector2(350.0f, 0.0f));
                    }
                }
            }
        }
    }

    // ---- the modes' controls, the main loop ----
    RefPointer<StartupDelayedCommand> startupCommand;
    if (!failed) {
        if (!editMode->Initialize(bgAutomaticMode, viewMode, controlManager->GetLayer(1), 0)) {
            ReportSetupError(0xa, "Could not load assets, try restarting LDD");
            failed = true;
        } else if (!bgAutomaticMode->Initialize(editMode, viewMode, controlManager->GetLayer(1), 0)) {
            ReportSetupError(0xb, "Could not load assets, try restarting LDD");
            failed = true;
        } else if (!viewMode->Initialize(editMode, bgAutomaticMode, controlManager->GetLayer(1), 0)) {
            ReportSetupError(0xc, "Could not load assets, try restarting LDD");
            failed = true;
        } else {
            window->SetHidden(false);
            startupCommand = new StartupDelayedCommand(true, editMode, startMode);
            startMode = 0;
            consoleWindow->SetParent(controlManager->GetLayer(1), 0);
            application->RunLoop();
            sceneControl->GetScene()->SetListener(0);
            startupCommand = 0;
        }
    }
    globals = LuaPlus::LuaObject();
    consoleWindow = 0;
    console = 0;
    viewMode = 0;
    bgAutomaticMode = 0;
    editMode = 0;
    backgroundObserver = 0;
    sceneControl = 0;
    undoManager = 0;
    if (failed)
        return 3;

    // ---- teardown ----
    loadRequest = 0;
    if (Mode::GetCurrentMode())
        Mode::GetCurrentMode()->Exit();
    Application::GetInstance()->OnQuit();
    application->ReleaseResources();
    if (sceneManager) {
        sceneManager->SetDefaultTool(0);
        sceneManager->SetActiveTool(0);
    }
    scriptManager = 0;
    ObjectRegistry::GetInstance()->UnregisterAll();
    Thread::ReleaseObjects();
    sceneManager = 0;
    cursor = 0;
    reorientation = 0;
    controlManager = 0;
    render = 0;
    window = 0;
    application = 0;
    return 0;
}
#endif // !LDD_STUB_APPMAIN_SMOKE

namespace LEGO {

// =============================================================================================
// LDDErrorHandler, graphics failure
// =============================================================================================

// Mac 0x140700 (-> 0x140770), Win 0x5c847b
void LDDErrorHandler::HandleError(ReportedError& error)
{
    Show(error);
}

// Mac 0x140710 (-> 0x140770; thunk 0x140750)
void LDDErrorHandler::Visit(ReportedError& error)
{
    Show(error);
}

// Mac 0x140770, Win 0x5c847b
void LDDErrorHandler::Show(ReportedError& error)
{
    ErrorCode code = error.GetCode();
    if (ErrorGetFacility(code) == (kLDDErrorFacility & 0xFFFFFFFFFFULL)) {
        if (ErrorGetValue(error.GetCode()) == 8) {
            // the brick database: reinstall on the next start
            {
                RefPointer<Preferences> preferences = Preferences::GetInstance();
                bool cleanup = true;
                String value;
                ToString(cleanup, value);
                preferences->SetValue(String("Cleanup"), value);
            }
            RefPointer<Preferences> preferences = Preferences::GetInstance();
            preferences->Save();
        }
        LocalizedString title(String("SETUPERRORTITLE"));
        Platform::ShowMessage(error.GetDescription(), &title, 0);
    }
    error.VisitChildren(*this);
}

// Mac 0x13fae0, Win 0x5c8a3b (`file` / `line` are not used by either build)
void ShowGraphicFailure(const char* /*file*/, UInt32 /*line*/)
{
    Application* application = Application::GetInstance();
    LocalizedString title(String("GraphicFailureTitle"));
    LocalizedString text(String("GraphicFailureText"));
    UInt32 level = 0;
    {
        RefPointer<Preferences> preferences = Preferences::GetInstance();
        String value;
        if (preferences->GetValue(String("CompatibilityModeLevel"), value))
            FromString(level, value);
    }
    if (level < 100) {
        if (!Platform::AskOkCancel(text, &title, 0)) {
#if defined(_WIN32)
            ExitProcess(3);
#else
            abort();
#endif
        }
        {
            RefPointer<Preferences> preferences = Preferences::GetInstance();
            String value;
            ToString((UInt32)100, value);
            preferences->SetValue(String("CompatibilityModeLevel"), value);
        }
        {
            RefPointer<Preferences> preferences = Preferences::GetInstance();
            preferences->RemoveValue(String("ImproperShutDown"));
        }
        {
            RefPointer<Preferences> preferences = Preferences::GetInstance();
            bool show = true;
            String value;
            ToString(show, value);
            preferences->SetValue(String("ShowCompatibilityModeWarningAtStartup"), value);
        }
        application->StopRunLoop(false);
    } else {
        Platform::ShowMessage(text, &title, 0);
    }
}

// =============================================================================================
// ExitCommand, BackgroundControlObserver
// =============================================================================================

// Mac 0x141880, Win 0x5c9c8f
void ExitCommand::Do()
{
    Application::GetInstance()->Quit();
}

// Mac 0x1418a0
void ExitCommand::UpdateEnable()
{
    SetEnable(true);
}

// Mac AppMain 0x13bf7b (inline), Win 0x5c956b
BackgroundControlObserver::BackgroundControlObserver(BrickConnectSceneControl* sceneControl)
    : m_sceneControl(sceneControl)
{
}

// Mac 0x141280/0x141350, Win sdd 0x5c9730
BackgroundControlObserver::~BackgroundControlObserver()
{
}

// Mac 0x141390, Win 0x5c95ec
void BackgroundControlObserver::OnKeyboardFocusGained(Control* /*control*/, bool /*flag*/,
                                                      Control* /*previous*/, UInt32 /*modifiers*/)
{
    m_sceneControl->SetKeyboardFocus(true);
}

// =============================================================================================
// Win single instance
// =============================================================================================

#if defined(_WIN32)
// Win 0x5c85bb
bool ActivateOtherInstance(const String& applicationName)
{
    SECURITY_ATTRIBUTES attributes;
    attributes.nLength = sizeof(attributes);
    attributes.lpSecurityDescriptor = 0;
    attributes.bInheritHandle = FALSE;
    String mutexName("LDD20");
    // NOTE: the mutex handle is never closed (it marks the running instance until exit).
    HANDLE mutex = PlatformCreateMutex(&attributes, true, &mutexName);
    if (mutex != 0 && GetLastError() != ERROR_ALREADY_EXISTS)
        return false;
    FindWindowData data;
    data.name = applicationName;
    data.window = 0;
    EnumWindows(FindApplicationWindow, (LPARAM)&data);
    if (data.window != 0) {
        String file;
        {
            RefPointer<Preferences> preferences = Preferences::GetInstance();
            preferences->GetValue(String("Open"), file);
        }
        if (!file.IsEmpty()) {
            ATOM atom = GlobalAddAtomW((LPCWSTR)file.GetBuffer());
            if (atom != 0)
                PostMessageW(data.window, 0x8000, (WPARAM)atom, 0);
        }
        SetForegroundWindow(data.window);
        if (IsIconic(data.window))
            ShowWindow(data.window, SW_RESTORE);
    }
    return true;
}
#endif

} // namespace LEGO
