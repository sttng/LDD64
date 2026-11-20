// LEGO Digital Designer — restored source
// TU 221 "ProfileTimer.cpp" (name inferred): Mac 0x2665a0-0x26729f, Win 0x42df40-0x42e6df, with the
// BasicArray<UInt64> instance (dtor Mac 0x266650 / 0x267230, Win 0x42e5e0 / 0x42e6b0, PushBack
// Win 0x42e600, inlined on Mac). See ProfileTimer.h.
// Mac GetReport is SSE-vectorised; the plain Win loop here gives the same result.
#include "ProfileTimer.h"
#include "SystemTime.h"

namespace LEGO {

bool ProfileTimer::s_samplingEnabled = true;
bool ProfileTimer::s_reportEnabled = true;

// Mac 0x2665a0 (C2), 0x266680 (C1); Win 0x42dfe0 (GetMicroseconds inlined: QueryPerformanceCounter)
ProfileTimer::ProfileTimer(const String& name, bool listTimes)
    : m_start(GetMicroseconds()),
      m_name(name),
      m_listTimes(listTimes),
      m_traceHook(TraceHook::Get())
{
}

// Mac 0x266690 (D2) / 0x266790 (D1), Win 0x42e0b0
ProfileTimer::~ProfileTimer()
{
    TraceReport();
}

// Win 0x42e1b0 (ESI = this); Mac inlined in 0x266690
void ProfileTimer::TraceReport()
{
    if (s_reportEnabled && !m_samples.IsEmpty())
        Trace(GetReport(), true);
    m_samples.Clear();
}

// Mac 0x2667a0, Win 0x42e140 (0x42df40)
void ProfileTimer::Start()
{
    if (s_samplingEnabled)
        m_start = GetMicroseconds();
}

// Mac 0x2667d0, Win 0x42e160 (0x42df90 elapsed, 0x42e600 PushBack)
void ProfileTimer::Stop()
{
    if (s_samplingEnabled) {
        UInt64 elapsed = (UInt64)(GetMicroseconds() - m_start);
        m_samples.PushBack(elapsed);
    }
}

// Mac 0x266a60, Win 0x42e190
void ProfileTimer::SetSamplingEnabled(bool enabled)
{
    s_samplingEnabled = enabled;
}

// Mac 0x266a80, Win 0x42e1a0
void ProfileTimer::SetReportEnabled(bool enabled)
{
    s_reportEnabled = enabled;
}

// Mac 0x267130, Win 0x42e510
String ProfileTimer::FormatMicroseconds(UInt64 microseconds)
{
    String text;
    text << microseconds;
    if (text.GetLength() > 3)
        text.Insert(text.GetLength() - 3, 1, '.');
    if (text.GetLength() > 7)
        text.Insert(text.GetLength() - 7, 1, '.');
    if (text.GetLength() > 11)
        text.Insert(text.GetLength() - 11, 1, '.');
    return text;
}

// Mac 0x266aa0, Win 0x42e230
String ProfileTimer::GetReport() const
{
    if (m_samples.IsEmpty()) {
        String text(m_name);
        text << ": There are no sampled times";
        return text;
    }
    UInt64 minimum = 0xffffffffffffffffULL;
    UInt64 maximum = 0;
    UInt64 total = 0;
    for (UInt32 i = 0; i < m_samples.GetSize(); ++i) {
        const UInt64 sample = m_samples[i];
        if (sample < minimum)
            minimum = sample;
        if (sample > maximum)
            maximum = sample;
        total += sample;
    }
    const UInt64 average = total / m_samples.GetSize();
    String text(m_name);
    text << ": Avg(" << FormatMicroseconds(average)
         << ") Total(" << FormatMicroseconds(total)
         << ") Samples(" << (UInt32)m_samples.GetSize()
         << ") Min(" << FormatMicroseconds(minimum)
         << ") Max(" << FormatMicroseconds(maximum)
         << ")";
    if (m_listTimes) {
        text << " Times(\r\n";
        for (UInt32 i = 0; i < m_samples.GetSize(); ++i)
            text << (UInt64)m_samples[i] << "\r\n";
        text << ")";
    }
    return text;
}

} // namespace LEGO
