// LDD64 Linux platform layer (not restored code): LinuxSound.h on GStreamer.
#if defined(LDD_LINUX)
#include "LinuxSound.h"
#if LDD_LINUX_GSTREAMER
#include <gst/gst.h>
#include <string.h>

namespace LEGO {
namespace LinuxSound {

namespace {
struct Sound {
    GstElement* playbin;
    GBytes*     data;
    guint64     offset;      // the appsrc's read position
    guint       busWatch;
    bool        loop;
    bool        playing;     // between Play and the end (or Stop)
};

// appsrc (random access): the bytes from the current offset
void OnNeedData(GstElement* source, guint length, gpointer user)
{
    Sound* s = (Sound*)user;
    gsize size = 0;
    const guint8* bytes = (const guint8*)g_bytes_get_data(s->data, &size);
    if (s->offset >= size) {
        GstFlowReturn r;
        g_signal_emit_by_name(source, "end-of-stream", &r);
        return;
    }
    gsize n = length ? length : 4096;
    if (n > size - s->offset)
        n = size - s->offset;
    GstBuffer* buffer = gst_buffer_new_allocate(0, n, 0);
    gst_buffer_fill(buffer, 0, bytes + s->offset, n);
    GST_BUFFER_OFFSET(buffer) = s->offset;
    s->offset += n;
    GstFlowReturn r;
    g_signal_emit_by_name(source, "push-buffer", buffer, &r);
    gst_buffer_unref(buffer);
}

gboolean OnSeekData(GstElement*, guint64 offset, gpointer user)
{
    ((Sound*)user)->offset = offset;
    return TRUE;
}

void OnSourceSetup(GstElement*, GstElement* source, gpointer user)
{
    Sound* s = (Sound*)user;
    g_object_set(source, "stream-type", 2 /* GST_APP_STREAM_TYPE_RANDOM_ACCESS */, "format", GST_FORMAT_BYTES,
                 "size", (gint64)g_bytes_get_size(s->data), NULL);
    g_signal_connect(source, "need-data", G_CALLBACK(OnNeedData), s);
    g_signal_connect(source, "seek-data", G_CALLBACK(OnSeekData), s);
}

gboolean OnBus(GstBus*, GstMessage* message, gpointer user)
{
    Sound* s = (Sound*)user;
    switch (GST_MESSAGE_TYPE(message)) {
    case GST_MESSAGE_EOS:
        if (s->loop) {
            gst_element_seek_simple(s->playbin, GST_FORMAT_TIME, (GstSeekFlags)(GST_SEEK_FLAG_FLUSH), 0);
        } else {
            s->playing = false;
            gst_element_set_state(s->playbin, GST_STATE_READY);
        }
        break;
    case GST_MESSAGE_ERROR: {
        GError* error = 0;
        gst_message_parse_error(message, &error, 0);
        g_warning("LDD: sound: %s", error ? error->message : "error");
        if (error)
            g_error_free(error);
        s->playing = false;
        gst_element_set_state(s->playbin, GST_STATE_NULL);
        break;
    }
    default:
        break;
    }
    return TRUE;
}
} // namespace

void* Create(const UInt8* data, UInt32 size)
{
    static bool initialized = false;
    if (!initialized) {
        GError* error = 0;
        if (!gst_init_check(0, 0, &error)) {
            if (error)
                g_error_free(error);
            return 0;
        }
        initialized = true;
    }
    GstElement* playbin = gst_element_factory_make("playbin", 0);
    if (!playbin)
        return 0;
    Sound* s = new Sound;
    s->playbin = playbin;
    s->data = g_bytes_new(data, size);
    s->offset = 0;
    s->loop = false;
    s->playing = false;
    g_object_set(playbin, "uri", "appsrc://", "volume", 25.0 / 255.0, NULL);
    // no video: LDD's sounds are audio files
    gint flags = 0;
    g_object_get(playbin, "flags", &flags, NULL);
    g_object_set(playbin, "flags", flags & ~0x1 /* GST_PLAY_FLAG_VIDEO */, NULL);
    g_signal_connect(playbin, "source-setup", G_CALLBACK(OnSourceSetup), s);
    GstBus* bus = gst_element_get_bus(playbin);
    s->busWatch = gst_bus_add_watch(bus, OnBus, s);
    gst_object_unref(bus);
    return s;
}

void Destroy(void* sound)
{
    Sound* s = (Sound*)sound;
    if (!s)
        return;
    gst_element_set_state(s->playbin, GST_STATE_NULL);
    g_source_remove(s->busWatch);
    gst_object_unref(s->playbin);
    g_bytes_unref(s->data);
    delete s;
}

bool IsPlaying(void* sound)
{
    Sound* s = (Sound*)sound;
    return s && s->playing;
}

bool Play(void* sound)
{
    Sound* s = (Sound*)sound;
    if (!s)
        return false;
    if (s->playing)
        return true;
    s->offset = 0;
    gst_element_set_state(s->playbin, GST_STATE_READY);
    s->playing = gst_element_set_state(s->playbin, GST_STATE_PLAYING) != GST_STATE_CHANGE_FAILURE;
    return s->playing;
}

bool Stop(void* sound)
{
    Sound* s = (Sound*)sound;
    if (!s)
        return false;
    bool was = s->playing;
    s->playing = false;
    gst_element_set_state(s->playbin, GST_STATE_READY);
    return was;   // (NSSound stop: YES when it was playing)
}

void SetVolume(void* sound, float volume)
{
    Sound* s = (Sound*)sound;
    if (s)
        g_object_set(s->playbin, "volume", (gdouble)volume, NULL);
}

void SetLooping(void* sound, bool loop)
{
    Sound* s = (Sound*)sound;
    if (s)
        s->loop = loop;
}

} // namespace LinuxSound
} // namespace LEGO

#else // silent

namespace LEGO {
namespace LinuxSound {
void* Create(const UInt8*, UInt32) { static char silent; return &silent; }
void  Destroy(void*) {}
bool  IsPlaying(void*) { return false; }
bool  Play(void*) { return true; }
bool  Stop(void*) { return true; }
void  SetVolume(void*, float) {}
void  SetLooping(void*, bool) {}
} // namespace LinuxSound
} // namespace LEGO
#endif
#endif
