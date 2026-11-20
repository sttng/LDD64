// LEGO Digital Designer — restored source
// LEGO::BIBrick — the unit the building-guide generators place: either a BIBrickPrimitive (the
//   primitive elements of one brick, or of one flexible part, of the scene) or a
//   BIBrickSubstep (a set of primitives shown as one sub-assembly). The BIBrickRepository owns
//   them. Every BIBrick knows its primitive elements, its primitives (itself for a primitive)
//   and the Connections to the primitives it is connected to (directions in which the
//   connections allow it to be inserted, with weights).
//
// Home TU 213 "BIBrick.cpp" (Mac 0x25e350-0x25f869). BIBrick::Connection: destructor in TU 148
// (BI.cpp, Mac 0x20dd30), methods in TU 213.
//
// Mac layout (sizeof 0xdc; BIBrickPrimitive 0xf8, BIBrickSubstep 0x100):
//   +0x00 vptr, +0x04 refcount (Object)
//   +0x08 m_type                 Type (1 primitive, 2 substep)
//   +0x0c m_transforms           BasicArray<Pair<PrimitiveElement*, Matrix3D> > (not used)
//   +0x1c m_group                the BIAnalyzer that proposed a substep
//   +0x20 m_elements             BasicArray<PrimitiveElement*>
//   +0x30 m_primitives           BasicSet<BIBrickPrimitive*>
//   +0x4c m_boundingBoxValid, +0x50 m_boundingBox
//   +0x68 m_volumeValid, +0x6c m_volume
//   +0x70 m_centerValid, +0x74 m_center (sum, see ComputeCenter)
//   +0x80 m_dfsNumber, +0x84 m_dfsLow, +0x88 m_dfsParent: state of the articulation-point
//         search of BIGraph (Win +0x88 / +0x8c / +0x90; not touched by the constructor)
//   +0x8c m_dfsChildren          BasicSet<BIBrick*> (the same search)
//   +0xa8 m_id                   running number (static counter, Mac 0x77aee8)
//   +0xac m_category             2 = minifigure part (UpdateCategory)
//   +0xb0 m_connections          BasicMap<BIBrickPrimitive*, RefPointer<Connection> >
//   +0xcc m_direction            cached GetDirection result (cleared by the constructor)
//   +0xd8 m_enabled              true
// Mac vtable 0x7715a8 {D1 0x25f580, D0 0x25f590, Object slots, 19 pure}; Win vtable 0xa2c14c
// {sdd 0x56e1c0, …}. MSVC reverses the overload groups (IsEqual, Intersects, Split,
// GetPrimitives): Win slot 5 = IsEqual(set), slot 6 = IsEqual(brick), slots 10-16 = Intersects
// in reverse order (Win 0x578410 = the BIEvent array overload, shared by both subclasses
// through identical-code folding, 0x578330 the array and 0x5783a0 the set overloads).
#pragma once
#include "LEGO/Types.h"
#include "Object.h"
#include "BasicArray.h"
#include "BasicMap.h"
#include "Geometry.h"

namespace LEGO {

class PrimitiveElement;
class PrimitiveConnection;
class BIBrickPrimitive;
class BIBrickSubstep;
class BIEvent;
class BGScene;
class BIAnalyzer;

class BIBrick : public Object {
public:
    enum Type {
        TypePrimitive = 1,
        TypeSubstep   = 2
    };
    // GetPoint (names inferred)
    enum Point {
        PointMinimum       = 0,
        PointMaximum       = 1,
        PointCenter        = 2,
        PointCenterOfMass  = 3
    };
    // m_category values (name inferred)
    enum Category {
        CategoryMinifig = 2
    };

    // Insertion directions of this brick relative to one connected primitive (unit vectors
    // in world space) with a weight each, plus the strength of the connection.
    // sizeof 0x2c. Mac vtable 0x76fa88 {D1 0x20dd30, D0 0x20dd90}, Win vtable 0xa2b8fc
    // {sdd 0x553000}. Constructor inlined (BI::ComputeConnections).
    class Connection : public Object {
    public:
        Connection() : m_strength(0.0f) {}
        virtual ~Connection();                                             // Mac 0x20dd30 (TU 148)

        // Appends a direction. Mac 0x25f140 (name inferred)
        void Add(const Vector3& direction, Real32 weight);
        // True if one of the directions equals `direction` within 0.1 per component; `weight`
        // is raised to the largest weight of those. Mac 0x25f090 (name inferred)
        bool FindDirection(const Vector3& direction, Real32& weight) const;

        BasicArray<Vector3> m_directions;   // +0x08
        BasicArray<Real32>  m_weights;      // +0x18
        Real32              m_strength;     // +0x28 BI::GetConnectionStrength (name inferred)
    };
    typedef BasicMap<BIBrickPrimitive*, RefPointer<Connection> > ConnectionMap;

    // Mac 0x25e350 (inlined into the subclass constructors on Win)
    explicit BIBrick(UInt32 type);
    virtual ~BIBrick();                                                    // Mac 0x25f580 (body 0x25f680), Win 0x56e110

    // ---- slots 4-22 (Win numbering, Mac index + 1), all pure in BIBrick ------------------------
    virtual UInt32 GetPrimitiveCount() const = 0;                                        // 4
    // Same brick: the same primitive, resp. a substep with the same primitives.
    virtual bool IsEqual(const BIBrick* brick) const = 0;                                // 5
    virtual bool IsEqual(const BasicSet<BIBrickPrimitive*>& primitives) const = 0;       // 6
    // Weight of the brick (primitive 1, substep its constructor argument). (name inferred)
    virtual Real32 GetWeight() const = 0;                                                // 7
    // Every primitive of `brick` is one of this brick's. (name inferred)
    virtual bool Contains(const BIBrick* brick) const = 0;                               // 8
    // Every primitive of this brick is in `primitives`. (name inferred)
    virtual bool IsContainedIn(const BasicSet<BIBrickPrimitive*>& primitives) const = 0; // 9
    // At least one common primitive (with the brick / with one brick of the container).
    // (parameter types of the BasicArray overloads inferred)
    virtual bool Intersects(const BIBrick* brick) const = 0;                             // 10
    virtual bool Intersects(const BasicArray<BIBrick*>& bricks) const = 0;               // 11
    virtual bool Intersects(const BasicSet<BIBrick*>& bricks) const = 0;                 // 12
    virtual bool Intersects(const BasicArray<BIBrickPrimitive*>& primitives) const = 0;  // 13
    virtual bool Intersects(const BasicSet<BIBrickPrimitive*>& primitives) const = 0;    // 14
    virtual bool Intersects(const BasicArray<BIBrickSubstep*>& substeps) const = 0;      // 15
    virtual bool Intersects(const BasicArray<RefPointer<BIEvent> >& events) const = 0;   // 16
    // Sorts this brick's primitives into `inside` (they intersect `brick` / a brick of the
    // events) and `outside`; returns !inside.IsEmpty(). (names inferred)
    virtual bool Split(const BIBrick* brick, BasicSet<BIBrickPrimitive*>& inside,
                       BasicSet<BIBrickPrimitive*>& outside) const = 0;                  // 17
    virtual bool Split(const BasicArray<RefPointer<BIEvent> >& events, BasicSet<BIBrickPrimitive*>& inside,
                       BasicSet<BIBrickPrimitive*>& outside) const = 0;                  // 18
    virtual void GetPrimitives(BasicArray<BIBrickPrimitive*>& primitives) const = 0;     // 19
    virtual void GetPrimitives(BasicSet<BIBrickPrimitive*>& primitives) const = 0;       // 20
    // One of this brick's primitives is connected to a primitive of `brick`. (name inferred)
    virtual bool IsConnectedTo(const BIBrick* brick) const = 0;                          // 21
    // Adds the primitives this brick is connected to. (name inferred)
    virtual void GetConnectedPrimitives(BasicSet<BIBrickPrimitive*>& primitives) const = 0; // 22

    // ---- non-virtual (TU 213) -------------------------------------------------------------------
    // Mac 0x25e5d0 / 0x25e5f0 (names inferred)
    void SetEnabled(bool enabled);
    bool IsEnabled() const;
    // Direction to insert the brick into / remove it from `scene` (cached; length 10). Mac
    // 0x25e600 (name inferred)
    Vector3 GetDirection(BGScene* scene, bool flag);
    // Mac 0x25e7b0 / 0x25e7d0, Win 0x56e500 (names inferred)
    void ClearDirection();
    void SetDirection(const Vector3& direction);
    // World bounding box of the elements (cached). Mac 0x25e7f0
    BoundingBox GetBoundingBox();
    // Sum of the squared bone bounding-box diagonals of the elements (cached). Mac 0x25e8d0
    // (name inferred)
    Real32 GetVolume();
    // Center of the elements' bone boxes weighted by their squared diagonals, divided by
    // `volume`. Mac 0x25e960 (name inferred)
    Vector3 ComputeCenter(Real32 volume);
    // ComputeCenter(GetVolume()). Mac 0x25ead0 (name inferred)
    Vector3 GetCenterOfMass();
    // Mac 0x25eb70 (name inferred)
    Vector3 GetPoint(UInt32 point);
    // Mac 0x25ecb0 (names inferred)
    bool ContainsElement(const PrimitiveElement* element) const;
    // One element's primitive has the design ID. Mac 0x25ecf0, Win 0x56e9d0
    bool ContainsDesign(UInt32 designID) const;
    // An element's primitive (or its brick's assembly) has the "maingroupid" annotation `id`.
    // Mac 0x25ed30, Win 0x56ea40
    bool HasMainGroup(UInt32 id) const;
    // An element's primitive has a "maingroupid" annotation in `ids`. Mac 0x25ef10
    bool HasMainGroup(const BasicSet<UInt32>& ids) const;
    // m_category = CategoryMinifig when the "maingroupid" of an element is one of the minifig
    // groups 265, 266, 268, 366, 368. Mac 0x25f3a0 (name inferred)
    void UpdateCategory();

    UInt32 GetType() const { return m_type; }
    UInt32 GetID() const { return m_id; }
    const BIAnalyzer* GetGroup() const { return m_group; }
    UInt32 GetCategory() const { return m_category; }
    const BasicArray<PrimitiveElement*>& GetElements() const { return m_elements; }
    const BasicSet<BIBrickPrimitive*>& GetPrimitiveSet() const { return m_primitives; }
    const ConnectionMap& GetConnections() const { return m_connections; }
    ConnectionMap& GetConnections() { return m_connections; }

    // ---- data (Mac offsets) -------------------------------------------------------------------
    UInt32                                   m_type;               // +0x08
    BasicArray<Pair<PrimitiveElement*, Matrix3D> > m_transforms;   // +0x0c (not used here)
    const BIAnalyzer*                        m_group;              // +0x1c (name inferred)
    BasicArray<PrimitiveElement*>            m_elements;           // +0x20
    BasicSet<BIBrickPrimitive*>              m_primitives;         // +0x30
    bool                                     m_boundingBoxValid;   // +0x4c
    BoundingBox                              m_boundingBox;        // +0x50
    bool                                     m_volumeValid;        // +0x68
    Real32                                   m_volume;             // +0x6c
    bool                                     m_centerValid;        // +0x70
    Vector3                                  m_center;             // +0x74
    // State of BIGraph::ComputeArticulationPoints (names inferred): the DFS
    // number (0xffffffff: not visited), the lowest-numbered brick reachable through the DFS
    // subtree and one back edge, the DFS parent and the DFS children.
    UInt32                                   m_dfsNumber;          // +0x80
    BIBrick*                                 m_dfsLow;             // +0x84
    BIBrick*                                 m_dfsParent;          // +0x88
    BasicSet<BIBrick*>                       m_dfsChildren;        // +0x8c
    UInt32                                   m_id;                 // +0xa8
    UInt32                                   m_category;           // +0xac
    ConnectionMap                            m_connections;        // +0xb0
    Vector3                                  m_direction;          // +0xcc
    bool                                     m_enabled;            // +0xd8

private:
    static UInt32 s_nextID;   // Mac 0x77aee8
};

} // namespace LEGO
