// Vortex 5.0.0 build 800 "vortexldd41" - the Vinc error table and reporter (restored;
// Mac 557c 0x4c16b7, table Mac 0x779560 / Win 0xac9f30, identical in both builds).
#include "Vinc/VincIntersect.h"
#include "Me/MeMessage.h"

#include <stdlib.h>
#include <string.h>

extern "C" {

// {id, type (0 fatal, 1 warning, 2 info), count, -, message}. NOTE: the messages are passed to
// MeWarning/MeInfo/MeFatalError as the format string (id 23 contains "%s" without an argument);
// kept as in the original.
VincErrorEntry VincErrorTable[43] = {
    {0, 0, 5, 0, ""},
    {1, 1, 5, 0, ""},
    {2, 0, 5, 0, "Your evaluation version of the Collision library has timed out.\nPlease contact Critical Mass Labs for a full development version.\n"},
    {3, 0, 5, 0, "Function called with null pointer as argument "},
    {4, 0, 5, 0, "object not of correct derived type: expecting object of type "},
    {5, 0, 5, 0, "Allocation failed for object of type: "},
    {6, 0, 5, 0, "Assertion failed: "},
    {7, 1, 5, 0, "Invalid use of VincModelSetTransform(),: This VincModel's transform is automatically updated from a VindBody transform "},
    {8, 0, 5, 0, "Invalid VincModelID."},
    {9, 0, 5, 0, "Invalid VincSpaceID."},
    {10, 0, 5, 0, "Invalid VincGeometryID."},
    {11, 0, 5, 0, "Invalid VincSpacePairID."},
    {12, 0, 5, 0, "Invalid VincModelPair pointer."},
    {13, 0, 5, 0, "Invalid VincIntersectResult pointer."},
    {14, 0, 5, 0, "VincModelPair contains null model pointer(s),."},
    {15, 0, 5, 0, "Invalid VincDtBridgeID."},
    {16, 0, 5, 0, "Invalid VincRwWorldID."},
    {17, 0, 5, 0, "Invalid VincRwBSPID."},
    {18, 0, 5, 0, "Expected non null transform pointer."},
    {19, 0, 5, 0, "SpaceUpdate called on unbuilt space.  Invoke SpaceBuild(),"},
    {20, 0, 5, 0, "Attempt to SetBody(), without Initializing dynamics bridge (BridgeInit(),),"},
    {21, 0, 5, 0, "Failed to register new geometry type. Ensure sufficient number of geometries in VincInit()."},
    {22, 0, 5, 0, "Invalid Intersect Result. Output contact array pointer must exist."},
    {23, 0, 5, 0, "Invalid Intersect Request (%s),."},
    {24, 0, 5, 0, "Out of memory for VincModelPairs in VincSpace. Resizing not implemented. Please use a larger number of pairs in VincAxisSortCreate."},
    {25, 0, 5, 0, "Out of memory for VincModels in VincSpace. Resizing not implemented. Please use a larger number of models in VincAxisSortCreate."},
    {26, 1, 5, 0, "Ran out of memory for PairData objects."},
    {27, 1, 5, 0, "Internal error: Ran out of contact in pool, "},
    {28, 1, 5, 0, "TriangleMesh has no triangles, cannot build triangle mesh."},
    {29, 0, 5, 0, "TriangleMesh is not built - the function VincTriangleMeshBuild(mesh), shoud be called."},
    {30, 1, 5, 0, "No intersection function available for this pair combination. Pair will be ignored. This could also be due to an invalid Request for the given pair: "},
    {31, 1, 5, 0, "Invalid Request "},
    {32, 1, 5, 0, "Attempt to register an interaction involving an unregistered geometry type. Interaction ignored. Pair combination was "},
    {33, 1, 5, 0, "Attempt to find an interaction involving an unregistered geometry type. Empty interaction will be used for debug version, but release version will produce undefined behaviour."},
    {34, 1, 5, 0, "Unimplemented case in VincSafeTime in "},
    {35, 1, 5, 0, "Attempt to register an interaction involving an invalid geometry type. Failure to register geometry type before registering interactions?"},
    {36, 1, 5, 0, "Attempt to use a state-modification function while VincSpaceIsChanging() is false. Call VincSpaceBeginChanges() first. Function name: "},
    {37, 1, 5, 0, "Attempt to use a state-query function while VincSpaceIsChanging() is true. Call VincSpaceEndChanges() first. Function name: "},
    {38, 1, 5, 0, "VincModelDestroy: model being destroyed while still in an VincSpace."},
    {39, 1, 5, 0, "VincBoxBox: internal error due to invalid transform."},
    {40, 1, 5, 0, "VincGeometryGetMassProperties: Inertia tensor matrix is non-symmetric."},
    {41, 1, 5, 0, "VincTerm: VincVincFramework already terminated."},
    {42, 0, 5, 0, "No error"},
};

// Mac 0x4c16b7, Win 0x7feea0 (name inferred)
void VincError(VincErrorEntry* table, int id, const char* extra)
{
    while (table->id != 42 && id != table->id)
        ++table;
    char text[512];
    text[0] = 0;
    memset(text + 1, 0, sizeof(text) - 1);
    const char* message = "";
    if ((unsigned int)(id - 1) < 0x29)
        message = table->message;
    if (table->count > 0) {
        strcpy(text, "Vinc: ");
        strcat(text, message);
        strcat(text, extra);
        // NOTE: count is never decremented (all entries are 5): this text never appears
        if (table->count == 1)
            strcat(text, "\n[This message will not be repeated again!]");
        strcat(text, "\n");
        switch (table->type) {
        case 1:
            MeWarning(0, text);
            break;
        case 2:
            MeInfo(0, text);
            break;
        case 0:
            MeFatalError(0, text);
            abort();
        }
    }
}

} // extern "C"
