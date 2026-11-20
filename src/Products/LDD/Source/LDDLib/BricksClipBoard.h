// LEGO Digital Designer — restored source
// LEGO::BricksClipBoard — the edit mode's clip board: the element-brick snapshots the cut / copy
//   commands store and the paste command hands to the click-stick tool.
// LEGO::BricksClipBoardObserver — Observer<BricksClipBoard, …> (PasteCommand updates its enable
//   state through it).
//
// BricksClipBoard: home TU 085 "EditMode.cpp" (the EditMode constructor creates the only one).
//   RTTI: BricksClipBoard : Notifier<BricksClipBoardObserver>. Mac layout (inline constructor in
//   EditMode Mac 0x1855c0): NotifierT +0x00..+0x24, bricks +0x24 (BasicArray<ObjectRefPointer<
//   PrimitiveElementBrick>>, vptr 0x760dd8), Object +0x34 (sizeof 0x3c); vtable 0x769710 {D1
//   0x191830, D0 0x191890, OnFinalRelease 0x191450}. Win: constructor 0x62a6a8 (sizeof 0x40),
//   vftable 0x9d1408 (Object-in, sdd 0x62c6d4 / 0x62a71b).
// BricksClipBoardObserver: home TU 249 (PasteCommand): Mac vtable 0x739cfc {D1 0x2739e0, D0
//   0x273a10, OnFinalRelease 0x273a60, OnAttached 0x273ab0, OnDetached 0x273ac0, slot 5 0x273ad0
//   (empty)}; Win 0x9d867c. Slot 5 takes the "clip board is empty" flag: CutCommand's
//   notification (Mac 0x26c360) passes bricks.begin == end, the Win overriders `ret 4`.
// The writer of the bricks is CutCommand (TU 233); the accessors and the notification are
// inline here. Names inferred.
#pragma once
#include "LEGO/Types.h"
#include "BasicArray.h"
#include "ObjectRefPointer.h"
#include "Notifier.h"

namespace LEGO {

class BricksClipBoard;
class PrimitiveElementBrick;

class BricksClipBoardObserver : public Observer<BricksClipBoard, BricksClipBoardObserver> {
public:
    BricksClipBoardObserver() {}
    virtual ~BricksClipBoardObserver() {}                                   // Mac 0x2739e0/0x273a10
    // The clip board content changed; `empty`: no bricks. Slot 5, Mac 0x273ad0 (name inferred)
    virtual void OnClipBoardChanged(bool /*empty*/) {}
};

class BricksClipBoard : public Notifier<BricksClipBoardObserver> {
public:
    typedef BasicArray<ObjectRefPointer<PrimitiveElementBrick> > BrickArray;

    BricksClipBoard() {}                                                    // Mac inline (0x1855c0), Win 0x62a6a8
    virtual ~BricksClipBoard() {}                                           // Mac 0x191830/0x191890

    const BrickArray& GetBricks() const { return m_bricks; }
    BrickArray&       GetBricks()       { return m_bricks; }

    // Tells the observers (slot 5) whether the clip board is empty.
    // Weak instance emitted with CutCommand, Mac 0x26c360 (Win inlined into 0x66e889).
    // (name inferred)
    void NotifyChanged()
    {
        Iterator pass(*this);
        while (BricksClipBoardObserver* observer = pass.Next(m_observers))
            observer->OnClipBoardChanged(m_bricks.IsEmpty());
    }

private:
    BrickArray m_bricks;   // Mac +0x24, Win +0x28
};

} // namespace LEGO
