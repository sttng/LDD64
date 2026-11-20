// LEGO Digital Designer — restored source
// LEGO::BIGeneratorConstructive — the application's current generator: Generate lets a
//   BIGeneratorDestructive take the model apart with the application's program (relaxed until
//   it succeeds) and keeps the resulting building order; Next / Previous step through it,
//   ToggleBrick re-plans around a brick, GetCandidates lists the bricks placeable next.
//
// Home TU 212 "BIGeneratorConstructive.cpp" (Mac 0x25ab70-0x25e34f; Win: vtable 0xa2c6cc,
// functions 0x592f90-0x595a00, few paired by func_match).
// RTTI: class LEGO::BIGeneratorConstructive : BIGenerator (: BIFacade, virtual Object).
// Mac vtable 0x736c8c (45 entries), Win vtable 0xa2c6cc (43 entries, see BIGenerator.h).
//
// Mac layout (sizeof 0x9c; names inferred):
//   +0x00..+0x7b BIGenerator
//   +0x7c m_mode   constructor argument (1 from BGAutomaticApplication; not read here)
//   +0x80 m_saved  BasicArray<RefPointer<BIEvent> >: the events as of the last change made
//                  through this generator (the source of re-planning, SelectNext)
//   +0x90 m_scene  BGAutomaticScene of the application's brick scene (referenced)
//   +0x94 Object (virtual base)
#pragma once
#include "BIGenerator.h"

namespace LEGO {

class BIBrickSubstep;
class BGAutomaticScene;

class BIGeneratorConstructive : public BIGenerator {
public:
    typedef BasicArray<RefPointer<BIEvent> > EventList;
    typedef BasicArray<Pair<bool, RefPointer<BIEvent> > > ChangeList;

    // Mac 0x25ab70 (C1; C2 inlined), Win: inlined into BGAutomaticApplication::CreateGenerator
    // (Mac 0x346880, Win 0x566fb0)
    BIGeneratorConstructive(BGApplication* application, UInt32 mode);
    virtual ~BIGeneratorConstructive();                                    // Mac 0x25e010 / D0 0x25e070 (body 0x25e1d0), Win 0x592ff0 / sdd 0x593090

    virtual String GetName() const;                                        // idx 1: Mac 0x25dfe0, Win 0x592f90 ("Constructive")
    // Mac 0x25acd0, Win 0x593270
    virtual Int32 Create(BIBrick* brick, bool analyze);                   // idx 4
    // SetEvents(events); 0. Mac 0x25ae70, Win 0x593420
    virtual Int32 Create(const EventList& events);                         // idx 5
    // Mac 0x25b3c0 / 0x25b440, Win 0x5935e0 / 0x593650
    virtual BIEvent* Next();                                               // idx 7
    virtual BIEvent* Previous();                                           // idx 8
    // Mac 0x25aec0, Win 0x593460
    virtual BIEvent* Next(const BasicSet<BIBrick*>& bricks);               // idx 9
    // Null. Mac 0x25b4c0 / 0x25b4d0, Win 0x57b9e0
    virtual BIEvent* Next(Object* unknown);                                   // idx 10
    virtual BIEvent* Previous(Object* unknown);                               // idx 11
    // Mac 0x25b4e0 / 0x25b590 (identical), Win 0x5936c0 (both)
    virtual BIEvent* Next(BIEvent* event);                                 // idx 12
    virtual BIEvent* Next(BIEventJumper* jumper);                          // idx 13
    virtual BGScene* GetScene();                                           // idx 22: Mac 0x25dfd0, Win 0x57b910
    // Mac 0x25bb50, Win 0x593aa0
    virtual bool ToggleBrick(BIBrick* brick, ChangeList& changes);        // idx 25
    // Mac 0x25c990, Win 0x593ca0
    virtual EventList GetCandidates(BIBrick* brick);                       // idx 26
    // Clears the events and the scene and generates the guide of the remaining bricks
    // (Generate() >= 0). Mac 0x25db00, Win 0x594ac0
    virtual bool BuildCameraSteps();                                       // idx 28
    // Nothing. Mac 0x25c980, Win 0x56b1c0 (shared)
    virtual void Unknown29();                                              // idx 30
    // Mac 0x25db70, Win 0x595460
    virtual Int32 Generate(const BasicSet<BIBrick*>& bricks, bool analyze);   // idx 32
    using BIGenerator::Generate;

    // ---- non-virtual (names inferred) ---------------------------------------------------------
    // Puts the event before the current position back and tries another candidate brick
    // (the bricks tried are excluded through m_allowedBricks; when none is left, the
    // exclusions are dropped once); `changes` receives the event undone and the event applied.
    // False when nothing changed. Mac 0x25b640
    bool Retry(ChangeList& changes);
    // The program's choice among the brick events of `events` that are not placed nor
    // excluded, that PlaceBrick accepts ("Brick found" / "Trying another..." / "No bricks
    // found!"). Mac 0x25af50
    RefPointer<BIEvent> SelectNext(const EventList& events);
    // Places `brick` at the current position: bricks before it that intersect it are split
    // (their primitives outside it are kept) and the order before it is checked (Order), else
    // the rest of `events` is re-planned after it (BIGeneratorDestructive::GenerateReverse).
    // Mac 0x25bd60
    bool PlaceBrick(BIBrick* brick, const EventList& events);

    UInt32             m_mode;    // +0x7c
    EventList          m_saved;   // +0x80
    BGAutomaticScene*  m_scene;   // +0x90 (referenced)
};

} // namespace LEGO
