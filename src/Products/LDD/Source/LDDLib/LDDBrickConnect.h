// LEGO Digital Designer — restored source
// LEGO::LDDBrickConnect — the application's BrickConnect: its factories create LDDBrick and
// LDDPart objects (render-side state: palette brick, flex surfaces, the scene manager that
// supplies materials and meshes). Created by BrickConnectSceneManager::Initialize.
//
// Restored in LDDPart.cpp (Mac TU 351, 0x332610-0x33355b; Win 0x653eef-0x654430). Directory:
// LDDLib (see LDDPart.h).
//
// RTTI: class LEGO::LDDBrickConnect : BrickConnect (Win RTTI attribute 0).
//   Mac sizeof 0xa0 (constructor inlined into BrickConnectSceneManager::Initialize 0x5c900:
//       operator new(0xa0), BrickConnect base constructor 0xc1fc0, +0x94 = scene manager):
//       vptr +0 (vtable 0x74cd90 {0 CreateScene 0xc2860, 1 CreateBrick 0x332980, 2 CreatePart
//       0x332a20, 3 CreatePrimitiveElement 0xc2ae0, 4 CreateBrickGroup 0xc2b50, 5/6 D1 0x332ec0 /
//       D0 0x332f20, 7 OnFinalRelease 0xc4930, 8 CloneBrick 0x332b10, 9 ClonePart 0xc2dd0,
//       10 ClonePrimitiveElement 0x332c30}), m_sceneManager +0x94, Object at +0x98 (vtable
//       0x74cdd4 {thunks 0x332f90, 0x333000, 0xc4950, …}).
//   Win sizeof 0xa8 (constructor 0x60969d: thiscall(physics, database, sceneManager,
//       mostDerived)): vfptr +0 (0x9ce980 {0x490a10, CreateBrick 0x6541d7, CreatePart 0x6543b2,
//       0x490b60, 0x490be0, CloneBrick 0x65420f, 0x490e30, ClonePrimitiveElement 0x653eef}),
//       vbptr +4, m_sceneManager +0x9c, vtordisp +0xa0, Object at +0xa4 (vftable 0x9ce96c
//       {sdd 0x609e20 (vtordisp thunk -> 0x60a8a3), OnFinalRelease thunk 0x609e49, …});
//       destructor 0x609e28.
// Every member and method name is inferred.
#pragma once
#include "LEGO/Types.h"
#include "BrickConnect.h"

namespace LEGO {

class BrickConnectSceneManager;
class AbstractBrick;

class LDDBrickConnect : public BrickConnect {
public:
    // Inlined on Mac (0x5c900), Win 0x60969d.
    LDDBrickConnect(BrickPhysics* physics, BrickDatabase* database,
                    BrickConnectSceneManager* sceneManager)
        : BrickConnect(physics, database), m_sceneManager(sceneManager)
    {
    }

    // --- slot 1: new LDDBrick(designID) (no palette brick). Mac 0x332980, Win 0x6541d7
    virtual Brick* CreateBrick(UInt32 designID);
    // --- slot 2: new LDDPart(primitive, scene manager). Mac 0x332a20, Win 0x6543b2
    virtual Part*  CreatePart(const Primitive* primitive);
    // --- slots 5/6: nothing of its own. Mac 0x332ec0 / 0x332f20 (thunks 0x332f90 / 0x333000),
    // Win 0x609e28 / sdd 0x609e20
    virtual ~LDDBrickConnect() {}
    // --- slot 8: new LDDBrick with the design ID and palette brick of `brick` (which must be
    // an LDDBrick; NOTE: static downcast, no check). Mac 0x332b10, Win 0x65420f
    virtual Brick* CloneBrick(const AbstractBrick* brick);
    // --- slot 10: BrickConnect's. Mac 0x332c30 (jmp 0xc2df0), Win 0x653eef (jmp 0x490e50)
    virtual PrimitiveElement* ClonePrimitiveElement(const PrimitiveElement* element);

    BrickConnectSceneManager* GetSceneManager() const { return m_sceneManager; }

    BrickConnectSceneManager* m_sceneManager;   // Mac +0x94 / Win +0x9c (not referenced)
};

} // namespace LEGO
