// LEGO Digital Designer — restored source
// LEGO::BGScene — abstract building-guide scene: the primitives placed so far, their connected
//   groups and the direction / collision queries of the BI strategies and BIBrick; the
//   generators apply their events to it (BIEvent::Apply). Only implementation BGAutomaticScene.
//
// RTTI: class LEGO::BGScene (no base class), Mac vtable 0x778290 / Win vtable 0xa2c7b4: 20
// pure entries; no virtual destructor (BGAutomaticScene's destructor overrides Object's and
// gets Mac slots 20/21 of its own). BGAutomaticScene : BGScene, Object (non-virtual Object at
// +4).
//
// Win slot order = Mac order except for the two overload pairs AddBrick / RemoveBrick, which
// MSVC reverses (Win 16 = the set overload Win 0x59c110, Win 17 = the brick one 0x59c080; Win
// 18 = RemoveBrick(set) 0x59c280, Win 19 = RemoveBrick(brick) 0x59c1f0) and GetPrimitives
// (Win 3 = the copying overload 0x59b690, Win 4 = the reference 0x59b4a0). Names inferred.
#pragma once
#include "LEGO/Types.h"
#include "Object.h"
#include "BasicMap.h"
#include "BasicArray.h"
#include "Geometry.h"

namespace LEGO {

class Scene;
class BIBrick;
class BIBrickPrimitive;
class BIGraph;

class BGScene {
public:
    // No primitive placed. Mac 0x35a610, Win 0x59b420
    virtual bool IsEmpty() const = 0;                                      // idx 0
    // The brick shares a primitive with the scene (BIBrick::Intersects with the scene's
    // primitive set). Mac 0x35a620, Win 0x59b440
    virtual bool Intersects(const BIBrick* brick) const = 0;               // idx 1
    // Every primitive of the brick is in the scene (BIBrick::IsContainedIn). Mac 0x35a640,
    // Win 0x59b470
    virtual bool Contains(const BIBrick* brick) const = 0;                 // idx 2
    // The primitives placed in the scene. Mac 0x35a660, Win 0x59b4a0 (slot 4)
    virtual const BasicSet<BIBrickPrimitive*>& GetPrimitives() const = 0;  // idx 3
    // Copies them. Mac 0x359940, Win 0x59b690 (slot 3)
    virtual void GetPrimitives(BasicSet<BIBrickPrimitive*>& primitives) const = 0;   // idx 4
    // The model scene the building guide works on. Mac 0x35a670,
    // Win 0x5d5710 (identical-code folded getter)
    virtual Scene* GetModelScene() = 0;                                    // idx 5
    // The connected groups of the placed primitives. Mac 0x35a5c0, Win 0x59c350
    virtual BasicArray<RefPointer<BIGraph> >& GetGraphs() = 0;            // idx 6
    // The graph that holds the primitive / the (first) primitive of the brick (BIGraph::
    // FindGraph / FindBrickGraph). Mac 0x35a5d0 / 0x35a5f0, Win 0x59c390 / 0x59c370
    virtual BIGraph* FindGraph(BIBrickPrimitive* primitive) = 0;           // idx 7
    virtual BIGraph* FindBrickGraph(BIBrick* brick) = 0;                   // idx 8
    // The bricks may be added: always true (the loop over them has an empty body).
    // Mac 0x35a580, Win 0x59c300
    virtual bool CanAddBricks(const BasicSet<BIBrick*>& bricks) = 0;       // idx 9
    // Always true; the argument is not used (its type is not known). Mac 0x35a0d0,
    // Win 0x5e4100 (identical-code folded `return true`)
    virtual bool IsValid(void* argument) = 0;                              // idx 10
    // Direction in which `brick` can be inserted into the scene (`mode` levels of the bricks
    // it rests on are checked too) / removed from it; `primitives` are added to the scene's
    // primitives for the test. Mac 0x359d00 / 0x3599d0, Win 0x59b9c0 / 0x59b6b0
    virtual bool GetInsertDirection(const BIBrick* brick, Vector3& direction, UInt32 mode, bool flag,
                                    BasicSet<BIBrickPrimitive*>& primitives) = 0;   // idx 11
    virtual bool GetRemoveDirection(const BIBrick* brick, Vector3& direction, bool flag,
                                    BasicSet<BIBrickPrimitive*>& primitives) = 0;   // idx 12
    // Every connection of the brick to the scene (plus `primitives`) allows `direction`.
    // Mac 0x35a0e0, Win 0x59bce0
    virtual bool IsDirectionFree(const BIBrick* brick, Vector3 direction,
                                 const BasicSet<BIBrickPrimitive*>& primitives) = 0;   // idx 13
    // The brick moved by 10 * `direction` collides with no enabled placed primitive.
    // Mac 0x35a290, Win 0x59bf50
    virtual bool IsMoveFree(const BIBrick* brick, Vector3 direction) = 0;  // idx 14
    // Removes every brick. Mac 0x3598f0, Win 0x59b660
    virtual void Clear() = 0;                                              // idx 15
    // Adds / removes the primitives of the brick(s) (only with `update`; always true).
    // Mac 0x35a3a0 / 0x35a410 / 0x35a4b0 / 0x35a520 (see the header comment for Win)
    virtual bool AddBrick(BIBrick* brick, bool update) = 0;                // idx 16
    virtual bool AddBrick(const BasicSet<BIBrick*>& bricks, bool update) = 0;     // idx 17
    virtual bool RemoveBrick(BIBrick* brick, bool update) = 0;             // idx 18
    virtual bool RemoveBrick(const BasicSet<BIBrick*>& bricks, bool update) = 0;  // idx 19

protected:
    BGScene() {}
    ~BGScene() {}
};

} // namespace LEGO
