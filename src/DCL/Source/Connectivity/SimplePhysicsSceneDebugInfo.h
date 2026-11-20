// LEGO Digital Designer — restored source
// LEGO::PhysicsSceneDebugInfo (the debug drawing interface: points, lines and triangles) and
// LEGO::SimplePhysicsSceneDebugInfo, the array based implementation that Scene::GetDebugInfo
// fills (SimplePhysicsSceneDebugInfo.cpp).
//
// RTTI: class LEGO::SimplePhysicsSceneDebugInfo : PhysicsSceneDebugInfo (: IUnknown).
//   Mac sizeof 0x38 (Scene::GetDebugInfo 0xb8ec0 / 0xbaa40): vptr +0 (vtable 0x778e00
//       {QueryInterface 0xc9b80, AddRef 0xc9c70, Release 0xc9c90, GetPointCount 0xc9d30,
//       GetPoints 0xc9d50, GetLineCount 0xc9d60, GetLines 0xc9d80, GetTriangleCount 0xc9d90,
//       GetTriangles 0xc9db0} (025s)), m_points +0x04, m_lines +0x14, m_triangles +0x24,
//       m_refCount +0x34 (starts at 1).
//   Win the same layout (constructor 0x46ed10, QueryInterface 0x46ebf0, AddRef 0x46ec50,
//       Release 0x46ec70, counts 0x46eca0 / 0x46ecc0 / 0x46ece0, arrays 0x52f4b0 / 0x4c1900 /
//       0x46ed00, destructor body 0x46eda0).
// Every member and method name is inferred (the element structs are named by the RTTI of their
// arrays: BasicArray<PhysicsSceneDebugInfo::Point / Line / Triangle>).
#pragma once
#include "LEGO/Types.h"
#include "BasicArray.h"
#include "Geometry.h"
#include "Color.h"
#include "BrickPhysics.h"

namespace LEGO {

class PhysicsSceneDebugInfo : public IUnknown {
public:
    // 0x1c bytes
    struct Point {
        Vector3 m_position;   // +0x00
        Color   m_color;      // +0x0c
    };
    // 0x28 bytes (push: Mac 0xc11b0, Win 0x49b640)
    struct Line {
        Vector3 m_from;       // +0x00
        Vector3 m_to;         // +0x0c
        Color   m_color;      // +0x18
    };
    // 0x58 bytes (push: Mac 0xc9e50, Win 0x49b700)
    struct Triangle {
        Vector3 m_vertices[3];   // +0x00
        Vector3 m_normals[3];    // +0x24
        Color   m_color;         // +0x48
    };

    // Mac 0x778e50 (function-local static), Win 0x46eb20
    static const InterfaceID& GetIID()
    {
        static const InterfaceID id = {{0x874238d5, 0x414fa8a4, 0x95c4639a, 0x8874d75c}};
        return id;
    }

    virtual UInt32          GetPointCount() const = 0;      // slot 3
    virtual const Point*    GetPoints() const = 0;          // slot 4
    virtual UInt32          GetLineCount() const = 0;       // slot 5
    virtual const Line*     GetLines() const = 0;           // slot 6
    virtual UInt32          GetTriangleCount() const = 0;   // slot 7
    virtual const Triangle* GetTriangles() const = 0;       // slot 8
};

class SimplePhysicsSceneDebugInfo : public PhysicsSceneDebugInfo {
public:
    // What Scene::GetDebugInfo draws (bits of its `flags`). (names inferred)
    enum Flags {
        FlagFields              = 0x001,   // the connection fields of the elements (lines)
        FlagCollisionVolumes    = 0x002,   // the collision volumes, each in a random colour
        FlagVolumeColorPerElement = 0x004, // the same, one random colour per element
        FlagFlexAxes            = 0x008,   // the axes of the elements of flexible parts
        FlagBodyBox             = 0x010,   // the bounding box of each body (yellow)
        FlagBodySphere          = 0x020,   // the bounding sphere of each body (yellow)
        FlagRigidBox            = 0x040,   // the box of each PrimitiveRigidElement (magenta)
        FlagRigidSphere         = 0x080,   // the sphere of each PrimitiveRigidElement (magenta)
        FlagElementBox          = 0x100,   // the bone box of each element (cyan)
        FlagElementSphere       = 0x200,   // the bounding sphere of each element (cyan)
        FlagVolumeMargin        = 0x400,   // collision volumes grown by 0.01
        FlagVolumeWireframe     = 0x800    // collision volumes as lines (else triangles)
    };

    // Mac inline (Scene::GetDebugInfo 0xb8ec0 / 0xbaa40), Win 0x46ed10
    SimplePhysicsSceneDebugInfo() : m_refCount(1) {}

    // --- IUnknown: this interface or IUnknown. Mac 0xc9b80, Win 0x46ebf0
    virtual Int32 LEGO_COM_CALL QueryInterface(const InterfaceID& id, void** object);
    // Mac 0xc9c70 (OSAtomicAdd32), Win 0x46ec50 (InterlockedIncrement)
    virtual UInt32 LEGO_COM_CALL AddRef();
    // Deletes the object with the last reference. Mac 0xc9c90, Win 0x46ec70
    virtual UInt32 LEGO_COM_CALL Release();

    // Mac 0xc9d30, Win 0x46eca0
    virtual UInt32 GetPointCount() const { return m_points.GetSize(); }
    // Mac 0xc9d50, Win 0x52f4b0
    virtual const Point* GetPoints() const { return m_points.Begin(); }
    // Mac 0xc9d60, Win 0x46ecc0
    virtual UInt32 GetLineCount() const { return m_lines.GetSize(); }
    // Mac 0xc9d80, Win 0x4c1900
    virtual const Line* GetLines() const { return m_lines.Begin(); }
    // Mac 0xc9d90, Win 0x46ece0
    virtual UInt32 GetTriangleCount() const { return m_triangles.GetSize(); }
    // Mac 0xc9db0, Win 0x46ed00
    virtual const Triangle* GetTriangles() const { return m_triangles.Begin(); }

    BasicArray<Point>    m_points;      // +0x04 (nothing in TU 025 adds points)
    BasicArray<Line>     m_lines;       // +0x14
    BasicArray<Triangle> m_triangles;   // +0x24
    volatile Int32       m_refCount;    // +0x34

protected:
    // Not virtual (the vtables end with slot 8; Release deletes the object). Win 0x46eda0 (Mac
    // inlined into Release)
    ~SimplePhysicsSceneDebugInfo() {}
};

} // namespace LEGO
