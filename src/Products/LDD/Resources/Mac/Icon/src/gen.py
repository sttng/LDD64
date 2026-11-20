#!/usr/bin/env python3
"""Generate the LDD64 app icon artwork (layer SVGs + composites).

Usage: gen.py OUTDIR [--elev DEG] [--stud-r R] [--stud-h H] [--width PX] [--tag NAME]
All geometry is computed here; nothing is traced from other artwork.
"""
import argparse
import math
import os

CANVAS = 1024

# ---- palette (sRGB) -------------------------------------------------------
RED = {
    "stud_top": "#FF6A5C",
    "top": "#F4392D",
    "left": "#D3261C",
    "right": "#A3150E",
    "stud_side": "#B71B13",
}
BG_LIGHT = ("#FFFFFF", "#E7E8EC")
BG_DARK = ("#313131", "#141414")


def f(v):
    return f"{v:.2f}".rstrip("0").rstrip(".")


def sub(a, b):
    return (a[0] - b[0], a[1] - b[1])


def add(a, b):
    return (a[0] + b[0], a[1] + b[1])


def mul(a, k):
    return (a[0] * k, a[1] * k)


def unit(a):
    n = math.hypot(*a)
    return (a[0] / n, a[1] / n)


def corner(P, V, N, rho):
    """Rounded corner at V between neighbours P (before) and N (after).
    Returns tangent point on the P side, tangent point on the N side, centre."""
    d1, d2 = unit(sub(P, V)), unit(sub(N, V))
    cosang = max(-1.0, min(1.0, d1[0] * d2[0] + d1[1] * d2[1]))
    half = math.acos(cosang) / 2
    t = rho / math.tan(half)
    c = add(V, mul(unit(add(d1, d2)), rho / math.sin(half)))
    return add(V, mul(d1, t)), add(V, mul(d2, t)), c


def ray_hit(V, G, C, rho):
    """First intersection of the ray V->G with circle (C, rho)."""
    w = unit(sub(G, V))
    m = sub(V, C)
    b = 2 * (m[0] * w[0] + m[1] * w[1])
    c = m[0] ** 2 + m[1] ** 2 - rho ** 2
    disc = b * b - 4 * c
    t = (-b - math.sqrt(max(disc, 0))) / 2
    return add(V, mul(w, t))


def pt(p):
    return f"{f(p[0])} {f(p[1])}"


def arc(rho, p):
    return f"A{f(rho)} {f(rho)} 0 0 1 {pt(p)}"


def build(elev_deg=30.0, stud_r=0.33, stud_h=0.28, width=640.0, rho=22.0, cy_shift=0.0):
    phi = math.radians(elev_deg)
    sin, cos = math.sin(phi), math.cos(phi)
    S = width / (2 * math.sqrt(2))  # px per brick unit along the screen x axis
    W, H = 2.0, 1.2  # 2x2 footprint, standard brick height (in stud pitches)

    def proj(x, y, z):
        return (S * (x - y) / math.sqrt(2), S * (x + y) / math.sqrt(2) * sin - S * z * cos)

    A = proj(0, 0, H)   # back apex of top face
    B = proj(W, 0, H)   # right
    G = proj(W, W, H)   # front (inner vertex)
    F = proj(0, W, H)   # left
    C = proj(W, 0, 0)
    D = proj(W, W, 0)
    E = proj(0, W, 0)

    studs = []
    for (x, y) in ((0.5, 0.5), (1.5, 0.5), (0.5, 1.5), (1.5, 1.5)):  # back to front
        base = proj(x, y, H)
        top = proj(x, y, H + stud_h)
        studs.append((base, top, S * stud_r, S * stud_r * sin))

    # bounding box -> centre on canvas
    ys = [A[1], D[1]] + [s[1][1] - s[3] for s in studs]
    xs = [E[0], C[0]]
    ox = CANVAS / 2 - (min(xs) + max(xs)) / 2
    oy = CANVAS / 2 - (min(ys) + max(ys)) / 2 + cy_shift

    def T(p):
        return (p[0] + ox, p[1] + oy)

    A, B, C, D, E, F, G = map(T, (A, B, C, D, E, F, G))
    studs = [(T(b), T(t), a, bb) for (b, t, a, bb) in studs]

    # silhouette corners (clockwise on screen): A B C D E F
    a1, a2, _ = corner(F, A, B, rho)
    b1, b2, bc = corner(A, B, C, rho)
    c1, c2, _ = corner(B, C, D, rho)
    d1, d2, dc = corner(C, D, E, rho)
    e1, e2, _ = corner(D, E, F, rho)
    f1, f2, fc = corner(E, F, A, rho)
    xb = ray_hit(B, G, bc, rho)
    xd = ray_hit(D, G, dc, rho)
    xf = ray_hit(F, G, fc, rho)

    top = (f"M{pt(a1)}{arc(rho, a2)}L{pt(b1)}{arc(rho, xb)}L{pt(G)}L{pt(xf)}{arc(rho, f2)}Z")
    right = (f"M{pt(G)}L{pt(xb)}{arc(rho, b2)}L{pt(c1)}{arc(rho, c2)}L{pt(d1)}{arc(rho, xd)}Z")
    left = (f"M{pt(G)}L{pt(xd)}{arc(rho, d2)}L{pt(e1)}{arc(rho, e2)}L{pt(f1)}{arc(rho, xf)}Z")

    outline = (f"M{pt(a1)}{arc(rho, a2)}L{pt(b1)}{arc(rho, b2)}L{pt(c1)}{arc(rho, c2)}"
               f"L{pt(d1)}{arc(rho, d2)}L{pt(e1)}{arc(rho, e2)}L{pt(f1)}{arc(rho, f2)}Z")
    brick = (
        f'<path fill="{RED["left"]}" d="{outline}"/>\n'
        f'<path fill="{RED["left"]}" d="{left}"/>\n'
        f'<path fill="{RED["right"]}" d="{right}"/>\n'
        f'<path fill="{RED["top"]}" d="{top}"/>'
    )

    stud_parts = []
    for (b, t, ra, rb) in studs:
        side = (f"M{f(t[0] - ra)} {f(t[1])}L{f(b[0] - ra)} {f(b[1])}"
                f"A{f(ra)} {f(rb)} 0 0 0 {f(b[0] + ra)} {f(b[1])}"
                f"L{f(t[0] + ra)} {f(t[1])}Z")
        stud_parts.append(f'<path fill="{RED["stud_side"]}" d="{side}"/>')
        cap = (f"M{f(t[0] - ra)} {f(t[1])}A{f(ra)} {f(rb)} 0 1 0 {f(t[0] + ra)} {f(t[1])}"
               f"A{f(ra)} {f(rb)} 0 1 0 {f(t[0] - ra)} {f(t[1])}Z")
        stud_parts.append(f'<path fill="{RED["stud_top"]}" d="{cap}"/>')
    studs_svg = "\n".join(stud_parts)

    bbox = (min(xs) + ox, min(ys) + oy, max(xs) + ox, max(ys) + oy)
    return brick, studs_svg, bbox


def svg(body, defs=""):
    return (f'<svg xmlns="http://www.w3.org/2000/svg" width="{CANVAS}" height="{CANVAS}" '
            f'viewBox="0 0 {CANVAS} {CANVAS}">\n{defs}{body}\n</svg>\n')


def bg(colors, gid="bg"):
    defs = (f'<defs><linearGradient id="{gid}" x1="0" y1="0" x2="0" y2="1">'
            f'<stop offset="0" stop-color="{colors[0]}"/><stop offset="1" stop-color="{colors[1]}"/>'
            f'</linearGradient></defs>\n')
    rect = f'<rect width="{CANVAS}" height="{CANVAS}" fill="url(#{gid})"/>'
    return defs, rect


def squircle(x, y, w, r):
    """Continuous-corner rounded square (the usual Bezier approximation)."""
    k = [1.528665, 1.08849296, 0.86840694, 0.63149379, 0.37282383, 0.16905956, 0.07491139]
    a, b, c, d, e, g, h = (v * r for v in k)
    X, Y = x + w, y + w
    return (
        f"M{f(x + a)} {f(y)}L{f(X - a)} {f(y)}"
        f"C{f(X - b)} {f(y)} {f(X - c)} {f(y)} {f(X - d)} {f(y + h)}"
        f"C{f(X - e)} {f(y + g)} {f(X - g)} {f(y + e)} {f(X - h)} {f(y + d)}"
        f"C{f(X)} {f(y + c)} {f(X)} {f(y + b)} {f(X)} {f(y + a)}"
        f"L{f(X)} {f(Y - a)}"
        f"C{f(X)} {f(Y - b)} {f(X)} {f(Y - c)} {f(X - h)} {f(Y - d)}"
        f"C{f(X - g)} {f(Y - e)} {f(X - e)} {f(Y - g)} {f(X - d)} {f(Y - h)}"
        f"C{f(X - c)} {f(Y)} {f(X - b)} {f(Y)} {f(X - a)} {f(Y)}"
        f"L{f(x + a)} {f(Y)}"
        f"C{f(x + b)} {f(Y)} {f(x + c)} {f(Y)} {f(x + d)} {f(Y - h)}"
        f"C{f(x + e)} {f(Y - g)} {f(x + g)} {f(Y - e)} {f(x + h)} {f(Y - d)}"
        f"C{f(x)} {f(Y - c)} {f(x)} {f(Y - b)} {f(x)} {f(Y - a)}"
        f"L{f(x)} {f(y + a)}"
        f"C{f(x)} {f(y + b)} {f(x)} {f(y + c)} {f(x + h)} {f(y + d)}"
        f"C{f(x + g)} {f(y + e)} {f(x + e)} {f(y + g)} {f(x + d)} {f(y + h)}"
        f"C{f(x + c)} {f(y)} {f(x + b)} {f(y)} {f(x + a)} {f(y)}Z"
    )


def legacy(colors, brick, studs_svg, inset=100.0):
    """Pre-Tahoe macOS grid: rounded shape inset on a 1024 canvas, standard drop shadow.
    inset=100 gives the 824 px shape used from 256 px up; small sizes use a larger shape."""
    size = CANVAS - 2 * inset
    m = size / 824.0
    sq = squircle(inset, inset, size, 185.4 * m)
    k = size / CANVAS
    defs = (
        '<defs>'
        f'<linearGradient id="bg" x1="0" y1="0" x2="0" y2="1">'
        f'<stop offset="0" stop-color="{colors[0]}"/><stop offset="1" stop-color="{colors[1]}"/></linearGradient>'
        f'<clipPath id="shape"><path d="{sq}"/></clipPath>'
        '<filter id="shadow" x="-20%" y="-20%" width="140%" height="140%" color-interpolation-filters="sRGB">'
        '<feGaussianBlur in="SourceAlpha" stdDeviation="{f(14 * m)}"/><feOffset dy="{f(12 * m)}"/>'
        '<feComponentTransfer><feFuncA type="linear" slope="0.5"/></feComponentTransfer>'
        '</filter>'
        '</defs>\n'
    )
    body = (
        f'<path d="{sq}" filter="url(#shadow)"/>\n'
        f'<g clip-path="url(#shape)"><g transform="translate({f(inset)} {f(inset)}) scale({k})">\n'
        f'<rect width="{CANVAS}" height="{CANVAS}" fill="url(#bg)"/>\n{brick}\n{studs_svg}\n</g></g>'
    )
    return svg(body, defs)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("outdir")
    ap.add_argument("--elev", type=float, default=30.0)
    ap.add_argument("--stud-r", type=float, default=0.33)
    ap.add_argument("--stud-h", type=float, default=0.28)
    ap.add_argument("--width", type=float, default=640.0)
    ap.add_argument("--rho", type=float, default=22.0)
    ap.add_argument("--shift", type=float, default=0.0)
    ap.add_argument("--tag", default="")
    a = ap.parse_args()
    os.makedirs(a.outdir, exist_ok=True)
    brick, studs_svg, bbox = build(a.elev, a.stud_r, a.stud_h, a.width, a.rho, a.shift)
    print("bbox", [round(v, 1) for v in bbox])
    t = a.tag

    def w(name, text):
        with open(os.path.join(a.outdir, name), "w") as fh:
            fh.write(text)

    w(f"1-brick{t}.svg", svg(brick))
    w(f"2-studs{t}.svg", svg(studs_svg))
    for name, colors in (("default", BG_LIGHT), ("dark", BG_DARK)):
        d, rect = bg(colors)
        w(f"fullbleed-{name}{t}.svg", svg(f"{rect}\n{brick}\n{studs_svg}", d))
        w(f"macos-{name}{t}.svg", legacy(colors, brick, studs_svg))


if __name__ == "__main__":
    main()
