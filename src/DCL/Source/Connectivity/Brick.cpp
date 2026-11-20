// LEGO Digital Designer — restored source
// LEGO::Brick (see Brick.h).
//
// Original object file: Scene.cpp (Mac TU 025); this is sub-TU 025c, Mac 0x933b0-0x94100
// (+ constructor 0xc2910 of 025r), Win 0x46f000-0x46f440 / 0x471470-0x4718d0.
#include "Brick.h"
#include "Part.h"
#include "PrimitiveElement.h"
#include "Scene.h"
#include "SceneGraphUtil.h"

namespace LEGO {

// Mac 0xc2910 (025r), Win 0x46f110
Brick::Brick(UInt32 designID)
    : m_designID(designID), m_hidden(false)
{
}

// Mac 0xc33f0 (complete) / 0xc3450 (deleting), thunks 0xc34c0 / 0xc3520 / 0xc3590 / 0xc35f0,
// base-object variant 0xcb970; Win 0x46f200 (sdd 0x46f410)
Brick::~Brick()
{
}

// Mac 0x93650, Win 0x471590
Part* Brick::GetPart(UInt32 index) const
{
    return m_parts[index];
}

// Mac 0x933d0 (thunk 0x93450), Win 0x471470
void Brick::GetBoundingBox(BoundingBox& box, bool recompute) const
{
    box.Reset();
    for (BasicArray<RefPointer<Part> >::ConstIterator it = m_parts.Begin(); it != m_parts.End(); ++it) {
        BoundingBox partBox(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
        (*it)->GetBoundingBox(partBox, recompute);
        box.Union(partBox);
    }
}

// Mac 0x934d0 (thunk 0x934f0), Win 0x471500
Scene* Brick::GetScene() const
{
    // NOTE: m_parts[0] without a size check (a brick without parts is never asked).
    return m_parts[0]->GetScene();
}

// Mac 0x93510, Win 0x471510
void Brick::SetHidden(bool hidden)
{
    if (m_hidden == hidden)
        return;
    m_hidden = hidden;
    Scene* scene = GetScene();
    bool updateBodies = false;
    bool updateGroups = false;
    if (scene) {
        updateBodies = scene->m_flag124;
        updateGroups = scene->m_flag125;
    }
    for (BasicArray<RefPointer<Part> >::ConstIterator it = m_parts.Begin(); it != m_parts.End(); ++it)
        (*it)->SetHidden(hidden, updateBodies, updateGroups, scene);
}

// Mac 0x93670 (push_back instance 0x93970), Win 0x4715a0 (push_back 0x49c600)
void Brick::GetInternalConnections(BasicArray<PrimitiveConnectionInfo>& connections) const
{
    UInt32 partCount = m_parts.GetSize();
    if (partCount == 1 && m_parts[0]->m_elements.GetSize() == 1)
        return;                                 // one part of one element: nothing inside
    for (UInt32 i = 0; i < m_parts.GetSize(); ++i) {
        const Part* part = m_parts[i];
        for (UInt32 a = 0; a < part->m_elements.GetSize(); ++a) {
            PrimitiveElement* element = part->m_elements[a];
            for (UInt32 f = 0; f < element->m_fields.GetSize(); ++f) {
                ConnectionField* field = element->m_fields[f];
                Matrix3D transform(element->m_transform, field->GetTransform());
                for (UInt32 j = i; j < m_parts.GetSize(); ++j) {
                    const Part* otherPart = m_parts[j];
                    // each element pair once: later elements of the same part, all of later parts
                    for (UInt32 b = (j == i) ? a + 1 : 0; b < otherPart->m_elements.GetSize(); ++b) {
                        PrimitiveElement* otherElement = otherPart->m_elements[b];
                        for (UInt32 g = 0; g < otherElement->m_fields.GetSize(); ++g) {
                            ConnectionField* otherField = otherElement->m_fields[g];
                            Matrix3D otherTransform(otherElement->m_transform, otherField->GetTransform());
                            PrimitiveConnectionInfo info;
                            UInt32 type = field->CanConnect(transform, *otherField, otherTransform,
                                                            info.m_indices);
                            if ((type & (ConnectionField::CONNECTION_RIGID |
                                         ConnectionField::CONNECTION_FREE)) != 0) {
                                info.m_element = element;
                                info.m_otherElement = otherElement;
                                info.m_field = field;
                                info.m_otherField = otherField;
                                connections.PushBack(info);
                            }
                        }
                    }
                }
            }
        }
    }
}

// Win 0x46f2c0 (Mac inlined into Part::SetBrick 0x94f60)
void Brick::AddPart(Part* part)
{
    RefPointer<Part> reference(part);
    m_parts.PushBack(reference);
}

// Mac 0x95010, Win 0x46f360
void Brick::RemovePart(Part* part)
{
    RefPointer<Part> reference(part);           // keeps the part alive during the erase
    SceneGraph::EraseValue(m_parts, part);
}

} // namespace LEGO
