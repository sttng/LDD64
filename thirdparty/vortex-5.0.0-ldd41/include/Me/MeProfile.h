/* Vortex 5.0.0 build 800 "vortexldd41" - Me timers and the section profiler
 * (restored; src/Me/MeProfile.c). Time stamps are CPU cycle counts (rdtsc);
 * MeProfileTicksPerSecond is measured once (Mac: "sysctl hw.cpufrequency", default 2.8 GHz;
 * Win: rdtsc over 100 clock() ticks x 10). Names from strings: MeTimerStopTimer,
 * MeProfileEndSectionFn, GetOutputInfo; the others are inferred (Karma MeProfile style). */
#ifndef ME_MEPROFILE_H
#define ME_MEPROFILE_H

#include "Me/MeCall.h"

#ifdef __cplusplus
extern "C" {
#endif

#if defined(_MSC_VER)
typedef unsigned __int64 MeU64;
#else
typedef unsigned long long MeU64;
#endif

/* 0x18 bytes: cycle count + two further counters that are always 0 in this build */
typedef struct MeProfileTimestamp {
    MeU64 cpuCycles; /* +0x00 */
    MeU64 count1;    /* +0x08 */
    MeU64 count2;    /* +0x10 */
} MeProfileTimestamp;

/* 0x4c bytes (Mac; MSVC pads to 0x50) */
typedef struct MeTimer {
    MeProfileTimestamp start;   /* +0x00 */
    MeProfileTimestamp end;     /* +0x18 */
    MeProfileTimestamp total;   /* +0x30 accumulated */
    unsigned char running;      /* +0x48 */
} MeTimer;

/* Mac 0x7794c0, Win 0xaf25b0 */
extern MeU64 MeProfileTicksPerSecond;

void MEAPI MeProfileTimestampZero(MeProfileTimestamp* t);               /* Mac 0x48a3b2 */
MeProfileTimestamp* MEAPI MeProfileGetTimestamp(MeProfileTimestamp* t); /* Mac 0x48aa12, Win 0x7acbd0 */
void MEAPI GetOutputInfo(void);         /* Mac 0x48aa5d, Win 0x7acc50: measures MeProfileTicksPerSecond */
int MEAPI MeProfileStartTimers(void);   /* Mac 0x48aa49, Win 0x7acca0: returns 0 */
int MEAPI MeProfileStopTimers(void);    /* Mac 0x48aa53: returns 0 */

void MEAPI MeTimerInit(MeTimer* timer);          /* Mac 0x48a3e3 */
void MEAPI MeTimerStartTimer(MeTimer* timer);    /* Mac 0x48a55b, Win 0x758400 */
double MEAPI MeTimerStopTimer(MeTimer* timer);   /* Mac 0x48a415, Win 0x758470: seconds of this run */
void MEAPI MeTimerInitFrequency(void);           /* Mac 0x48a5c4: GetOutputInfo() */

/* profiler: mode/parameter are stored (Mac: mode as a char), frame/section bookkeeping */
void MEAPI MeProfileStartFrames(int mode, int parameter, int frames); /* Mac 0x48a948, Win 0x7572a0 */
void MEAPI MeProfileStopFrames(void);                                 /* Mac 0x48a884 */
void MEAPI MeProfileStartSectionFn(const char* name, int exclusive);  /* Mac 0x48a75b, Win 0x758830 */
double MEAPI MeProfileEndSectionFn(const char* name);                 /* Mac 0x48a5d1: milliseconds */

#define MeProfileStartSection(name, exclusive) MeProfileStartSectionFn(name, exclusive)
#define MeProfileEndSection(name) MeProfileEndSectionFn(name)

#ifdef __cplusplus
}
#endif

#endif
