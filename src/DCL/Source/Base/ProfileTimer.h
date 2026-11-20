// LEGO Digital Designer — restored source
// LEGO::ProfileTimer — sums a section's durations (µs), traces "<name>: Avg(…) Total(…)
// Samples(…) Min(…) Max(…)" on destruction (static timers report at exit). TU 221 (names
// inferred): Mac 0x2665a0-0x26729f, Win 0x42df40-0x42e6df. No vtable. Layout Mac: start +0, name
// +8, samples +0x18, listTimes +0x28, TraceHook* +0x2c (0x30); Win: +0, +8, +0xc, +0x1c, +0x20
// (0x28).
// Process-wide switches (on at start, set by Mac 0x13ad50): sampling, report at destruction.
#pragma once
#include "LEGO/Types.h"
#include "Object.h"
#include "BasicArray.h"
#include "LEGO/String.h"
#include "Trace.h"

namespace LEGO {

class ProfileTimer {
public:
    // `listTimes`: the report also lists every sample. Keeps the TraceHook alive.
    ProfileTimer(const String& name, bool listTimes);                // Mac 0x2665a0 (C2) / 0x266680 (C1)  Win 0x42dfe0
    // Traces the report (when enabled and there are samples).
    ~ProfileTimer();                                                 // Mac 0x266690 / 0x266790  Win 0x42e0b0

    void Start();                                                    // Mac 0x2667a0  Win 0x42e140 (+ 0x42df40)
    // Adds now - start as a sample.
    void Stop();                                                     // Mac 0x2667d0  Win 0x42e160 (+ 0x42df90, push 0x42e600)

    // "<name>: Avg(a) Total(t) Samples(n) Min(m) Max(x)" [+ " Times(\r\n" t1 "\r\n" … ")"],
    // durations formatted by FormatMicroseconds; "<name>: There are no sampled times" when empty.
    String GetReport() const;                                        // Mac 0x266aa0  Win 0x42e230
    // Traces the report if reporting is on and there are samples, then forgets the samples.
    void TraceReport();                                              // Win 0x42e1b0 (Mac inlined in the dtor)

    UInt32 GetSampleCount() const { return m_samples.GetSize(); }   // (accessors, names inferred)
    UInt64 GetSample(UInt32 index) const { return m_samples[index]; }
    const String& GetName() const { return m_name; }

    static void SetSamplingEnabled(bool enabled);                    // Mac 0x266a60  Win 0x42e190
    static void SetReportEnabled(bool enabled);                      // Mac 0x266a80  Win 0x42e1a0
    static bool IsSamplingEnabled() { return s_samplingEnabled; }
    static bool IsReportEnabled() { return s_reportEnabled; }

    // Decimal microseconds with '.' before the last 3, 6 (and 9) digits: 1234567 -> "1.234.567".
    static String FormatMicroseconds(UInt64 microseconds);           // Mac 0x267130  Win 0x42e510

private:
    ProfileTimer(const ProfileTimer&);
    ProfileTimer& operator=(const ProfileTimer&);

    Int64                  m_start;        // +0x00
    String                 m_name;         // +0x08
    BasicArray<UInt64>     m_samples;      // Mac +0x18, Win +0x0c
    bool                   m_listTimes;    // Mac +0x28, Win +0x1c
    RefPointer<TraceHook>  m_traceHook;    // Mac +0x2c, Win +0x20

    static bool s_samplingEnabled;         // Mac 0x771738, Win 0xae142a (initially true)
    static bool s_reportEnabled;           // Mac 0x771739, Win 0xae142b (initially true)
};

} // namespace LEGO
