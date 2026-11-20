// LEGO Digital Designer — restored source
// LEGO::BrickGroup — a named group of parts and sub-groups of the connectivity scene (the
// user's "group" command; the Scene lists the top-level groups).
// BrickGroup.cpp = sub-TU 025e of the original Scene.cpp (Mac 0x95c80-0x97310, constructor
// 0xc2ba0 in 025r, inline virtuals 0xc3a20-0xc3be0 in 025s, AddPart 0x94eb0 / RemovePart
// 0x94da0 in the 025d range; Win 0x46f640-0x46f850, 0x4722a0-0x472ad0). The Scene functions of
// the 025e range (0x96240, 0x963a0, 0x96a40, 0x96b00, 0x97230) are in Scene.cpp.
// RTTI: class LEGO::BrickGroup : BehaviorElement.
//   Mac sizeof 0xa0 (constructor 0xc2ba0, BrickConnect::CreateBrickGroup 0xc2b50): vptr +0
//       (vtable 0x7065bc {GetBoundingBox 0x95c90, GetScene 0x95d60, D1 0xc3a20, D0 0xc3a80,
//       GetType 0x95c80}), m_behaviors +0x04, own members from +0x14, Object at +0x98.
//       Base-object destructor (VTT) 0xcbcf0.
//   Win sizeof 0x98 (constructor 0x46f640, BrickConnect::CreateBrickGroup 0x490be0): vfptr +0,
//       vbptr +4, behaviors +8, m_scene +0x18, m_parent +0x1c, m_name +0x20, m_parts +0x24,
//       m_groups +0x34, m_transform +0x44, m_vector84 +0x84, Object at +0x90 (sdd 0x46f820).
// Every member and method name is inferred.
#pragma once
#include "LEGO/Types.h"
#include "BehaviorElement.h"
#include "BasicArray.h"
#include "Geometry.h"
#include "LEGO/String.h"

namespace LEGO {

class Part;

class BrickGroup : public BehaviorElement {
public:
    // Empty top-level group of `scene` (not yet listed by the scene). Mac 0xc2ba0 (025r), Win 0x46f640
    explicit BrickGroup(Scene* scene);

    // Union of the parts' and sub-groups' boxes. Mac 0x95c90, Win 0x4722a0
    virtual void   GetBoundingBox(BoundingBox& box, bool recompute) const;
    virtual Scene* GetScene() const { return m_scene; }        // Mac 0x95d60
    // Only the member destructors. Mac 0xc3a20 / 0xc3a80 (base 0xcbcf0), Win sdd 0x46f820
    virtual ~BrickGroup();
    virtual UInt32 GetType() const { return TypeBrickGroup; }  // Mac 0x95c80

    // ---- non-virtual API ----------------------------------------------------------------------

    // Moves the group below `parent` (NULL: top level). Old and new parent are recorded as
    // changed; joining / leaving Scene::m_groups records the group with Scene::AddMovedGroup.
    // Ends with Scene::NotifyGroupChanges. Mac 0x95d70, Win 0x472380
    void SetParent(BrickGroup* parent);
    BrickGroup* GetParent() const { return m_parent; }
    // `part` is in this group (`recursive`: or in a sub-group). Mac 0x96a90, Win 0x4728e0
    bool Contains(const Part* part, bool recursive) const;
    // Appends the parts of this group and of all sub-groups (depth first). Mac 0x96dc0, Win 0x4728a0
    void GetAllParts(BasicArray<RefPointer<Part> >& parts) const;
    // Moves those of `parts` that are in this group (or a sub-group) to the parent group;
    // `moved` receives the moved parts, `oldGroups` their previous groups (both optional).
    // Inside Scene::BeginUpdate / EndUpdate. Mac 0x96790, Win 0x472580
    void RemoveParts(const BasicArray<RefPointer<Part> >& parts, BasicArray<RefPointer<Part> >* moved,
                     BasicArray<RefPointer<BrickGroup> >* oldGroups);
    // Moves those of `parts` that are not in this group (nor a sub-group) here; outputs as
    // RemoveParts. Mac 0x96b20, Win 0x472710
    void AddParts(const BasicArray<RefPointer<Part> >& parts, BasicArray<RefPointer<Part> >* moved,
                  BasicArray<RefPointer<BrickGroup> >* oldGroups);

    // Part list maintenance of Part::SetGroup (the group references its parts); both record
    // the group as changed in the Scene and run Scene::NotifyGroupChanges.
    // Mac 0x94eb0 / 0x94da0 (025d), Win 0x472940 / 0x4729f0. RemovePart: unordered erase.
    void AddPart(Part* part);
    void RemovePart(Part* part);

    const String& GetName() const { return m_name; }
    void          SetName(const String& name) { m_name = name; }
    const BasicArray<RefPointer<Part> >&       GetParts() const { return m_parts; }
    const BasicArray<RefPointer<BrickGroup> >& GetGroups() const { return m_groups; }

    // ---- data (Mac offsets) -------------------------------------------------------------------
    Scene*                              m_scene;       // +0x14 the constructor argument
    BrickGroup*                         m_parent;      // +0x18 NULL: top level (not referenced)
    String                              m_name;        // +0x1c
    BasicArray<RefPointer<Part> >       m_parts;       // +0x2c
    BasicArray<RefPointer<BrickGroup> > m_groups;      // +0x3c sub-groups
    Matrix3D                            m_transform;   // +0x4c identity (meaning unknown; not used in TU 025)
    Vector3                             m_vector8c;    // +0x8c zero (meaning unknown; not used in TU 025)
};

} // namespace LEGO
