// Vortex 5.0.0 build 800 "vortexldd41" - name-path string constants (restored).
// Mac TU 460 0x43249e-0x43271a: a TU whose only code is the static initialiser of three
// std::string constants (Mac 0x77cdcc "/", 0x77cdd0 "base://", 0x77cdd4 "null") and their
// destructors. Their users are dead-stripped on Mac; they are exported here for the name-path
// code. Names inferred.
#ifndef VX_VXNAMEPATHSTRINGS_H
#define VX_VXNAMEPATHSTRINGS_H

#include <string>

namespace Vx {

extern const std::string VxNamePathSeparator; // Mac 0x77cdcc: "/"
extern const std::string VxNamePathBase;      // Mac 0x77cdd0: "base://"
extern const std::string VxNamePathNull;      // Mac 0x77cdd4: "null"

} // namespace Vx

#endif
