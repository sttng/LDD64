#!/usr/bin/env python3
"""LDD64 app icon, second pass: shaded plastic brick in a three-quarter view.

All geometry is computed from brick dimensions (2x2 footprint, stud pitch = 1);
nothing is traced from other artwork.
"""
import math
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from gen import CANVAS, add, arc, corner, f, mul, pt, ray_hit, squircle, sub, unit  # noqa: E402

BACKGROUNDS = {
    # name: (top colour, bottom colour, ground-shadow colour, ground-shadow opacity)
    "light": ("#FFFFFF", "#E3E5EB", "#1B1F2A", 0.26),
    "sun": ("#FFE98A", "#FFC233", "#8A4200", 0.38),
    "sky": ("#CDE9FF", "#74B8FA", "#0B2E5C", 0.30),
    "dark": ("#34353A", "#141416", "#000000", 0.55),
}

RED = {
    "top": ("#FF5648", "#EF372B"),        # left -> right across the top face
    "left": ("#E42D22", "#C51F15"),       # top -> bottom
    "right": ("#B81A11", "#970F09"),      # top -> bottom
    "edge": "#FF9A8E",
    "stud_top": ("#FF6A5B", "#F53E31"),   # back -> front
    "stud_side": ("#C4211A", "#F0463A", "#BD1C14", "#950F09"),  # left, highlight, mid, right
    "stud_rim": "#FFAB9F",
    "contact": "#8E0E08",
}


class Defs:
    def __init__(self, prefix=""):
        self.items = []
        self.prefix = prefix
        self.n = 0

    def uid(self, stem):
        self.n += 1
        return f"{self.prefix}{stem}{self.n}"

    def lin(self, p1, p2, stops):
        i = self.uid("g")
        s = "".join(f'<stop offset="{o}" stop-color="{c}"/>' for o, c in stops)
        self.items.append(
            f'<linearGradient id="{i}" gradientUnits="userSpaceOnUse" x1="{f(p1[0])}" y1="{f(p1[1])}" '
            f'x2="{f(p2[0])}" y2="{f(p2[1])}">{s}</linearGradient>')
        return f"url(#{i})"

    def blur(self, sd):
        i = self.uid("b")
        self.items.append(
            f'<filter id="{i}" x="-50%" y="-50%" width="200%" height="200%">'
            f'<feGaussianBlur stdDeviation="{f(sd)}"/></filter>')
        return f"url(#{i})"

    def clip(self, d):
        i = self.uid("c")
        self.items.append(f'<clipPath id="{i}"><path d="{d}"/></clipPath>')
        return f"url(#{i})"

    def render(self):
        return "".join(self.items)


def brick(defs, theta=35.0, phi=28.0, width=690.0, stud_r=0.31, stud_h=0.26, rho=15.0,
          fx=True, shadow=("#000000", 0.25), centre=(CANVAS / 2, CANVAS / 2 - 6)):
    """Returns dict of SVG fragments: ground, body, studs (and geometry)."""
    th, ph = math.radians(theta), math.radians(phi)
    ct, st, sp, cp = math.cos(th), math.sin(th), math.sin(ph), math.cos(ph)
    H = 1.2

    def raw(x, y, z):
        u = (x - 1) * ct - (y - 1) * st
        v = (x - 1) * st + (y - 1) * ct
        return (u, v * sp - z * cp)

    us = [raw(x, y, 0)[0] for x in (0, 2) for y in (0, 2)]
    S = width / (max(us) - min(us))
    stud_xy = sorted(((x, y) for x in (0.5, 1.5) for y in (0.5, 1.5)),
                     key=lambda p: raw(p[0], p[1], 0)[1])  # back to front
    ext_x = [raw(x, y, z)[0] for x in (0, 2) for y in (0, 2) for z in (0, H)]
    ext_y = [raw(x, y, z)[1] for x in (0, 2) for y in (0, 2) for z in (0, H)]
    for (x, y) in stud_xy:
        t = raw(x, y, H + stud_h)
        ext_y.append(t[1] - stud_r * sp)
    ox = centre[0] - S * (min(ext_x) + max(ext_x)) / 2
    oy = centre[1] - S * (min(ext_y) + max(ext_y)) / 2

    def P(x, y, z):
        r = raw(x, y, z)
        return (ox + S * r[0], oy + S * r[1])

    A, B, G, F = P(0, 0, H), P(2, 0, H), P(2, 2, H), P(0, 2, H)
    C, D, E = P(2, 0, 0), P(2, 2, 0), P(0, 2, 0)
    Z0 = P(0, 0, 0)

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

    if fx:
        g_top = defs.lin(F, B, [(0, RED["top"][0]), (1, RED["top"][1])])
        g_left = defs.lin((0, F[1]), (0, D[1]), [(0, RED["left"][0]), (1, RED["left"][1])])
        g_right = defs.lin((0, B[1]), (0, D[1]), [(0, RED["right"][0]), (1, RED["right"][1])])
    else:
        g_top, g_left, g_right = RED["top"][1], RED["left"][0], RED["right"][0]

    body = [
        f'<path fill="{RED["left"][1]}" d="{outline}"/>',
        f'<path fill="{g_left}" d="{left}"/>',
        f'<path fill="{g_right}" d="{right}"/>',
        f'<path fill="{g_top}" d="{top}"/>',
    ]
    if fx:
        # catch-light along the two front top edges and, fainter, the front vertical edge
        body.append(f'<path d="M{pt(xf)}L{pt(G)}L{pt(xb)}" fill="none" stroke="{RED["edge"]}" '
                    f'stroke-opacity="0.75" stroke-width="5" stroke-linejoin="round" stroke-linecap="round"/>')
        body.append(f'<path d="M{pt(G)}L{pt(xd)}" fill="none" stroke="{RED["edge"]}" '
                    f'stroke-opacity="0.28" stroke-width="4" stroke-linecap="round"/>')

    studs, contacts = [], []
    ra, rb = S * stud_r, S * stud_r * sp
    for (x, y) in stud_xy:
        b, t = P(x, y, H), P(x, y, H + stud_h)
        if fx:
            contacts.append(f'<ellipse cx="{f(b[0] + ra * 0.34)}" cy="{f(b[1] + rb * 0.30)}" '
                            f'rx="{f(ra * 1.04)}" ry="{f(rb * 1.04)}" fill="{RED["contact"]}" fill-opacity="0.42"/>')
            g_side = defs.lin((b[0] - ra, 0), (b[0] + ra, 0),
                              [(0, RED["stud_side"][0]), (0.2, RED["stud_side"][1]),
                               (0.55, RED["stud_side"][2]), (1, RED["stud_side"][3])])
            g_cap = defs.lin((0, t[1] - rb), (0, t[1] + rb), [(0, RED["stud_top"][0]), (1, RED["stud_top"][1])])
        else:
            g_side, g_cap = RED["stud_side"][2], RED["stud_top"][0]
        side = (f"M{f(t[0] - ra)} {f(t[1])}L{f(b[0] - ra)} {f(b[1])}"
                f"A{f(ra)} {f(rb)} 0 0 0 {f(b[0] + ra)} {f(b[1])}L{f(t[0] + ra)} {f(t[1])}Z")
        cap = (f"M{f(t[0] - ra)} {f(t[1])}A{f(ra)} {f(rb)} 0 1 0 {f(t[0] + ra)} {f(t[1])}"
               f"A{f(ra)} {f(rb)} 0 1 0 {f(t[0] - ra)} {f(t[1])}Z")
        studs.append(f'<path fill="{g_side}" d="{side}"/>')
        studs.append(f'<path fill="{g_cap}" d="{cap}"/>')
        if fx:
            # front rim catch-light: lower arc of the cap between 200 and 320 degrees (screen angles)
            a0, a1_ = math.radians(155), math.radians(40)
            p0 = (t[0] + ra * math.cos(a0), t[1] + rb * math.sin(a0))
            p1 = (t[0] + ra * math.cos(a1_), t[1] + rb * math.sin(a1_))
            studs.append(f'<path d="M{pt(p0)}A{f(ra)} {f(rb)} 0 0 0 {pt(p1)}" fill="none" '
                         f'stroke="{RED["stud_rim"]}" stroke-opacity="0.7" stroke-width="3.5" stroke-linecap="round"/>')

    ground = ""
    if fx:
        blur = defs.blur(15)
        clip = defs.clip(top)
        quad = f"M{pt(Z0)}L{pt(C)}L{pt(D)}L{pt(E)}Z"
        ground = (f'<path d="{quad}" transform="translate(10 20)" fill="{shadow[0]}" '
                  f'fill-opacity="{shadow[1]}" filter="{blur}"/>')
        soft = defs.blur(5)
        body.append(f'<g clip-path="{clip}"><g filter="{soft}">' + "".join(contacts) + "</g></g>")

    return {"ground": ground, "body": "\n".join(body), "studs": "\n".join(studs),
            "bbox": (ox + S * min(ext_x), oy + S * min(ext_y), ox + S * max(ext_x), oy + S * max(ext_y))}


def tile(defs):
    """Top-down 2x2 brick filling the whole canvas (the icon itself is the brick)."""
    bg = defs.lin((0, 0), (0, CANVAS), [(0, "#FB483B"), (1, "#D5231A")])
    out = [f'<rect width="{CANVAS}" height="{CANVAS}" fill="{bg}"/>']
    r, lift = 148.0, 30.0
    blur = defs.blur(16)
    shadows, studs = [], []
    for cy in (256, 768):
        for cx in (256, 768):
            ty, by = cy - lift / 2, cy + lift / 2
            shadows.append(f'<ellipse cx="{cx + 6}" cy="{f(by + 30)}" rx="{r}" ry="{r}" fill="#7C0A05" fill-opacity="0.5"/>')
            g_side = defs.lin((cx - r, 0), (cx + r, 0), [(0, "#BC1D15"), (0.22, "#E03327"), (0.6, "#AE170F"), (1, "#8C0D08")])
            g_cap = defs.lin((0, ty - r), (0, ty + r), [(0, "#FF7566"), (1, "#F6402F")])
            side = (f"M{cx - r} {f(ty)}L{cx - r} {f(by)}A{r} {r} 0 0 0 {cx + r} {f(by)}L{cx + r} {f(ty)}Z")
            studs.append(f'<path fill="{g_side}" d="{side}"/>')
            studs.append(f'<circle cx="{cx}" cy="{f(ty)}" r="{r}" fill="{g_cap}"/>')
            a0, a1 = math.radians(160), math.radians(20)
            p0 = (cx + r * math.cos(a0), ty + r * math.sin(a0))
            p1 = (cx + r * math.cos(a1), ty + r * math.sin(a1))
            studs.append(f'<path d="M{pt(p0)}A{r} {r} 0 0 0 {pt(p1)}" fill="none" stroke="#FFB3A8" '
                         f'stroke-opacity="0.65" stroke-width="4" stroke-linecap="round"/>')
    out.append(f'<g filter="{blur}">' + "".join(shadows) + "</g>")
    out.extend(studs)
    return "\n".join(out)


def background(defs, name):
    top, bottom, _, _ = BACKGROUNDS[name]
    g = defs.lin((0, 0), (0, CANVAS), [(0, top), (1, bottom)])
    return f'<rect width="{CANVAS}" height="{CANVAS}" fill="{g}"/>'


def fullbleed(inner, defs):
    return (f'<svg xmlns="http://www.w3.org/2000/svg" width="{CANVAS}" height="{CANVAS}" '
            f'viewBox="0 0 {CANVAS} {CANVAS}">\n<defs>{defs.render()}</defs>\n{inner}\n</svg>\n')


def macos_grid(inner, defs, inset=100.0):
    """Pre-Tahoe macOS grid: rounded shape inset on the canvas with the template drop shadow."""
    size = CANVAS - 2 * inset
    m = size / 824.0
    sq = squircle(inset, inset, size, 185.4 * m)
    extra = (f'<clipPath id="shape"><path d="{sq}"/></clipPath>'
             '<filter id="shadow" x="-20%" y="-20%" width="140%" height="140%">'
             f'<feGaussianBlur in="SourceAlpha" stdDeviation="{f(14 * m)}"/><feOffset dy="{f(12 * m)}"/>'
             '<feComponentTransfer><feFuncA type="linear" slope="0.5"/></feComponentTransfer></filter>')
    return (f'<svg xmlns="http://www.w3.org/2000/svg" width="{CANVAS}" height="{CANVAS}" '
            f'viewBox="0 0 {CANVAS} {CANVAS}">\n<defs>{defs.render()}{extra}</defs>\n'
            f'<path d="{sq}" filter="url(#shadow)"/>\n'
            f'<g clip-path="url(#shape)"><g transform="translate({f(inset)} {f(inset)}) scale({size / CANVAS})">\n'
            f'{inner}\n</g></g>\n</svg>\n')


def compose(bg_name, **kw):
    defs = Defs()
    _, _, sc, so = BACKGROUNDS[bg_name]
    parts = brick(defs, shadow=(sc, so), **kw)
    inner = "\n".join([background(defs, bg_name), parts["ground"], parts["body"], parts["studs"]])
    return inner, defs, parts


if __name__ == "__main__":
    out = sys.argv[1]
    os.makedirs(out, exist_ok=True)
    for name in BACKGROUNDS:
        inner, defs, parts = compose(name)
        open(os.path.join(out, f"brick-{name}.svg"), "w").write(macos_grid(inner, defs))
        open(os.path.join(out, f"brick-{name}-fullbleed.svg"), "w").write(fullbleed(inner, defs))
    print("bbox", [round(v, 1) for v in parts["bbox"]])
    d = Defs()
    inner = tile(d)
    open(os.path.join(out, "tile.svg"), "w").write(macos_grid(inner, d))
    open(os.path.join(out, "tile-fullbleed.svg"), "w").write(fullbleed(inner, d))
