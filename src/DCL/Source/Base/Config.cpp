// LEGO Digital Designer — restored source
// LEGO::Config (INI-style files). TU "Config.cpp" Mac 0x3620-0x5b0f (35 functions), Win
// 0x4335f0-0x4352ff (+ helpers 0x433280-0x43552f). Names inferred. Mac static init 0x5ad0.
//
// Format (Load): whitespace " \t\v\f\r\n" and comments ("; ", "// ", "rem " in any case, to the
// end of the line; "/* */") are skipped; "[name]" starts a section (no ']': this and the next
// line are skipped); "key = value" (trimmed; keys before any header go to section ""); a repeated
// key keeps its first entry and gets the last value.
// NOTE: Save quotes a key/value with leading/trailing whitespace, Load keeps the quotes; trimming
//   also skips leading comment syntax.
//
// Templates emitted here (not written out): Mac 0x3730/0x5850 BasicSet<Section*> D1/D0 (Win
//   0x435210/0x4352e0), 0x3760/0x5800 BasicList<Section*> (Win 0x435130/0x4352c0), 0x5600/0x5680
//   BasicSet<Key*> (Win 0x435070/0x4352a0), 0x5630/0x5720 BasicList<Key*> (Win 0x434f90/0x435280),
//   0x58a0/0x56d0 _Rb_tree::_M_erase, 0x58f0/0x59e0 _M_insert_unique; Win ctors 0x4350d0/0x435180
//   (Section containers), 0x434f30/0x434fe0 (Key containers).
#include "Config.h"
#include "Directory.h"
#include "StringConversion.h"
#include "SafeFileWrite.h"   // (M2) Save
#include <string.h>

#include <iostream>

namespace LEGO {

namespace {

// " \t\v\f\r\n" and " \t\v\f" (Mac 0x75f6a8 / 0x75f6b6, Win 0xae142c / 0xae1420).
const UniChar16 s_whitespaceAndNewlines[] = { '\t', '\n', '\v', '\f', '\r', ' ', 0 };
const UniChar16 s_whitespace[]            = { '\t', '\v', '\f', ' ', 0 };

// Win 0x433510 (inlined on Mac). (name inferred)
inline bool IsValidPosition(const String& text, UInt32 position)
{
    return position != String::npos && position < text.GetLength();
}

void SkipWhitespace(const String& text, UInt32& position, bool skipNewlines);

// Moves `position` to the next line. Win 0x434a30 (inlined on Mac). (name inferred)
void SkipLine(const String& text, UInt32& position)
{
    if (IsValidPosition(text, position)) {
        position = text.FindFirstOf(String("\r\n"), position);
        SkipWhitespace(text, position, true);
    }
}

// Advances `position` past whitespace and comments. Mac 0x4c50 (regparm: ECX text, EDX
// position), Win 0x434ad0 (loop instead of tail recursion). (name inferred)
void SkipWhitespace(const String& text, UInt32& position, bool skipNewlines)
{
    if (!IsValidPosition(text, position))
        return;
    position = text.FindFirstNotOf(skipNewlines ? s_whitespaceAndNewlines : s_whitespace, position);
    if (!IsValidPosition(text, position))
        return;

    UniChar16 c = text.GetAt(position);
    if (c == ';') {
        SkipLine(text, position);
        return;
    }
    if (text.GetLength() <= position + 1)
        return;
    UniChar16 c2 = text.GetAt(position + 1);
    if (c == '/') {
        if (c2 == '*') {
            position = text.Find(String("*/"), position + 2);
            if (!IsValidPosition(text, position))
                return;
            position += 2;
            SkipWhitespace(text, position, true);
            return;
        }
        if (c2 == '/') {
            SkipLine(text, position);
            return;
        }
    }
    if (text.GetLength() <= position + 3)
        return;
    UniChar16 c3 = text.GetAt(position + 2);
    UniChar16 c4 = text.GetAt(position + 3);
    if ((c | 0x20) == 'r' && (c2 | 0x20) == 'e' && (c3 | 0x20) == 'm' && c4 == ' ')
        SkipLine(text, position);
}

// Removes leading whitespace/comments and trailing whitespace. Mac 0x4f70 (regparm: ECX),
// Win 0x434e40. (name inferred)
void Trim(String& text)
{
    UInt32 position = 0;
    SkipWhitespace(text, position, true);
    text.Erase(0, position);
    UInt32 last = text.FindLastNotOf(s_whitespaceAndNewlines, String::npos);
    if (IsValidPosition(text, last))
        text.Erase(last + 1, String::npos);
    else
        text.Erase(0, String::npos);
}

// Reads `out` from `position` up to (not including) one of `delimiters`, trimmed; leaves
// `position` at the delimiter. Shared body of ReadKeyName / ReadValue. (name inferred)
inline void ReadToken(const String& text, UInt32& position, bool skipNewlines,
                      const char* delimiters, String& out)
{
    SkipWhitespace(text, position, skipNewlines);
    if (!IsValidPosition(text, position))
        return;
    UInt32 end = text.FindFirstOf(String(delimiters), position);
    if (IsValidPosition(text, end))
        out.Assign(text, position, end - position);
    else
        out.Assign(text, position, String::npos);
    position = end;
    Trim(out);
}

// Key name up to '=' or the end of the line. Win 0x434c60 (inlined on Mac). (name inferred)
void ReadKeyName(const String& text, UInt32& position, String& out)
{
    ReadToken(text, position, true, "\r\n=", out);
}

// Value up to the end of the line. Win 0x434d50 (inlined on Mac). (name inferred)
void ReadValue(const String& text, UInt32& position, String& out)
{
    ReadToken(text, position, false, "\r\n", out);
}

// Wraps `text` in double quotes when it starts or ends with whitespace (Save).
// Mac 0x5770 (regparm: ECX), Win 0x434ec0. (name inferred)
void QuoteIfNeeded(String& text)
{
    bool quote = false;
    if (text.GetLength() != 0)
        quote = text.IsWhitespaceAt(0) || text.IsWhitespaceAt(text.GetLength() - 1);
    if (quote && text.GetLength() != 0) {
        text.Insert(0, 1, '"');
        text.Append(1, '"');
    }
}

// Owner of a heap block (Win dtor 0x435270: `if (m_owned) operator delete(m_ptr)`; Mac
// inlined). (name inferred)
// NOTE: both builds release the new[] block with scalar operator delete; fixed: delete[] (low).
class BufferHolder {
public:
    explicit BufferHolder(UInt8* p) : m_ptr(p), m_owned(p != 0) {}
    ~BufferHolder()
    {
        if (m_owned)
            delete[] m_ptr;
    }
    UInt8* Get() const { return m_ptr; }

private:
    BufferHolder(const BufferHolder&);
    BufferHolder& operator=(const BufferHolder&);

    UInt8* m_ptr;
    bool   m_owned;
};

} // namespace

// ---------------------------------------------------------------------------------------------
// Config::Section

// Mac 0x5510 (D1), Win 0x4334a0
Config::Section::~Section()
{
    Clear();
}

// Mac 0x5440, Win 0x434560
void Config::Section::Clear()
{
    m_keySet.Clear();
    while (m_keys.GetSize() != 0) {
        Key* key = m_keys.Front();
        m_keys.PopFront();
        delete key;
    }
}

// Mac 0x3c90, Win 0x4348d0
Config::Key* Config::Section::FindKey(const String& name) const
{
    Key probe(name);
    BasicSet<Key*, KeyLess>::ConstIterator it = m_keySet.Find(&probe);
    return it != m_keySet.End() ? *it : 0;
}

// Mac 0x3f80, Win 0x434630
Config::Key* Config::Section::AddKey(const String& name, const String& value)
{
    Key* key = 0;
    if (name.GetLength() != 0) {
        key = new Key(name, value);
        m_keys.PushBack(key);
        m_keySet.Insert(key);
    }
    return key;
}

// Mac 0x40f0, Win 0x434700
bool Config::Section::RemoveKey(const String& name)
{
    for (BasicList<Key*>::Iterator it = m_keys.Begin(); it != m_keys.End(); ++it) {
        Key* key = *it;
        if (!key->m_name.EqualsNoCase(name))
            continue;
        m_keys.Erase(it);
        // Erase exactly this entry of the (unique) set.
        std::pair<BasicSet<Key*, KeyLess>::Iterator, BasicSet<Key*, KeyLess>::Iterator> range =
            m_keySet.Std().equal_range(key);
        for (BasicSet<Key*, KeyLess>::Iterator s = range.first; s != range.second; ++s) {
            if (*s == key) {
                m_keySet.Erase(s);
                break;
            }
        }
        delete key;
        return true;
    }
    return false;
}

// Mac inlined into 0x4040, Win 0x4349b0
bool Config::Section::EnumerateKeys(KeyEnumerator& enumerator) const
{
    for (BasicList<Key*>::ConstIterator it = m_keys.Begin(); it != m_keys.End(); ++it) {
        if (!enumerator.OnKey((*it)->m_name, (*it)->m_value))
            break;
    }
    return true;
}

// ---------------------------------------------------------------------------------------------
// Config

// Mac 0x3620 (C2) / 0x37b0 (C1), Win inlined into its callers
Config::Config()
    : m_path(String(), 0)
{
}

// Mac 0x37c0 (D2) / 0x3ae0 (D1) / 0x3af0 (D0), Win 0x433610 / sdd 0x4335f0
Config::~Config()
{
    Clear();
}

// Mac 0x3940, Win 0x4336a0
void Config::Clear()
{
    m_sectionSet.Clear();
    while (m_sections.GetSize() != 0) {
        Section* section = m_sections.Front();
        m_sections.PopFront();
        delete section;
    }
    m_file = 0;
    m_path = Path(String(), 0);
}

// Mac 0x3b90, Win 0x434490
Config::Section* Config::FindSection(const String& name) const
{
    Section probe(name);
    BasicSet<Section*, SectionLess>::ConstIterator it = m_sectionSet.Find(&probe);
    return it != m_sectionSet.End() ? *it : 0;
}

// Mac 0x3ea0, Win 0x4343d0
Config::Section* Config::AddSection(const String& name)
{
    Section* section = 0;
    if (FindSection(name) == 0) {
        section = new Section(name);
        m_sections.PushBack(section);
        m_sectionSet.Insert(section);
    }
    return section;
}

// Mac 0x3b30, Win 0x4337f0
bool Config::GetValue(const String& section, const String& key, String& value) const
{
    Section* s = FindSection(section);
    if (s != 0) {
        Key* k = s->FindKey(key);
        if (k != 0) {
            value = k->m_value;
            return true;
        }
    }
    value.Clear();
    return false;
}

// Mac 0x3de0, Win 0x433850
bool Config::SetValue(const String& section, const String& key, const String& value)
{
    Section* s = FindSection(section);
    if (s == 0) {
        s = AddSection(section);
        if (s == 0)
            return false;
    }
    Key* k = s->FindKey(key);
    if (k == 0) {
        k = s->AddKey(key, String());
        if (k == 0)
            return false;
    }
    k->m_value = value;
    return true;
}

// Mac 0x40b0, Win inlined
bool Config::RemoveKey(const String& section, const String& key)
{
    Section* s = FindSection(section);
    if (s != 0)
        return s->RemoveKey(key);
    return false;
}

// Mac 0x4040 (Section::EnumerateKeys inlined), Win inlined
bool Config::EnumerateKeys(const String& section, KeyEnumerator& enumerator) const
{
    Section* s = FindSection(section);
    if (s == 0)
        return false;
    return s->EnumerateKeys(enumerator);
}

// Mac 0x4290, Win 0x433920
bool Config::Load(const Path& path, bool keepOpen)
{
    if (!m_file.IsNull())
        return false;
    Clear();

    // Win calls Path 0x40b890(false) (= the bare name) and the RefPointer getter 0x4332e0 for
    // the parent directory; Mac inlines both.
    if (path.GetName().GetLength() == 0)
        return false;
    RefPointer<Directory> directory(path.GetParentDirectory());
    if (directory.IsNull() || !directory->FileExists(path.GetName()))
        return false;

    Section* section = AddSection(String(""));
    if (section == 0)
        return false;

    String text;
    RefPointer<File> file = path.OpenFile(OpenMode(true, keepOpen, !keepOpen), false);
    if (file.IsNull())
        return false;

    UInt64 size = file->GetSize();
    if (size == 0) {
        m_path = path;
        if (keepOpen)
            m_file = file;
        return true;
    }

    // PORT64: a 64-bit build refuses a file of 4 GB and more (allocated with the low 32 bits,
    // read with all 64); compiled out on 32-bit
    if (sizeof(void*) > 4 && size >= 0xFFFFFFFFu)
        return false;
    BufferHolder buffer(new UInt8[(UInt32)size + 1]);
    *(UInt16*)buffer.Get() = 0;
    if (!file->ReadAt(0, size, buffer.Get(), 0))
        return false;

    const UniChar16* chars = (const UniChar16*)buffer.Get();
    if (chars[0] == 0xFFFE)
        text = DecodeString(kTextEncodingUTF16BE, chars + 1, (UInt32)(size >> 1) - 1); // Mac 0x1a630, Win 0x4327e0
    else if (chars[0] == 0xFEFF)
        text = String(chars + 1, (UInt32)(size >> 1) - 1);                // UTF-16LE
    else
        text = String((const char*)buffer.Get(), (UInt32)size);

    UInt32 position = 0;
    SkipWhitespace(text, position, true);
    while (IsValidPosition(text, position)) {
        if (text.IsCharAt(position, '[')) {
            UInt32 lineEnd = text.FindFirstOf(String("\r\n"), position);
            UInt32 close = text.ReverseFind(']', lineEnd);
            if (close == String::npos || close < position) {
                // NOTE: no ']' on this line: skips this line and the next one.
                position = lineEnd;
                SkipLine(text, position);
            } else {
                String name(text, position + 1, close - position - 1);
                Trim(name);
                position = close + 1;
                section = FindSection(name);
                if (section == 0) {
                    section = AddSection(name);
                    if (section == 0)
                        return false;
                }
            }
            SkipLine(text, position);
        } else {
            // NOTE: an entry starting with '=' (empty key) does not advance `position`: both
            // builds loop forever on such a file.
            String name;
            ReadKeyName(text, position, name);
            if (name.GetLength() != 0) {
                Key* key = section->FindKey(name);
                if (key == 0)
                    key = section->AddKey(name, String());
                if (!IsValidPosition(text, position))
                    break;
                if (text.IsCharAt(position, '=')) {
                    ++position;
                    ReadValue(text, position, key->m_value);
                }
            }
        }
        SkipWhitespace(text, position, true);
    }

    m_path = path;
    if (keepOpen)
        m_file = file;
    return true;
}

// Mac 0x5000, Win 0x433fa0
// NOTE: the original rewrites the file in place and ignores the results of WriteAt / Write /
//   SetSize: a failed or partial write (a full disk) leaves a truncated file (the preferences
//   are lost at the next start) and Save still returns true; fixed: a file that is not held
//   open (no m_file, !keepOpen) is written whole with WriteWholeFile (on the disk "<name>.tmp",
//   read back, then renamed over the file: all or nothing), a held file has every result
//   checked; false when anything failed (M2). The callers (Preferences::Save, LDDMain,
//   LDDApplication, PreferencesControl) ignore the result: no localized message fits, no dialog.
bool Config::Save(const Path& path, bool keepOpen)
{
    RefPointer<File> file = m_file;
    const bool whole = file.IsNull() && !keepOpen;
    if (file.IsNull() && !whole) {
        file = path.OpenFile(OpenMode(keepOpen, true), true);
        if (file.IsNull())
            return false;
        if (keepOpen)
            m_file = file;
    }

    String text;
    for (BasicList<Section*>::Iterator s = m_sections.Begin(); s != m_sections.End(); ++s) {
        Section* section = *s;
        if (section->m_name.GetLength() != 0) {
            text.Append("[");
            text.Append(section->m_name);
            text.Append("]\r\n");
        }
        for (BasicList<Key*>::Iterator k = section->m_keys.Begin(); k != section->m_keys.End(); ++k) {
            Key* key = *k;
            String item(key->m_name);
            QuoteIfNeeded(item);
            text.Append(item);
            text.Append("=");
            item = key->m_value;
            QuoteIfNeeded(item);
            text.Append(item);
            text.Append("\r\n");
        }
        text.Append("\r\n");
    }

    bool written;
    if (whole) {
        if (text.IsASCII()) {
            AString ascii = text.ToAString();
            written = WriteWholeFile(path, ascii.GetBuffer(), text.GetLength());
        } else {
            const UInt32 size = 2 + text.GetLength() * 2;
            UInt8* buffer = new UInt8[size];
            const UInt16 bom = 0xFEFF;
            memcpy(buffer, &bom, 2);
            memcpy(buffer + 2, text.GetBuffer(), size - 2);
            written = WriteWholeFile(path, buffer, size);
            delete[] buffer;
        }
    } else if (text.IsASCII()) {
        AString ascii = text.ToAString();
        written = file->WriteAt(0, text.GetLength(), ascii.GetBuffer(), 0);
    } else {
        UInt16 bom = 0xFEFF;
        written = file->WriteAt(0, 2, &bom, 0) &&
                  file->Write(text.GetLength() * 2, text.GetBuffer(), 0);
    }
    if (!whole)
        written = file->SetSize(file->GetPosition()) && written;
    m_path = path;
    return written;
}

} // namespace LEGO
