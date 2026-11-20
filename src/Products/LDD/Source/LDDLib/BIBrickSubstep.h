// LEGO Digital Designer — restored source
// LEGO::BIBrickSubstep — a BIBrick made of several BIBrickPrimitives that the guide shows as
//   one sub-assembly (a building-guide substep). Its elements are those of its primitives, its
//   connections those of the union to the primitives outside it.
//
// Home TU 355 "BIBrickSubstep.cpp" (Mac 0x3355e0-0x33686f). Mac sizeof 0x100: BIBrick +
// +0xdc m_unknownDC (0), +0xe0 m_weight, +0xe4 m_attributes (BasicMap<Attribute, bool>).
// Mac vtable 0x74cf38, Win vtable 0xa2c1e4.
#pragma once
#include "BIBrick.h"

namespace LEGO {

class BIGraph;

class BIBrickSubstep : public BIBrick {
public:
    // Flags set by the analyzers (1 = minifig, BGAnalyzerMinifig). (name inferred)
    typedef Int32 Attribute;
    typedef BasicMap<Attribute, bool> AttributeMap;

    // A substep of `primitives`; `group` (the proposing analyzer) goes to BIBrick::m_group.
    // Computes the elements, the connections (BI::ComputeConnections) and the category.
    // Mac 0x3355e0 (C2) / 0x335820 (C1)
    BIBrickSubstep(const BasicSet<BIBrickPrimitive*>& primitives, const BIAnalyzer* group, Real32 weight);
    virtual ~BIBrickSubstep();                                             // Mac 0x3361b0, Win sdd 0x577d90

    virtual UInt32 GetPrimitiveCount() const;                                            // Mac 0x3362a0, Win 0x577b60
    virtual bool IsEqual(const BIBrick* brick) const;                                    // Mac 0x335890, Win 0x577f40
    virtual bool IsEqual(const BasicSet<BIBrickPrimitive*>& primitives) const;           // Mac 0x335830, Win 0x577ea0
    virtual Real32 GetWeight() const;                                                    // Mac 0x3362b0, Win 0x577b80
    virtual bool Contains(const BIBrick* brick) const;                                   // Mac 0x335aa0, Win 0x5781a0
    virtual bool IsContainedIn(const BasicSet<BIBrickPrimitive*>& primitives) const;     // Mac 0x335ad0, Win 0x5781d0
    virtual bool Intersects(const BIBrick* brick) const;                                 // Mac 0x335b80
    virtual bool Intersects(const BasicArray<BIBrick*>& bricks) const;                   // Mac 0x335c90, Win 0x578330
    virtual bool Intersects(const BasicSet<BIBrick*>& bricks) const;                     // Mac 0x335ce0, Win 0x5783a0
    virtual bool Intersects(const BasicArray<BIBrickPrimitive*>& primitives) const;      // Mac 0x335d30, Win 0x578330
    virtual bool Intersects(const BasicSet<BIBrickPrimitive*>& primitives) const;        // Mac 0x335d80, Win 0x5783a0
    virtual bool Intersects(const BasicArray<BIBrickSubstep*>& substeps) const;          // Mac 0x335dd0, Win 0x578330
    virtual bool Intersects(const BasicArray<RefPointer<BIEvent> >& events) const;       // Mac 0x335e20, Win 0x578410
    virtual bool Split(const BIBrick* brick, BasicSet<BIBrickPrimitive*>& inside,
                       BasicSet<BIBrickPrimitive*>& outside) const;                      // Mac 0x335e90, Win 0x5784c0
    virtual bool Split(const BasicArray<RefPointer<BIEvent> >& events, BasicSet<BIBrickPrimitive*>& inside,
                       BasicSet<BIBrickPrimitive*>& outside) const;                      // Mac 0x335ef0, Win 0x578550
    virtual void GetPrimitives(BasicArray<BIBrickPrimitive*>& primitives) const;         // Mac 0x3362c0, Win 0x577ba0
    virtual void GetPrimitives(BasicSet<BIBrickPrimitive*>& primitives) const;           // Mac 0x336520, Win 0x577c10
    virtual bool IsConnectedTo(const BIBrick* brick) const;                              // Mac 0x335f60, Win 0x578620
    virtual void GetConnectedPrimitives(BasicSet<BIBrickPrimitive*>& primitives) const;  // Mac 0x335fb0, Win 0x5786a0

    // A new substep of this one's primitives without `primitives` when more than one primitive
    // remains, at least one was removed and the rest is connected in `graph`; else null.
    // Weight 0.9 / size, the same group. The result is not referenced. Mac 0x335900,
    // Win 0x577ff0 (name inferred)
    BIBrickSubstep* Subtract(const BasicSet<BIBrickPrimitive*>& primitives, BIGraph* graph) const;
    // The attribute is set and true. Mac 0x336000 (name inferred)
    bool GetAttribute(Attribute attribute) const;
    // Copies every attribute of `attributes`. Mac 0x3360b0 (name inferred)
    void SetAttributes(const AttributeMap& attributes);

    UInt32       m_unknownDC;    // +0xdc 0
    Real32       m_weight;       // +0xe0
    AttributeMap m_attributes;   // +0xe4
};

} // namespace LEGO
