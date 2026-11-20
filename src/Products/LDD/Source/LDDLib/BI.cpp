// LEGO Digital Designer — restored source
// TU 148 "BI.cpp" (Mac 0x20af60-0x20f5a9): BI (the building guide of a scene control), the
// BIBrick::Connection destructor and the helpers shared by the BI classes. See BI.h.
#include "BI.h"
#include "BIEvent.h"
#include "BIBrick.h"
#include "BIBrickPrimitive.h"
#include "BIBrickRepository.h"
#include "Scene.h"
#include "Brick.h"
#include "PrimitiveElement.h"
#include "PrimitiveConnection.h"
#include "ConnectionField.h"
#include "ConnectionFieldAxle.h"
#include "ConnectivityWorldLookupTables.h"
#include "Primitive.h"
#include <math.h>

namespace LEGO {

// ---- BI ---------------------------------------------------------------------------------------

// Mac 0x20af90
BI::BI()
{
    m_storage = new BIStorage;
}

// Mac 0x20d9d0 / D0 0x20da30 (thunks 0x20daa0 / 0x20db00), Win sdd 0x5533d0
BI::~BI()
{
}

// Mac 0x20b0d0
void BI::Reset()
{
    m_storage->Clear();
    Iterator it(*this);
    while (BuildingGuideObserver* observer = it.Next(m_observers))
        observer->OnBuildingGuideChanged(false);
}

// Mac 0x20b380
void BI::SetStorage(const RefPointer<BIStorage>& storage)
{
    m_storage = storage;
    Iterator it(*this);
    while (BuildingGuideObserver* observer = it.Next(m_observers))
        observer->OnBuildingGuideChanged(storage->HasSteps());
}

// Mac 0x20b620
RefPointer<BIStorage> BI::GetStorage() const
{
    return m_storage;
}

// Mac 0x20b650
void BI::SetName(const String& name)
{
    m_storage->SetName(name);
}

// Mac 0x20b670
const String& BI::GetName() const
{
    return m_storage->GetName();
}

// Mac 0x20b350, Win 0x6137c1
bool BI::HasSteps() const
{
    return m_storage->HasSteps();
}

// ---- BIBrick::Connection (destructor emitted here) --------------------------------------------

// Mac 0x20dd30 / D0 0x20dd90, Win sdd 0x553000
BIBrick::Connection::~Connection()
{
}

// ---- helpers ----------------------------------------------------------------------------------

// Mac 0x77aeac (constructed by the TU initialiser 0x20f510)
static BasicMap<PrimitiveElement*, BasicSet<PrimitiveElement*> > s_elementGraph;

// Mac 0x20b690
void BI::UpdateElementGraph(Scene* scene)
{
    s_elementGraph.Clear();
    for (BasicArray<RefPointer<PrimitiveElement> >::Iterator it = scene->m_primitiveElements.Begin();
         it != scene->m_primitiveElements.End(); ++it) {
        PrimitiveElement* element = *it;
        s_elementGraph[element] = BasicSet<PrimitiveElement*>();
        const PrimitiveElement::ConnectionMap& connections = element->GetConnections();
        for (PrimitiveElement::ConnectionMap::ConstIterator c = connections.Begin(); c != connections.End(); ++c) {
            PrimitiveElement* other = c->second->GetOther(element);
            s_elementGraph[element].Insert(other);
        }
    }
}

const BasicMap<PrimitiveElement*, BasicSet<PrimitiveElement*> >& BI::GetElementGraph()
{
    return s_elementGraph;
}

// Mac 0x20b8b0
void BI::GetBricks(const BasicSet<BIBrickPrimitive*>& primitives, BasicSet<BIBrick*>& bricks)
{
    for (BasicSet<BIBrickPrimitive*>::ConstIterator it = primitives.Begin(); it != primitives.End(); ++it)
        bricks.Insert(*it);
}

// Mac 0x20b900
void BI::GetPrimitives(const BasicSet<BIBrick*>& bricks, BasicSet<BIBrickPrimitive*>& primitives)
{
    for (BasicSet<BIBrick*>::ConstIterator it = bricks.Begin(); it != bricks.End(); ++it)
        (*it)->GetPrimitives(primitives);
}

// Mac 0x20ba70
void BI::GetBricks(const BasicArray<RefPointer<BIEvent> >& events, BasicArray<BIBrick*>& bricks)
{
    for (BasicArray<RefPointer<BIEvent> >::ConstIterator it = events.Begin(); it != events.End(); ++it)
        if (BIBrick* brick = (*it)->GetBrick())
            bricks.PushBack(brick);
}

// Mac 0x20bce0
void BI::GetBricks(const BasicArray<RefPointer<BIEvent> >& events, BasicSet<BIBrick*>& bricks)
{
    for (BasicArray<RefPointer<BIEvent> >::ConstIterator it = events.Begin(); it != events.End(); ++it)
        if (BIBrick* brick = (*it)->GetBrick())
            bricks.Insert(brick);
}

// Mac 0x20bd30
void BI::GetPrimitives(const BasicArray<RefPointer<BIEvent> >& events, BasicSet<BIBrickPrimitive*>& primitives)
{
    for (BasicArray<RefPointer<BIEvent> >::ConstIterator it = events.Begin(); it != events.End(); ++it)
        if (BIBrick* brick = (*it)->GetBrick())
            brick->GetPrimitives(primitives);
}

// Mac 0x20bd80
void BI::GetSteps(const BasicArray<RefPointer<BIEvent> >& events, BasicArray<Step>& steps)
{
    bool newStep = true;
    StorageScene2::Camera* camera = 0;
    for (UInt32 i = 0; i < events.GetSize(); ++i) {
        BIEvent* event = events[i];
        if (event->m_type == BIEvent::TypeStep) {
            camera = static_cast<BIEventStep*>(event)->m_camera;
            newStep = true;
            continue;
        }
        if (event->m_type == BIEvent::TypeBrick && event->GetBrick()->GetType() == BIBrick::TypeSubstep)
            newStep = true;
        if (newStep) {
            steps.PushBack(Step());
            steps.Back().m_camera = camera;
            newStep = false;
        }
        steps.Back().m_events.PushBack(RefPointer<BIEvent>(event));
    }
}

// Mac 0x20c190
bool BI::IsWire(const Brick* brick)
{
    return ConnectivityWorldLookupTables::GetWires(brick->GetDesignID()) != 0;
}

// Direction `axis` of `field` (field space) in world space (element transform `transform`).
static Vector3 FieldDirection(Real32 x, Real32 y, Real32 z, const ConnectionField* field, const Matrix3D& transform)
{
    Vector3 axis(x, y, z);
    Vector3 local = Rotate(axis, field->GetTransform());
    return Rotate(local, transform);
}

// Both directions of `direction` with the same weight.
static void AddBoth(BIBrick::Connection* connection, const Vector3& direction, Real32 weight)
{
    connection->Add(Vector3(direction), weight);
    connection->Add(-direction, weight);
}

// Mac 0x20c1c0
void BI::ComputeConnections(BIBrick* brick, BIBrick::ConnectionMap& connections, const BasicSet<BIBrickPrimitive*>& filter)
{
    BIBrickRepository& repository = BIBrickRepository::GetInstance();
    for (BasicArray<PrimitiveElement*>::ConstIterator e = brick->m_elements.Begin(); e != brick->m_elements.End(); ++e) {
        PrimitiveElement* element = *e;
        const PrimitiveElement::ConnectionMap& elementConnections = element->GetConnections();
        for (PrimitiveElement::ConnectionMap::ConstIterator c = elementConnections.Begin(); c != elementConnections.End(); ++c) {
            PrimitiveConnection* primitiveConnection = c->second;
            PrimitiveElement* other = primitiveConnection->GetOther(element);
            if (brick->ContainsElement(other))
                continue;
            if (filter.GetSize() != 0 && !filter.Contains(repository.GetPrimitive(other, false)))
                continue;
            const PrimitiveConnection::Connection& first = primitiveConnection->GetConnections()[0];
            const ConnectionField* field = primitiveConnection->GetElement() == element ? first.m_field : first.m_otherField;
            const ConnectionField* otherField = primitiveConnection->GetElement() == other ? first.m_field : first.m_otherField;
            Matrix3D transform;
            element->GetTransform(transform);
            Matrix3D otherTransform;
            other->GetTransform(otherTransform);
            RefPointer<BIBrick::Connection>& slot = connections[repository.GetPrimitive(other, false)];
            BIBrick::Connection* connection = slot;
            if (connection == 0) {
                connection = new BIBrick::Connection;
                connections[repository.GetPrimitive(other, false)] = connection;
                connection = connections[repository.GetPrimitive(other, false)];
            }
            switch (field->GetType()) {
            case ConnectionField::TYPE_PLANAR: {
                Vector3 up = FieldDirection(0.0f, 1.0f, 0.0f, field, transform);
                if (field->GetKind() & 1)
                    connection->Add(-up, 0.0f);
                else
                    connection->Add(Vector3(up), 0.0f);
                break;
            }
            case ConnectionField::TYPE_HINGE: {
                // The frame of the field whose kind bit 0 is clear (this element's unless set).
                const ConnectionField* frameField = (field->GetKind() & 1) ? otherField : field;
                const Matrix3D& frame = (field->GetKind() & 1) ? otherTransform : transform;
                Vector3 x = FieldDirection(1.0f, 0.0f, 0.0f, frameField, frame);
                Vector3 y = FieldDirection(0.0f, 1.0f, 0.0f, frameField, frame);
                Vector3 z = FieldDirection(0.0f, 0.0f, 1.0f, frameField, frame);
                AddBoth(connection, x, 0.0f);
                AddBoth(connection, y, 0.0f);
                AddBoth(connection, z, 0.0f);
                break;
            }
            case ConnectionField::TYPE_AXLE: {
                const ConnectionFieldAxle* axle = static_cast<const ConnectionFieldAxle*>(field);
                const ConnectionFieldAxle* otherAxle = static_cast<const ConnectionFieldAxle*>(otherField);
                if (axle->IsGrabbing()) {
                    AddBoth(connection, FieldDirection(1.0f, 0.0f, 0.0f, field, transform), 0.2f);
                    AddBoth(connection, FieldDirection(0.0f, 1.0f, 0.0f, field, transform), 0.2f);
                } else if (otherAxle->IsGrabbing()) {
                    AddBoth(connection, FieldDirection(-1.0f, 0.0f, 0.0f, otherField, otherTransform), 0.2f);
                    AddBoth(connection, FieldDirection(0.0f, 1.0f, 0.0f, otherField, otherTransform), 0.2f);
                } else {
                    // Free axles: along the axis, where neither end is capped.
                    Real32 length = axle->GetLength();
                    if (length <= otherAxle->GetLength())
                        length = otherAxle->GetLength();
                    Vector3 axis = FieldDirection(0.0f, 1.0f, 0.0f, field, transform);
                    Vector3 otherAxis = FieldDirection(0.0f, 1.0f, 0.0f, otherField, otherTransform);
                    Real32 dot = axis.Dot(otherAxis);
                    bool forward, backward;
                    if (fabsf(dot + -1.0f) < 0.001f) {
                        forward = !axle->IsStartCapped() && !otherAxle->IsEndCapped();
                        backward = !axle->IsEndCapped() && !otherAxle->IsStartCapped();
                    } else {
                        forward = !axle->IsStartCapped() && !otherAxle->IsStartCapped();
                        backward = !axle->IsEndCapped() && !otherAxle->IsEndCapped();
                    }
                    if (forward)
                        connection->Add(Vector3(axis), length);
                    if (backward)
                        connection->Add(-axis, length);
                }
                break;
            }
            case ConnectionField::TYPE_BALL:
                // y, z, x (Mac 0x20cb9a)
                AddBoth(connection, FieldDirection(0.0f, 1.0f, 0.0f, field, transform), 0.0f);
                AddBoth(connection, FieldDirection(0.0f, 0.0f, 1.0f, field, transform), 0.0f);
                AddBoth(connection, FieldDirection(1.0f, 0.0f, 0.0f, field, transform), 0.0f);
                break;
            case ConnectionField::TYPE_FIXED:
                // y, z (Mac 0x20cda6)
                AddBoth(connection, FieldDirection(0.0f, 1.0f, 0.0f, field, transform), 0.0f);
                AddBoth(connection, FieldDirection(0.0f, 0.0f, 1.0f, field, transform), 0.0f);
                break;
            case ConnectionField::TYPE_RAIL: {
                Vector3 up = FieldDirection(0.0f, 1.0f, 0.0f, field, transform);
                connection->Add(Vector3(up), 0.0f);
                break;
            }
            case ConnectionField::TYPE_SLIDER: {
                Vector3 x = FieldDirection(1.0f, 0.0f, 0.0f, field, transform);
                connection->Add(-x, 0.0f);
                AddBoth(connection, FieldDirection(0.0f, 1.0f, 0.0f, field, transform), 0.0f);
                break;
            }
            default:
                break;
            }
            connection->m_strength = GetConnectionStrength(primitiveConnection);
        }
    }
}

// Mac 0x20d3b0
Real32 BI::GetConnectionStrength(const PrimitiveConnection* connection)
{
    UInt32 points = 0;
    const BasicArray<PrimitiveConnection::Connection>& fields = connection->GetConnections();
    for (BasicArray<PrimitiveConnection::Connection>::ConstIterator it = fields.Begin(); it != fields.End(); ++it) {
        switch (it->m_field->GetType()) {
        case ConnectionField::TYPE_PLANAR:
            points += it->m_indices.GetSize();
            break;
        case ConnectionField::TYPE_HINGE:
        case ConnectionField::TYPE_AXLE:
        case ConnectionField::TYPE_BALL:
            points += 10;
            break;
        case ConnectionField::TYPE_FIXED:
            points += 20;
            break;
        case ConnectionField::TYPE_RAIL:
        case ConnectionField::TYPE_SLIDER:
            points += 1;
            break;
        }
    }
    Real32 strength = (Real32)points * 100.0f;
    Real32 size = connection->GetElement()->GetBone().m_bounding.GetSize().GetLength();
    Real32 otherSize = connection->GetOtherElement()->GetBone().m_bounding.GetSize().GetLength();
    if (otherSize <= size)
        size = otherSize;
    return strength / size;
}

// Mac 0x20d4f0
void BI::Trace(const String& /*message*/, UInt32 /*level*/)
{
}

} // namespace LEGO
