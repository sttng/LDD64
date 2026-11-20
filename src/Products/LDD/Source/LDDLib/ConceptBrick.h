// LEGO Digital Designer — restored source
// LEGO::ConceptBrick — the primitive elements of one brick of the scene, the unit the falling
//   bricks animation (FallingBricks, DeconstructionCommand) moves: one step per concept brick.
//
// TU 116 "ConceptBrick.cpp" (Mac 0x1d2090-0x1d2baf; Win 0x68d7d1-0x68d914). Besides the class
// the TU holds the grouping function (CreateConceptBricks) and its template instances
// (BasicMap<Brick*, ConceptBrick*, Less<Brick*> >: tree insert Mac 0x1d28b0 / 0x1d2aa0, erase
// 0x1d2860 = Win 0x68d8d2, destructors 0x1d2550 / 0x1d2810 = Win sdd 0x68d8f9).
//
// RTTI: ConceptBrick : Object (non-virtual base: AddRef / Release are direct Object calls).
// Vtables: Mac 0x76ed38 {D1 0x1d26c0, D0 0x1d2760, OnFinalRelease 0x9250, AddRef, Release};
//   Win 0x9dcae4 {sdd 0x68d879, OnFinalRelease, AddRef, Release}.
// Layout (both builds): vptr +0, refcount +4, m_elements +0x08 (vptr +8, begin +0xc, end +0x10,
//   capacity end +0x14); sizeof 0x18 (Mac operator new 0x18).
// Class name from RTTI; member and function names inferred.
#pragma once
#include "LEGO/Types.h"
#include "Object.h"
#include "BasicArray.h"
#include "PrimitiveElement.h"

namespace LEGO {

class Scene;

class ConceptBrick : public Object {
public:
    // One element so far: `element`. Mac 0x1d2090, Win 0x68d7d1
    explicit ConceptBrick(PrimitiveElement* element);
    virtual ~ConceptBrick();                                                 // Mac 0x1d26c0/0x1d2760, Win 0x68d83f (sdd 0x68d879)

    const BasicArray<RefPointer<PrimitiveElement> >& GetElements() const { return m_elements; }
    void AddElement(PrimitiveElement* element) { m_elements.PushBack(RefPointer<PrimitiveElement>(element)); }

    // `elements` grouped by the brick of their part (PrimitiveElement::m_part->m_brick): one
    // ConceptBrick per brick, appended to `conceptBricks` in the order of the first element of
    // each brick. NOTE: `elements` is taken by value (the Mac caller copies the array first).
    // Mac 0x1d21c0 (Win inlined) (name inferred)
    static void CreateConceptBricks(BasicArray<RefPointer<ConceptBrick> >& conceptBricks,
                                    BasicArray<RefPointer<PrimitiveElement> > elements);
    // The same for every primitive element of `scene` (Scene +0x68). Mac 0x1d2580 (Win
    // inlined into DeconstructionCommand::Do 0x65a670) (name inferred)
    static void CreateConceptBricks(BasicArray<RefPointer<ConceptBrick> >& conceptBricks, Scene* scene);

private:
    BasicArray<RefPointer<PrimitiveElement> > m_elements;   // +0x08
};

} // namespace LEGO
