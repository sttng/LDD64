// LEGO Digital Designer — restored source
// TU 213 "BIBrick.cpp" (Mac 0x25e350-0x25f869): BIBrick base, BIBrick::Connection methods.
// See BIBrick.h. The Windows build inlines most of these (Win 0x56e110-0x56f3cf).
#include "BIBrick.h"
#include "BIBrickPrimitive.h"
#include "BGScene.h"
#include "PrimitiveElement.h"
#include "Primitive.h"
#include "Part.h"
#include "Brick.h"
#include "BrickDatabase.h"
#include "StringStream.h"
#include "LEGO/String.h"
#include <math.h>

namespace LEGO {

UInt32 BIBrick::s_nextID = 0;   // Mac 0x77aee8

// BasicMap<UInt32, Vector3> constructed by the TU's initialiser (Mac 0x25f7d0, object 0x77aeec)
// and not referenced by any function of the TU. (name inferred)
static BasicMap<UInt32, Vector3> s_unusedVectors;

// Mac 0x25e350
BIBrick::BIBrick(UInt32 type)
    : m_type(type),
      m_group(0),
      m_boundingBoxValid(false),
      m_boundingBox(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f),
      m_volumeValid(false),
      m_volume(0.0f),
      m_centerValid(false),
      // NOTE: +0xac is not initialised by the original constructor (only the substep
      // constructor's UpdateCategory writes it); 0 here so that it is never read uninitialised.
      m_category(0),
      m_enabled(true)
{
    // NOTE: +0x80..+0x88 (the BIGraph search state) are not initialised by the original
    // constructor; 0 here.
    m_dfsNumber = 0;
    m_dfsLow = 0;
    m_dfsParent = 0;
    m_id = s_nextID++;
    m_direction.Clear();
}

// Mac 0x25f580 / D0 0x25f590 (body 0x25f680), Win 0x56e110 / sdd 0x56e1c0
BIBrick::~BIBrick()
{
}

// Mac 0x25e5d0
void BIBrick::SetEnabled(bool enabled)
{
    m_enabled = enabled;
}

// Mac 0x25e5f0
bool BIBrick::IsEnabled() const
{
    return m_enabled;
}

// Mac 0x25e600
Vector3 BIBrick::GetDirection(BGScene* scene, bool flag)
{
    if (m_direction.IsAlmostZero()) {
        Vector3 direction;
        BasicSet<BIBrickPrimitive*> primitives;
        bool found;
        if (scene->Intersects(this))
            found = scene->GetRemoveDirection(this, direction, flag, primitives);
        else
            found = scene->GetInsertDirection(this, direction, 1, flag, primitives);
        if (found) {
            direction.Normalize();
            m_direction = direction * 10.0f;
        }
    }
    return m_direction;
}

// Mac 0x25e7b0
void BIBrick::ClearDirection()
{
    m_direction.Clear();
}

// Mac 0x25e7d0, Win 0x56e500
void BIBrick::SetDirection(const Vector3& direction)
{
    m_direction = direction;
}

// Mac 0x25e7f0
BoundingBox BIBrick::GetBoundingBox()
{
    if (!m_boundingBoxValid) {
        m_boundingBoxValid = true;
        m_boundingBox = BoundingBox(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
        m_boundingBox.Reset();
        Matrix3D transform;
        for (BasicArray<PrimitiveElement*>::ConstIterator it = m_elements.Begin(); it != m_elements.End(); ++it) {
            (*it)->GetTransform(transform);
            BoundingBox box = (*it)->GetBone().m_bounding * transform;
            m_boundingBox.Union(box);
        }
    }
    return m_boundingBox;
}

// Mac 0x25e8d0
Real32 BIBrick::GetVolume()
{
    if (!m_volumeValid) {
        m_volumeValid = true;
        m_volume = 0.0f;
        for (BasicArray<PrimitiveElement*>::ConstIterator it = m_elements.Begin(); it != m_elements.End(); ++it) {
            Vector3 size = (*it)->GetBone().m_bounding.GetSize();
            Real32 v = size.GetLengthSquared();
            m_volume = v + m_volume;
        }
    }
    return m_volume;
}

// Mac 0x25e960
Vector3 BIBrick::ComputeCenter(Real32 volume)
{
    if (!m_centerValid) {
        m_centerValid = true;
        m_center = Vector3(0.0f, 0.0f, 0.0f);
        for (BasicArray<PrimitiveElement*>::ConstIterator it = m_elements.Begin(); it != m_elements.End(); ++it) {
            Matrix3D transform;
            (*it)->GetTransform(transform);
            const BoundingBox& bounding = (*it)->GetBone().m_bounding;
            Vector3 center = bounding.GetCenter();
            Vector3 world = center * transform;
            Vector3 size = bounding.GetSize();
            Real32 weight = size.GetLengthSquared();
            m_center += world * weight;
        }
    }
    return m_center / volume;
}

// Mac 0x25ead0
Vector3 BIBrick::GetCenterOfMass()
{
    return ComputeCenter(GetVolume());
}

// Mac 0x25eb70
Vector3 BIBrick::GetPoint(UInt32 point)
{
    switch (point) {
    case PointMinimum:
        return GetBoundingBox().m_min;
    case PointMaximum:
        return GetBoundingBox().m_max;
    case PointCenter:
        return GetBoundingBox().GetCenter();
    case PointCenterOfMass:
        return GetCenterOfMass();
    default:
        return Vector3();
    }
}

// Mac 0x25ecb0
bool BIBrick::ContainsElement(const PrimitiveElement* element) const
{
    for (BasicArray<PrimitiveElement*>::ConstIterator it = m_elements.Begin(); it != m_elements.End(); ++it)
        if (*it == element)
            return true;
    return false;
}

// Mac 0x25ecf0, Win 0x56e9d0
bool BIBrick::ContainsDesign(UInt32 designID) const
{
    for (BasicArray<PrimitiveElement*>::ConstIterator it = m_elements.Begin(); it != m_elements.End(); ++it)
        if ((*it)->GetPrimitive()->GetDesignID() == designID)
            return true;
    return false;
}

// "maingroupid" annotation of a primitive / an assembly (inlined lookups).
static bool FindMainGroup(const Primitive::AnnotationMap& annotations, const String& key, UInt32& id)
{
    Primitive::AnnotationMap::ConstIterator it = annotations.Find(key);
    if (it == annotations.End())
        return false;
    FromString(id, it->second);
    return true;
}

// Mac 0x25ed30, Win 0x56ea40
bool BIBrick::HasMainGroup(UInt32 id) const
{
    static const String s_key("maingroupid");   // Mac 0x77e2c8 (guard 0x77e2d8)
    for (BasicArray<PrimitiveElement*>::ConstIterator it = m_elements.Begin(); it != m_elements.End(); ++it) {
        const Primitive* primitive = (*it)->GetPrimitive();
        UInt32 group = 0;
        if (FindMainGroup(primitive->GetAnnotations(), s_key, group) && group == id)
            return true;
        Brick* brick = (*it)->GetPart()->GetBrick();
        if (brick) {
            const BrickDatabase::Brick* assembly = primitive->GetDatabase()->GetBrick(brick->GetDesignID());
            UInt32 assemblyGroup = 0;
            // NOTE: the original dereferences the lookup result without a null check (an unknown
            // assembly reads through a null pointer); fixed: checked.
            if (assembly)
                FindMainGroup(assembly->m_annotations, s_key, assemblyGroup);
            // NOTE: without the annotation the group stays 0, so id 0 matches every brick.
            if (assemblyGroup == id)
                return true;
        }
    }
    return false;
}

// Mac 0x25ef10
bool BIBrick::HasMainGroup(const BasicSet<UInt32>& ids) const
{
    static const String s_key("maingroupid");   // Mac 0x77e2dc (guard 0x77e2ec)
    for (BasicArray<PrimitiveElement*>::ConstIterator it = m_elements.Begin(); it != m_elements.End(); ++it) {
        UInt32 group = 0;
        if (FindMainGroup((*it)->GetPrimitive()->GetAnnotations(), s_key, group) && ids.Contains(group))
            return true;
    }
    return false;
}

// Mac 0x25f3a0
void BIBrick::UpdateCategory()
{
    // Function-local static, filled while empty (Mac 0x77e2f0, guard 0x77e30c).
    static BasicSet<UInt32> s_minifigGroups;
    if (s_minifigGroups.GetSize() == 0) {
        s_minifigGroups.Insert(0x109);
        s_minifigGroups.Insert(0x10a);
        s_minifigGroups.Insert(0x10c);
        s_minifigGroups.Insert(0x16e);
        s_minifigGroups.Insert(0x170);
    }
    BasicSet<UInt32> groups(s_minifigGroups);
    if (HasMainGroup(groups))
        m_category = CategoryMinifig;
}

// ---- BIBrick::Connection ----------------------------------------------------------------------

// Mac 0x25f140
void BIBrick::Connection::Add(const Vector3& direction, Real32 weight)
{
    m_directions.PushBack(direction);
    m_weights.PushBack(weight);
}

// Mac 0x25f090
bool BIBrick::Connection::FindDirection(const Vector3& direction, Real32& weight) const
{
    bool found = false;
    UInt32 i = 0;
    for (BasicArray<Vector3>::ConstIterator it = m_directions.Begin(); it != m_directions.End(); ++it, ++i) {
        if (fabsf(it->x - direction.x) < 0.1f && fabsf(it->y - direction.y) < 0.1f &&
            fabsf(it->z - direction.z) < 0.1f) {
            Real32 w = m_weights[i];
            Real32 current = weight;
            if (current <= w)
                current = w;
            weight = current;
            found = true;
        }
    }
    return found;
}

} // namespace LEGO
