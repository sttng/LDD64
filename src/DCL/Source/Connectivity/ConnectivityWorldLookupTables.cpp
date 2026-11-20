// LEGO Digital Designer — restored source
// TU "ConnectivityWorldLookupTables.cpp"  Mac TU 410, 0x38e8b0-0x38ff51 (20 functions: the class,
// two lookups and the std::map / std::vector helpers 0x38f6a0-0x38fe50; initialiser 0x38fe50
// constructs the two maps). Win 0x4d0bf0-0x4d1650 (+ map helpers to 0x4d2470).
// See ConnectivityWorldLookupTables.h.
#include "ConnectivityWorldLookupTables.h"
#include "StringStream.h"

namespace LEGO {

std::map<UInt32, ConnectivityWorldLookupTables::WireList> ConnectivityWorldLookupTables::s_wires;
std::map<UInt32, Matrix3D> ConnectivityWorldLookupTables::s_wireMarkers;

// Mac 0x38ea20 (Win: inlined at the caller)
ConnectivityWorldLookupTables::ConnectivityWorldLookupTables(BrickDatabase* database)
    : m_database(database)
{
    BasicArray<UInt32> ids;
    database->GetBrickIDs(ids);
    for (const UInt32* id = ids.Begin(); id != ids.End(); ++id)
        AddAssembly(*database->GetBrick(*id));
}

// Mac 0x38eb10 (D1), 0x38ecc0 (D0); Win 0x4d0c30 (body), 0x4d0bf0 (deleting)
ConnectivityWorldLookupTables::~ConnectivityWorldLookupTables()
{
    s_wires.clear();
    s_wireMarkers.clear();
}

// Mac 0x38e8f0
const ConnectivityWorldLookupTables::WireList* ConnectivityWorldLookupTables::GetWires(UInt32 assemblyID)
{
    std::map<UInt32, WireList>::const_iterator it = s_wires.find(assemblyID);
    if (it == s_wires.end())
        return 0;
    return &it->second;
}

// Mac 0x38e960
bool ConnectivityWorldLookupTables::GetWireMarker(UInt32 designID, Matrix3D& marker)
{
    std::map<UInt32, Matrix3D>::const_iterator it = s_wireMarkers.find(designID);
    if (it == s_wireMarkers.end())
        return false;
    marker = it->second;
    return true;
}

// Mac 0x38edd0, Win 0x4d0ca0
void ConnectivityWorldLookupTables::AddAssembly(const BrickDatabase::Brick& assembly)
{
    if (assembly.m_parts.GetSize() < 2)
        return;
    const BrickDatabase::AnnotationMap& annotations = assembly.m_annotations;
    BrickDatabase::AnnotationMap::ConstIterator annotation = annotations.Begin();
    for (;; ++annotation) {
        if (annotation == annotations.End())
            return;
        if (annotation->first.CompareNoCase(String("wire")) == 0)
            break;
    }

    WireList& wires = s_wires[assembly.m_designID];
    UInt32 position = 0;
    UInt32 value = 0;
    while (FromString(value, annotation->second, position) && value != 0) {
        wires.push_back(value);
        ++position;
    }
    ProcessWireMarkers(assembly);
}

// Mac 0x38f000, Win 0x4d0ed0
// "yaw pitch roll tx ty tz" (degrees). Only the first "wireMarker" annotation of a primitive is
// used.
// NOTE: the error messages are built and dropped (both builds; the trace output they were made
// for is compiled out). A primitive missing from the database is not checked for (both builds).
void ConnectivityWorldLookupTables::ProcessWireMarkers(const BrickDatabase::Brick& assembly)
{
    static const Real32 DEGREES_TO_RADIANS = 0.017453292f;
    for (UInt32 part = 0; part < assembly.m_parts.GetSize(); ++part) {
        UInt32 designID = assembly.m_parts[part].m_designID;
        const Primitive* primitive = m_database->GetPrimitive(designID);
        const Primitive::AnnotationMap& annotations = primitive->GetAnnotations();
        for (Primitive::AnnotationMap::ConstIterator it = annotations.Begin(); it != annotations.End(); ++it) {
            if (!(it->first == String("wireMarker")))
                continue;

            String text(it->second);
            Real32 yaw;
            Real32 pitch;
            Real32 roll;
            Vector3 translation;
            UInt32 position = 0;
            const char* error = 0;
            if (!FromString(yaw, text, position)) {
                error = "Error decoding yaw part wire marker for part: ";
            } else if (++position, !FromString(pitch, text, position)) {
                error = "Error decoding pitch part wire marker for part: ";
            } else if (++position, !FromString(roll, text, position)) {
                error = "Error decoding roll part wire marker for part: ";
            } else if (++position, !FromString(translation.x, text, position)) {
                error = "Error decoding translation.x part wire marker for part: ";
            } else if (++position, !FromString(translation.y, text, position)) {
                error = "Error decoding translation.y part wire marker for part: ";
            } else if (++position, !FromString(translation.z, text, position)) {
                error = "Error decoding translation.z part wire marker for part: ";
            }
            if (error) {
                String message(error);
                message << designID << ", data is '" << text << "'";
                break;
            }
            Matrix3D marker;
            // NOTE: Matrix3D::SetEulerAngles takes (pitch, yaw, roll); the values are passed in
            // the order of the annotation (both builds).
            marker.SetEulerAngles(yaw * DEGREES_TO_RADIANS, pitch * DEGREES_TO_RADIANS,
                                  roll * DEGREES_TO_RADIANS);
            marker.SetTranslation(translation);
            s_wireMarkers[designID] = marker;
            break;
        }
    }
}

} // namespace LEGO
