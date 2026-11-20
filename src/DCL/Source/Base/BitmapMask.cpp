// LEGO Digital Designer — restored source
// BitmapMask (Mac TU 026, 0xd69f0-0xd712f; Win 0x5124c0-0x5128ff) and the BitmapMaskFactory
// members emitted in Mac TU 060 (0x13ff20-0x140700; Win 0x5cadbf-0x5cb659). Names inferred.
#include "BitmapMask.h"
#include "File.h"
#include "Stream.h"

namespace LEGO {

// Mac 0x1402e0 (TU 060), Win 0x5caee4
BitmapMask::BitmapMask() : m_data(0), m_ownsData(false), m_factory(0)
{
}

// Mac 0xd6fc0 (D1), 0xd7050 (D0)
BitmapMask::~BitmapMask()
{
    if (m_ownsData && m_data != 0)
        delete[] m_data;
}

void BitmapMask::SetData(UInt8* data, bool ownsData)
{
    if (m_data != data && m_data != 0 && m_ownsData)
        delete[] m_data;
    m_data = data;
    m_ownsData = ownsData;
}

// Mac 0xd6e40 (thunk 0xd6f70), Win 0x5caf5c
void BitmapMask::OnFinalRelease()
{
    if (m_factory != 0) {
        m_factory->Remove(m_name);
        m_factory = 0;
    }
    Object::OnFinalRelease();
}

// Mac 0xd6bc0, Win 0x5125b6.
// Mac reads the 0x3a header bytes as a packed struct (type == 0x4d42), Win through a Stream
// that byte-swaps the type only (== 0x424d); the same check.
bool BitmapMask::Load(const Path& path)
{
    RefPointer<File> file = path.OpenFile(OpenMode(true, false, true, false), false);
    if (file.IsNull())
        return false;
    UInt8 header[0x3a];
    if (!file->ReadAt(0, sizeof(header), header, 0))
        return false;
    Stream stream(header, sizeof(header), true);
    UInt16 type;
    UInt32 fileSize;
    UInt16 reserved;
    UInt32 dataOffset;
    UInt32 infoSize;
    UInt32 width;
    UInt32 height;
    UInt16 planes;
    UInt16 bitCount;
    UInt32 compression;
    stream.Read(type);                                           // byte-swapped: "BM" = 0x424d
    stream.SetSwap(false);
    stream.Read(fileSize);
    stream.Read(reserved);
    stream.Read(reserved);
    stream.Read(dataOffset);
    stream.Read(infoSize);
    stream.Read(width);
    stream.Read(height);
    stream.Read(planes);
    stream.Read(bitCount);
    stream.Read(compression);
    if (type != 0x424d)
        return false;
    UInt64 size = file->GetSize();
    if (planes != 1 || infoSize != 0x28 || (UInt64)fileSize != size || bitCount != 8 || compression != 1)
        return false;
    Int32 absWidth = (Int32)width < 0 ? -(Int32)width : (Int32)width;
    if (absWidth < 1)
        return false;
    Int32 absHeight = (Int32)height < 0 ? -(Int32)height : (Int32)height;
    if (absHeight < 1)
        return false;
    m_size.x = (Real32)absWidth;
    m_size.y = (Real32)absHeight;
    UInt32 dataSize = (UInt32)file->GetSize() - dataOffset;
    SetData(new UInt8[dataSize], true);
    if (!file->ReadAt(dataOffset, dataSize, m_data, 0))
        return false;
    m_name = path.GetPath(false);
    return true;
}

// Mac 0xd69f0, Win 0x5124c0.
// Walks the RLE8 stream from the bottom row (the file's first) up to the row of `point`.
UInt8 BitmapMask::GetValue(const Vector2& point) const
{
    if (!(0.0f <= point.x && 0.0f <= point.y && point.x < m_size.x && point.y < m_size.y))
        return 0;
    UInt32 row = (UInt32)((Int32)m_size.y - 1);
    if ((Int32)row < 0)
        return 0;
    UInt32 x = (UInt32)(Int64)point.x;
    UInt32 y = (UInt32)(Int64)point.y;
    const UInt8* source = m_data;
    for (;;) {
        UInt32 column = 0;
        for (;;) {
            UInt8 count = source[0];
            if (count != 0) {                                    // encoded run
                column += count;
                if (column > x && y == row)
                    return source[1];
                source += 2;
                continue;
            }
            UInt8 code = source[1];
            if (code >= 3) {                                     // absolute run (word aligned)
                if (y == row && x < column + code)
                    return source[2 + x - column];
                column += code;
                source += 2 + ((code + 1) & 0x1fe);
                continue;
            }
            if (code == 2) {                                     // delta: right dx, up dy rows
                row -= source[3];
                if (row < y)
                    return 0;
                column += source[2];
                if (column > x && y == row)
                    return 0;
                source += 4;
                continue;
            }
            if (code == 1)                                       // end of bitmap
                return 0;
            source += 2;                                         // end of line
            break;
        }
        --row;
        if ((Int32)row < 0)
            return 0;
    }
}

// ---------------------------------------------------------------------------------------------
// BitmapMaskFactory (Mac TU 060)
// ---------------------------------------------------------------------------------------------

// Mac 0x1401e0, Win 0x5cb5d6: the masks outlive the factory without it.
BitmapMaskFactory::~BitmapMaskFactory()
{
    for (MaskMap::Iterator it = m_masks.Begin(); it != m_masks.End(); ++it)
        it->second->m_factory = 0;
}

// Mac 0xd6e80
void BitmapMaskFactory::Remove(const String& name)
{
    MutexLocker lock(m_mutex.Get());
    m_masks.Erase(m_masks.Find(name));
}

// Mac 0x13ff70, Win 0x5cb4e7
Object* BitmapMaskFactory::CreateObject(const ObjectFactoryArgs& args)
{
    const BitmapMask::LoadArgs* loadArgs = dynamic_cast<const BitmapMask::LoadArgs*>(&args);
    if (loadArgs == 0)
        return 0;
    String name = loadArgs->m_path.GetPath(false);
    MaskMap::Iterator it = m_masks.Find(name);
    if (it != m_masks.End())
        return it->second;
    BitmapMask* mask = new BitmapMask;
    if (!mask->Load(loadArgs->m_path)) {
        mask->Release();   // deletes the new mask (reference count 0 -> -1)
        return 0;
    }
    mask->m_factory = this;
    m_masks[name] = mask;
    return mask;
}

} // namespace LEGO
