#!/usr/bin/env python3
"""LDD64 app icon, third pass: perspective camera fitted to the original LDD icon's pose.

Camera (fitted to the original's silhouette): yaw 39.5 deg, pitch 32.5 deg, eye distance 4.93
stud pitches from the brick centre. Brick 2x2, height 1.2, stud radius 0.31, stud height 0.19.
"""
import math
import os
import sys

import numpy as np

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from gen import CANVAS, arc, corner, f, pt, ray_hit  # noqa: E402
from gen2 import BACKGROUNDS, RED, Defs, fullbleed, macos_grid  # noqa: E402

H = 1.2
CAM = dict(yaw=39.48, pitch=32.46, dist=4.932)
STUD_R, STUD_H = 0.31, 0.19


def projector(yaw, pitch, dist):
    cy, sy = math.cos(math.radians(yaw)), math.sin(math.radians(yaw))
    cp, sp = math.cos(math.radians(pitch)), math.sin(math.radians(pitch))

    def P(X):
        X = np.asarray(X, float) - np.array([1, 1, H / 2])
        x = X[..., 0] * cy - X[..., 1] * sy
        y = X[..., 0] * sy + X[..., 1] * cy
        z = X[..., 2]
        k = dist / (dist - (y * cp + z * sp))
        return np.stack([x * k, -(z * cp - y * sp) * k], -1)
    return P


def circle(cx, cy, z, r, n=96):
    t = np.linspace(0, 2 * np.pi, n, endpoint=False)
    return np.stack([cx + r * np.cos(t), cy + r * np.sin(t), np.full(n, z)], -1)


def hull(points):
    pts = sorted(map(tuple, points))

    def cross(o, a, b):
        return (a[0] - o[0]) * (b[1] - o[1]) - (a[1] - o[1]) * (b[0] - o[0])
    lo, up = [], []
    for p in pts:
        while len(lo) >= 2 and cross(lo[-2], lo[-1], p) <= 0:
            lo.pop()
        lo.append(p)
    for p in reversed(pts):
        while len(up) >= 2 and cross(up[-2], up[-1], p) <= 0:
            up.pop()
        up.append(p)
    return lo[:-1] + up[:-1]


def poly(points):
    return "M" + "L".join(pt(p) for p in points) + "Z"


FONT = "/usr/share/fonts/opentype/inter/Inter-Bold.otf"


def glyph_contours(text, font_path=FONT, steps=10):
    """Outline contours of `text` in font units, curves flattened. Returns (contours, w, h)."""
    from fontTools.pens.recordingPen import RecordingPen
    from fontTools.ttLib import TTFont
    font = TTFont(font_path)
    gs, cmap = font.getGlyphSet(), font.getBestCmap()
    contours, cur, x0 = [], [], 0.0
    for ch in text:
        name = cmap[ord(ch)]
        rec = RecordingPen()
        gs[name].draw(rec)
        pos = None
        for op, args in rec.value:
            pts = [(a[0] + x0, a[1]) for a in args]
            if op == "moveTo":
                cur = [pts[0]]
                pos = pts[0]
            elif op == "lineTo":
                cur.append(pts[0])
                pos = pts[0]
            elif op in ("curveTo", "qCurveTo"):
                ctrl = [pos] + pts
                if op == "qCurveTo" and len(ctrl) > 3:  # implied on-curve points
                    seq = [ctrl[0]]
                    for i in range(1, len(ctrl) - 2):
                        seq += [ctrl[i], ((ctrl[i][0] + ctrl[i + 1][0]) / 2, (ctrl[i][1] + ctrl[i + 1][1]) / 2)]
                    seq += ctrl[-2:]
                    segs = [seq[i:i + 3] for i in range(0, len(seq) - 1, 2)]
                else:
                    segs = [ctrl]
                for sg in segs:
                    n = len(sg) - 1
                    for k in range(1, steps + 1):
                        t = k / steps
                        pts_ = list(sg)
                        for _ in range(n):  # de Casteljau
                            pts_ = [((1 - t) * a[0] + t * b[0], (1 - t) * a[1] + t * b[1]) for a, b in zip(pts_, pts_[1:])]
                        cur.append(pts_[0])
                    pos = sg[-1]
            elif op in ("closePath", "endPath"):
                if cur:
                    contours.append(cur)
                cur = []
        x0 += gs[name].width
    xs = [q[0] for c in contours for q in c]
    ys = [q[1] for c in contours for q in c]
    return contours, (min(xs), min(ys), max(xs), max(ys))


def stud_label(P, sx, sy, z, text_w, contours, box, dx=0.0, dy=0.0):
    """Lay the label flat on the stud top (reading along +x, letter tops toward -y) and project."""
    x0, y0, x1, y1 = box
    k = text_w / (x1 - x0)
    cu, cv = (x0 + x1) / 2, (y0 + y1) / 2
    d = ""
    for c in contours:
        pts3 = np.array([[sx + (u - cu) * k + dx, sy - (v - cv) * k + dy, z] for (u, v) in c])
        d += poly(P(pts3))
    return d


def build(defs, width=700.0, centre=(CANVAS / 2, CANVAS / 2 - 4), shadow=("#000000", 0.3), rho=13.0,
          stud_r=STUD_R, stud_h=STUD_H, cam=CAM, label="LDD"):
    P0 = projector(**cam)
    probe = [(x, y, z) for x in (0, 2) for y in (0, 2) for z in (0, H)]
    for sx in (0.5, 1.5):
        for sy in (0.5, 1.5):
            probe += list(map(tuple, circle(sx, sy, H + stud_h, stud_r, 48)))
    sp = P0(probe)
    s = width / (sp[:, 0].max() - sp[:, 0].min())
    off = np.array(centre) - s * (sp.min(0) + sp.max(0)) / 2

    def P(X):
        return P0(X) * s + off

    A, B, G, F = (tuple(P(c)) for c in ([0, 0, H], [2, 0, H], [2, 2, H], [0, 2, H]))
    C, D, E = (tuple(P(c)) for c in ([2, 0, 0], [2, 2, 0], [0, 2, 0]))

    a1, a2, _ = corner(F, A, B, rho)
    b1, b2, bc = corner(A, B, C, rho)
    c1, c2, _ = corner(B, C, D, rho)
    d1, d2, dc = corner(C, D, E, rho)
    e1, e2, _ = corner(D, E, F, rho)
    f1, f2, fc = corner(E, F, A, rho)
    xb, xd, xf = ray_hit(B, G, bc, rho), ray_hit(D, G, dc, rho), ray_hit(F, G, fc, rho)
    top = f"M{pt(a1)}{arc(rho, a2)}L{pt(b1)}{arc(rho, xb)}L{pt(G)}L{pt(xf)}{arc(rho, f2)}Z"
    right = f"M{pt(G)}L{pt(xb)}{arc(rho, b2)}L{pt(c1)}{arc(rho, c2)}L{pt(d1)}{arc(rho, xd)}Z"
    left = f"M{pt(G)}L{pt(xd)}{arc(rho, d2)}L{pt(e1)}{arc(rho, e2)}L{pt(f1)}{arc(rho, xf)}Z"
    outline = (f"M{pt(a1)}{arc(rho, a2)}L{pt(b1)}{arc(rho, b2)}L{pt(c1)}{arc(rho, c2)}"
               f"L{pt(d1)}{arc(rho, d2)}L{pt(e1)}{arc(rho, e2)}L{pt(f1)}{arc(rho, f2)}Z")

    g_top = defs.lin(F, B, [(0, "#FF5A4B"), (0.55, "#F7412F"), (1, "#E8342A")])
    g_left = defs.lin(((F[0] + G[0]) / 2, (F[1] + G[1]) / 2), ((E[0] + D[0]) / 2, (E[1] + D[1]) / 2),
                      [(0, "#EA3226"), (1, "#C41F15")])
    g_right = defs.lin(((G[0] + B[0]) / 2, (G[1] + B[1]) / 2), ((D[0] + C[0]) / 2, (D[1] + C[1]) / 2),
                       [(0, "#B3180F"), (1, "#8E0D07")])

    # ground shadow: base footprint pushed down-right, blurred
    base = P([[0, 0, 0], [2, 0, 0], [2, 2, 0], [0, 2, 0]])
    ground = (f'<path d="{poly(base)}" transform="translate(14 24)" fill="{shadow[0]}" '
              f'fill-opacity="{shadow[1]}" filter="{defs.blur(16)}"/>')

    body = [f'<path fill="#C62016" d="{outline}"/>',
            f'<path fill="{g_left}" d="{left}"/>',
            f'<path fill="{g_right}" d="{right}"/>',
            f'<path fill="{g_top}" d="{top}"/>',
            f'<path d="M{pt(xf)}L{pt(G)}L{pt(xb)}" fill="none" stroke="#FFA195" stroke-opacity="0.8" '
            f'stroke-width="4.5" stroke-linejoin="round" stroke-linecap="round"/>',
            f'<path d="M{pt(G)}L{pt(xd)}" fill="none" stroke="#FF8A7D" stroke-opacity="0.3" '
            f'stroke-width="3.5" stroke-linecap="round"/>']

    # stud contact shadows on the top face (offset toward the viewer's right, away from the light)
    contacts = []
    order = sorted(((x, y) for x in (0.5, 1.5) for y in (0.5, 1.5)), key=lambda q: q[0] + q[1])
    for (sx, sy) in order:
        sh = P(circle(sx + 0.10, sy + 0.04, H, stud_r * 1.08))
        contacts.append(f'<path d="{poly(sh)}" fill="#7E0B05" fill-opacity="0.45"/>')
    body.append(f'<g clip-path="{defs.clip(top)}"><g filter="{defs.blur(6)}">{"".join(contacts)}</g></g>')

    studs = []
    for (sx, sy) in order:  # back to front
        bot = P(circle(sx, sy, H, stud_r))
        cap = P(circle(sx, sy, H + stud_h, stud_r))
        side = hull(np.vstack([bot, cap]))
        lx, rx = bot[:, 0].min(), bot[:, 0].max()
        g_side = defs.lin((lx, 0), (rx, 0), [(0, "#C3221A"), (0.22, "#EF4436"), (0.55, "#BF1D15"), (1, "#8F0E08")])
        cy0, cy1 = cap[:, 1].min(), cap[:, 1].max()
        g_cap = defs.lin((0, cy0), (0, cy1), [(0, "#FF7060"), (1, "#F54032")])
        studs.append(f'<path fill="{g_side}" d="{poly(side)}"/>')
        studs.append(f'<path fill="{g_cap}" d="{poly(cap)}"/>')
        # catch-light on the near rim of the cap
        n = len(cap)
        idx = [i % n for i in range(int(n * 0.10), int(n * 0.47))]
        rim = cap[idx]
        if label:
            contours, box = glyph_contours(label)
            zt, tw = H + stud_h, stud_r * 1.45
            studs.append(f'<path fill-rule="nonzero" fill="#B5160E" fill-opacity="0.75" '
                         f'd="{stud_label(P, sx, sy, zt, tw, contours, box, 0.013, 0.013)}"/>')
            studs.append(f'<path fill-rule="nonzero" fill="#FFB1A5" fill-opacity="0.85" '
                         f'd="{stud_label(P, sx, sy, zt, tw, contours, box, -0.008, -0.008)}"/>')
            studs.append(f'<path fill-rule="nonzero" fill="#F9503F" '
                         f'd="{stud_label(P, sx, sy, zt, tw, contours, box)}"/>')
        studs.append(f'<path d="M{"L".join(pt(p) for p in rim)}" fill="none" stroke="#FFB4A8" '
                     f'stroke-opacity="0.75" stroke-width="3.2" stroke-linecap="round" stroke-linejoin="round"/>')

    return {"ground": ground, "body": "\n".join(body), "studs": "\n".join(studs)}


def compose(bg_name, **kw):
    defs = Defs()
    top, bottom, sc, so = BACKGROUNDS[bg_name]
    g = defs.lin((0, 0), (0, CANVAS), [(0, top), (1, bottom)])
    parts = build(defs, shadow=(sc, so), **kw)
    inner = "\n".join([f'<rect width="{CANVAS}" height="{CANVAS}" fill="{g}"/>',
                       parts["ground"], parts["body"], parts["studs"]])
    return inner, defs


if __name__ == "__main__":
    out = sys.argv[1]
    os.makedirs(out, exist_ok=True)
    for name in ("sun", "dark"):
        inner, defs = compose(name)
        open(os.path.join(out, f"brick-{name}.svg"), "w").write(macos_grid(inner, defs))
        open(os.path.join(out, f"brick-{name}-fullbleed.svg"), "w").write(fullbleed(inner, defs))
