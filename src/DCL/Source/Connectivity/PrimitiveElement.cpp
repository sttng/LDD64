// LEGO Digital Designer — restored source
// LEGO::PrimitiveElement, LEGO::FilterConnectionFieldObserver and the MiniFig observers (see
// PrimitiveElement.h).
//
// Original object file: Scene.cpp (Mac TU 025); this is sub-TU 025j, Mac 0x9d7f0-0xa0210, plus
// DisconnectFields (0x97310, 025f), the inline virtuals of 025s (0xc4200-0xc4360) and the observers
// (025s 0xc5e90-0xc7400); Win 0x475ed0-0x477d10. The Mac range also holds
// PrimitiveElement::CreateManipulator (0x9dea0, with ConnectionManipulator, 025i), the BrickConnect
// geometry caches (0x9e0b0 / 0x9e280 / 0x9e440, in BrickConnect.cpp), the atexit destructors of the
// four observer statics (0x9e680-0x9e770) and the BasicMap destructor instance 0x9e7c0.
#include "PrimitiveElement.h"
#include "PrimitiveConnection.h"
#include "PrimitiveRigidElement.h"
#include "RigidElement.h"
#include "Part.h"
#include "Scene.h"
#include "BrickConnect.h"
#include "ConnectionFieldPlanar.h"
#include "MathUtil.h"

namespace LEGO {

namespace {
const Real32 kKnobSpacing = 0.4f;    // planar grid pitch (Mac 0x5917ef + PIC base)

// Design IDs with special connection rules (Mac 0x9e7f0 switch).
enum {
    kMiniFigTorso        = 3814,
    kMiniFigHip          = 3815,
    kMiniFigMiniLegs     = 41879,
    kMiniFigSkeletonBody = 60115
};

// Odd-kind planar fields carry the knobs whose collision features the element creates.
inline bool IsKnobField(const ConnectionField* field)
{
    return field->GetType() == ConnectionField::TYPE_PLANAR && (field->GetKind() & 1) != 0;
}

// std::vector::resize(n, value) on a BasicArray. (inline)
template <class T>
void Resize(BasicArray<T>& array, UInt32 size, const T& value)
{
    if (size <= array.GetSize()) {
        while (array.GetSize() > size)
            array.PopBack();
    } else {
        array.Insert(array.End(), size - array.GetSize(), value);
    }
}

// World transform of a primitive rigid element (see PrimitiveRigidElement.cpp). (inline)
void GetRigidWorldTransform(const PrimitiveRigidElement* rigid, Matrix3D& transform)
{
    if (rigid->m_rigidElement == 0) {
        transform = rigid->m_transform;
    } else {
        rigid->m_rigidElement->m_body->GetTransform(transform);
        transform.Multiply(rigid->m_transform);
    }
}
} // namespace

// =============================================================================================
// FilterConnectionFieldObserver and the MiniFig observers
// =============================================================================================

// The element owning `field` (fields of a PrimitiveElement have it as their context). (inline)
static PrimitiveElement* GetElement(ConnectionField& field)
{
    ConnectionFieldContext* context = field.GetContext();
    return context != 0 ? static_cast<PrimitiveElement*>(context) : 0;
}

// Mac 0xc6490, Win 0x476410
void FilterConnectionFieldObserver::OnConnect(bool first, ConnectionField& field, ConnectionField& other,
                                              const ConnectionField::ConnectionIndices&,
                                              ConnectionField::Attributes*)
{
    PrimitiveElement* element = GetElement(first ? field : other);
    for (BasicArray<UInt32>::Iterator it = m_freeFields.Begin(); it != m_freeFields.End(); ++it) {
        ConnectionField* f = element->GetField(*it);
        f->SetFlags(f->GetFlags() & ~1u);
    }
    for (BasicArray<UInt32>::Iterator it = m_connectedFields.Begin(); it != m_connectedFields.End(); ++it) {
        ConnectionField* f = element->GetField(*it);
        f->SetFlags(f->GetFlags() | 1u);
    }
}

// Mac 0xc65b0, Win 0x476480
void FilterConnectionFieldObserver::OnDisconnect(bool first, ConnectionField& field, ConnectionField& other,
                                                 const ConnectionField::ConnectionIndices&)
{
    PrimitiveElement* element = GetElement(first ? field : other);
    for (BasicArray<UInt32>::Iterator it = m_freeFields.Begin(); it != m_freeFields.End(); ++it) {
        ConnectionField* f = element->GetField(*it);
        f->SetFlags(f->GetFlags() | 1u);
    }
    for (BasicArray<UInt32>::Iterator it = m_connectedFields.Begin(); it != m_connectedFields.End(); ++it) {
        ConnectionField* f = element->GetField(*it);
        f->SetFlags(f->GetFlags() & ~1u);
    }
}

// Win 0x4763a0 (Mac inlined into the PrimitiveElement constructor)
void FilterConnectionFieldObserver::Apply(PrimitiveElement& element)
{
    if (m_fieldIndex < 0 || (m_freeFields.IsEmpty() && m_connectedFields.IsEmpty()))
        return;
    ConnectionField* observed = element.GetField((UInt32)m_fieldIndex);
    observed->SetObserver(this);
    observed->SetFlags(observed->GetFlags() & ~1u);
    for (BasicArray<UInt32>::Iterator it = m_freeFields.Begin(); it != m_freeFields.End(); ++it) {
        ConnectionField* f = element.GetField(*it);
        f->SetFlags(f->GetFlags() | 1u);
    }
    for (BasicArray<UInt32>::Iterator it = m_connectedFields.Begin(); it != m_connectedFields.End(); ++it) {
        ConnectionField* f = element.GetField(*it);
        f->SetFlags(f->GetFlags() & ~1u);
    }
}

namespace {
// Torso / skeleton body: the fixed field of kind 31 at `fixedY` is observed; the kind 201
// hinges at `hingeY` are free-only, the other kind 201 hinges connected-only.
void InitTorsoObserver(FilterConnectionFieldObserver& observer, const Primitive::Bone& bone,
                       Real32 fixedY, Real32 hingeY)
{
    const BasicArray<RefPointer<ConnectionField> >& fields = bone.m_connectionFields;
    for (UInt32 i = 0; i < fields.GetSize(); ++i) {
        const ConnectionField* field = fields[i];
        if (field->GetType() == ConnectionField::TYPE_FIXED && field->GetKind() == 31 &&
            Abs(field->GetTransform().GetTranslation().y - fixedY) < 0.001f)
            observer.m_fieldIndex = (Int32)i;
        if (field->GetType() == ConnectionField::TYPE_HINGE && field->GetKind() == 201) {
            if (Abs(field->GetTransform().GetTranslation().y - hingeY) >= 0.001f)
                observer.m_connectedFields.PushBack(i);
            else
                observer.m_freeFields.PushBack(i);
        }
    }
}

// Hip / mini legs: the fixed field of kind 5 at `y` is observed, the other kind 5 fixed
// fields are free-only.
void InitHipObserver(FilterConnectionFieldObserver& observer, const Primitive::Bone& bone, Real32 y)
{
    const BasicArray<RefPointer<ConnectionField> >& fields = bone.m_connectionFields;
    for (UInt32 i = 0; i < fields.GetSize(); ++i) {
        const ConnectionField* field = fields[i];
        if (field->GetType() != ConnectionField::TYPE_FIXED || field->GetKind() != 5)
            continue;
        if (Abs(field->GetTransform().GetTranslation().y - y) >= 0.001f)
            observer.m_freeFields.PushBack(i);
        else
            observer.m_fieldIndex = (Int32)i;
    }
}
} // namespace

// Mac 0xc5e90 (025s), Win 0x476560
MiniFigTorso3814ConnectionFieldObserver::MiniFigTorso3814ConnectionFieldObserver(const Primitive::Bone& bone)
{
    InitTorsoObserver(*this, bone, 1.28f, 1.43f);
}

// Mac 0xc66d0 (025s), Win 0x476710
MiniFigSkeletonBody60115ConnectionFieldObserver::MiniFigSkeletonBody60115ConnectionFieldObserver(const Primitive::Bone& bone)
{
    InitTorsoObserver(*this, bone, 1.76f, 1.91f);
}

// Mac 0xc6cd0 (025s), Win 0x476840
MiniFigHip3815ConnectionFieldObserver::MiniFigHip3815ConnectionFieldObserver(const Primitive::Bone& bone)
{
    // NOTE: |y| (Mac: no subtraction of a constant).
    InitHipObserver(*this, bone, 0.0f);
}

// Mac 0xc7040 (025s), Win 0x476930
MiniFigMiniLegs41879ConnectionFieldObserver::MiniFigMiniLegs41879ConnectionFieldObserver(const Primitive::Bone& bone)
{
    InitHipObserver(*this, bone, 0.9588f);
}

// =============================================================================================
// PrimitiveElement
// =============================================================================================

// Inlined (Mac 0x9e7f0, 0x97310)
UInt32 PrimitiveElement::GetFeatureType(const ConnectionPoint& point)
{
    switch (point.m_type) {
    case 0x1e: return 5;
    case 0x1f: return 6;
    case 0x24: return 7;
    default:
        if (point.m_flags & 4)
            return 2;
        if (point.m_flags & 8)
            return 3;
        return (point.m_flags & 0x10) ? 4 : 1;
    }
}

// Mac 0x9e7f0, Win 0x476a70
PrimitiveElement::PrimitiveElement(const Primitive* primitive, UInt32 boneIndex, const Matrix3D& transform,
                                   BrickPhysics* physics, BrickConnect* brickConnect)
    : m_transform(transform), m_rigid(0), m_part(0), m_bone(0), m_boneIndex(boneIndex),
      m_boundingSphere(primitive->GetBone(boneIndex).m_bounding), m_primitive(primitive),
      m_physics(0), m_flagAC(false)
{
    m_bone = &primitive->GetBone(boneIndex);
    const Primitive::Bone& bone = *m_bone;

    // feature count: the collision volumes and every fixed point of the knob fields
    // NOTE: the count includes the border points of the grids, which get no feature below
    // (the trailing array entries stay unset; both builds).
    UInt32 knobCount = 0;
    for (BasicArray<RefPointer<ConnectionField> >::ConstIterator it = bone.m_connectionFields.Begin();
         it != bone.m_connectionFields.End(); ++it) {
        if (!IsKnobField(*it))
            continue;
        const ConnectionFieldPlanar* planar = static_cast<const ConnectionFieldPlanar*>(it->Get());
        for (UInt32 i = 0; i < planar->GetPointCount(); ++i)
            if (planar->GetPoint(i).m_flags & ConnectionPoint::FLAG_FIXED)
                ++knobCount;
    }
    UInt32 count = bone.m_collisionVolumes.GetSize() + knobCount;
    PhysicsPlacedConfiguredGeometry* features = 0;
    if (count != 0)
        features = new PhysicsPlacedConfiguredGeometry[count];

    // the collision volumes
    UInt32 feature = 0;
    for (BasicArray<Primitive::CollisionVolume>::ConstIterator v = bone.m_collisionVolumes.Begin();
         v != bone.m_collisionVolumes.End(); ++v) {
        PhysicsGeometry* geometry;
        if (v->m_type == Primitive::CollisionVolume::Capsule)
            geometry = brickConnect->GetCapsule(v->m_size.y, v->m_size.x);
        else if (v->m_type == Primitive::CollisionVolume::Sphere)
            geometry = brickConnect->GetSphere(v->m_size.x);
        else if (v->m_type == Primitive::CollisionVolume::Box)
            geometry = brickConnect->GetBox(v->m_size);
        else
            continue;
        features[feature].m_geometry = geometry;
        features[feature].m_transform = v->m_transform;
        features[feature].m_type = 1;
        ++feature;
    }

    // NOTE: the bone's feature lookup is built by the first element of the bone (the
    // primitive is shared and const elsewhere; both builds write to it here).
    Primitive::CollisionVolumeToAndFromFeatureLookup* lookup = 0;
    Primitive::CollisionVolumeToAndFromFeatureLookup& boneLookup =
        const_cast<Primitive::Bone&>(bone).m_featureLookup;
    if (boneLookup.m_count == 0x7fffffff) {
        boneLookup.m_count = bone.m_collisionVolumes.GetSize();
        Primitive::CollisionVolumeToAndFromFeatureLookup::Feature none = {0, 0};
        Resize(boneLookup.m_features, knobCount, none);
        Resize(boneLookup.m_volumes, bone.m_connectionFields.GetSize(), BasicArray<UInt32>());
        lookup = &boneLookup;
    }

    // the fields (clones with this element as context) and the knob features
    PhysicsGeometry* knob = brickConnect->GetKnobGeometry();
    Matrix3D offset;
    for (BasicArray<RefPointer<ConnectionField> >::ConstIterator it = bone.m_connectionFields.Begin();
         it != bone.m_connectionFields.End(); ++it) {
        ConnectionField* clone = 0;
        (*it)->Clone(clone);
        clone->SetContext(static_cast<ConnectionFieldContext*>(this));
        clone->SetIndex(m_fields.GetSize());
        m_fields.PushBack(RefPointer<ConnectionField>(clone));
        if (!IsKnobField(clone))
            continue;
        const ConnectionFieldPlanar* planar = static_cast<const ConnectionFieldPlanar*>(clone);
        UInt32 width = planar->GetWidth();
        UInt32 height = planar->GetHeight();
        for (UInt32 row = 1; row < height; ++row) {
            if (width <= 1)
                continue;
            Real32 z = (Real32)row * kKnobSpacing;
            for (UInt32 column = 1; column < width; ++column) {
                UInt32 index = (width + 1) * row + column;
                const ConnectionPoint& point = planar->GetPoint(index);
                if (!(point.m_flags & ConnectionPoint::FLAG_FIXED))
                    continue;
                offset.SetTranslation(Vector3((Real32)column * kKnobSpacing, 0.0f, z));
                features[feature].m_geometry = knob;
                features[feature].m_transform.Multiply(clone->GetTransform(), offset);
                features[feature].m_type = GetFeatureType(point);
                if (lookup != 0) {
                    Primitive::CollisionVolumeToAndFromFeatureLookup::Feature& f =
                        lookup->m_features[feature - lookup->m_count];
                    f.m_volume = clone->GetIndex();
                    f.m_feature = index;
                    BasicArray<UInt32>& points = lookup->m_volumes[clone->GetIndex()];
                    if (points.IsEmpty())
                        Resize(points, planar->GetPointCount(), (UInt32)0x7fffffff);
                    points[index] = feature;
                }
                ++feature;
            }
        }
    }

    // the bone's <PhysicsAttributes> block (+0xa8: inertia tensor, centre of mass,
    // mass, friction type) is the engine's PhysicsMassProperties (Mac 0x9e7f0 passes &bone+0xa8)
    m_physics = physics->CreatePrimitiveElement(
        features, count, *reinterpret_cast<const PhysicsMassProperties*>(&bone.m_inertiaTensor));
    m_physics->SetUserData(this);
    delete[] features;

    // minifigure parts: special connection rules (one observer per design, shared by all
    // elements of it)
    switch (primitive->GetDesignID()) {
    case kMiniFigTorso: {
        static MiniFigTorso3814ConnectionFieldObserver observer(*m_bone);
        observer.Apply(*this);
        break;
    }
    case kMiniFigHip: {
        static MiniFigHip3815ConnectionFieldObserver observer(*m_bone);
        observer.Apply(*this);
        break;
    }
    case kMiniFigMiniLegs: {
        static MiniFigMiniLegs41879ConnectionFieldObserver observer(*m_bone);
        observer.Apply(*this);
        break;
    }
    case kMiniFigSkeletonBody: {
        static MiniFigSkeletonBody60115ConnectionFieldObserver observer(*m_bone);
        observer.Apply(*this);
        break;
    }
    default:
        break;
    }
}

// Mac 0x9fcc0 (complete) / 0x9fd90 (deleting), base-object 0x9fa60, thunks 0x9fd20 / 0x9fe00;
// Win sdd 0x4a1b30
PrimitiveElement::~PrimitiveElement()
{
    if (m_physics != 0)
        m_physics->Release();
}

// Mac 0xc4200 (025s), Win 0x46f8a0
void PrimitiveElement::GetBoundingBox(BoundingBox& box, bool) const
{
    box = m_bone->m_bounding;
}

// Mac 0xc4230 (025s), Win 0x4705c0
Scene* PrimitiveElement::GetScene() const
{
    return m_rigid != 0 ? m_rigid->GetScene() : 0;
}

// Mac 0xc4260 (025s, thunk 0xc4360), Win 0x470500
void PrimitiveElement::GetTransform(Matrix3D& transform) const
{
    if (m_rigid == 0) {
        transform = m_transform;
        return;
    }
    GetRigidWorldTransform(m_rigid, transform);
    transform.Multiply(m_transform);
}

// Mac 0x9d800 (thunk 0x9d820), Win 0x475ee0
bool PrimitiveElement::IsSameRigid(const ConnectionFieldContext* other) const
{
    const PrimitiveElement* element = static_cast<const PrimitiveElement*>(other);
    return m_rigid->m_rigidElement == element->m_rigid->m_rigidElement;
}

// Mac 0x9d840 (thunk 0x9d880), Win 0x475f10: the Scene's object at Mac +0x38 / Win +0x3c
// (Scene::m_listener).
void* PrimitiveElement::GetRigid() const
{
    Scene* scene = GetScene();
    return scene != 0 ? scene->GetListener() : 0;
}

// Mac 0x9fe80 (thunk 0x9ff20), Win 0x4a1ae0
void PrimitiveElement::OnFinalRelease()
{
    m_fields.Clear();
    m_connections.Clear();
    Object::OnFinalRelease();
}

// Mac 0x9dbc0, Win 0x476190
bool PrimitiveElement::IsFullyConnected() const
{
    for (BasicArray<RefPointer<ConnectionField> >::ConstIterator it = m_fields.Begin();
         it != m_fields.End(); ++it)
        if (!(*it)->IsOccupied())
            return false;
    return true;
}

// Mac 0x9dc30, Win 0x4761d0
void PrimitiveElement::GetFeature(UInt32 fieldIndex, UInt32 point,
                                  BasicArray<PhysicsGeometryIdentifier>& features) const
{
    const BasicArray<UInt32>& points = m_bone->m_featureLookup.m_volumes[fieldIndex];
    if (points.IsEmpty())
        return;
    UInt32 feature = points[point];
    if (feature == 0x7fffffff)
        return;
    PhysicsGeometryIdentifier identifier;
    identifier.m_element = m_physics;
    identifier.m_feature = feature;
    features.PushBack(identifier);
}

// Mac 0x9dc10
void PrimitiveElement::GetFeature(const ConnectionField& field, UInt32 point,
                                  BasicArray<PhysicsGeometryIdentifier>& features) const
{
    GetFeature(field.GetIndex(), point, features);
}

// One placed geometry and its parts. Mac 0x9d940 (EDX = geometry, ECX = transform), Win 0x475f30
static void VisitGeometry(PrimitiveElement::CollisionVolumeVisitor& visitor, const Matrix3D& transform,
                          const PhysicsPlacedConfiguredGeometry& placed)
{
    Matrix3D placedTransform(transform, placed.m_transform);
    PhysicsGeometry* geometry = placed.m_geometry;
    void* object = 0;
    switch (geometry->GetKind()) {
    case PhysicsGeometry::KindSphere:
        if (geometry->QueryInterface(PhysicsGeometrySphere::GetIID(), &object) < 0)
            return;
        visitor.Visit(static_cast<PhysicsGeometrySphere*>(object), placedTransform);
        break;
    case PhysicsGeometry::KindBox:
        if (geometry->QueryInterface(PhysicsGeometryBox::GetIID(), &object) < 0)
            return;
        visitor.Visit(static_cast<PhysicsGeometryBox*>(object), placedTransform);
        break;
    case PhysicsGeometry::KindCapsule:
        if (geometry->QueryInterface(PhysicsGeometryCapsule::GetIID(), &object) < 0)
            return;
        visitor.Visit(static_cast<PhysicsGeometryCapsule*>(object), placedTransform);
        break;
    case PhysicsGeometry::KindConfigurable: {
        if (geometry->QueryInterface(PhysicsGeometryConfigable::GetIID(), &object) < 0)
            return;
        PhysicsGeometryConfigable* configurable = static_cast<PhysicsGeometryConfigable*>(object);
        PhysicsPlacedConfiguredGeometry part;
        part.m_geometry = 0;
        part.m_type = 1;
        UInt32 count = configurable->GetGeometryCount(placed.m_type);
        for (UInt32 i = 0; i < count; ++i) {
            if (configurable->GetGeometry(placed.m_type, i, part)) {
                VisitGeometry(visitor, placedTransform, part);
                part.m_geometry->Release();
            }
        }
        break;
    }
    default:
        return;
    }
    static_cast<IUnknown*>(object)->Release();
}

// Mac 0x9d8b0, Win 0x476100
void PrimitiveElement::VisitCollisionVolumes(CollisionVolumeVisitor& visitor, const Matrix3D& transform) const
{
    PhysicsPlacedConfiguredGeometry placed;
    for (UInt32 i = 0; i < m_physics->GetGeometryCount(); ++i) {
        if (!m_physics->GetGeometry(i, placed))
            continue;
        if (placed.m_type != 0)
            VisitGeometry(visitor, transform, placed);
        placed.m_geometry->Release();
    }
}

// Mac 0x9e020, Win 0x476350
void PrimitiveElement::SetPart(Part* part)
{
    if (m_part == part)
        return;
    if (part != 0)
        part->m_elements[m_boneIndex] = this;
    if (m_part != 0)
        m_part->m_elements[m_boneIndex] = 0;
    m_part = part;
}

// The knob-side element / field and the index slot of a planar connection. (inline)
namespace {
struct KnobSide {
    PrimitiveElement*      m_element;     // owner of the knob field
    ConnectionFieldPlanar* m_field;       // the odd-kind (knob) field
    ConnectionFieldPlanar* m_otherField;  // the even-kind field
    bool                   m_first;       // the knob field is the first field (m_index)
};
} // namespace

// Mac 0x9ffd0 (callers: Scene, Mac 0xbe6c0)
bool PrimitiveElement::Connect(const PrimitiveConnectionInfo& info, PrimitiveElement* other,
                               PrimitiveConnection*& connection)
{
    ConnectionField::Attributes attributes;
    attributes.m_isFlexible = false;
    info.m_field->Connect(*info.m_otherField, info.m_indices, &attributes);
    if (attributes.m_isFlexible)
        m_physics->AddFlexibleConnection(other->m_physics);

    if (info.m_field->GetType() == ConnectionField::TYPE_PLANAR) {
        // knobs covered by the other field get collision type 0 (2 for tube points of type 9 / 11
        // with flag 0x20; points of type 16 keep theirs)
        bool first = (info.m_field->GetKind() & 1) != 0;
        ConnectionFieldPlanar* knobField = static_cast<ConnectionFieldPlanar*>(first ? info.m_field : info.m_otherField);
        ConnectionFieldPlanar* otherField = static_cast<ConnectionFieldPlanar*>(first ? info.m_otherField : info.m_field);
        PrimitiveElement* knobElement = first ? info.m_element : info.m_otherElement;
        const BasicArray<UInt32>& features = knobElement->m_bone->m_featureLookup.m_volumes[knobField->GetIndex()];
        for (ConnectionField::ConnectionIndices::ConstIterator it = info.m_indices.Begin();
             it != info.m_indices.End(); ++it) {
            UInt32 knob = first ? it->m_index : it->m_otherIndex;
            UInt32 tube = first ? it->m_otherIndex : it->m_index;
            if (!(knobField->GetPoint(knob).m_flags & ConnectionPoint::FLAG_FIXED))
                continue;
            const ConnectionPoint& point = otherField->GetPoint(tube);
            if ((point.m_type | 2) == 0xb && (point.m_flags & 0x20) != 0)
                knobElement->m_physics->SetFeatureType(features[knob], 2);
            else if (point.m_type != 0x10)
                knobElement->m_physics->SetFeatureType(features[knob], 0);
        }
    }

    ConnectionMap::Iterator it = m_connections.Find(other);
    if (it != m_connections.End()) {
        connection = it->second;
        return connection->AddConnection(info, attributes);
    }
    connection = new PrimitiveConnection(info, attributes);
    return true;
}

// Mac 0x97310 (025f)
void PrimitiveElement::DisconnectFields(PrimitiveConnection* connection)
{
    for (BasicArray<PrimitiveConnection::Connection>::Iterator c = connection->m_connections.Begin();
         c != connection->m_connections.End(); ++c) {
        c->m_field->Disconnect(*c->m_otherField, c->m_indices);
        if (c->m_attributes.m_isFlexible)
            connection->m_element->m_physics->RemoveFlexibleConnection(connection->m_otherElement->m_physics);
        if (c->m_field->GetType() != ConnectionField::TYPE_PLANAR)
            continue;
        // the knobs get their collision type back
        bool first = (c->m_field->GetKind() & 1) != 0;
        ConnectionFieldPlanar* knobField = static_cast<ConnectionFieldPlanar*>(first ? c->m_field : c->m_otherField);
        PrimitiveElement* knobElement = first ? connection->m_element : connection->m_otherElement;
        const BasicArray<UInt32>& features = knobElement->m_bone->m_featureLookup.m_volumes[knobField->GetIndex()];
        for (ConnectionField::ConnectionIndices::ConstIterator it = c->m_indices.Begin();
             it != c->m_indices.End(); ++it) {
            UInt32 knob = first ? it->m_index : it->m_otherIndex;
            const ConnectionPoint& point = knobField->GetPoint(knob);
            if (point.m_flags & ConnectionPoint::FLAG_FIXED)
                knobElement->m_physics->SetFeatureType(features[knob], GetFeatureType(point));
        }
    }
    connection->m_connections.Clear();
}

} // namespace LEGO
