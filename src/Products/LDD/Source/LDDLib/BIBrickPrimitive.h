// LEGO Digital Designer — restored source
// LEGO::BIBrickPrimitive — a BIBrick made of the primitive elements of one scene brick (or one
//   flexible part, or one element of a design that is always kept separate); its primitive set
//   holds only itself. Created by BIBrickRepository::Build.
//
// Home TU 354 "BIBrickPrimitive.cpp" (Mac 0x334c20-0x3355df). AddElement (Mac 0x2607f0) is in
// TU 214 (BIBrickRepository.cpp). Mac sizeof 0xf8: BIBrick + m_elementTransforms (+0xdc,
// BasicMap<PrimitiveElement*, Matrix3D>, template code in TU 232). Mac vtable 0x74cec0,
// Win vtable 0xa2c484 (see BIBrick.h for the Win slot order).
#pragma once
#include "BIBrick.h"

namespace LEGO {

class BIBrickPrimitive : public BIBrick {
public:
    // Mac 0x334c20 (the Win constructor is inlined into the repository)
    BIBrickPrimitive();
    virtual ~BIBrickPrimitive();                                           // Mac 0x3351e0, Win sdd 0x58ab20

    virtual UInt32 GetPrimitiveCount() const;                                            // Mac 0x3352d0, Win 0x58a960
    virtual bool IsEqual(const BIBrick* brick) const;                                    // Mac 0x3352e0, Win 0x58a970
    virtual bool IsEqual(const BasicSet<BIBrickPrimitive*>& primitives) const;           // Mac 0x3352f0, Win 0x58a990
    virtual Real32 GetWeight() const;                                                    // Mac 0x335310, Win 0x58a9e0
    virtual bool Contains(const BIBrick* brick) const;                                   // Mac 0x335320, Win 0x58aa50
    virtual bool IsContainedIn(const BasicSet<BIBrickPrimitive*>& primitives) const;     // Mac 0x335330, Win 0x58aa70
    virtual bool Intersects(const BIBrick* brick) const;                                 // Mac 0x334cf0, Win 0x58abb0
    virtual bool Intersects(const BasicArray<BIBrick*>& bricks) const;                   // Mac 0x334d20, Win 0x578330
    virtual bool Intersects(const BasicSet<BIBrick*>& bricks) const;                     // Mac 0x334d70, Win 0x5783a0
    virtual bool Intersects(const BasicArray<BIBrickPrimitive*>& primitives) const;      // Mac 0x334dc0, Win 0x578330
    virtual bool Intersects(const BasicSet<BIBrickPrimitive*>& primitives) const;        // Mac 0x334e10, Win 0x5783a0
    virtual bool Intersects(const BasicArray<BIBrickSubstep*>& substeps) const;          // Mac 0x334e60, Win 0x578330
    virtual bool Intersects(const BasicArray<RefPointer<BIEvent> >& events) const;       // Mac 0x334eb0, Win 0x578410
    virtual bool Split(const BIBrick* brick, BasicSet<BIBrickPrimitive*>& inside,
                       BasicSet<BIBrickPrimitive*>& outside) const;                      // Mac 0x334f20, Win 0x58abf0
    virtual bool Split(const BasicArray<RefPointer<BIEvent> >& events, BasicSet<BIBrickPrimitive*>& inside,
                       BasicSet<BIBrickPrimitive*>& outside) const;                      // Mac 0x334f80, Win 0x58ac60
    virtual void GetPrimitives(BasicArray<BIBrickPrimitive*>& primitives) const;         // Mac 0x335340, Win 0x58aa20
    virtual void GetPrimitives(BasicSet<BIBrickPrimitive*>& primitives) const;           // Mac 0x335570, Win 0x58a9f0
    virtual bool IsConnectedTo(const BIBrick* brick) const;                              // Mac 0x334fe0, Win 0x58acd0
    virtual void GetConnectedPrimitives(BasicSet<BIBrickPrimitive*>& primitives) const;  // Mac 0x335030, Win 0x58ad50

    // Adds this primitive and, recursively, the connected primitives that are in `within`
    // to `result` (connected component inside `within`). Mac 0x335090 (name inferred)
    void CollectConnected(BasicSet<BIBrickPrimitive*>& result, const BasicSet<BIBrickPrimitive*>& within);
    // Appends an element with its world transform. Mac 0x2607f0 (TU 214; name inferred)
    void AddElement(PrimitiveElement* element, const Matrix3D& transform);

    BasicMap<PrimitiveElement*, Matrix3D> m_elementTransforms;   // +0xdc
};

} // namespace LEGO
