/* Vortex 5.0.0 build 800 "vortexldd41" - Me timers and section profiler
 * (restored). Mac __text 0x48a3b2-0x48aa5d + 0x48aa12-0x48ac0d (TU 530 Me
 * block); Win 0x7572a0, 0x758400-0x758830, 0x7acba0-0x7acca0. Win keeps more of the profiler
 * (MeProfileEndFrame 0x758650, MeProfileOutputResults 0x758950, MeTimerGetTime 0x758590 ...)
 * that LDD never calls; they are not restored here (dead-stripped on Mac). */
#include "Me/MeProfile.h"
#include "Me/MeMemory.h"
#include "Me/MeMessage.h"
#include <stdio.h>
#include <string.h>
#include <time.h>
#if defined(_MSC_VER)
#include <intrin.h>
#elif defined(__i386__) || defined(__x86_64__)
#include <x86intrin.h>
#elif defined(__APPLE__)
/* PORT64 (Apple silicon): no time stamp counter instruction; the Mach absolute time stands in
   for it, and GetOutputInfo gives its rate (hw.cpufrequency does not exist there) */
#include <mach/mach_time.h>
#define __rdtsc() mach_absolute_time()
#define ME_PROFILE_MACH_TIME 1
#endif

MeU64 MeProfileTicksPerSecond = 0; /* Mac 0x7794c0, Win 0xaf25b0 */

/* one profiled section (Mac 0x44 bytes) (name inferred) */
typedef struct MeProfileSection {
    char* name;                     /* +0x00 */
    MeU64 total[3];                 /* +0x04 accumulated counters */
    unsigned char exclusive;        /* +0x1c: ended when another section starts */
    struct MeProfileSection* next;  /* +0x20 */
    unsigned char running;          /* +0x24 */
    MeProfileTimestamp start;       /* +0x28 */
    unsigned char count;            /* +0x40 */
} MeProfileSection;

/* per-frame results (only freed in this build: the frame code is dead-stripped on Mac) */
typedef struct MeProfileFrame {
    struct MeProfileFrame* next;    /* +0x00 */
    int m04;
    MeProfileSection* sections;     /* +0x08 */
} MeProfileFrame;

typedef struct MeProfileConfig { /* Mac 8 bytes */
    char mode;      /* +0x00 (Win: int) */
    int parameter;  /* +0x04 */
} MeProfileConfig;

static MeProfileConfig* sConfig = 0;        /* Mac 0x780168, Win 0xaf25a4 */
static unsigned char sEnabled = 0;          /* Mac 0x78016c, Win 0xaf25a8 */
static MeProfileSection* sSections = 0;     /* Mac 0x780170 */
static MeProfileFrame* sFrames = 0;         /* Mac 0x780174 */
static int sFrameCount = 0;                 /* Mac 0x780178 */
static float sSmoothing = 0.2f;             /* Mac 0x78017c (name inferred) */
static int sHistory = 10;                   /* Mac 0x780180 (name inferred) */
static int sFramesToProfile = 0;            /* Mac 0x780184 */

/* Mac 0x48a3b2 */
void MEAPI MeProfileTimestampZero(MeProfileTimestamp* t)
{
    t->cpuCycles = 0;
    t->count1 = 0;
    t->count2 = 0;
}

/* Mac 0x48a3e3 */
void MEAPI MeTimerInit(MeTimer* timer)
{
    MeProfileTimestampZero(&timer->start);
    MeProfileTimestampZero(&timer->end);
    MeProfileTimestampZero(&timer->total);
    timer->running = 0;
}

/* Mac 0x48aa12, Win 0x7acbd0 (rdtsc: Win 0x7acba0) */
MeProfileTimestamp* MEAPI MeProfileGetTimestamp(MeProfileTimestamp* t)
{
    t->cpuCycles = (MeU64)__rdtsc();
    t->count1 = 0;
    t->count2 = 0;
    return t;
}

/* Mac 0x48a415, Win 0x758470: returns the seconds of this run.
 * NOTE: the message says "MeTimerStopTimer" in both start and stop (original strings). */
double MEAPI MeTimerStopTimer(MeTimer* timer)
{
    MeProfileTimestamp now;
    if (!timer->running)
        MeWarning(0, "MeTimerStopTimer: Timer is not running\n");
    timer->end = *MeProfileGetTimestamp(&now);
    timer->total.cpuCycles = timer->total.cpuCycles + timer->end.cpuCycles - timer->start.cpuCycles;
    timer->total.count1 = timer->total.count1 + timer->end.count1 - timer->start.count1;
    timer->total.count2 = timer->total.count2 + timer->end.count2 - timer->start.count2;
    timer->running = 0;
    return (double)(timer->end.cpuCycles - timer->start.cpuCycles) / (double)MeProfileTicksPerSecond;
}

/* Mac 0x48a55b, Win 0x758400 */
void MEAPI MeTimerStartTimer(MeTimer* timer)
{
    MeProfileTimestamp now;
    if (timer->running)
        MeWarning(0, "MeTimerStopTimer: Timer already running\n");
    timer->running = 1;
    timer->start = *MeProfileGetTimestamp(&now);
}

/* Mac 0x48a5c4 */
void MEAPI MeTimerInitFrequency(void)
{
    GetOutputInfo();
}

/* Mac 0x48a5d1 (Win 0x757500 per V2.1): ends a running section, returns the section's
 * accumulated time in ms. NOTE: the accumulated total, not this run's time (original). */
double MEAPI MeProfileEndSectionFn(const char* name)
{
    MeProfileTimestamp now;
    MeProfileSection* s;
    if (!sEnabled)
        return 0;
    MeProfileGetTimestamp(&now);
    for (s = sSections; s != 0; s = s->next) {
        if (strcmp(s->name, name) == 0 && s->running == 1) {
            s->total[0] = s->total[0] + now.cpuCycles - s->start.cpuCycles;
            s->total[1] = s->total[1] + now.count1 - s->start.count1;
            s->total[2] = s->total[2] + now.count2 - s->start.count2;
            s->running = 0;
            return (double)s->total[0] / (double)MeProfileTicksPerSecond * 1000.0;
        }
    }
    MeWarning(3, "MeProfileEndSectionFn: You tried to stop %s which you hadn't started, which is a "
                 "bit silly.\n",
              name);
    return 0;
}

/* Mac 0x48a75b, Win 0x758830: an exclusive running section is ended when any section starts */
void MEAPI MeProfileStartSectionFn(const char* name, int exclusive)
{
    MeProfileSection* s = 0;
    MeProfileSection* last = sSections;
    MeProfileTimestamp now;
    int found = 0;
    if (!sEnabled)
        return;
    if (sSections != 0) {
        MeProfileSection* p = sSections;
        do {
            last = p;
            if (last->exclusive == 1 && last->running == 1)
                MeProfileEndSectionFn(last->name);
            if (strcmp(last->name, name) == 0) {
                found = 1;
                s = last;
            }
            p = last->next;
        } while (last->next != 0);
    }
    if (!found) {
        s = (MeProfileSection*)MeMemoryAPI.createZeroed(sizeof(MeProfileSection));
        if (sSections != 0)
            last->next = s;
        else
            sSections = s;
        s->name = (char*)MeMemoryAPI.createZeroed(strlen(name) + 1);
        strcpy(s->name, name);
    }
    s->exclusive = (unsigned char)exclusive;
    s->running = 1;
    s->count++;
    s->start = *MeProfileGetTimestamp(&now);
}

/* Mac 0x48a884.
 * NOTE: the section and frame list heads are left dangling (reset by the next StartFrames). */
void MEAPI MeProfileStopFrames(void)
{
    MeProfileSection* s;
    MeProfileFrame* f;
    MeMemoryAPI.destroy(sConfig);
    sConfig = 0;
    s = sSections;
    while (s != 0) {
        MeProfileSection* next;
        MeMemoryAPI.destroy(s->name);
        next = s->next;
        MeMemoryAPI.destroy(s);
        s = next;
    }
    f = sFrames;
    while (f != 0) {
        MeProfileFrame* nextFrame;
        s = f->sections;
        while (s != 0) {
            MeProfileSection* next;
            MeMemoryAPI.destroy(s->name);
            next = s->next;
            MeMemoryAPI.destroy(s);
            s = next;
        }
        nextFrame = f->next;
        MeMemoryAPI.destroy(f);
        f = nextFrame;
    }
    MeProfileStopTimers();
    sEnabled = 0;
}

/* Mac 0x48a948, Win 0x7572a0 */
void MEAPI MeProfileStartFrames(int mode, int parameter, int frames)
{
    sFramesToProfile = frames;
    sSections = 0;
    sFrames = 0;
    sSmoothing = 0.2f;
    sHistory = 10;
    sFrameCount = 0;
    if (sConfig == 0)
        sConfig = (MeProfileConfig*)MeMemoryAPI.createZeroed(8);
    sConfig->parameter = parameter;
    sConfig->mode = (char)mode;
    MeProfileStartTimers();
    GetOutputInfo();
    sEnabled = 1;
}

/* Mac 0x48aa49, Win 0x7acca0 */
int MEAPI MeProfileStartTimers(void)
{
    return 0;
}

/* Mac 0x48aa53 */
int MEAPI MeProfileStopTimers(void)
{
    return 0;
}

/* Mac 0x48aa5d, Win 0x7acc50 */
void MEAPI GetOutputInfo(void)
{
#if defined(_WIN32)
    /* Win: cycles over 100 clock() ticks (0.1 s), times 10 */
    MeU64 t0 = (MeU64)__rdtsc();
    clock_t c0 = clock();
    MeU64 t1;
    while (clock() < c0 + 100) {
    }
    t1 = (MeU64)__rdtsc();
    MeProfileTicksPerSecond = (t1 - t0) * 10;
#elif defined(ME_PROFILE_MACH_TIME)
    /* PORT64 (Apple silicon): ticks of mach_absolute_time per second */
    mach_timebase_info_data_t timebase;
    mach_timebase_info(&timebase);
    MeProfileTicksPerSecond = (MeU64)1000000000u * timebase.denom / timebase.numer;
#elif defined(__APPLE__)
    float hz = 0.0f;
    char line[1024];
    FILE* f = popen("sysctl hw.cpufrequency", "r");
    if (f != 0) {
        int n = -1;
        while (fgets(line, 0x3ff, f) != 0) {
            n = sscanf(line, "hw.cpufrequency: %f", &hz);
            if (n != 0)
                break;
        }
        pclose(f);
        if (n != -1) {
            MeProfileTicksPerSecond = (MeU64)hz;
            return;
        }
    }
    MeWarning(0, "GetOutputInfo() : failed to read CPU clock speed from sysctl, setting 2.8GHz by "
                 "default");
    MeProfileTicksPerSecond = 2800000000u;
#else
    /* HOST-ONLY (the Linux-hosted Mac branch): no sysctl hw.cpufrequency; the Mac fallback value
     * without its warning */
    MeProfileTicksPerSecond = 2800000000u;
#endif
}
