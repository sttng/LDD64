# CM Labs Vortex 5.0.0 build 800 "vortexldd41" (restored)

The CM Labs Vortex Toolkit 5.0.0 (build 800), custom build `vortexldd41`, as statically linked
into LDD 4.3.11 on both platforms. **Not CM Labs source code**: Vortex is proprietary, and no
source or SDK headers were available. Everything here is reconstructed from the two LDD binaries
(Mac: Apple GCC 4.x i386, prebuilt by CM Labs; Windows: MSVC 2008 /LTCG).

Identification: `"5.0.0 (build 800)"` in the Me/Vx foundation strings of both binaries; the
Windows `__FILE__` `c:\work\martinc\vortexldd41\vortex\mdt\src\mdtworld.cpp`.

LDD64 builds it with Boost 1.92 (in place of Vortex's private Boost 1.42 copy, `boostVx`) and
`thirdparty/qhull-2020.2-vortex` (qhull with the Vortex changes).

## Layout

Directory names follow the source tree implied by the binaries (`vortex\mdt\src\…`, class and
function prefixes).

| Path | Content |
|---|---|
| `include/<Module>/` | public headers (`Me`, `Vx`, `VxData`, `Mdt`, `Kea`, `Vinc`, `Cx`), one per class family |
| `include/Vx/VxPlatform.h` | `VxReal` = double, `VxReal3/4/4x4`, `VX_STATIC_ASSERT` |
| `include/Mdt/MdtTypes.h`, `include/Vinc/VincTypes.h` | Mdt / Vinc handles and the struct layouts known from both builds |
| `include/{Vx,Kea,Cx}/detail/*RttiSkeletons.h` | generated skeletons of the RTTI classes without a header of their own (bases, `vfNN` slots with Mac / Windows addresses) |
| `src/<Module>/` | the sources, one file per original source where known; private headers next to them |

The modules: **Me** (C, the MathEngine heritage: memory, messages, pools, math, profiler; on
Windows every non-variadic Me function is `__stdcall`, `MEAPI`), **Vx** (the object layer: bases,
parts, constraints, universe, collision geometries), **VxData** (fields), **Mdt** / **Kea**
(dynamics and solver), **Vinc** / **Cx** (collision: geometries, pair tests, the broad phase).
`Me/MeCrtMath.h` reproduces the VS2008 C library's sin / cos / asin / acos / atan2 bit for bit on
Windows; include it after `<math.h>` where they are called.

## LDD64 changes

Large models (all with the same states, pair lists and results as the original):
- **Broad phase** (`Cx/CxSmallSort`): no table of 4 bits for every pair of objects (2 GB at
  131,072 objects) and no limit of 65,536 objects per space. A pair's overlap count comes from the
  order of the endpoints; disabled pairs are kept as ranges of partner handles; the rest in a small
  hash table ("Pair states without the table" in `src/Cx/CxSmallSort.cpp`). Search trees over the
  axis lists replace the walk over a whole list when an object is sorted in ("sortIn without the
  walk").
- **Jumps**: a carried group leaves the axis lists and is sorted back in at its new bounds
  (`CxSmallSort::takeOut` / `putBack`, `VincSpaceTakeOutModels` / `VincSpacePutBackModels`); a
  single object that would pass many endpoints is moved the same way
  (`VincSpaceSetRelocateCrossings`, used while LDD loads or pastes).
- **Pairs**: `CxSmallSort::disablePairs` (`VincSpaceDisablePairs`) disables a model's pairs with
  the rest of its part in one call; pair groups (`groupNew` / `groupJoin` / `groupLeave`,
  `VxCollisionSensor::setPairGroup`) let a member join and leave with its pairs' records;
  `VxPart::addCollisionGeometry` disables the new geometry's pairs once instead of twice.
- **Memory**: a space allocates its pair-state table and pair nodes when it first needs them;
  collision geometries share equal relative transforms and take composed ones from a pool;
  growing `VinPool`s add blocks of at most 4 MB.

## Conventions

- **Offsets** are `Mac +0xNN, Win +0xNN`. Mac i386: doubles 4-aligned in structs, a 4-byte
  `std::string`. Windows MSVC 2008: doubles 8-aligned, a 28-byte `std::string`. Members are
  typed, so the compiler reproduces both layouts; regions not yet understood are typed
  placeholders (`m3c`, `void* m58`).
- **Slots**: `slot N` is the Mac (Itanium) index; Windows has the same up to the destructor, then
  one less (Mac D1 + D0, Windows one scalar-deleting destructor). `vfNN` is a slot of unknown name.
- **Names** found as strings in either binary are used as they are; others follow the Vortex /
  MathEngine conventions and are marked `(name inferred)`.
- Every declaration carries its addresses: `Mac 0x…`, `Win 0x…`.
- Classes allocate through their own `operator new` / `delete` (`VX_DECLARE_ALLOCATOR` → `vxAlloc`
  / `vxFree` → `MeMemoryAPI`).
- `#if defined(_WIN32)` separates Windows (MSVC) from the Mac branch, which Linux also builds.
