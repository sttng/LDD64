/* Vortex 5.0.0 build 800 "vortexldd41" - Polak-Ribiere conjugate-gradient minimisation (the
 * Numerical Recipes frprmn / linmin / mnbrak / dbrent / f1dim / df1dim scheme for at most 4
 * variables) used by the cylinder pair tests (restored; Mac 558e 0x549fd8-
 * 0x54af4f, Win 0x81d380-0x81e0e0, the vector helpers out of line on Win only). All names inferred.
 * NOTE: differences from the book, kept: dbrent starts with b = min(ax, cx) like a (the bracket
 * is empty; the first test compares |x - a| with tol2), uses tol * (|x| + 1e-10), strict
 * comparisons (fu < fx, fu < fw) and bisects towards a only when dx > 0; linmin does not scale xi.
 * On 100 iterations without convergence dbrent returns f(*xmin), xmin being the bracket end cx
 * (Win passes &cx; Mac an uninitialised local - the Win behaviour is used here). */
#include "Vinc/VincPairSupport.h"

#include <math.h>

#define VINC_MIN_N 4

static int s_n;                          /* Mac 0x780408, Win 0xafcef0 */
static VincMinimizeFn s_f;               /* Mac 0x780400, Win 0xafcef4 */
static VincMinimizeGradientFn s_df;      /* Mac 0x780404, Win 0xafcef8 */
static MeReal s_p[VINC_MIN_N];           /* Mac 0x780420, Win 0xafceb0 */
static MeReal s_xi[VINC_MIN_N];          /* Mac 0x780440, Win 0xafced0 */

/* Win 0x81d380: dst = src */
static void VecCopy(int n, MeReal* dst, const MeReal* src)
{
    int i;
    for (i = 0; i < n; ++i)
        dst[i] = src[i];
}

/* Win 0x81d600: dst = s * a */
static void VecScale(int n, MeReal* dst, const MeReal* a, MeReal s)
{
    int i;
    for (i = 0; i < n; ++i)
        dst[i] = s * a[i];
}

/* Win 0x81d4f0: dst += a */
static void VecAdd(int n, MeReal* dst, const MeReal* a)
{
    int i;
    for (i = 0; i < n; ++i)
        dst[i] = dst[i] + a[i];
}

/* Win 0x81d720: a . b */
static MeReal VecDot(int n, const MeReal* a, const MeReal* b)
{
    MeReal s = 0;
    int i;
    for (i = 0; i < n; ++i)
        s = s + a[i] * b[i];
    return s;
}

/* Mac 0x549fd8, Win 0x81d810 */
static MeReal F1Dim(MeReal x)
{
    MeReal xt[VINC_MIN_N];
    VecScale(s_n, xt, s_xi, x);
    VecAdd(s_n, xt, s_p);
    return s_f(xt);
}

/* Mac 0x54af4f, Win 0x81d860 */
static MeReal DF1Dim(MeReal x)
{
    MeReal xt[VINC_MIN_N], df[VINC_MIN_N];
    VecScale(s_n, xt, s_xi, x);
    VecAdd(s_n, xt, s_p);
    s_df(df, xt);
    return VecDot(s_n, df, s_xi);
}

typedef MeReal (*VincScalarFn)(MeReal x);

/* Mac 0x54a71e, Win 0x81d8c0: bracket a minimum of f starting from (ax, bx) */
static void Bracket(MeReal* ax, MeReal* bx, MeReal* cx, MeReal* fa, MeReal* fb, MeReal* fc, VincScalarFn f)
{
    const MeReal gold = 1.618033988749895;
    MeReal u, fu, ulim, r, q, m, t;
    *fa = f(*ax);
    *fb = f(*bx);
    if (*fa < *fb) {
        t = *ax; *ax = *bx; *bx = t;
        t = *fb; *fb = *fa; *fa = t;
    }
    *cx = *bx + (*bx - *ax) * gold;
    *fc = f(*cx);
    while (*fc < *fb) {
        r = (*fb - *fc) * (*bx - *ax);
        q = (*fb - *fa) * (*bx - *cx);
        m = fabs(q - r);
        if (m <= 1e-20)
            m = 1e-20;
        m = q - r < 0 ? -fabs(m) : fabs(m);
        u = ((*bx - *cx) * q - (*bx - *ax) * r) / (m * -2.0) + *bx;
        ulim = (*cx - *bx) * 100.0 + *bx;
        if ((*bx - u) * (u - *cx) > 0) {
            fu = f(u);
            if (fu < *fc) {
                *ax = *bx;
                *bx = u;
                *fa = *fb;
                *fb = fu;
                return;
            }
            if (*fb < fu) {
                *cx = u;
                *fc = fu;
                return;
            }
            u = *cx + (*cx - *bx) * gold;
            fu = f(u);
        } else if ((*cx - u) * (u - ulim) > 0) {
            fu = f(u);
            if (fu < *fc) {
                *bx = *cx;
                *cx = u;
                u = (u - *bx) * gold + u;
                *fb = *fc;
                *fc = fu;
                fu = f(u);
            }
        } else if ((ulim - *cx) * (u - ulim) < 0) {
            u = (*cx - *bx) * gold + *cx;
            fu = f(u);
        } else {
            u = ulim;
            fu = f(u);
        }
        *ax = *bx;
        *bx = *cx;
        *cx = u;
        *fa = *fb;
        *fb = *fc;
        *fc = fu;
    }
}

/* Mac 0x54a057, Win 0x81daf0: minimum of f in the bracket with derivative df (see the NOTE above) */
static MeReal LineMinimize(MeReal* xmin, MeReal ax, MeReal bx, MeReal cx, VincScalarFn f, VincScalarFn df, MeReal tol)
{
    MeReal a = ax <= cx ? ax : cx; /* fmin (Mac libm) */
    MeReal b = a;                  /* NOTE: the original's quirk (should be max) */
    MeReal x = bx, w = bx, v = bx;
    MeReal fx = f(bx), fw, fv, dx, dw, dv;
    MeReal e = 0, d = 0, dPrev = 0, u, fu, du, xm, tol1, tol2;
    int iter;
    dx = df(bx);
    fw = fx;
    fv = fx;
    dw = dx;
    dv = dx;
    for (iter = 99;; --iter) {
        xm = (a + b) * 0.5;
        tol1 = tol * (fabs(x) + 1e-10);
        tol2 = tol1 + tol1;
        if (fabs(x - xm) <= (b - a) * -0.5 + tol2) {
            *xmin = x;
            return fx;
        }
        if (fabs(e) <= tol1) {
            e = (dx > 0 ? a : b) - x;
            d = e * 0.5;
        } else {
            MeReal d1 = (b - a) + (b - a), d2, u1, u2;
            int ok1, ok2;
            d2 = d1;
            if (dw != dx)
                d1 = ((w - x) * dx) / (dx - dw);
            if (dv != dx)
                d2 = ((v - x) * dx) / (dx - dv);
            u1 = x + d1;
            u2 = x + d2;
            ok1 = (a - u1) * (u1 - b) > 0 && !(0 <= dx * d1);
            ok2 = (a - u2) * (u2 - b) > 0 && !(0 <= dx * d2);
            if (!ok1 && !ok2) {
                e = (dx > 0 ? a : b) - x;
                d = e * 0.5;
            } else {
                if (ok1 && ok2)
                    d = fabs(d2) <= fabs(d1) ? d2 : d1;
                else if (ok1)
                    d = d1;
                else
                    d = d2;
                if (fabs(e * 0.5) < fabs(d)) {
                    e = (dx > 0 ? a : b) - x;
                    d = e * 0.5;
                } else {
                    e = dPrev;
                    if ((d + x) - a < tol2 || b - (d + x) < tol2)
                        d = xm - x < 0 ? -fabs(tol1) : fabs(tol1);
                }
            }
        }
        if (fabs(d) < tol1) {
            u = (d < 0 ? -fabs(tol1) : fabs(tol1)) + x;
            fu = f(u);
            if (fx < fu) {
                *xmin = x;
                return fx;
            }
        } else {
            u = d + x;
            fu = f(u);
        }
        du = df(u);
        if (fu < fx) {
            if (u < x)
                b = x;
            else
                a = x;
            v = w; fv = fw; dv = dw;
            w = x; fw = fx; dw = dx;
            x = u; fx = fu; dx = du;
        } else {
            if (u < x)
                a = u;
            else
                b = u;
            if (fu < fw || w == x) {
                v = w; fv = fw; dv = dw;
                w = u; fw = fu; dw = du;
            } else if (fu < fv || v == x || v == w) {
                v = u; fv = fu; dv = du;
            }
        }
        if (iter == 0)
            return f(*xmin);
        dPrev = d;
    }
}

/* Mac 0x54ab1c, Win 0x81dfa0: minimise along xi from p (p moves to the minimum) */
static MeReal LineSearch(int n, MeReal* p, const MeReal* xi, MeReal tol, VincMinimizeFn f, VincMinimizeGradientFn df)
{
    MeReal ax = 0, xx = 1, bx, fa, fx, fb, fret;
    int i;
    s_f = f;
    s_df = df;
    s_n = n;
    VecCopy(n, s_p, p);
    VecCopy(n, s_xi, xi);
    Bracket(&ax, &xx, &bx, &fa, &fx, &fb, F1Dim);
    fret = LineMinimize(&bx, ax, xx, bx, F1Dim, DF1Dim, tol);
    for (i = 0; i < n; ++i)
        p[i] = bx * xi[i] + p[i];
    return fret;
}

/* Win 0x81e070 (dead-stripped on the Mac): minimum of a function of one variable near [0, 1] */
MeReal VincMinimize1D(MeReal* xmin, MeReal tol, MeReal (*f)(MeReal), MeReal (*df)(MeReal))
{
    MeReal ax = 0, bx = 1, cx, fa, fb, fc;
    Bracket(&ax, &bx, &cx, &fa, &fb, &fc, f);
    return LineMinimize(xmin, ax, bx, cx, f, df, tol);
}

/* Mac 0x54ac77, Win 0x81e0e0 */
MeReal VincConjugateGradientMinimize(int n, MeReal* p, MeReal lineTol, MeReal ftol, VincMinimizeFn f,
                                     VincMinimizeGradientFn df)
{
    MeReal g[VINC_MIN_N], h[VINC_MIN_N], xi[VINC_MIN_N], t[VINC_MIN_N];
    MeReal fp, fret, gg, dgg;
    int i, its;
    fp = f(p);
    df(xi, p);
    for (i = 0; i < n; ++i)
        xi[i] = -xi[i];
    VecCopy(n, g, xi);
    VecCopy(n, h, xi);
    for (its = 99;; --its) {
        fret = LineSearch(n, p, xi, lineTol, f, df);
        if (fabs(fret - fp) + fabs(fret - fp) <= (fabs(fret) + fabs(fp) + 1e-10) * ftol || n <= 0)
            break;
        gg = VecDot(n, g, g);
        if (gg == 0)
            break;
        fp = f(p);
        df(xi, p);
        for (i = 0; i < n; ++i)
            t[i] = xi[i] + g[i];
        dgg = VecDot(n, t, xi);
        for (i = 0; i < n; ++i)
            g[i] = -xi[i];
        for (i = 0; i < n; ++i)
            h[i] = (dgg / gg) * h[i] + g[i];
        VecCopy(n, xi, h);
        if (its == 0)
            break;
    }
    return fret;
}
