// LEGO Digital Designer — restored source
// LDDUtilities (Mac TU 063, Win 0x635e9f-0x639696): SaveScreenshot / EncodePNG; the rest of the
// TU is in LDDUtilities*.cpp (see LDDUtilities.h).
#include "LDDUtilities.h"
#include "BrickConnectSceneControl.h"
#include "Bitmap.h"
#include "Path.h"
#include "File.h"
#include "SafeFileWrite.h"
#if defined(_WIN32)
#include "PNGBitmapCodec.h"
#include "DataStream.h"
#else
#include "MacImageIO.h"
#endif

namespace LEGO {

#if defined(_WIN32)
// Win 0x637015
RefPointer<DataStreamInterface> EncodePNG(const Bitmap& bitmap)
{
    RefPointer<BitmapCodec> codec = new PNGBitmapCodec;
    UInt32 size = codec->GetEncodedSize(bitmap);
    RefPointer<DataStream> stream = new DataStream(new UInt8[size], size, true);
    codec->SetStream(stream);
    RefPointer<DataStreamInterface> result;
    if (codec->Encode(bitmap, 0))
        result = stream.Get();
    return result;
}
#endif

// Mac 0x149470, Win 0x637120
bool SaveScreenshot(BrickConnectSceneControl* control, const Path& path, Vector2 size,
                    Texture* target, bool fitCamera, RenderQueue* queue, bool noPostProcessing)
{
    if (size.GetArea() == 0.0f)
        size = control->GetSize();
    Bitmap bitmap;
    bitmap.m_width = (UInt32)(Int64)size.x;
    bitmap.m_height = (UInt32)(Int64)size.y;
#if defined(_WIN32)
    bitmap.m_format = kBitmapFormatBGRA32;
    bitmap.m_pitch = (32 >> 3) * bitmap.m_width;          // Win 0x4f5180 (width, 32 bits per pixel)
#else
    bitmap.m_depth = 1;
    bitmap.m_format = kBitmapFormatBGRA32;
    bitmap.m_pitch = bitmap.m_width * 4;
#endif
    bitmap.m_data = new UInt8[bitmap.m_pitch * bitmap.m_height];

    bool result;
    if (queue != 0)
        result = control->RenderScene(queue, bitmap, target, fitCamera, noPostProcessing);
    else
        result = control->RenderToBitmap(bitmap, target, fitCamera, noPostProcessing);
    if (result) {
#if defined(_WIN32)
        RefPointer<DataStreamInterface> png = EncodePNG(bitmap);
#else
        RefPointer<DataStreamInterface> png = EncodePNGWithImageIO(bitmap);
#endif
        // NOTE: the original reports success when the PNG cannot be encoded (nothing written),
        // writes the file in place (an existing picture is cut to nothing first, so a failed
        // write leaves a broken one) and ignores the SetSize result. fixed: a failed encoding
        // fails; the PNG is written all or nothing (WriteWholeFile: "<name>.tmp" next to it on
        // the disk storage, read back, then renamed over the file; inside an archive such as
        // the .lxf thumbnail, written in place and read back) (M2).
        result = !png.IsNull() && WriteWholeFile(path, png->GetData(), png->GetSize());
    }
    delete[] (UInt8*)bitmap.m_data;
    delete[] (UInt8*)bitmap.m_palette;
    return result;
}

} // namespace LEGO
