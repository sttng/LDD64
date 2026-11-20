// LDD64 Linux platform layer (not restored code): sound for SimpleSound's Linux branch (NSSound
// on the Mac): an MP3 or WAV file in memory played, stopped, looped, linear volume 0..1, through
// GStreamer's playbin. Without GStreamer at build time (LDD_LINUX_GSTREAMER 0) it is silent.
#pragma once
#if defined(LDD_LINUX)
#include "LEGO/Types.h"

namespace LEGO {
namespace LinuxSound {

// Takes a copy of `data`; 0 when the sound cannot be set up.
void* Create(const UInt8* data, UInt32 size);
void  Destroy(void* sound);
bool  IsPlaying(void* sound);
bool  Play(void* sound);        // from the start unless it is playing
bool  Stop(void* sound);
void  SetVolume(void* sound, float volume);
void  SetLooping(void* sound, bool loop);

} // namespace LinuxSound
} // namespace LEGO
#endif
