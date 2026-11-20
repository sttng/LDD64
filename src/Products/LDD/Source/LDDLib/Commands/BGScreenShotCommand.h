// LEGO Digital Designer — restored source
// LEGO::BGScreenShotCommand — the screenshot of the building-guide mode: the mode's bricks
//   (BGMode::QueueParts) go into an own render queue (2000 entries, made on first use) that
//   ScreenShotCommand::TakeScreenShot renders. "screenShot" of LDD.commands.bgAutomaticMode.
//
// TU 407 "BGScreenShotCommand.cpp": Mac 0x38d370-0x38d71b, Win 0x65ebb5-0x65ec37 (+ ctor
// 0x65eaff, sdd 0x65eaf4). Mac 0x38d6a0 = <iostream> + a header static (not code).
// RTTI: si {ScreenShotCommand}, virtual base Object. Mac vtable 0x7566fc {D1 0x38d4f0,
//   D0 0x38d5e0, OnFinalRelease (ScreenShotCommand's), Do 0x38d440, SetEnable, GetEnable,
//   UpdateEnable / the observer entries (ScreenShotCommand's)}; Win 0x9d7158 {Do 0x65ebd4, …}.
// Layout (Mac, constructor 0x38d370 = ScreenShotCommand C2 0x275c10 + members): ScreenShotCommand
//   +0x00..+0x30, m_queue +0x30, m_mode +0x34, m_render +0x38, Object +0x3c (sizeof 0x44).
//   Win: m_queue +0x38, m_mode +0x3c, m_render +0x40, Object +0x48.
// Names: class RTTI; members inferred.
#pragma once
#include "LEGO/Types.h"
#include "ScreenShotCommand.h"

namespace LEGO {

class BGMode;
class Render;
class RenderQueue;

class BGScreenShotCommand : public ScreenShotCommand {
public:
    // The render of the scene control's manager (RenderControlManager::GetRender).
    // Mac 0x38d370, Win 0x65eaff
    BGScreenShotCommand(BrickConnectSceneControl* control, BGMode* mode);
    virtual ~BGScreenShotCommand();                                         // Mac 0x38d4f0/0x38d5e0, Win sdd 0x65ebb5

    virtual void Do();                                                      // 3/0 Mac 0x38d440, Win 0x65ebd4

private:
    RefPointer<RenderQueue> m_queue;    // Mac +0x30, Win +0x38
    BGMode*                 m_mode;     // Mac +0x34, Win +0x3c
    Render*                 m_render;   // Mac +0x38, Win +0x40
};

} // namespace LEGO
