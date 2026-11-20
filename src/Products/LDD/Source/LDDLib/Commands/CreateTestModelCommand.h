// LEGO Digital Designer — restored source
// LEGO::CreateTestModelCommand — developer command ("createTestModel" / "createFilteredTestModel"
//   in LDD.commands.editMode): after Application::CloseDocument a new scene gets one brick of
//   every palette brick (or of those the palette control's filter accepts), each at the next
//   free place of a 20 x 20 x n grid (7.5 apart) where its slightly grown bounding box hits
//   nothing; the designs are lazy-loaded, then the scene is merged into the scene control.
//
// TU 291 "CreateTestModelCommand.cpp": Mac 0x29ef20-0x29fdcb, Win 0x66b594-0x66bad5. The
// constructor is inline (Mac 0x18dfe0, emitted in EditMode.cpp; Win 0x62ae31). The TU also emits
// the BasicArray<CommandObserver*> base-destructor helper Mac 0x29fc70 (Notifier.h); Mac 0x29fd50
// = <iostream> + a header static (not code).
// RTTI: CreateTestModelCommand : Command. Mac vtable 0x74268c {D1 0x29fa90, D0 0x29faf0,
//   OnFinalRelease (NotifierT's), Do 0x29ef20, SetEnable, GetEnable, UpdateEnable 0x29fb60};
//   Win 0x9d1950 {Do 0x66b59b, …, UpdateEnable 0x630465}, Object-in 0x9d193c {sdd 0x62c232}.
//   CreateTestModelCommand::CollidePicker : Scene::SpherePicker, Mac vtable 0x77532c {Test
//   (SpherePicker's), Visit 0x29fc60}, Win 0x9d8190 {0x66b594, 0x46fe40}.
// Layout (Mac): Command +0x00..+0x28, scene control +0x28 (not referenced), spacing +0x2c (7.5),
//   filtered +0x30, palette control +0x34 (referenced), Object +0x38. Win: spacing +0x30,
//   filtered +0x34, scene control +0x38.
// Names inferred.
#pragma once
#include "LEGO/Types.h"
#include "Command.h"
#include "Scene.h"
#include "BrickPaletteControl.h"

namespace LEGO {

class BrickConnectSceneControl;

class CreateTestModelCommand : public Command {
public:
    // Hit flag of a sphere pick: any element in the sphere. (name from RTTI)
    class CollidePicker : public Scene::SpherePicker {
    public:
        explicit CollidePicker(const Sphere& sphere) : Scene::SpherePicker(sphere), m_hit(false) {}
        // Mac 0x29fc60, Win 0x66b594
        virtual void Visit(PrimitiveElement* /*element*/, const Matrix3D& /*transform*/,
                           const Sphere& /*sphere*/) { m_hit = true; }
        bool m_hit;   // +0x14
    };

    // Enabled. Mac 0x18dfe0 (inline, emitted in EditMode.cpp), Win 0x62ae31
    CreateTestModelCommand(BrickConnectSceneControl* sceneControl, BrickPaletteControl* palette, bool filtered)
        : Command(true), m_sceneControl(sceneControl), m_spacing(7.5f), m_filtered(filtered),
          m_palette(palette) {}
    virtual ~CreateTestModelCommand();                                      // Mac 0x29fa90/0x29faf0, Win sdd 0x62c232

    // See the class comment. A palette part whose primitive the database lacks ends the command
    // (nothing is merged).
    virtual void Do();                                                      // 3/0 Mac 0x29ef20, Win 0x66b59b
    // Enabled.
    virtual void UpdateEnable();                                            // 6/3 Mac 0x29fb60, Win 0x630465

private:
    BrickConnectSceneControl*       m_sceneControl;   // Mac +0x28
    Real32                          m_spacing;        // Mac +0x2c
    bool                            m_filtered;       // Mac +0x30
    RefPointer<BrickPaletteControl> m_palette;        // Mac +0x34
};

} // namespace LEGO
