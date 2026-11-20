// LEGO Digital Designer — restored source
// LEGO::AbstractBrick — the read-only brick interface (design ID and parts); LEGO::Brick — one
// brick of the connectivity scene: its Parts (one per primitive), behaviors and name.
// Brick.cpp = sub-TU 025c of the original Scene.cpp (Mac 0x933b0-0x94100, constructor 0xc2910 in
// 025r, inline virtuals 025s 0xc3220-0xc3600; Win 0x46f000-0x46f440, 0x471470-0x4718d0).
// RTTI: class LEGO::AbstractBrick : virtual Object.
//   Mac vtable 0x70637c {pure GetDesignID, pure GetPartCount, pure GetPart, D1 0xc3220,
//       D0 0xc3240}. Win vfptr {pure ×3}, vbptr, destructor only in the Object part (sdd 0x46f000).
// RTTI: class LEGO::Brick : AbstractBrick, BehaviorElement (Win RTTI attribute 3).
//   Mac sizeof 0x48 (constructor 0xc2910, BrickConnect::CreateBrick 0xc28c0): AbstractBrick
//       vptr +0 (vtable 0x7062d4 {GetDesignID 0xc33c0, GetPartCount 0xc33d0, GetPart 0x93650,
//       D1 0xc33f0, D0 0xc3450, GetType 0x933b0, GetBoundingBox 0x933d0, GetScene 0x934d0}),
//       BehaviorElement +0x04 (vtable 0x706300 {thunks 0x93450, 0x934f0, 0xc34c0, 0xc3520,
//       0x933c0}), own members from +0x18, Object at +0x40 (vtable 0x70632c).
//       Base-object destructor (VTT) 0xcb970.
//   Win sizeof 0x44 (constructor 0x46f110, BrickConnect::CreateBrick 0x490a80): AbstractBrick
//       vfptr +0 / vbptr +4, BehaviorElement vfptr +8 / vbptr +0xc / behaviors +0x10, m_parts
//       +0x20, m_designID +0x30, m_name +0x34, m_hidden +0x38, Object at +0x3c. Destructor
//       0x46f200 (sdd 0x46f410).
// Every member and method name is inferred.
#pragma once
#include "LEGO/Types.h"
#include "Object.h"
#include "BehaviorElement.h"
#include "BasicArray.h"
#include "LEGO/String.h"
#include "Part.h"   // covariant GetPart

namespace LEGO {

struct PrimitiveConnectionInfo;

class AbstractBrick : public virtual Object {
public:
    AbstractBrick() {}
    virtual UInt32 GetDesignID() const = 0;               // slot 0
    virtual UInt32 GetPartCount() const = 0;              // slot 1
    // slot 2. Returns the AbstractPart interface: BrickPalette::Brick (Mac 0x1fa3a0) returns its
    // BrickPalette::Part here; Brick overrides it with a covariant Part*.
    virtual AbstractPart* GetPart(UInt32 index) const = 0;
    virtual ~AbstractBrick() {}                           // slots 3/4: Mac 0xc3220 / 0xc3240, Win sdd 0x46f000

private:
    AbstractBrick(const AbstractBrick&);
    AbstractBrick& operator=(const AbstractBrick&);
};

class Brick : public AbstractBrick, public BehaviorElement {
public:
    // No parts yet (Part::SetBrick adds them). Mac 0xc2910 (025r), Win 0x46f110
    explicit Brick(UInt32 designID);

    // --- AbstractBrick (Mac primary slots 0-4)
    virtual UInt32 GetDesignID() const { return m_designID; }                  // Mac 0xc33c0 (025s)
    virtual UInt32 GetPartCount() const { return m_parts.GetSize(); }          // Mac 0xc33d0 (025s), Win 0x46f100
    virtual Part*  GetPart(UInt32 index) const;                                // Mac 0x93650, Win 0x471590
    // Only the member destructors. Mac 0xc33f0 / 0xc3450 (base 0xcb970), Win 0x46f200 / sdd 0x46f410
    virtual ~Brick();
    // --- BehaviorElement (Mac primary slots 5-7, secondary thunks)
    virtual UInt32 GetType() const { return TypeBrick; }                       // Mac 0x933b0 (thunk 0x933c0)
    // Union of the parts' boxes. Mac 0x933d0 (thunk 0x93450), Win 0x471470
    virtual void   GetBoundingBox(BoundingBox& box, bool recompute) const;
    // The first part's scene (NOTE: no check for a brick without parts). Mac 0x934d0 (thunk
    // 0x934f0), Win 0x471500
    virtual Scene* GetScene() const;

    // ---- non-virtual API ----------------------------------------------------------------------

    // Sets the hidden flag of the parts and their primitive elements (hidden elements block their
    // connection points); no-op when unchanged. Scene m_flag124: the elements' bodies are marked
    // changed (with m_flag120 also those of visible connected elements); m_flag125: the parts'
    // groups. Mac 0x93510, Win 0x471510
    void SetHidden(bool hidden);
    bool IsHidden() const { return m_hidden; }
    // Appends every field pair of two primitive elements of this brick whose CanConnect reports a
    // free or rigid connection (fields placed by the elements' m_transform); each element pair
    // once. Nothing for a brick of one part with one element. Mac 0x93670, Win 0x4715a0
    void GetInternalConnections(BasicArray<PrimitiveConnectionInfo>& connections) const;

    // Part list maintenance of Part::SetBrick: push_back / ordered erase (the part is
    // referenced by the list). Win 0x46f2c0 (Mac inlined into 0x94f60) / Mac 0x95010, Win 0x46f360
    void AddPart(Part* part);
    void RemovePart(Part* part);

    const String& GetName() const { return m_name; }
    void          SetName(const String& name) { m_name = name; }
    const BasicArray<RefPointer<Part> >& GetParts() const { return m_parts; }

    // ---- data (Mac offsets) -------------------------------------------------------------------
    BasicArray<RefPointer<Part> > m_parts;      // +0x18
    UInt32                        m_designID;   // +0x28
    String                        m_name;       // +0x2c (meaning inferred; copied by the Scene's brick clone, Mac 0xa7050)
    bool                          m_hidden;     // +0x3c false
};

} // namespace LEGO
