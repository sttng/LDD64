#!/usr/bin/env python3
"""Resources of the original LDD.exe -> files the restored build compiles (APP_PLAN item 0.2).

The original .rsrc (no dialogs / menus / string tables) holds 2 group icons (101, 103), 16 group
cursors (102, 104-110, 112, 115, 116, 127, 128, 131, 133, 134), VERSIONINFO 1 and the asInvoker
manifest 1; languages 1030 (Danish) and 1033 (English US).

  extract_rsrc.py <LDD.exe> <out dir> [--check-rc <committed LDD.rc>]
      writes <out dir>/{icon_<id>.ico, cursor_<id>.cur, LDD.exe.manifest} (LEGO artwork: build
      output only, never committed); with --check-rc also warns when the committed .rc differs from
      --write-rc output and copies it + resource.h next to them (zig's rc resolves file names there).
  extract_rsrc.py <LDD.exe> --write-rc <dir>
      (re)generates <dir>/LDD.rc and resource.h (committed in src/Products/LDD/Resources/Win/).
  extract_rsrc.py --list <exe>
      normalised resource listing (type, name, language, size, SHA-1; image ids replaced by group
      + position) for comparing two executables:
      diff <(extract_rsrc.py --list orig/win/LDD.exe) <(extract_rsrc.py --list build/app/win/LDD.exe)

The .rc lists the groups in the order of their original image ids (icons 1-26, cursors 27-42), so
rc.exe and zig's resinator, which number images in statement order, give the original ids.
"""
import hashlib
import os
import shutil
import struct
import sys

import pefile

RT_CURSOR, RT_ICON, RT_GROUP_CURSOR, RT_GROUP_ICON, RT_VERSION, RT_MANIFEST = 1, 3, 12, 14, 16, 24
TYPE_NAMES = {1: 'RT_CURSOR', 3: 'RT_ICON', 12: 'RT_GROUP_CURSOR', 14: 'RT_GROUP_ICON',
              16: 'RT_VERSION', 24: 'RT_MANIFEST'}
LANG_MACRO = {1030: 'LANGUAGE 0x06, 0x01   /* Danish (Denmark), 1030 */',
              1033: 'LANGUAGE 0x09, 0x01   /* English (US), 1033 */'}


def resources(pe):
    """{type: {name: [(lang, bytes)]}}"""
    out = {}
    if not hasattr(pe, 'DIRECTORY_ENTRY_RESOURCE'):
        return out
    for t in pe.DIRECTORY_ENTRY_RESOURCE.entries:
        tid = t.id if t.id is not None else str(t.name)
        for n in t.directory.entries:
            nid = n.id if n.id is not None else str(n.name)
            for l in n.directory.entries:
                d = pe.get_data(l.data.struct.OffsetToData, l.data.struct.Size)
                out.setdefault(tid, {}).setdefault(nid, []).append((l.id, d))
    return out


def group_entries(data, cursor):
    """GRPICONDIR / GRPCURSORDIR entries: list of dicts."""
    _, typ, count = struct.unpack_from('<HHH', data, 0)
    ents = []
    for i in range(count):
        off = 6 + 14 * i
        if cursor:
            w, h, planes, bits, size, rid = struct.unpack_from('<HHHHIH', data, off)
            ents.append(dict(w=w, h=h, planes=planes, bits=bits, size=size, id=rid))
        else:
            w, h, colors, res, planes, bits, size, rid = struct.unpack_from('<BBBBHHIH', data, off)
            ents.append(dict(w=w, h=h, colors=colors, res=res, planes=planes, bits=bits, size=size, id=rid))
    return ents


def build_ico(group, images):
    ents = group_entries(group, False)
    out = struct.pack('<HHH', 0, 1, len(ents))
    off = 6 + 16 * len(ents)
    body = b''
    for e in ents:
        img = images[e['id']]
        out += struct.pack('<BBBBHHII', e['w'], e['h'], e['colors'], e['res'], e['planes'], e['bits'], len(img), off)
        off += len(img)
        body += img
    return out + body


def build_cur(group, images):
    ents = group_entries(group, True)
    out = struct.pack('<HHH', 0, 2, len(ents))
    off = 6 + 16 * len(ents)
    body = b''
    for e in ents:
        img = images[e['id']]
        hx, hy = struct.unpack_from('<HH', img, 0)
        dib = img[4:]
        # BITMAPINFOHEADER: biWidth, biHeight (= 2 * height: XOR + AND mask), biBitCount
        bw, bh = struct.unpack_from('<ii', dib, 4)
        bits, = struct.unpack_from('<H', dib, 14)
        colors = 0 if bits >= 8 else (1 << bits)
        out += struct.pack('<BBBBHHII', bw & 0xff, (bh // 2) & 0xff, colors, 0, hx, hy, len(dib), off)
        off += len(dib)
        body += dib
    return out + body


def single(res, typ):
    names = res.get(typ, {})
    return sorted(names.items())


def version_rc(pe):
    ffi = pe.VS_FIXEDFILEINFO[0]
    strings, trans = {}, None
    langid = '000004b0'
    for fi in pe.FileInfo[0]:
        if fi.Key == b'StringFileInfo':
            for st in fi.StringTable:
                langid = st.LangID.decode()
                for k, v in st.entries.items():
                    strings[k.decode()] = v.decode('utf-8')
        elif fi.Key == b'VarFileInfo':
            for v in fi.Var:
                trans = v.entry
    def ver(ms, ls):
        return '%d,%d,%d,%d' % (ms >> 16, ms & 0xffff, ls >> 16, ls & 0xffff)
    def rcstr(s):
        o = ''
        for ch in s:
            if ch == '"':
                o += '""'
            elif ch == '\\':
                o += '\\\\'
            elif 32 <= ord(ch) < 127:
                o += ch
            else:
                o += '\\x%04X' % ord(ch)
        return 'L"%s"' % o
    lines = ['VS_VERSION_INFO VERSIONINFO',
             ' FILEVERSION %s' % ver(ffi.FileVersionMS, ffi.FileVersionLS),
             ' PRODUCTVERSION %s' % ver(ffi.ProductVersionMS, ffi.ProductVersionLS),
             ' FILEFLAGSMASK 0x%xL' % ffi.FileFlagsMask,
             ' FILEFLAGS 0x%xL' % ffi.FileFlags,
             ' FILEOS 0x%xL' % ffi.FileOS,
             ' FILETYPE 0x%xL' % ffi.FileType,
             ' FILESUBTYPE 0x%xL' % ffi.FileSubtype,
             'BEGIN',
             '    BLOCK "StringFileInfo"',
             '    BEGIN',
             '        BLOCK "%s"' % langid,
             '        BEGIN']
    for k, v in strings.items():
        lines.append('            VALUE "%s", %s' % (k, rcstr(v)))
    lines += ['        END', '    END']
    if trans:
        a, b = [int(x, 16) for x in list(trans.values())[0].split()]
        lines += ['    BLOCK "VarFileInfo"', '    BEGIN',
                  '        VALUE "Translation", 0x%x, %d' % (a, b), '    END']
    lines.append('END')
    return lines


def generate_rc(pe):
    res = resources(pe)
    rc = ['// LEGO Digital Designer — resource script (Win). GENERATED by tools/extract_rsrc.py from',
          '// orig/win/LDD.exe; do not edit. The icon / cursor / manifest files named here are LEGO',
          '// artwork extracted at build time into build/app/win/rsrc (not committed); tools/build_app.sh',
          '// copies this file next to them before compiling it.',
          '// Statement order = original image ids (icons 1-26, then cursors 27-42).',
          '#include "resource.h"', '']
    hdr = ['// LEGO Digital Designer — resource ids (Win). GENERATED by tools/extract_rsrc.py; names inferred',
           '// (the original resource.h is not known: ids only, as used by LoadIcon / LoadCursor calls).',
           '#pragma once', '', '#ifndef VS_VERSION_INFO', '#define VS_VERSION_INFO      1', '#endif', '']
    groups = []
    for typ, kind in ((RT_GROUP_ICON, 'ICON'), (RT_GROUP_CURSOR, 'CURSOR')):
        for name, langs in single(res, typ):
            for lang, data in langs:
                first = min(e['id'] for e in group_entries(data, typ == RT_GROUP_CURSOR))
                groups.append((typ, first, name, lang, kind))
    groups.sort(key=lambda g: (g[0] != RT_GROUP_ICON, g[1]))
    for typ, first, name, lang, kind in groups:
        macro = ('IDI_ICON_%d' if kind == 'ICON' else 'IDC_CURSOR_%d') % name
        hdr.append('#define %-20s %d' % (macro, name))
        fn = ('icon_%d.ico' if kind == 'ICON' else 'cursor_%d.cur') % name
        rc.append(LANG_MACRO[lang])
        rc.append('%s %s "%s"' % (macro, kind, fn))
    for name, langs in single(res, RT_VERSION):
        for lang, _ in langs:
            rc.append('')
            rc.append(LANG_MACRO[lang])
            rc += version_rc(pe)
    for name, langs in single(res, RT_MANIFEST):
        for lang, _ in langs:
            rc.append('')
            rc.append(LANG_MACRO[lang])
            rc.append('%d 24 "LDD.exe.manifest"   /* RT_MANIFEST */' % name)
    rc.append('')
    hdr.append('')
    return '\n'.join(rc), '\n'.join(hdr)


def extract(pe, out):
    os.makedirs(out, exist_ok=True)
    res = resources(pe)
    icons = {n: l[0][1] for n, l in res.get(RT_ICON, {}).items()}
    curs = {n: l[0][1] for n, l in res.get(RT_CURSOR, {}).items()}
    written = []
    def put(fn, data):
        p = os.path.join(out, fn)
        if not os.path.exists(p) or open(p, 'rb').read() != data:
            open(p, 'wb').write(data)
        written.append(fn)
    for name, langs in res.get(RT_GROUP_ICON, {}).items():
        put('icon_%d.ico' % name, build_ico(langs[0][1], icons))
    for name, langs in res.get(RT_GROUP_CURSOR, {}).items():
        put('cursor_%d.cur' % name, build_cur(langs[0][1], curs))
    for name, langs in res.get(RT_MANIFEST, {}).items():
        put('LDD.exe.manifest', langs[0][1])
    return written


def listing(pe):
    res = resources(pe)
    lines = []
    # individual images named by (group, index)
    image_name = {}
    for gtyp, ityp in ((RT_GROUP_ICON, RT_ICON), (RT_GROUP_CURSOR, RT_CURSOR)):
        for name, langs in res.get(gtyp, {}).items():
            for lang, data in langs:
                for i, e in enumerate(group_entries(data, gtyp == RT_GROUP_CURSOR)):
                    image_name[(ityp, e['id'])] = '%d[%d]' % (name, i)
    for typ in sorted(res, key=str):
        for name in sorted(res[typ], key=str):
            for lang, data in res[typ][name]:
                shown = name
                if typ in (RT_GROUP_ICON, RT_GROUP_CURSOR):
                    # zero the image ids (last WORD of each 14-byte entry)
                    d = bytearray(data)
                    for i in range(struct.unpack_from('<H', d, 4)[0]):
                        struct.pack_into('<H', d, 6 + 14 * i + 12, 0)
                    data = bytes(d)
                elif typ in (RT_ICON, RT_CURSOR):
                    shown = image_name.get((typ, name), '?%s' % name)
                lines.append('%-16s %-10s %5s %7d %s' % (TYPE_NAMES.get(typ, typ), shown, lang, len(data),
                                                      hashlib.sha1(data).hexdigest()[:16]))
    return sorted(lines)


def main(argv):
    if len(argv) >= 2 and argv[0] == '--list':
        print('\n'.join(listing(pefile.PE(argv[1]))))
        return 0
    if len(argv) < 2:
        print(__doc__)
        return 2
    pe = pefile.PE(argv[0])
    if argv[1] == '--write-rc':
        rc, hdr = generate_rc(pe)
        os.makedirs(argv[2], exist_ok=True)
        open(os.path.join(argv[2], 'LDD.rc'), 'w').write(rc)
        open(os.path.join(argv[2], 'resource.h'), 'w').write(hdr)
        print('wrote %s/LDD.rc, resource.h' % argv[2])
        return 0
    out = argv[1]
    extract(pe, out)
    if '--check-rc' in argv:
        committed = argv[argv.index('--check-rc') + 1]
        rc, hdr = generate_rc(pe)
        if not os.path.exists(committed) or open(committed).read() != rc:
            print('extract_rsrc.py: WARNING %s differs from the generated script '
                  '(tools/extract_rsrc.py %s --write-rc %s)' % (committed, argv[0], os.path.dirname(committed)))
        src_dir = os.path.dirname(committed)
        for fn in ('LDD.rc', 'resource.h'):
            s = os.path.join(src_dir, fn)
            if os.path.exists(s):
                d = os.path.join(out, fn)
                if not os.path.exists(d) or open(d, 'rb').read() != open(s, 'rb').read():
                    shutil.copyfile(s, d)
    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
