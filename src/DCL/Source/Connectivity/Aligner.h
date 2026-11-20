// LEGO Digital Designer — restored source
// LEGO::Aligner — guides a solver anchor (AnchorPhysicsSolverAnchor) so that a field of the
//   pulled body ("own") lines up with a compatible field of another body ("target"), then joins
//   them with solver constraints. Made by the String / ClickPull solver callback anchors:
//   AxleAligner for axle fields, PlanarAligner for planar (stud) fields.
// Aligner::Field — one field of a primitive element (RTTI: BasicMultimap<float, Aligner::Field,
//   Less<float>>, StringPullSolverCallback::Anchor).
// TU "AxleAligner.cpp" (Mac TU 347 0x32c250-0x32dc4f with the Aligner destructors; Win
// 0x4d52a0-0x4d7b5f). RTTI: Aligner : Object (abstract; Mac vtable 0x777664 {D1 0x32dbc0,
// D0 0x32dbd0, OnFinalRelease, AddRef, Release, 4 pure}, Win 0xa2565c {sdd 0x4d52a0, ...}).
// Class names from RTTI; member and method names inferred.
#pragma once
#include "LEGO/Types.h"
#include "Object.h"

namespace LEGO {

class AnchorPhysicsSolverAnchor;
class PrimitiveElement;
class ConnectionField;

class Aligner : public Object {
public:
    // A connection field (point `m_index` for planar fields) of a primitive element. 12 bytes.
    struct Field {
        PrimitiveElement*      m_element;   // +0x00
        const ConnectionField* m_field;     // +0x04
        UInt32                 m_index;     // +0x08
        Field() : m_element(0), m_field(0), m_index(0) {}
        Field(PrimitiveElement* element, const ConnectionField* field, UInt32 index)
            : m_element(element), m_field(field), m_index(index) {}
        bool operator==(const Field& o) const
        {
            return m_element == o.m_element && m_field == o.m_field && m_index == o.m_index;
        }
    };

    // `anchor` is not referenced. (inlined in the subclass constructors, Mac 0x32c250 / 0x333560)
    Aligner(AnchorPhysicsSolverAnchor* anchor, const Field& field, const Field& target)
        : m_anchor(anchor), m_field(field), m_target(target) {}
    virtual ~Aligner() {}                            // Mac 0x32dbc0 / 0x32dbd0, Win sdd 0x4d52a0

    // ---- Mac slots 5-8 / Win slots 4-7 ----
    // (Re)starts the alignment for the current field pair; ends with Step(0).
    virtual void Start() = 0;
    // Gives up: the constraints go, the aligner moves towards IsDone.
    virtual void Stop() = 0;
    virtual bool IsDone() const = 0;
    // One solver step (`time` unused).
    virtual void Step(Real32 time) = 0;

    AnchorPhysicsSolverAnchor* GetAnchor() const { return m_anchor; }
    const Field& GetField() const { return m_field; }
    const Field& GetTarget() const { return m_target; }
    // The callbacks restart an aligner whose pair is unchanged (all six words equal; Mac
    // 0x32e2a8 / 0x32fbd0).
    bool IsPair(const Field& field, const Field& target) const
    {
        return m_field == field && m_target == target;
    }

protected:
    AnchorPhysicsSolverAnchor* m_anchor;   // +0x08
    Field                      m_field;    // +0x0c the pulled body's field
    Field                      m_target;   // +0x18 the field it aligns to
};

} // namespace LEGO
