// LEGO Digital Designer — restored source
// LEGO::BrickModeGlobalObject — what all BrickModes share, registered once as 'BMGO' in the
//   ObjectRegistry: the scene holder control (parent of the BrickConnectSceneControl, moved into
//   the current BrickMode's client area), the "none" tool, and the common commands it publishes
//   to Lua (LDD.tools.noneTool, LDD.commands.common.openManual / openPrivacyPolicyCommand /
//   whatsThis / about / preferences; ldd.commands.showHideCameraControl is read back).
// LEGO::WhatsThisCommand — toggles LDDApplication's extended tooltips; enabled by the preference
//   SHOWTOOLTIPS.
//
// Home TU 084 "BrickModeGlobalObject.cpp" (Mac 0x181830-0x18554b, Win 0x62feb3-0x631090);
// bodies in BrickModeGlobalObject.cpp. Win-only command classes AboutCommand /
// PreferencesCommand live there too (the Mac about / preferences commands are plain disabled
// Commands: the application menu of Cocoa handles them).
//
// BrickModeGlobalObject: RTTI Object only (non-virtual base). Mac vtable 0x76941c {D1 0x184670,
//   D0 0x184680, OnFinalRelease 0x9250, AddRef, Release}; Win 0x9d29dc {sdd 0x63008e, …}.
//   Layout (Mac sizeof 0x30, constructor inlined in 0x181cb0; Win ctor 0x62ff2c): vptr,
//   refcount, then the members below at +0x08..+0x2c (Win identical offsets).
// WhatsThisCommand: Command + PreferencesObserver. Mac vtable 0x769454 {D1 0x184930, D0
//   0x1849a0, OnFinalRelease 0x184a30, Do 0x184ac0, SetEnable, GetEnable, UpdateEnable 0x184b10,
//   OnPreferenceChanged 0x184bf0 and OnAttached 0x184c70 (Mac-only primary entries of the
//   PreferencesObserver overriders)}; Win 0x9d2c4c {Do 0x630a3b, SetEnable, GetEnable,
//   UpdateEnable 0x630ac8}, PreferencesObserver-in 0x9d2c3c {OnAttached 0x631d53, OnDetached,
//   OnPreferenceChanged 0x630a73}. Mac layout: Command +0x00..+0x28, PreferencesObserver +0x28
//   (subject +0x2c), Object +0x30 (sizeof 0x38); Win: observer +0x2c, Object +0x3c (0x44).
#pragma once
#include "LEGO/Types.h"
#include "Object.h"
#include "Command.h"
#include "Preferences.h"
#include "Path.h"

namespace LEGO {

class Control;
class ControlManager;
class BrickConnectSceneControl;
class BrickConnectSceneControlTool;

class BrickModeGlobalObject : public Object {
public:
    enum { kRegistryId = 0x424d474f };                                     // 'BMGO'

    BrickModeGlobalObject() {}                                             // inlined (Win 0x62ff2c)
    virtual ~BrickModeGlobalObject();                                      // Mac 0x184670 (body 0x1846c0)/0x184680, Win sdd 0x63008e

    // Creates and registers the global object once: the none tool, the commands, the scene
    // holder (a Control: shown/enabled/active, stretched vertically) with `sceneControl` in it,
    // and the Lua entries. True when it exists or was registered. Mac 0x181cb0, Win 0x630ca1
    // (name inferred)
    static bool Create(ControlManager* manager, BrickConnectSceneControl* sceneControl);
    // The registered object's scene holder / none tool.
    // NOTE: both read a null object when Create was not called (original quirk: crash).
    static Control* GetSceneHolder();                                       // Mac 0x1818c0, Win 0x6307c5
    static BrickConnectSceneControlTool* GetNoneTool();                     // Mac 0x1830e0, Win 0x63077a

    // The "openManual" target: Help/<language>-manual (Platform directory 8) with
    // "<language>-manual.html" appended. Mac 0x1827e0, Win 0x6300b8 (name inferred)
    static Path GetManualPath();

    // ---- data (Mac = Win offsets; names inferred from the Lua names) ----
    RefPointer<BrickConnectSceneControlTool> m_noneTool;              // +0x08
    RefPointer<Command>                      m_openManualCommand;     // +0x0c OpenPathAsURLCommand
    RefPointer<Command>                      m_openPrivacyPolicyCommand; // +0x10 OpenURLCommand
    RefPointer<Command>                      m_whatsThisCommand;      // +0x14 WhatsThisCommand
    RefPointer<Command>                      m_showHideCameraControlCommand; // +0x18 from Lua
    RefPointer<Command>                      m_aboutCommand;          // +0x1c
    RefPointer<Command>                      m_preferencesCommand;    // +0x20
    RefPointer<Object>                       m_unused24;              // +0x24 (never set in TU 084)
    RefPointer<Object>                       m_unused28;              // +0x28 (never set in TU 084)
    RefPointer<Control>                      m_sceneHolder;           // +0x2c (0x160-byte Control)
};

class WhatsThisCommand : public Command, public PreferencesObserver {
public:
    // Toggleable; toggled = LDDApplication::IsExtendedTooltip; observes the Preferences (which
    // runs UpdateEnable). Mac 0x182d20, Win 0x630968
    WhatsThisCommand();
    virtual ~WhatsThisCommand();                                            // Mac 0x184930/0x1849a0, Win sdd 0x630b94

    // Toggles the extended tooltips. Mac 0x184ac0, Win 0x630a3b
    virtual void Do();
    // Enabled = preference SHOWTOOLTIPS; toggled = the tooltip state. Mac 0x184b10, Win 0x630ac8
    virtual void UpdateEnable();
    // SHOWTOOLTIPS changed → UpdateEnable. Mac 0x184bf0 (thunk 0x184dd0), Win 0x630a73
    virtual void OnPreferenceChanged(const String& key);

protected:
    // Observer<Preferences> then NotifierT<Command> parts. Mac 0x184a30, Win 0x630bb3
    virtual void OnFinalRelease();
    // UpdateEnable. Mac 0x184c70 (thunk 0x184db0), Win 0x631d53
    virtual void OnAttached(Preferences* preferences);
    using PreferencesObserver::OnDetached;
};

} // namespace LEGO
