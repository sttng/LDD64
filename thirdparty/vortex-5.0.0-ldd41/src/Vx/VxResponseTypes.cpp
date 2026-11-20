// Vortex 5.0.0 build 800 "vortexldd41" - the out-of-line destructors of the collision response
// handlers (restored). Mac TU 515 0x477fa8-0x478500 (file guess: the TU of
// VxResponseStart_Sensor_Sensor; it also carries the MdtGetMaxConstraintSize header static,
// initialiser 0x4785c2, and coalesced copies of inline Mdt grouping getters 0x478500-0x4785b6
// that VxUniverse's print uses - V4.1). Win: one shared scalar deleting dtor 0x70c810 (ICF).
#include "Vx/VxResponse.h"

namespace Vx {

VxResponseStart_Sensor_Sensor::~VxResponseStart_Sensor_Sensor() {} // Mac D1 0x47841c, D0 0x477fa8
VxResponseEndBody_Body::~VxResponseEndBody_Body() {}               // Mac D1 0x47809c, D0 0x478022
VxResponseBody_Body::~VxResponseBody_Body() {}                     // Mac D1 0x478180, D0 0x478106
VxResponseStartBody_Body::~VxResponseStartBody_Body() {}           // Mac D1 0x4781ea, D0 0x4782ce
VxResponseStart_Sensor_Body::~VxResponseStart_Sensor_Body() {}     // Mac D1 0x4783b2, D0 0x478254
VxResponseStart_Sensor_PS::~VxResponseStart_Sensor_PS() {}         // Mac D1 0x478348, D0 0x478486

} // namespace Vx
