/* Vortex 5.0.0 build 800 "vortexldd41" - calling convention of the Me C API.
 * The Win build compiles the Me layer with __stdcall (MathEngine Karma's MEAPI): e.g.
 * MeChunkGetMem Win 0x790b60 "ret 8", VinPoolInit 0x758e80 "ret 0x18", the default message
 * handlers "ret 0xc", the outputs "ret 8", MallocCreateAligned (a MeMemoryAPI entry) "ret 8".
 * Variadic functions (MeWarning ...) and qsort/bsearch callbacks stay cdecl. */
#ifndef ME_MECALL_H
#define ME_MECALL_H

#if defined(_WIN32)
#define MEAPI __stdcall
#else
#define MEAPI
#endif

#endif
