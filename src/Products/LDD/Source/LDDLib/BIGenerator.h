// LEGO Digital Designer — restored source
// LEGO::BIGenerator — base of the guide generators (BIGeneratorConstructive /
//   BIGeneratorDestructive): the event sequence built so far, the current position in it and
//   the BGApplication it works for.
//
// Home TU 149 "BIGenerator.cpp" (Mac 0x20f5b0-0x2136bf). Both classes derive from Object
// virtually: Mac BIGenerator sizeof 0x84 (Object at +0x7c), constructors take a VTT
// (base-object constructor Mac 0x20f5b0, called by the subclasses).
// Mac vtables: BIFacade 0x72b8dc {pure, pure, D1 0x212910, D0 0x212930}, BIGenerator 0x72b7dc
// (45 entries, see the slot comments: "idx" = Mac index; Ghidra's vfNN = idx - 1 from idx 4 on).
// Windows: the destructor overrides Object's in the virtual base and gets no slot of its own in
// the primary vftable (BIGenerator Win vftable 0xa2ba6c, 43 entries: Win slot = idx - 2 from
// idx 4 on); MSVC groups and reverses the overloads Create (idx 4/5, Win 3/2), Next / Previous
// (idx 7-13, Win 5-11) and Generate (Win 29 / 30).
//
// Mac layout of BIGenerator (names inferred):
//   +0x00 vptr, +0x04 BIFacade::m_parent (0), +0x08 m_application,
//   +0x0c m_allowedBricks (BasicSet<BIBrick*>), +0x28 m_current, +0x2c m_reference2c,
//   +0x30 m_events, +0x40 m_bricks40, +0x5c m_bricks5c (BasicSet<BIBrick*>, used by the
//   subclasses), +0x78 m_changed (true), +0x7c Object.
#pragma once
#include "LEGO/Types.h"
#include "Object.h"
#include "LEGO/String.h"
#include "BasicArray.h"
#include "BasicMap.h"
#include "BI.h"
#include "BIFacade.h"

namespace LEGO {

class BIEvent;
class BIBrick;
class BIBrickPrimitive;
class BIEventJumper;
class BGScene;
class BGApplication;

class BIGenerator : public BIFacade {
public:
    // Mac 0x20f5b0 (base-object constructor with VTT)
    explicit BIGenerator(BGApplication* application);
    virtual ~BIGenerator();                                                // idx 2/3: Mac 0x2129f0 / 0x212a50, Win sdd 0x560360

    virtual UInt32 GetFacadeType() const;                                  // idx 0: Mac 0x2129e0, Win 0x58a960 (1)
    // ---- slots implemented by the subclasses (names inferred) ------------------------------
    // MSVC groups overloads and reverses them (the Win order of idx 7-13 is: Next(BIEventJumper*),
    // Next(BIEvent*), Next(Object*), Next(set), Next(), Previous(Object*), Previous(); Win slots
    // 5-11, see the BIGeneratorConstructive / BIGeneratorDestructive vtables).
    // Creates the guide for the bricks of `brick` (`analyze`: let the application analyze
    // it first); 0 on success, -1 on failure / cancel. (Win slot 3: overload pair, reversed)
    virtual Int32 Create(BIBrick* brick, bool analyze) = 0;                // idx 4
    // Takes over `events` (Constructive) / creates the guide of all remaining bricks
    // (Destructive). (Win slot 2)
    virtual Int32 Create(const BasicArray<RefPointer<BIEvent> >& events) = 0;   // idx 5
    // The event `offset` positions after the current one (1 = the next event), null outside.
    // Mac 0x212540 (name inferred)
    virtual BIEvent* GetEvent(Int32 offset);                               // idx 6
    // One event forward (applied to the scene) / back (undone); null at the end / start.
    virtual BIEvent* Next() = 0;                                           // idx 7
    virtual BIEvent* Previous() = 0;                                       // idx 8
    // Next() when the brick of the next event is one of `bricks`, else null.
    virtual BIEvent* Next(const BasicSet<BIBrick*>& bricks) = 0;           // idx 9
    // Null in both generators (meaning unknown).
    virtual BIEvent* Next(Object* unknown) = 0;                               // idx 10
    virtual BIEvent* Previous(Object* unknown) = 0;                           // idx 11
    // Inserts `event` at the current position and steps over it; returns it.
    virtual BIEvent* Next(BIEvent* event) = 0;                             // idx 12
    virtual BIEvent* Next(BIEventJumper* jumper) = 0;                      // idx 13
    // ---- implemented here --------------------------------------------------------------------
    // Clears the scene, back to the start. Mac 0x211890 (name inferred)
    virtual bool Rewind();                                                 // idx 14
    // Adds every remaining brick to the scene and moves to the end. Mac 0x2118f0 (name inferred)
    virtual bool FastForward();                                            // idx 15
    // Position of the event of `brick` relative to the current one. Mac 0x212730 (name inferred)
    virtual bool FindBrick(const BIBrick* brick, Int32& offset);           // idx 16
    virtual UInt32 GetCurrent() const;                                     // idx 17: Mac 0x212ac0
    // The event count when IsComplete, else 0. Mac 0x212ad0 (name inferred)
    virtual UInt32 GetCompleteCount();                                     // idx 18
    virtual bool IsAtStart() const;                                        // idx 19: Mac 0x211b10
    // No next event and complete. Mac 0x211b20 (name inferred)
    virtual bool IsAtEnd();                                                // idx 20
    // Every primitive of the current substep (BGApplication::GetSubsteps().Back()) is placed. Mac 0x211f00 (name inferred)
    virtual bool IsComplete();                                             // idx 21
    virtual BGScene* GetScene() = 0;                                       // idx 22 (subclasses: +0x90)
    // Removes the next event and returns it. Mac 0x20fc30 (name inferred)
    virtual RefPointer<BIEvent> TakeNextEvent();                           // idx 23
    // Drops the events after the current one. Mac 0x20fe60 (name inferred)
    virtual void Truncate();                                               // idx 24
    // Adds `brick` to the guide at the current position (it is not placed yet) or removes
    // it (it is); `changes` receives the events applied (true) / undone (false). A null
    // brick replaces the last placed brick by another candidate.
    virtual bool ToggleBrick(BIBrick* brick, BasicArray<Pair<bool, RefPointer<BIEvent> > >& changes) = 0;   // idx 25
    // Brick events of the bricks that could be placed / removed now (for `brick`, or all).
    virtual BasicArray<RefPointer<BIEvent> > GetCandidates(BIBrick* brick) = 0;   // idx 26
    // Orders `events` for the given placed primitives (here: by a BIGeneratorDestructive of the
    // same application). Mac 0x20fec0 (name inferred)
    virtual bool Order(BasicArray<RefPointer<BIEvent> >& events, BasicSet<BIBrickPrimitive*>& primitives);  // idx 27
    // Replaces the events by the connected groups of the current substep (BGApplication::GetSubsteps().Back()), with step events between
    // camera changes. Mac 0x20f690 (name inferred)
    virtual bool BuildCameraSteps();                                       // idx 28
    // Inserts `brick` (replacing the events of the bricks it intersects) and reorders the rest.
    // Mac 0x20ff70 (name inferred)
    virtual bool InsertBrick(BIBrick* brick);                              // idx 29
    virtual void Unknown29() = 0;                                          // idx 30
    // Generate(the remaining bricks, true). Mac 0x210fc0 (name inferred)
    virtual Int32 Generate();                                              // idx 31
    // Generates the guide of `bricks` (`analyze`: substeps from the analyzers); Constructive:
    // the number of program relaxations needed (0 also on failure), Destructive: 0.
    virtual Int32 Generate(const BasicSet<BIBrick*>& bricks, bool analyze) = 0;  // idx 32
    // The events split into steps (BI::GetSteps). Mac 0x211b70 (name inferred)
    virtual void GetSteps(BasicArray<BI::Step>& steps);                    // idx 33
    virtual void GetEvents(BasicArray<RefPointer<BIEvent> >& events) const;         // idx 34: Mac 0x212b00
    virtual UInt32 GetEventCount() const;                                  // idx 35: Mac 0x212b40
    virtual void GetBrickEvents(BasicArray<RefPointer<BIEvent> >& events) const;    // idx 36: Mac 0x211770
    virtual void SetEvents(const BasicArray<RefPointer<BIEvent> >& events);        // idx 37: Mac 0x2126f0
    virtual void GetEventsBefore(BasicArray<RefPointer<BIEvent> >& events) const;   // idx 38: Mac 0x212570
    virtual void GetBrickEventsBefore(BasicArray<RefPointer<BIEvent> >& events) const;  // idx 39: Mac 0x2117c0
    // Brick events from the last one back to the current one. (name inferred)
    virtual void GetBrickEventsReversed(BasicArray<RefPointer<BIEvent> >& events) const;  // idx 40: Mac 0x2125c0
    virtual void GetEventsAfter(BasicArray<RefPointer<BIEvent> >& events) const;    // idx 41: Mac 0x212620
    virtual void GetBrickEventsAfter(BasicArray<RefPointer<BIEvent> >& events) const;   // idx 42: Mac 0x212680
    // The bricks of the trailing substep events. (name inferred)
    virtual void GetTrailingSubsteps(BasicArray<BIBrick*>& bricks) const;  // idx 43: Mac 0x211c60
    virtual void Unknown43();                                              // idx 44: Mac 0x212b60 (empty)

    // ---- non-virtual (names inferred) ---------------------------------------------------------
    // Primitives of the current substep (BGApplication::GetSubsteps().Back()) not yet placed: placed = the brick events (all, or before
    // the current one), the bricks of `extra`; with `fromApplication` the application's
    // placed primitives and the repository's free primitives instead. Mac 0x210ad0
    void CollectRemainingPrimitives(BasicSet<BIBrickPrimitive*>& primitives, bool beforeCurrent, bool fromApplication,
                                    const BasicArray<RefPointer<BIEvent> >& extra);
    // Free bricks not intersecting the placed primitives (same sources; with `beforeCurrent`
    // the brick events after the first marker count as placed too). Mac 0x2110b0
    void CollectRemainingBricks(BasicSet<BIBrick*>& bricks, bool beforeCurrent, bool fromApplication,
                                const BasicArray<RefPointer<BIEvent> >& extra);
    // In m_allowedBricks or its group enabled by the application. Mac 0x211820
    bool IsAllowed(BIBrick* brick);
    // Mac 0x211b50
    bool HasEvents() const;
    // The brick events of the current step (before and after the current position).
    // Mac 0x211fd0
    void GetCurrentStepBricks(BasicArray<RefPointer<BIEvent> >& events);
    // First brick of `type` at offset >= `start` (GetEvent). Mac 0x212270 / 0x2122f0, last one
    // at offset <= `start` Mac 0x212360
    bool FindBrickOfType(UInt32 type, Int32& offset, Int32 start);
    BIEvent* FindNextBrickOfType(UInt32 type, Int32 start);
    BIEvent* FindPreviousBrickOfType(UInt32 type, Int32 start);
    // The brick of events [from, to): a primitive, a new (unregistered) substep or null.
    // Mac 0x2123d0
    RefPointer<BIBrick> MakeBrick(UInt32 from, UInt32 to);
    // Appends a marker event unless there is one. Mac 0x212790
    bool AppendMarker();
    // Removes the first marker event (index into `index`). Mac 0x212870
    bool RemoveMarker(UInt32& index);

    BGApplication*                   m_application;     // +0x08
    BasicSet<BIBrick*>               m_allowedBricks;   // +0x0c
    UInt32                           m_current;         // +0x28
    RefPointer<Object>               m_reference2c;     // +0x2c (released by the destructor)
    BasicArray<RefPointer<BIEvent> > m_events;          // +0x30
    BasicSet<BIBrick*>               m_bricks40;        // +0x40
    BasicSet<BIBrick*>               m_bricks5c;        // +0x5c
    bool                             m_changed;         // +0x78
};

} // namespace LEGO
