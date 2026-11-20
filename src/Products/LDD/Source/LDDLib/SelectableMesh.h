// LEGO Digital Designer — restored source
// LEGO::SelectableMesh — a SingleMesh with a selected flag; the rotation bands of the hinge
//   tool (LDDUtilities' CreateSelectableMesh, HingeTool).
//
// Header only in the original as well: the inline members are emitted in TU 063 (LDDUtilities;
// Mac D1 0x1503f0, D0 0x150450, Object-in thunks 0x1504c0 / 0x150520); the Win constructor
// is out of line (Win 0x636dfb, thiscall(material, vertexBuffer, mostDerived)).
// RTTI: si, base SingleMesh. Mac vtable 0x767a4c {IsOpaque 0x529f0, IsDecal 0x52a10, Bind
// 0x52c00, BeginDraw 0x52c50, EndDraw 0x52c70, D1, D0} (no own virtuals), Object-in 0x767a80.
// Mac layout: vptr, material +0x04, vertex buffer +0x08, m_selected +0x0c, Object +0x10
// (sizeof 0x18). Win: vptr, vbptr +4, material +8, vertex buffer +0xc, m_selected +0x10,
// Object +0x14 (vftables 0x9d37e0 / 0x9d37cc {sdd 0x6377f7}).
// Name of the member inferred (never read in the restored code).
#pragma once
#include "LEGO/Types.h"
#include "Object.h"
#include "Mesh.h"

namespace LEGO {

class SelectableMesh : public SingleMesh {
public:
    // Win 0x636dfb (Mac inlined into 0x14db80)
    SelectableMesh(Material* material, VertexBuffer* buffer)
        : SingleMesh(material, buffer), m_selected(false) {}
    virtual ~SelectableMesh() {}                                     // Mac 0x1503f0/0x150450, Win sdd 0x6377f7

    bool IsSelected() const { return m_selected; }
    void SetSelected(bool selected) { m_selected = selected; }

protected:
    bool m_selected;   // +0x0c false
};

} // namespace LEGO
