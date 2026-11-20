// LEGO Digital Designer — restored source
// GeometryAccum (Mac TU 053, 0x119920-0x11accf; Win 0x5a8580-0x5a9ca0). See GeometryAccum.h.
// Template code of the TU: BasicArray<Patch>::PushBack 0x11a660, PatchMap::operator[] 0x11a810
// (+ map insert helpers 0x11a900 / 0x11aac0 / node copy 0x11ab80), the TU initialiser 0x11ac90;
// BrickConnectSceneManager's import stamp increment 0x11a800 (NextImportStamp).
#include "GeometryAccum.h"
#include "PrimitiveElement.h"
#include "Primitive.h"
#include "Part.h"
#include "ConnectionField.h"

#include <string.h>

namespace LEGO {

namespace {

// The culling records of a .g file (BrickGeometry kCulling, copied into ImportGeometry
// +0x34 / +0x38): `count` records, each starting with its byte size. (layout from the use here)
//   +0x00 size            +0x04 type bits (tested against the flags)
//   +0x08 first vertex    +0x0c vertex count
//   +0x10 first index     +0x14 index count
//   +0x18 offset of the replacement geometry (0: none) — used when partly hidden:
//           {UInt32 vertexCount, UInt32 indexCount, positions[vc], normals[vc] (if the
//            geometry has normals), texture coordinates[vc] (if it has them), indices[ic],
//            round-edge point indices[ic], round-edge data indices[ic]}
//   +0x1c number of test groups, then from +0x20 the groups:
//           {UInt32 size, UInt32 test count, tests...}, each test {UInt32 size,
//            UInt32 field index, UInt32 count, UInt32 data[]} -> GetConnectionState of
//            the element's field.
// Visibility of a record: per group the largest test result (stop at 2), over the groups the
// smallest (stop at 0): 0 hidden, 1 the replacement geometry, 2 the whole range.
inline const UInt32* Words(const UInt8* p) { return reinterpret_cast<const UInt32*>(p); }

} // namespace

// Mac 0x119f10, Win 0x5a87e0
void GeometryAccum::AddElement(ImportPrimitiveMap& imports, const PrimitiveElement* element,
                               UInt32 flags, const Matrix3D& transform, GeometryAccum& accum)
{
    if (element->m_flagAC)
        return;
    ImportPrimitiveMap::Iterator it = imports.Find(element->m_primitive->GetDesignID());
    if (it == imports.End())
        return;
    ImportPrimitive& import = it->second;
    const UInt32 faceCount = import.primitive->GetDecorationFaceCount() != 0
                                 ? import.primitive->GetDecorationFaceCount() : 1;
    const bool any = ((flags >> 30) & 1) != 0;
    for (UInt32 face = 0; face < faceCount; ++face) {
        ImportGeometry& geometry = import.geometries[face];
        AccumulatedPatch& materialPatches = accum.m_materials[element->m_part->GetMaterial(face)];
        AccumulatedPatch* decorationPatches = 0;
        if (face != 0 && import.primitive->GetDecorationFaceCount() != 0) {
            UInt32 decoration = element->m_part->GetDecoration(face);
            if (decoration != 0)
                decorationPatches = &accum.m_decorations[decoration];
        }

        if (geometry.cullingData == 0) {
            if ((flags & 2) == 0)
                continue;
            // the whole geometry
            Patch patch;
            patch.element = element;
            patch.transform = Matrix3D(transform);
            patch.geometry = &geometry;
            patch.positions = (const Vector3*)geometry.positions;
            patch.normals = (const Vector3*)geometry.normals;
            // NOTE: tests the patch's own (still null) round-edge index pointers, so whole
            // geometries never get round-edge indices (both builds; every shipped LOD0 .g has
            // culling data).
            if (patch.roundEdgePointIndices != 0 && patch.roundEdgeDataIndices != 0) {
                patch.roundEdgePointIndices = (const UInt32*)geometry.roundEdgePointIndices;
                patch.roundEdgeDataIndices = (const UInt32*)geometry.roundEdgeDataIndices;
            }
            patch.texCoords = (const Vector2*)geometry.texCoords;
            patch.vertexCount = geometry.vertexCount;
            patch.indices = geometry.indices;
            patch.indexCount = geometry.indexCount;
            patch.baseVertex = 0;
            patch.flags = 2;
            patch.face = 0;
            materialPatches.vertexCount += patch.vertexCount;
            materialPatches.indexCount += patch.indexCount;
            materialPatches.patches.PushBack(patch);
            if (decorationPatches != 0) {
                decorationPatches->vertexCount += patch.vertexCount;
                decorationPatches->indexCount += patch.indexCount;
                decorationPatches->patches.PushBack(patch);
            }
            continue;
        }

        const UInt8* record = geometry.cullingData;
        for (UInt32 n = geometry.cullingCount; n != 0; --n, record += Words(record)[0]) {
            const UInt32* r = Words(record);
            if ((r[1] & flags) == 0)
                continue;
            Int32 visibility = 2;
            if ((flags & kFlagFarDetail) != 0) {
                // level of detail (LDD64): without studs and tubes, the underside closed
                if ((r[1] & (kRecordStud | kRecordTube)) != 0)
                    continue;
                if (r[6] != 0)
                    visibility = 1;
            } else
            if ((Int32)flags >= 0 && r[7] != 0) {
                const UInt8* group = record + 0x20;
                for (UInt32 g = r[7]; g != 0; --g, group += Words(group)[0]) {
                    UInt32 tests = Words(group)[1];
                    Int32 best = 0;
                    const UInt8* test = group + 8;
                    for (; tests != 0; --tests, test += Words(test)[0]) {
                        const UInt32* t = Words(test);
                        const ConnectionField* field = element->m_fields[t[1]];
                        Int32 state = (Int32)field->GetConnectionState(any, t[2], t + 3);
                        if (state > best) {
                            best = state;
                            if (state == 2)
                                break;
                        }
                    }
                    if (best < visibility) {
                        visibility = best;
                        if (best == 0)
                            break;
                    }
                }
            }
            Patch patch;
            patch.element = element;
            patch.transform = Matrix3D(transform);
            patch.geometry = &geometry;
            if (visibility == 0)
                continue;                              // hidden (an empty patch, not added)
            if (visibility == 1 && r[6] != 0) {
                // the record's replacement geometry
                const UInt32* replacement = Words(record + r[6]);
                const UInt32 vc = replacement[0];
                const UInt32 ic = replacement[1];
                const UInt8* data = record + r[6] + 8;
                const UInt8* afterPositions = data + vc * 12;
                const UInt8* afterNormals = afterPositions + (geometry.normals ? vc * 12 : 0);
                const UInt8* afterTexCoords = afterNormals + (geometry.texCoords ? vc * 8 : 0);
                patch.positions = (const Vector3*)data;
                patch.normals = geometry.normals ? (const Vector3*)afterPositions : 0;
                patch.texCoords = geometry.texCoords ? (const Vector2*)afterNormals : 0;
                patch.roundEdgePointIndices =
                    geometry.roundEdgePointIndices ? (const UInt32*)(afterTexCoords + ic * 4) : 0;
                patch.roundEdgeDataIndices =
                    geometry.roundEdgeDataIndices ? (const UInt32*)(afterTexCoords + ic * 8) : 0;
                patch.vertexCount = vc;
                patch.indices = (const UInt32*)afterTexCoords;
                patch.indexCount = ic;
                patch.baseVertex = 0;
                patch.flags = r[1];
                patch.face = face;
            } else {
                // the record's range of the geometry
                const UInt32 first = r[2];
                patch.positions = (const Vector3*)geometry.positions + first;
                patch.normals = geometry.normals ? (const Vector3*)geometry.normals + first : 0;
                patch.texCoords = geometry.texCoords ? (const Vector2*)geometry.texCoords + first : 0;
                patch.vertexCount = r[3];
                if (geometry.roundEdgePointIndices != 0 && geometry.roundEdgeDataIndices != 0) {
                    patch.roundEdgePointIndices = (const UInt32*)geometry.roundEdgePointIndices + r[4];
                    patch.roundEdgeDataIndices = (const UInt32*)geometry.roundEdgeDataIndices + r[4];
                }
                patch.indices = geometry.indices + r[4];
                patch.indexCount = r[5];
                patch.baseVertex = first;
                patch.flags = r[1];
                patch.face = face;
            }
            if (patch.vertexCount == 0)
                continue;
            materialPatches.vertexCount += patch.vertexCount;
            materialPatches.indexCount += patch.indexCount;
            materialPatches.patches.PushBack(patch);
            if (decorationPatches != 0) {
                decorationPatches->vertexCount += patch.vertexCount;
                decorationPatches->indexCount += patch.indexCount;
                decorationPatches->patches.PushBack(patch);
            }
        }
    }
}

// Mac 0x119920, Win 0x5a8580
void GeometryAccum::WritePatch(const Patch& patch, Output& out, UInt32 stamp)
{
    // indices, rebased to the vertices written so far
    if (patch.indexCount != 0) {
        for (UInt32 i = 0; i < patch.indexCount; ++i)
            out.indices[i] = patch.indices[i] - patch.baseVertex + out.vertexBase;
        out.indices += patch.indexCount;
    }
    out.vertexBase += patch.vertexCount;

    for (UInt32 i = 0; i < patch.vertexCount; ++i) {
        *out.positions = patch.positions[i] * patch.transform;
        ++out.positions;
    }
    if (out.normals != 0) {
        for (UInt32 i = 0; i < patch.vertexCount; ++i) {
            *out.normals = Rotate(patch.normals[i], patch.transform);
            ++out.normals;
        }
    }
    if (out.texCoords != 0) {
        for (UInt32 i = 0; i < patch.vertexCount; ++i) {
            *out.texCoords = patch.texCoords[i];
            ++out.texCoords;
        }
    }

    if (out.roundEdgePoints == 0 || out.roundEdgeData == 0)
        return;
    ImportGeometry* geometry = patch.geometry;
    if (geometry->field40 != stamp) {
        // first patch of this geometry in this merge: its round-edge points and data
        memcpy((void*)out.roundEdgePoints, geometry->roundEdgePoints, geometry->roundEdgePointCount * 12);
        out.roundEdgePoints += geometry->roundEdgePointCount;
        geometry->field44 = out.roundEdgePointBase;
        out.roundEdgePointBase += geometry->roundEdgePointCount;
        memcpy(out.roundEdgeData, geometry->roundEdgeData, geometry->roundEdgeDataCount * 4);
        out.roundEdgeData += geometry->roundEdgeDataSize;
        geometry->field48 = out.roundEdgeDataBase;
        out.roundEdgeDataBase += geometry->roundEdgeDataSize;
        geometry->field40 = stamp;
    }
    if (patch.indexCount == 0)
        return;
    for (UInt32 i = 0; i < patch.indexCount; ++i)
        out.roundEdgeDataIndices[i] = patch.roundEdgeDataIndices[i] + geometry->field48;
    out.roundEdgeDataIndices += patch.indexCount;
    for (UInt32 i = 0; i < patch.indexCount; ++i)
        out.roundEdgePointIndices[i] = patch.roundEdgePointIndices[i] + geometry->field44;
    out.roundEdgePointIndices += patch.indexCount;
}

} // namespace LEGO
