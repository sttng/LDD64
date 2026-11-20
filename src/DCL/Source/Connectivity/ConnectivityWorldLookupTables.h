// LEGO Digital Designer — restored source
// LEGO::ConnectivityWorldLookupTables — static lookup tables built from the loaded brick
// database: assembly wires (annotation "wire") and primitive wire markers ("wireMarker"). The
// object fills them and clears them when destroyed.
// TU "ConnectivityWorldLookupTables.cpp": Mac TU 410, 0x38e8b0-0x38ff51; Win 0x4d0bf0-0x4d1650
// (+ the std::map helpers up to 0x4d2470). Class name from RTTI; member names inferred.
//
// Layout (Mac): +0x00 vptr, +0x04 BrickDatabase* m_database, +0x08 virtual base Object.
// Mac vtable 0x75743c: D1, D0, AddAssembly. Win vtable: AddAssembly (destructor in Object).
//
// The "assemblies" are the bricks of the database (BrickDatabase::Brick: the Mac code reads
// the id at +0x00, the parts at +0x04 (20 bytes each, design id first) and the annotation map
// at +0x5c / +0x60), walked with BrickDatabase::GetBrickIDs / GetBrick; bricks with fewer than
// two parts are skipped.
#pragma once
#include "Object.h"
#include "Geometry.h"
#include "BrickDatabase.h"

#include <map>
#include <vector>

namespace LEGO {

class ConnectivityWorldLookupTables : public virtual Object {
public:
    typedef std::vector<UInt32> WireList;

    // Runs AddAssembly for every assembly of `database`.                  Mac 0x38ea20 (Win inlined)
    explicit ConnectivityWorldLookupTables(BrickDatabase* database);
    // Clears both tables.          Mac 0x38eb10 / 0x38ecc0 (+ thunks 0x38ebe0, 0x38edb0), Win 0x4d0c30 / 0x4d0bf0
    virtual ~ConnectivityWorldLookupTables();

    // Assemblies of two or more parts with a "wire" annotation (case-insensitive key; value: a
    // list of UInt32, ended by the first 0 or parse error) get a wire list; then the wire
    // markers of their parts are read (ProcessWireMarkers).            Mac 0x38edd0, Win 0x4d0ca0
    virtual void AddAssembly(const BrickDatabase::Brick& assembly);

    // Wire list of an assembly, NULL if it has none.                     Mac 0x38e8f0 (Win inlined)
    static const WireList* GetWires(UInt32 assemblyID);
    // Wire marker of a primitive ("wireMarker": yaw pitch roll in degrees, tx ty tz).
    //                                                                      Mac 0x38e960 (Win inlined)
    static bool GetWireMarker(UInt32 designID, Matrix3D& marker);

private:
    // Parses the "wireMarker" annotation of the primitive of every part.
    //                                                                      Mac 0x38f000, Win 0x4d0ed0
    void ProcessWireMarkers(const BrickDatabase::Brick& assembly);

    BrickDatabase* m_database;      // +0x04

    static std::map<UInt32, WireList> s_wires;          // Mac 0x77f454, Win 0xb43970
    static std::map<UInt32, Matrix3D> s_wireMarkers;    // Mac 0x77f46c, Win 0xb43944
};

} // namespace LEGO
