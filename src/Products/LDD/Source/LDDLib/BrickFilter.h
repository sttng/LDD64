// LEGO Digital Designer — restored source
// LEGO::BrickFilter — decides which palette bricks the brick palette shows; LEGO::
// CompositeBrickFilter (all of its filters, at most one per type), LEGO::MaterialBrickFilter
// (bricks with a part of a colour), LEGO::BoxBrickFilter (the bricks of one bag, with counts).
// DescriptionBrickFilter (text search) is in DescriptionBrickFilter.h.
//
// Mac TU 073 "DescriptionBrickFilter.cpp" (0x1641b0-0x16610b); Win 0x627524-0x627cd1,
// 0x65f24f-0x660269 (constructors outlined on Win only).
//
// RTTI: class BrickFilter : virtual Object. Mac vtable 0x71135c {pure Accept, IsCounting
//   0x165160, GetCount 0x165170, D1 0x165180, D0 0x1651a0}, layout vptr, m_type +4, Object +8.
//   Win vfptr {purecall, 0x67e76a, 0x627581}, vbptr +4, m_type +8, Object part vftable
//   0x9d0364 {sdd 0x627c93}. Constructor Win 0x627524 (thiscall type, mostDerived).
// The type numbers come from the constructors (Win 0x62994f composite 0, 0x6275b9 material 2,
// 0x627679 box 4, 0x660077 description 5); 1 and 3 are not used by any filter class.
// Method and member names inferred.
#pragma once
#include "LEGO/Types.h"
#include "Object.h"
#include "BasicArray.h"
#include "BrickPalette.h"

namespace LEGO {

class BrickFilter : public virtual Object {
public:
    enum Type {                      // (names inferred)
        kTypeComposite   = 0,
        kTypeMaterial    = 2,
        kTypeBox         = 4,
        kTypeDescription = 5
    };

    explicit BrickFilter(UInt32 type) : m_type(type) {}                        // Win 0x627524

    // slot 0: the brick passes.
    virtual bool   Accept(const BrickPalette::Brick* brick) const = 0;
    // slot 1: the filter has counts (a countable bag). Mac 0x165160, Win 0x67e76a (ICF)
    virtual bool   IsCounting() const { return false; }
    // slot 2: the brick's count. Mac 0x165170, Win 0x627581 (ICF)
    virtual UInt32 GetCount(const BrickPalette::Brick* /*brick*/) const { return 0; }
    virtual ~BrickFilter() {}                                                  // Mac 0x165180 / 0x1651a0, Win sdd 0x627c93

    UInt32 GetType() const { return m_type; }

protected:
    UInt32 m_type;   // Mac +0x04 / Win +0x08
};

// Mac sizeof 0x20 (Object +0x18), vtable 0x7112f4 {0x164650, 0x1646a0, 0x1646f0, D1 0x165250,
// D0 0x165300}; Win constructor 0x62994f, vftable {0x65f461, 0x65f48b, 0x65f4b0}, sdd 0x62ac51.
class CompositeBrickFilter : public BrickFilter {
public:
    CompositeBrickFilter() : BrickFilter(kTypeComposite) {}                    // Win 0x62994f

    // Every filter accepts. Mac 0x164650, Win 0x65f461
    virtual bool   Accept(const BrickPalette::Brick* brick) const;
    // Any filter counts. Mac 0x1646a0, Win 0x65f48b
    virtual bool   IsCounting() const;
    // Sum of the counting filters' counts. Mac 0x1646f0, Win 0x65f4b0
    virtual UInt32 GetCount(const BrickPalette::Brick* brick) const;
    virtual ~CompositeBrickFilter() {}                                         // Mac 0x165250 / 0x165300

    // Replaces the filters of the same type by `filter`. Mac 0x1641b0, Win 0x65f9f4
    void SetFilter(const BrickFilter* filter);
    // Removes every filter with the type of `filter`. Mac 0x164250
    void RemoveFiltersOfType(const BrickFilter* filter);
    // Removes `filter` (its slot gets the last filter). Mac 0x164500
    void RemoveFilter(const BrickFilter* filter);
    // Mac 0x164600
    void Clear();

    const BasicArray<RefPointer<const BrickFilter> >& GetFilters() const { return m_filters; }

private:
    // std::find + `*it = back; pop_back` (Mac 0x165960); nothing for `end`.
    void EraseUnordered(RefPointer<const BrickFilter>* position);

    BasicArray<RefPointer<const BrickFilter> > m_filters;   // Mac +0x08 / Win +0x0c
};

// Mac vtable 0x7113b8 {0x164750, …}, Object +0xc; Win constructor 0x6275b9, vftable {0x65f4e4, …}.
class MaterialBrickFilter : public BrickFilter {
public:
    explicit MaterialBrickFilter(UInt32 material) : BrickFilter(kTypeMaterial), m_material(material) {}  // Win 0x6275b9

    // Some part lists the material (raw part arrays, slot 0 included). Mac 0x164750, Win 0x65f4e4
    virtual bool Accept(const BrickPalette::Brick* brick) const;
    virtual ~MaterialBrickFilter() {}                                          // Mac 0x1653a0 / 0x1653c0, Win sdd 0x627cb2

    UInt32 GetMaterial() const { return m_material; }

private:
    UInt32 m_material;   // Mac +0x08 / Win +0x0c
};

// Mac vtable 0x711478 {0x1647a0, 0x165470, 0x165490, D1 0x1654b0, D0 0x1654d0}, Object +0xc;
// Win constructor 0x627679, vftable {0x65f24f, 0x6276e3, 0x6276eb}, sdd 0x627cd1.
// NOTE: the bag is not referenced (both builds).
class BoxBrickFilter : public BrickFilter {
public:
    explicit BoxBrickFilter(const BrickPalette::Bag* bag) : BrickFilter(kTypeBox), m_bag(bag) {}  // Win 0x627679

    virtual bool   Accept(const BrickPalette::Brick* brick) const { return m_bag->Contains(brick); }    // Mac 0x1647a0, Win 0x65f24f
    virtual bool   IsCounting() const { return m_bag->IsCountable(); }                                  // Mac 0x165470, Win 0x6276e3
    virtual UInt32 GetCount(const BrickPalette::Brick* brick) const { return m_bag->GetCount(brick); }  // Mac 0x165490, Win 0x6276eb
    virtual ~BoxBrickFilter() {}                                               // Mac 0x1654b0 / 0x1654d0, Win sdd 0x627cd1

    const BrickPalette::Bag* GetBag() const { return m_bag; }

private:
    const BrickPalette::Bag* m_bag;   // Mac +0x08 / Win +0x0c
};

} // namespace LEGO
