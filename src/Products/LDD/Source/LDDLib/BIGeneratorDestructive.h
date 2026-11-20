// LEGO Digital Designer — restored source
// LEGO::BIGeneratorDestructive — takes the model apart: asks the application's BIProgram for the
//   brick to remove next (SelectBrick, destructive = true), removes it from its own scene and
//   feeds it to the BICameraChangeFinder, which returns the building order (the reverse) with
//   camera step events. With m_forceOnVeto, bricks the program can not decide are taken in set
//   order ("VETO"). Used by BIGenerator::Order and BIGeneratorConstructive.
//
// Home TU 199 "BIGeneratorDestructive.cpp" (Mac 0x24bae0-0x24eadf; Win 0x5797f0-0x57c0ff:
// most Win functions of the class are not paired by func_match, their addresses come from the
// Win vtable 0xa2c26c and the disassembly).
// RTTI: class LEGO::BIGeneratorDestructive : BIGenerator (: BIFacade, virtual Object).
// Mac vtable 0x7356bc (45 entries), Win vtable 0xa2c26c (43 entries, Win slot = Mac idx - 2
// from idx 4 on, overload groups reversed, see BIGenerator.h).
//
// Mac layout (sizeof 0x9c; names inferred):
//   +0x00..+0x7b BIGenerator
//   +0x7c m_substeps     BasicArray<BIBrickSubstep*>: the substeps this generator registered
//                        in the BIBrickRepository (marked used after every generation)
//   +0x8c m_forceOnVeto  (false; never set by the restored code)
//   +0x90 m_scene        BGAutomaticScene of the application's brick scene (referenced)
//   +0x94 Object (virtual base)
// Win: the same members after BIGenerator's (m_forceOnVeto Win +0xa8), Object at the end.
#pragma once
#include "BIGenerator.h"

namespace LEGO {

class BIBrickSubstep;
class BIProgram;
class BGAutomaticScene;

class BIGeneratorDestructive : public BIGenerator {
public:
    typedef BasicArray<RefPointer<BIEvent> > EventList;

    // Creates the scene: new BGAutomaticScene(application->GetBrickScene()).
    // Mac 0x24bb10 (C1; C2 inlined), Win 0x579960 (thiscall, stack: application, most-derived flag)
    explicit BIGeneratorDestructive(BGApplication* application);
    virtual ~BIGeneratorDestructive();                                     // Mac 0x24e7c0 / D0 0x24e820 (body 0x24e9b0), Win 0x579850 / sdd 0x5798f0

    virtual String GetName() const;                                        // idx 1: Mac 0x24e790, Win 0x5797f0 ("Destructive")
    // GenerateBricks(the bricks of `brick`'s primitives, no events, the application's
    // program, true) after BGApplication::AnalyzeBrick(brick); `analyze` is not used.
    // 0 / -1; 0 when cancelled. Mac 0x24bc40, Win 0x579a80
    virtual Int32 Create(BIBrick* brick, bool analyze);                   // idx 4
    // GenerateBricks(the remaining bricks, ...); `events` is not used. Mac 0x24c8b0, Win 0x579b70
    virtual Int32 Create(const EventList& events);                         // idx 5
    // One event forward / back without touching the scene. Mac 0x24e640 / 0x24e670,
    // Win 0x57b930 / 0x57b990
    virtual BIEvent* Next();                                               // idx 7
    virtual BIEvent* Previous();                                           // idx 8
    // Null. Mac 0x24e690 / 0x24e6a0 / 0x24e6b0 / 0x24e6c0 / 0x24e6d0, Win 0x57b9e0 (all)
    virtual BIEvent* Next(const BasicSet<BIBrick*>& bricks);               // idx 9
    virtual BIEvent* Next(Object* unknown);                                   // idx 10
    virtual BIEvent* Previous(Object* unknown);                               // idx 11
    virtual BIEvent* Next(BIEvent* event);                                 // idx 12
    virtual BIEvent* Next(BIEventJumper* jumper);                          // idx 13
    // The position only; false. Mac 0x24e6e0 / 0x24e700, Win 0x57b9f0 / 0x57ba10
    virtual bool Rewind();                                                 // idx 14
    virtual bool FastForward();                                            // idx 15
    // True. Mac 0x24e890, Win 0x59c480 (shared)
    virtual bool IsComplete();                                             // idx 21
    virtual BGScene* GetScene();                                           // idx 22: Mac 0x24e630, Win 0x57b910
    // Nothing (null). Mac 0x24e730, Win 0x57ba40
    virtual RefPointer<BIEvent> TakeNextEvent();                           // idx 23
    // Nothing. Mac 0x24e740, Win 0x56b1c0 (shared)
    virtual void Truncate();                                               // idx 24
    // False. Mac 0x24e750, Win 0x57baa0 (shared with BIEventUnknown::Apply)
    virtual bool ToggleBrick(BIBrick* brick, BasicArray<Pair<bool, RefPointer<BIEvent> > >& changes);   // idx 25
    // Empty. Mac 0x24e760, Win 0x57bab0
    virtual EventList GetCandidates(BIBrick* brick);                       // idx 26
    // True when every brick event of `events`, last first, can be removed from a scene of
    // `primitives` + the events (BIProgram::SelectBrick of that brick alone); `events` is not
    // changed. Mac 0x24e000, Win 0x57b500 (name inferred)
    virtual bool Order(EventList& events, BasicSet<BIBrickPrimitive*>& primitives);   // idx 27
    // False. Mac 0x24e720, Win 0x57ba30
    virtual bool BuildCameraSteps();                                       // idx 28
    // Nothing. Mac 0x24e8a0, Win 0x56b1c0 (shared)
    virtual void Unknown29();                                              // idx 30
    // 0. Mac 0x24e8b0, Win 0x59c690
    virtual Int32 Generate(const BasicSet<BIBrick*>& bricks, bool analyze);   // idx 32
    using BIGenerator::Generate;

    // ---- non-virtual (names inferred) ---------------------------------------------------------
    // Takes the model of `bricks` + the bricks of `placed` apart, brick by brick (the
    // program's choice; with `useGraphs`, first the smallest connected group of the scene that
    // only holds remaining bricks while the scene has more than one), and replaces the events
    // by the building order of the BICameraChangeFinder with steps (InsertSteps). `program`
    // null: the application's. False when the program got stuck (the events are cleared) or
    // the application cancelled (the events are left as they are). Mac 0x24bdd0, Win 0x57a120
    bool GenerateBricks(const BasicSet<BIBrick*>& bricks, const EventList& placed, BIProgram* program,
                        bool useGraphs);
    // Builds the guide in the order of `events` (backwards from the last one, the brick of an
    // event that intersects neither the bricks taken so far nor `placed` and that the program
    // accepts), on top of `placed`: the events become `placed` + the bricks taken, last taken
    // first. Used by BIGeneratorConstructive. Mac 0x24d720, Win 0x57abb0
    bool GenerateReverse(const EventList& events, const EventList& placed);
    // Removes from `remaining` every brick that intersects `brick`; for each such "Direction"
    // substep the rest of it (BIBrickSubstep::Subtract) is registered and added instead unless
    // an equal substep remains. Mac 0x24cfe0, Win 0x57bc30
    void RemoveIntersecting(BIBrick* brick, BasicSet<BIBrick*>& remaining, BGScene* scene);
    // Inserts step events around large bricks (substeps and single-element primitives larger
    // than 8 in two dimensions). Mac 0x24cae0, Win 0x579d80
    void InsertSteps();

    BasicArray<BIBrickSubstep*> m_substeps;       // +0x7c
    bool                        m_forceOnVeto;    // +0x8c
    BGAutomaticScene*           m_scene;          // +0x90 (referenced)
};

} // namespace LEGO
