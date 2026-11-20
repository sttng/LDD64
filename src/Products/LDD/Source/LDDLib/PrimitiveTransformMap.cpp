// LEGO Digital Designer — restored source
// TU 374 (Mac 0x362e30-0x362ec9; file name unknown, "PrimitiveTransformMap.cpp" inferred from
// its only content; no Win twin found): the static initialiser of an empty namespace-scope
// BasicMap<PrimitiveElement*, Matrix3D> (Mac 0x77f034, map vtable 0x778268; destroyed at
// exit) and <iostream>. Nothing in either build references the object.
#include <iostream>

#include "BasicMap.h"
#include "Geometry.h"

namespace LEGO {

class PrimitiveElement;

namespace {

// Mac 0x77f034 (initialiser 0x362e30). (name inferred)
BasicMap<PrimitiveElement*, Matrix3D> s_primitiveTransforms;

} // namespace

} // namespace LEGO
