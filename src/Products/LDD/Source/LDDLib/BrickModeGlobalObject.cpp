// LEGO Digital Designer — restored source
// LEGO::BrickMode, LEGO::BrickModeGlobalObject, LEGO::WhatsThisCommand (+ the Win-only
// AboutCommand / PreferencesCommand). See BrickMode.h / BrickModeGlobalObject.h.
// TU 084 "BrickModeGlobalObject.cpp": Mac 0x181830-0x18554b; Win 0x62feb3-0x62ffeb, 0x63008e-
// 0x630c99, 0x630ca1 (+ the Mode / Notifier template instances at 0x611930-0x611feb).
// Not code: the NotifierT<Mode, ModeObserver> / NotifierT<Command, CommandObserver> template
// instances (Mac 0x184180-0x1845d0, 0x184c80-0x184ee0, 0x1850e0 RemoveObserver, 0x1852d0),
// construction-vtable thunks, Mac 0x1854d0 = <iostream> + a header static.
#include "BrickMode.h"
#include "BrickModeGlobalObject.h"
#include "BrickConnectSceneControl.h"
#include "BrickConnectSceneControlTool.h"
#include "LDDApplication.h"
#include "Application.h"
#include "ControlManager.h"
#include "RenderControl.h"
#include "ObjectRegistry.h"
#include "Platform.h"
#include "Directory.h"
#include "TextLocale.h"
#include "Commands/OpenPathAsURLCommand.h"
#include "Commands/OpenURLCommand.h"
#include "LuaScriptManager.h"
#include "LuaBindings.h"
#include "LuaCommandAdapter.h"
#include "LuaTool.h"
#include "LuaPlus.h"

#include <math.h>

using namespace LuaPlus;

namespace LEGO {

namespace {

// Nearest integer, ties up (Mac: floorf / ceilf inline; Win 0x4c9630 RoundToNearest(v, 0)).
Real32 RoundToNearest(Real32 value)
{
    Real32 low = floorf(value);
    Real32 high = ceilf(value);
    return (value - low < high - value) ? low : high;
}

Real32 MaxOf(Real32 a, Real32 b)                                         // Win 0x40b4c0 (inlined on Mac)
{
    return a <= b ? b : a;
}

// The registered global object (null when there is none). The registry entry is kept
// referenced by the caller's RefPointer.
RefPointer<BrickModeGlobalObject> FindGlobalObject()
{
    RefPointer<Object> object = ObjectRegistry::GetInstance()->Find(BrickModeGlobalObject::kRegistryId);
    return RefPointer<BrickModeGlobalObject>(dynamic_cast<BrickModeGlobalObject*>(object.Get()));
}

// The Command of a Lua object: the object of a Command adapter, else a bound object that is a
// Command. (Win 0x446fc0 GetLuaCommand, inlined on the Mac; same as LuaWindow.cpp's helper)
Command* GetLuaCommand(LuaObject& object)
{
    BindableLuaObject* bound = GetBoundObject(object);
    if (!bound)
        return 0;
    if (AdapterBindableLuaObject<Command>* adapter = dynamic_cast<AdapterBindableLuaObject<Command>*>(bound))
        return adapter->GetAdaptedObject();
    return dynamic_cast<Command*>(bound);
}

#if defined(_WIN32) || defined(LDD_LINUX)
// Win-only (the Mac about / preferences menu items are Cocoa's; LDD64 Linux: the Windows menus). Win vtables 0x9d2a6c /
// 0x9d2a94; constructors 0x6303f7 / 0x630479 (enabled), Do 0x63046d / 0x6304e7.
class AboutCommand : public Command {
public:
    AboutCommand() : Command(true) {}
    virtual void Do() { Application::GetInstance()->ShowAboutBox(); }         // Win 0x63046d
    virtual void UpdateEnable() { SetEnable(true); }                          // Win 0x630465
};

class PreferencesCommand : public Command {
public:
    PreferencesCommand() : Command(true) {}
    virtual void Do() { Application::GetInstance()->ShowPreferences(); }      // Win 0x6304e7
    virtual void UpdateEnable() { SetEnable(true); }                          // Win 0x630465
};
#endif

} // namespace

// =============================================================================================
// BrickMode
// =============================================================================================

// Mac 0x1831f0 (C2; C1 inlined into the subclasses), Win 0x630519
BrickMode::BrickMode(ControlManager* manager, Window* window, BrickConnectSceneControl* sceneControl)
    : Mode(manager, window), m_sceneControl(sceneControl)
{
}

// Mac 0x183930 (D1, body 0x1836b0), 0x183bc0 (D0), Win 0x630810 (sdd 0x630c4d)
BrickMode::~BrickMode()
{
}

// Mac 0x183370
bool BrickMode::Create(Control* parent, Control* after)
{
    if (!AttachTo(parent, after))
        return false;
    m_toolbar = new RenderControl(m_manager);
    if (!m_toolbar->Initialize())
        return false;
    m_toolbar->Setup(this, 0, 7, Rect(0.0f, 0.0f, 0.0f, 0.0f), 0, false);
    m_toolbar->SetAxisMargins(0, 0.0f, 0.0f, 0);
    m_toolbar->SetAxisAlignment(1, 1, 1, 0.0f, 0);

    m_client = new Control(m_manager);
    m_client->Setup(this, 0, 7, Rect(0.0f, 0.0f, 0.0f, 0.0f), 0, false);
    m_client->SetAxisMargins(0, 0.0f, 0.0f, 0);
    // From the toolbar's bottom to the parent's bottom.
    m_client->SetAxisAlignments(1, 1, 3, 0.0f, m_toolbar, 3, 3, 0.0f, 0);
    return BrickModeGlobalObject::Create(m_manager, m_sceneControl);
}

// Mac 0x181830, Win 0x630be3
void BrickMode::Enter()
{
    Mode::Enter();
    ControlObserver::Attach(m_client);
    BrickModeGlobalObject::GetSceneHolder()->SetParent(m_client, m_client);
    if (m_sidePanel) {
        m_sidePanel->SetParent(m_client, 0);
        if (m_sidePanel)
            ControlObserver::Attach(m_sidePanel);
    }
    Layout();
}

// Mac 0x181c50, Win 0x630748
void BrickMode::Exit()
{
    if (m_sidePanel)
        ControlObserver::Detach(m_sidePanel);
    ControlObserver::Detach(m_client);
    Mode::Exit();
}

// Mac 0x183680 (thunk 0x183690), Win 0x630c6f
void BrickMode::OnRectangleChanged(Control*, const Vector2&, const Vector2&)
{
    Layout();
}

// Mac 0x1819d0, Win 0x630826
void BrickMode::Layout()
{
    Control* holder = BrickModeGlobalObject::GetSceneHolder();
    Real32 width = m_client->GetSize().x;
    Real32 holderWidth;
    Real32 sideWidth;
    Real32 x;
    if (!m_sidePanel) {
        holderWidth = MaxOf(holder->GetMinimumSize().x, width);
        sideWidth = 0.0f;
        x = 0.0f;
    } else {
        Rect side(m_sidePanel->GetPosition(), m_sidePanel->GetSize());
        side &= Rect(m_client->GetPosition(), m_client->GetSize());
        Real32 overlap = side.GetWidth();
        sideWidth = m_sidePanel->GetSize().x - overlap;
        holderWidth = MaxOf(holder->GetMinimumSize().x, width - overlap);
        x = MaxOf(m_sidePanel->GetMinimumSize().x - sideWidth, width - holderWidth);
    }
    holder->SetRectangle(Rect(Vector2(RoundToNearest(x), 0.0f),
                              Vector2(RoundToNearest(holderWidth), 0.0f)));
    if (m_sidePanel)
        m_sidePanel->SetSize(Vector2(RoundToNearest(sideWidth + x), 0.0f));
}

// Mac 0x181c90
Control* BrickMode::GetClientControl() const
{
    return m_client;
}

// Mac 0x181ca0
Control* BrickMode::GetToolbarControl() const
{
    return m_toolbar;
}

// Mac 0x183670, Win 0x62febe
BrickConnectSceneControl* BrickMode::GetSceneControl() const
{
    return m_sceneControl;
}

// Mac 0x183650, Win 0x62feb3
BrickConnectSceneManager* BrickMode::GetSceneManager() const
{
    return m_sceneControl->GetSceneManager();
}

// Mac 0x183eb0 (thunks 0x183f20, 0x183fa0, 0x184020, 0x184090, 0x184100), Win 0x630c7d
void BrickMode::OnFinalRelease()
{
    ControlObserver::OnFinalRelease();
    Mode::OnFinalRelease();
}

// =============================================================================================
// BrickModeGlobalObject
// =============================================================================================

// Mac 0x184670 (body 0x1846c0), 0x184680 (D0), Win 0x62ffeb (sdd 0x63008e)
BrickModeGlobalObject::~BrickModeGlobalObject()
{
}

// Mac 0x1818c0, Win 0x6307c5
Control* BrickModeGlobalObject::GetSceneHolder()
{
    RefPointer<BrickModeGlobalObject> global = FindGlobalObject();
    return global->m_sceneHolder;
}

// Mac 0x1830e0, Win 0x63077a
BrickConnectSceneControlTool* BrickModeGlobalObject::GetNoneTool()
{
    RefPointer<BrickModeGlobalObject> global = FindGlobalObject();
    return global->m_noneTool;
}

// Mac 0x1827e0, Win 0x6300b8
Path BrickModeGlobalObject::GetManualPath()
{
    RefPointer<Directory> directory = Platform::GetInstance()->GetSystemDirectory(8, true);
    directory = directory->GetDirectory(String("Help"), false);
    String name = TextLocale::GetInstance()->GetLanguage();
    name << "-manual";
    Path path(name, directory);
    String file;
    file << name << ".html";
    path.Append(file);
    return path;
}

// Mac 0x181cb0, Win 0x630ca1
bool BrickModeGlobalObject::Create(ControlManager* manager, BrickConnectSceneControl* sceneControl)
{
    if (!FindGlobalObject().IsNull())
        return true;

    RefPointer<BrickModeGlobalObject> global = new BrickModeGlobalObject();
    global->m_noneTool = new BrickConnectSceneControlTool(false);
    global->m_openManualCommand = new OpenPathAsURLCommand(GetManualPath(), true);
    global->m_openPrivacyPolicyCommand = new OpenURLCommand(String("http://lego.com/go/7/privacy"), true);
    global->m_whatsThisCommand = new WhatsThisCommand();
#if defined(_WIN32) || defined(LDD_LINUX)
    global->m_aboutCommand = new AboutCommand();
    global->m_preferencesCommand = new PreferencesCommand();
#else
    // Mac: disabled plain Commands (Cocoa's application menu shows about / preferences).
    global->m_aboutCommand = new Command(false, false);
    global->m_preferencesCommand = new Command(false, false);
#endif
    global->m_sceneHolder = new Control(manager);
    global->m_sceneHolder->Setup(0, 0, 7, Rect(0.0f, 0.0f, 0.0f, 0.0f), 0, false);
    global->m_sceneHolder->SetAxisMargins(1, 0.0f, 0.0f, 0);
    sceneControl->SetParent(global->m_sceneHolder, 0);

    if (!ObjectRegistry::GetInstance()->Register(kRegistryId, global))
        return false;

    RefPointer<LuaScriptManager> scripts = LuaScriptManager::GetInstance();
    LuaState* state = scripts->GetLuaState();
    LuaObject globals = state->GetGlobals();
    {
        LuaObject object = globals.Lookup("ldd.commands.showHideCameraControl");
        global->m_showHideCameraControlCommand = GetLuaCommand(object);
    }
    {
        LuaObject tools = globals.Lookup("LDD.tools");
        LuaObject tool = LuaTool::CreateLuaObject(global->m_noneTool, state);
        tools.SetObject("noneTool", tool);
    }
    LuaObject common = globals.Lookup("LDD.commands.common");
    struct Entry {
        const char* m_name;
        Command*    m_command;
    };
    const Entry entries[] = {
        { "openManual", global->m_openManualCommand },
        { "openPrivacyPolicyCommand", global->m_openPrivacyPolicyCommand },
        { "whatsThis", global->m_whatsThisCommand },
        { "about", global->m_aboutCommand },
        { "preferences", global->m_preferencesCommand }
    };
    for (UInt32 i = 0; i < sizeof(entries) / sizeof(entries[0]); ++i) {
        LuaObject object = LuaCommandAdapter::CreateLuaObject(entries[i].m_command, state);
        common.SetObject(entries[i].m_name, object);
    }
    return true;
}

// =============================================================================================
// WhatsThisCommand
// =============================================================================================

// Mac 0x182d20, Win 0x630968
WhatsThisCommand::WhatsThisCommand()
    : Command(true, true)
{
    SetToggled(LDDApplication::GetInstance()->IsExtendedTooltip());
    RefPointer<Preferences> preferences = Preferences::GetInstance();
    PreferencesObserver::Attach(preferences);
}

// Mac 0x184930 (D1), 0x1849a0 (D0), Win 0x630b2a (sdd 0x630b94)
WhatsThisCommand::~WhatsThisCommand()
{
}

// Mac 0x184ac0, Win 0x630a3b
void WhatsThisCommand::Do()
{
    bool enable = !LDDApplication::GetInstance()->IsExtendedTooltip();
    LDDApplication::GetInstance()->SetExtendedTooltip(enable);
    SetToggled(enable);
}

// Mac 0x184b10, Win 0x630ac8
void WhatsThisCommand::UpdateEnable()
{
    // NOTE: the original leaves the flag uninitialised when SHOWTOOLTIPS is not set (Mac
    // local_11 / Win local_18); false here.
    bool enabled = false;
    GetSubject()->GetValue(String("SHOWTOOLTIPS"), enabled);
    SetEnable(enabled);
    SetToggled(LDDApplication::GetInstance()->IsExtendedTooltip());
}

// Mac 0x184bf0 (thunk 0x184dd0), Win 0x630a73
void WhatsThisCommand::OnPreferenceChanged(const String& key)
{
    if (key.EqualsNoCase(String("SHOWTOOLTIPS")))
        UpdateEnable();
}

// Mac 0x184c70 (thunk 0x184db0), Win 0x631d53
void WhatsThisCommand::OnAttached(Preferences*)
{
    UpdateEnable();
}

// Mac 0x184a30 (thunks 0x184d90, 0x184f80), Win 0x630bb3
void WhatsThisCommand::OnFinalRelease()
{
    PreferencesObserver::OnFinalRelease();
    Command::OnFinalRelease();
}

} // namespace LEGO
