// LEGO Digital Designer — restored source
// LEGO::GeometryAccum — the geometry accumulator of the scene meshes: BrickConnectSceneControl
// (Mac 021e 0x46560, Win 0x622082) collects the primitive elements of one rigid body, per
// material (and per decoration) as Patches pointing into the CPU copies of the brick vertex
// buffers (ImportGeometry, filled by BrickConnectSceneManager Mac 0x60a50) with the element's
// transform; hidden faces are culled with the .g culling records and the connection state of
// the element's connection fields; then one merged vertex buffer per material / decoration is
// written from the patches (021g 0x4f6a0, Win 0x61a0e5; WritePatch below).
// RTTI names of its nested types: GeometryAccum::ImportGeometry, ImportPrimitive (TU 022),
// Patch, AccumulatedPatch (TU 021 / 053).
//
// Bodies in GeometryAccum.cpp: Mac TU 053 (0x119920-0x11accf; "BasicArray.cpp" guess from its
// BasicArray<Patch> code, file name inferred from the class), Win 0x5a8580 / 0x5a87e0 (+ template
// code 0x5a8d60-0x5a9ca0). In LDDLib beside its users. Member names inferred.
#pragma once
#include "LEGO/Types.h"
#include "BasicArray.h"
#include "BasicMap.h"
#include "Geometry.h"

namespace LEGO {

class Primitive;
class PrimitiveElement;

struct GeometryAccum {
    // The CPU copies of one uploaded vertex buffer (pointers into the buffer's own arrays) and
    // the culling data of its .g file. 0x4c bytes (both builds). Copied bitwise; the
    // destructor (Mac 0x640b0, Win 0x60b573) frees the arrays it owns.
    struct ImportGeometry {
        ImportGeometry()
            : positions(0), normals(0), texCoords(0), vertexCount(0), roundEdgeData(0),
              roundEdgeDataCount(0), roundEdgeDataSize(0), roundEdgePoints(0),
              roundEdgePointCount(0), roundEdgeDataIndices(0), roundEdgePointIndices(0),
              indices(0), indexCount(0), cullingData(0), cullingCount(0), ownsArrays(false),
              ownsCullingData(false), field40(0), field44(0), field48(0) {}
        ~ImportGeometry()                                            // Mac 0x640b0, Win 0x60b573
        {
            if (ownsArrays) {
                delete[] (UInt8*)positions;
                delete[] (UInt8*)normals;
                delete[] (UInt8*)texCoords;
                delete[] (UInt8*)roundEdgeData;
                delete[] (UInt8*)roundEdgePoints;
                delete[] (UInt8*)roundEdgeDataIndices;
                delete[] (UInt8*)roundEdgePointIndices;
                delete[] (UInt8*)indices;
            }
            if (ownsCullingData && cullingData)
                delete[] cullingData;
        }

        const void*   positions;              // +0x00 VertexBuffer::GetPositions (frame 0)
        const void*   normals;                // +0x04 GetNormals
        const void*   texCoords;              // +0x08 GetTexCoords(set 0), if any
        UInt32        vertexCount;            // +0x0c
        const void*   roundEdgeData;          // +0x10 GetRoundEdgeData
        UInt32        roundEdgeDataCount;     // +0x14
        UInt32        roundEdgeDataSize;      // +0x18 AlignRoundEdgeDataSize(count)
        const void*   roundEdgePoints;        // +0x1c GetRoundEdgePoints
        UInt32        roundEdgePointCount;    // +0x20
        const void*   roundEdgeDataIndices;   // +0x24 GetRoundEdgeDataIndices
        const void*   roundEdgePointIndices;  // +0x28 GetRoundEdgePointIndices
        const UInt32* indices;                // +0x2c GetIndices
        UInt32        indexCount;             // +0x30
        UInt8*        cullingData;            // +0x34 copy of BrickGeometry's culling block
        UInt32        cullingCount;           // +0x38
        bool          ownsArrays;             // +0x3c
        bool          ownsCullingData;        // +0x3d
        // Written by the mesh rebuild (WritePatch / BrickConnectSceneControl 0x4f6a0), not by
        // TU 022:
        UInt32        field40;                // +0x40 stamp of the last merge that copied the
                                              //       round-edge data (BrickConnectSceneManager
                                              //       import stamp, NextImportStamp)
        UInt32        field44;                // +0x44 index of its first round-edge point there
        UInt32        field48;                // +0x48 index of its first round-edge data word there
    };

    // One primitive and an ImportGeometry per geometry face (max(decoration faces, 1)).
    // 0x14 bytes Mac (BasicMap<unsigned long, ImportPrimitive> node value).
    struct ImportPrimitive {
        ImportPrimitive() : primitive(0) {}
        const Primitive*           primitive;    // +0x00
        BasicArray<ImportGeometry> geometries;   // +0x04 (vtable Mac 0x761468)
    };
    typedef BasicMap<UInt32, ImportPrimitive> ImportPrimitiveMap;   // design id -> (BCSM +0x1c0)

    // A range of one ImportGeometry placed in the body: 0x74 bytes (both builds; copied
    // member-wise by the BasicArray<Patch> code, Mac 0x11a660 / 0x11ab80).
    struct Patch {
        Patch()
            : positions(0), normals(0), texCoords(0), vertexCount(0), roundEdgeDataIndices(0),
              roundEdgePointIndices(0), indices(0), indexCount(0), baseVertex(0), flags(0),
              face(0), element(0), geometry(0) {}
        const Vector3*  positions;              // +0x00
        const Vector3*  normals;                // +0x04 (0: none)
        const Vector2*  texCoords;              // +0x08 (0: none)
        UInt32          vertexCount;            // +0x0c
        const UInt32*   roundEdgeDataIndices;   // +0x10 (0: none)
        const UInt32*   roundEdgePointIndices;  // +0x14 (0: none)
        const UInt32*   indices;                // +0x18
        UInt32          indexCount;             // +0x1c
        UInt32          baseVertex;             // +0x20 first vertex (subtracted from the indices)
        UInt32          flags;                  // +0x24 culling record type bits (2: whole geometry)
        UInt32          face;                   // +0x28 geometry face (0 for the whole geometry)
        const PrimitiveElement* element;        // +0x2c
        Matrix3D        transform;              // +0x30 element -> body
        ImportGeometry* geometry;               // +0x70 source (its round-edge data is shared)
    };

    // The patches of one material / decoration and their totals. 0x18 bytes Mac (map value).
    struct AccumulatedPatch {
        AccumulatedPatch() : vertexCount(0), indexCount(0) {}
        UInt32            vertexCount;   // +0x00
        UInt32            indexCount;    // +0x04
        BasicArray<Patch> patches;       // +0x08 (vtable Mac 0x760b98)
    };
    typedef BasicMap<UInt32, AccumulatedPatch> PatchMap;   // Mac vtable 0x760b7c

    // Write cursors of one merged vertex buffer (13 words, Mac 0x4f6a0 [ebp-0x114]); every
    // pointer advances as WritePatch writes. (names inferred)
    struct Output {
        Output()
            : positions(0), normals(0), texCoords(0), roundEdgeData(0), roundEdgePoints(0),
              roundEdgeDataIndices(0), unused18(0), roundEdgeDataBase(0), roundEdgePointIndices(0),
              unused24(0), roundEdgePointBase(0), indices(0), vertexBase(0) {}
        Vector3* positions;              // +0x00
        Vector3* normals;                // +0x04 (0: none)
        Vector2* texCoords;              // +0x08 (0: none)
        UInt32*  roundEdgeData;          // +0x0c (0: no round edges)
        Vector3* roundEdgePoints;        // +0x10 (0: no round edges)
        UInt32*  roundEdgeDataIndices;   // +0x14
        UInt32   unused18;               // +0x18 (always 0)
        UInt32   roundEdgeDataBase;      // +0x1c data words written so far
        UInt32*  roundEdgePointIndices;  // +0x20
        UInt32   unused24;               // +0x24 (always 0)
        UInt32   roundEdgePointBase;     // +0x28 points written so far
        UInt32*  indices;                // +0x2c
        UInt32   vertexBase;             // +0x30 vertices written so far
    };

    // ---- data: sizeof 0x38 Mac (two BasicMaps; built on the stack by Mac 0x46560) ----------
    PatchMap m_materials;     // +0x00 material id -> patches
    PatchMap m_decorations;   // +0x1c decoration id -> patches (the decorated faces again)

    // Adds the geometry of `element` placed at `transform` (element -> body): per geometry face
    // of its primitive (max(decoration faces, 1); the primitive must be in `imports`, else
    // nothing) the patches of the visible culling records whose type is in `flags` (bit 30 is
    // passed to ConnectionField::GetConnectionState; bit 31 set: no connection tests) into
    // m_materials[part material of the face] and, for a decorated face > 0, into
    // m_decorations[decoration]. Geometry without culling data is added whole when `flags` has
    // bit 1. Nothing for an element with m_flagAC. Mac 0x119f10, Win 0x5a87e0
    static void AddElement(ImportPrimitiveMap& imports, const PrimitiveElement* element,
                           UInt32 flags, const Matrix3D& transform, GeometryAccum& accum);
    // The culling record types of the shipped .g files (from their use in all 5,064 LOD0 files):
    // 1 a stud (one record each), 2 the outer shell, 4 the underside (with a flat replacement
    // that closes it), 8 a bottom tube. (LDD64)
    enum { kRecordStud = 1, kRecordShell = 2, kRecordUnderside = 4, kRecordTube = 8 };
    // AddElement flag (LDD64, level of detail): no studs and tubes, every other record without
    // connection tests, as its replacement where it has one (the closed underside).
    enum { kFlagFarDetail = 0x20000000 };

    // Appends `patch` to `out`: indices rebased to out.vertexBase, positions transformed and
    // normals rotated by patch.transform, texture coordinates copied when both have them; with
    // round edges (out.roundEdgePoints and out.roundEdgeData) the source geometry's round-edge
    // points and data are copied once per `stamp` (ImportGeometry +0x40..+0x48) and the
    // patch's round-edge indices rebased to them. Mac 0x119920, Win 0x5a8580
    static void WritePatch(const Patch& patch, Output& out, UInt32 stamp);
};

} // namespace LEGO
