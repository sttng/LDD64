// Vortex 5.0.0 build 800 "vortexldd41" - Kea vector helpers (restored).
// Mac 0x4a3933-0x4a3d0d (after the TU 555 initialiser, so part of TU 556), Win 0x80e520-0x80ee30.
// The Win build unrolls the loops by four; the operations per element are the same.
#include "Kea/KeaMath.h"

#include <math.h>

// Mac 0x4a3933, Win 0x80e520
void KeaCopy(int n, const MeReal* x, int incx, MeReal* y, int incy)
{
    for (int i = 0; i < n; ++i) {
        *y = *x;
        x += incx;
        y += incy;
    }
}

// Mac 0x4a396d, Win 0x80e640
void KeaScale(int n, MeReal alpha, MeReal* x, int incx)
{
    if (alpha == 0.0) {
        for (int i = 0; i < n; ++i) {
            *x = 0.0;
            x += incx;
        }
    } else {
        for (int i = 0; i < n; ++i) {
            *x = *x * alpha;
            x += incx;
        }
    }
}

// Mac 0x4a39d9, Win 0x80e6f0
void KeaAxpy(int n, MeReal alpha, const MeReal* x, int incx, MeReal* y, int incy)
{
    if (alpha == 1.0) {
        for (int i = 0; i < n; ++i) {
            *y = *y + *x;
            x += incx;
            y += incy;
        }
    } else {
        for (int i = 0; i < n; ++i) {
            *y = alpha * *x + *y;
            x += incx;
            y += incy;
        }
    }
}

// Mac 0x4a3a62, Win 0x80e840
void KeaSubtract(int n, const MeReal* x, MeReal* y)
{
    for (int i = 0; i < n; ++i)
        y[i] = y[i] - x[i];
}

// Mac 0x4a3a92, Win 0x80e8c0
void KeaMultiplyAdd(int n, MeReal alpha, const MeReal* x, int incx, const MeReal* y, int incy,
                    MeReal beta, MeReal* z, int incz)
{
    int i;
    if (beta == 0.0) {
        if (alpha == 1.0) {
            for (i = 0; i < n; ++i, x += incx, y += incy, z += incz)
                *z = *x * *y;
        } else {
            for (i = 0; i < n; ++i, x += incx, y += incy, z += incz)
                *z = alpha * *x * *y;
        }
    } else if (beta == 1.0) {
        if (alpha == 1.0) {
            for (i = 0; i < n; ++i, x += incx, y += incy, z += incz)
                *z = *x * *y + *z;
        } else {
            for (i = 0; i < n; ++i, x += incx, y += incy, z += incz)
                *z = alpha * *x * *y + *z;
        }
    } else if (alpha == 1.0) {
        for (i = 0; i < n; ++i, x += incx, y += incy, z += incz)
            *z = *x * *y + beta * *z;
    } else {
        for (i = 0; i < n; ++i, x += incx, y += incy, z += incz)
            *z = alpha * *x * *y + beta * *z;
    }
}

// Mac 0x4a3cc2, Win 0x80e5c0 (sqrt through the CRT on Win: correctly rounded either way)
MeReal KeaNorm2(int n, const MeReal* x, int incx)
{
    MeReal s = 0.0;
    for (int i = 0; i < n; ++i) {
        s = s + *x * *x;
        x += incx;
    }
    return sqrt(s);
}
