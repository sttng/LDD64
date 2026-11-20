/* Vortex 5.0.0 build 800 "vortexldd41" - contact simplification (restored).
 * Mac sub-TU 559a 0x54de3f-0x5502d0 (14 functions), Win 0x755390-0x7572a0. See
 * Vinc/VincContactSimplify.h for the overview. Names (name inferred) except VincContactSimplify /
 * VincContactSimplifyResult. A group's hull is a ring of pool nodes (maxCount + 1); a node's value
 * is twice the signed area (prev, node, next) along the normal (VincTriple), and the hull keeps
 * the nodes whose removal would lose the most area. */
#include "Vinc/VincContactSimplify.h"
#include "Vinc/VincModel.h"
#include "Me/MeMessage.h"

#include <float.h>
#include <math.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

/* ---- tolerances (Mac 0x779d50 / 0x779d58 / 0x779d60, Win 0xabc860 / 0xabc850 / 0xabc858) ---- */
static MeReal VincSimplifyDistanceSq = (MeReal)1e-4f;  /* Mac 0x779d58, Win 0xabc850: squared merge distance */
static MeReal VincSimplifyCosAngle = (MeReal)0.99f;    /* Mac 0x779d60, Win 0xabc858: merge normal cosine */
static MeReal VincSimplifyAreaTol = (MeReal)1e-4f;     /* Mac 0x779d50, Win 0xabc860: hull area tolerance */
/* Mac 0x779d48, Win 0xabc84c: 1 = keep the deepest contact of all, 0 = never (see the core; no
 * setter survived) (name inferred) */
int VincContactSimplifyKeepDeepest = 1;

/* ---- the hull ring (Mac node 0x14 bytes, Win 0x18) ---- */
typedef struct VincHullNode {
    VincContact* contact;            /* +0x00 */
    MeReal value;                    /* +0x04 / +0x08: VincTriple(prev, this, next) */
    struct VincHullNode* next;       /* +0x0c / +0x10 */
    struct VincHullNode* prev;       /* +0x10 / +0x14 */
} VincHullNode;

typedef struct VincHull {
    VincHullNode* nodes;             /* +0x00 the pool */
    VincHullNode* head;              /* +0x04 a node of the ring */
    VincHullNode* freeList;          /* +0x08 */
    int count;                       /* +0x0c nodes in the ring */
} VincHull;

/* (b - o) . (n x (a - o)) in the original's association order (Win 0x757240, out of line there;
 * Mac: inlined): twice the signed area of (o, a, b) seen along n */
static MeReal VincTriple(const MeReal* o, const MeReal* a, const MeReal* b, const MeReal* n)
{
    MeReal ax = a[0] - o[0], ay = a[1] - o[1], az = a[2] - o[2];
    return (b[2] - o[2]) * (n[0] * ay - n[1] * ax) + (b[1] - o[1]) * (ax * n[2] - n[0] * az) +
           (b[0] - o[0]) * (n[1] * az - ay * n[2]);
}

/* the node's value from its neighbours */
static MeReal VincHullNodeArea(const VincHullNode* node, const MeReal* n)
{
    return VincTriple(node->prev->contact->position, node->contact->position, node->next->contact->position, n);
}

/* Mac 0x54df02, Win 0x755460: the pool of `count` nodes, all free */
static void VincHullInit(VincHull* hull, int count)
{
    int i;
    for (i = 0; i < count - 1; ++i)
        hull->nodes[i].next = &hull->nodes[i + 1];
    hull->nodes[count - 1].next = 0;
    hull->head = 0;
    hull->freeList = hull->nodes;
    hull->count = 0;
}

/* Mac 0x54df58, Win 0x7554b0: a node for contact c (not linked; NULL when the pool is empty) */
static VincHullNode* VincHullAlloc(VincHull* hull, VincContact* c)
{
    VincHullNode* node = hull->freeList;
    if (node != 0) {
        hull->freeList = node->next;
        node->contact = c;
        node->value = 0.0;
        ++hull->count;
    }
    return node;
}

/* Mac 0x54df87, Win 0x7554e0: link node after `after` */
static void VincHullInsertAfter(VincHullNode* after, VincHullNode* node)
{
    node->next = after->next;
    node->prev = after;
    after->next->prev = node;
    after->next = node;
}

/* Mac 0x54dfa4, Win 0x755500: unlink node (NOTE: the head always moves to node->next) */
static void VincHullRemove(VincHull* hull, VincHullNode* node)
{
    hull->head = node->next;
    node->next->prev = node->prev;
    node->prev->next = node->next;
    node->next = hull->freeList;
    hull->freeList = node;
    --hull->count;
}

/* Mac 0x54e6bd, Win 0x755540: remove the node of the smallest area among the contacts that do not
 * penetrate by more than 1e-7 (else: of the smallest area times penetration) */
static void VincHullRemoveSmallest(VincHull* hull, const MeReal* n)
{
    VincHullNode* head = hull->head;
    VincHullNode* best = 0;
    VincHullNode* node = head;
    VincHullNode *p, *q;
    MeReal smallest = DBL_MAX;
    do {
        if (-1e-7 < node->contact->separation && node->value < smallest) {
            best = node;
            smallest = node->value;
        }
        node = node->next;
    } while (head != node);
    if (best == 0) {
        best = head;
        for (node = head->next; head != node; node = node->next)
            if (-(node->contact->separation * node->value) < -(best->contact->separation * best->value))
                best = node;
    }
    p = best->prev;
    q = best->next;
    p->value = VincTriple(p->prev->contact->position, p->contact->position, q->contact->position, n);
    q->value = VincTriple(p->contact->position, q->contact->position, q->next->contact->position, n);
    VincHullRemove(hull, best);
}

/* Mac 0x54e1e4, Win 0x7556a0: add contact c to the hull: the nodes between the two edges where the
 * side of c changes are replaced by c (nothing happens when c is inside) */
static void VincHullAddPoint(VincHull* hull, VincContact* c, const MeReal* n)
{
    VincHullNode* head = hull->head;
    VincHullNode* cur = head->next;
    VincHullNode* nx = cur->next;
    VincHullNode* first = 0;
    VincHullNode* last = 0;
    VincHullNode *x, *node;
    int prevSide = VincTriple(head->contact->position, c->position, cur->contact->position, n) < 0.0;
    int side, more;
    do {
        side = VincTriple(cur->contact->position, c->position, nx->contact->position, n) < 0.0;
        if (prevSide != side) {
            if (side)
                last = cur;
            else
                first = cur;
        }
        more = cur != head;
        prevSide = side;
        cur = nx;
        nx = nx->next;
    } while (more);
    if (first == 0)
        return;
    x = first->next;
    while (last != x) {
        VincHullRemove(hull, x);
        x = first->next;
    }
    node = VincHullAlloc(hull, c);
    VincHullInsertAfter(first, node);
    node->value = VincTriple(first->contact->position, c->position, x->contact->position, n);
    first->value = VincTriple(first->prev->contact->position, first->contact->position, c->position, n);
    x->value = VincTriple(c->position, x->contact->position, x->next->contact->position, n);
}

/* Mac 0x54de70, Win 0x7553f0: two contacts to be merged */
static int VincContactsClose(const VincContact* a, const VincContact* b)
{
    if ((a->position[0] - b->position[0]) * (a->position[0] - b->position[0]) +
            (a->position[1] - b->position[1]) * (a->position[1] - b->position[1]) +
            (a->position[2] - b->position[2]) * (a->position[2] - b->position[2]) <
        VincSimplifyDistanceSq)
        return VincSimplifyCosAngle <
               a->normal[0] * b->normal[0] + a->normal[1] * b->normal[1] + a->normal[2] * b->normal[2];
    return 0;
}

/* a face contact (dims 2 on the first model, or any dimension on the second) */
static int VincContactIsFace(const VincContact* c)
{
    return (char)c->dims == 2 || (c->dims >> 8) != 0;
}

/* sum of squares from 0 (the original's loop order) */
static MeReal VincSumSq3(MeReal x, MeReal y, MeReal z)
{
    MeReal s = 0.0;
    s = x * x + s;
    s = y * y + s;
    s = z * z + s;
    return s;
}

/* Win 0x755940 (out of line there; Mac: inlined in the core): the middle point mid lies within
 * tol (relative to its distance from the nearer end) of the segment a - b */
static int VincHullIsCollinear(const MeReal* a, const MeReal* b, const MeReal* mid, MeReal tolSq)
{
    MeReal d0 = b[0] - a[0], d1 = b[1] - a[1], d2 = b[2] - a[2];
    MeReal e0 = mid[0] - a[0], e1 = mid[1] - a[1], e2 = mid[2] - a[2];
    MeReal len = d0 * d0 + d1 * d1 + d2 * d2;
    MeReal u0, u1, u2, t, m;
    if (len > 0.0) {
        MeReal inv;
        len = sqrt(len);
        inv = 1.0 / len;
        u0 = d0 * inv;
        u1 = d1 * inv;
        u2 = d2 * inv;
    } else {
        u0 = 1.0;
        u1 = 0.0;
        u2 = 0.0;
    }
    t = u0 * e0 + u1 * e1 + u2 * e2;
    if (!(1e-14 < t && t <= len))
        return 0;
    e0 = e0 - t * u0;
    e1 = e1 - t * u1;
    e2 = e2 - t * u2;
    m = len - t;
    if (t < m)
        m = t;
    return VincSumSq3(e0, e1, e2) / (m * m) < tolSq;
}

/* Mac 0x54de3f, Win 0x755390 */
void MEAPI VincContactSimplifySetTolerances(MeReal distance, MeReal cosAngle)
{
    VincSimplifyDistanceSq = distance * distance;
    VincSimplifyCosAngle = cosAngle;
    VincSimplifyAreaTol = distance * distance;
}

/* Mac 0x54dfd2, Win 0x7553c0 */
void MEAPI VincContactSimplifyGetTolerances(MeReal* distance, MeReal* cosAngle)
{
    *distance = sqrt(VincSimplifyDistanceSq);
    *cosAngle = VincSimplifyCosAngle;
}

/* Mac 0x54e8e3, Win 0x755a70 */
int MEAPI VincContactSimplifyCore(MeReal* normal, VincContact* contacts, int count, VincContact* out, int maxCount,
                                  VincRequest* request, MeReal* area)
{
    VincHull hull;
    VincHullNode *first, *node;
    int i, j, k, n, deepest;
    MeReal best;
    /* NOTE (Win): the "mcd" license gate runs here; unlicensed, the hull count is forced to 0
     * after the hull is built (only the deepest contact survives) */
    if (count == 0 || maxCount == 0)
        return 0;
    if (count == 1) {
        memcpy(out, contacts, sizeof *out);
        return 1;
    }
    /* the original allocates the pool on the stack (alloca / MSVC _alloca_probe_16) */
    hull.nodes = (VincHullNode*)malloc((size_t)(maxCount + 1) * sizeof(VincHullNode));
    VincHullInit(&hull, maxCount + 1);

    /* merge duplicates (the deeper one is kept) */
    for (i = 0; i + 1 < count; ++i) {
        j = i + 1;
        do {
            if (!VincContactsClose(&contacts[i], &contacts[j])) {
                ++j;
            } else {
                --count;
                if (contacts[j].separation < contacts[i].separation)
                    memcpy(&contacts[i], &contacts[j], sizeof *contacts);
                memcpy(&contacts[j], &contacts[count], sizeof *contacts);
            }
        } while (j < count);
    }
    if (request->contactMaxCount > 8) {
        if (area != 0)
            *area = 1.0;
        if (request->contactMaxCount < count)
            count = request->contactMaxCount;
        memcpy(out, contacts, (size_t)count * sizeof *contacts);
        free(hull.nodes);
        return count;
    }

    /* the deepest contact (face contacts first when the request asks for them) */
    deepest = -1;
    if (VincContactSimplifyKeepDeepest == 0) {
        if (request->faceNormalsFirst == 0)
            goto hull;
        /* NOTE: with the flag cleared the search starts from -DBL_MAX: no contact is ever deeper */
        best = -DBL_MAX;
    } else {
        best = DBL_MAX;
        if (request->faceNormalsFirst == 0)
            goto all;
    }
    for (i = 0; i < count; ++i)
        if (VincContactIsFace(&contacts[i]) && contacts[i].separation < best) {
            deepest = i;
            best = contacts[i].separation;
        }
    if (deepest != -1)
        goto found;
    if (VincContactSimplifyKeepDeepest == 0)
        goto hull;
all:
    for (i = 0; i < count; ++i)
        if (contacts[i].separation < best) {
            deepest = i;
            best = contacts[i].separation;
        }
found:
    if (deepest >= 0 && (count == 1 || maxCount == 1)) {
        memcpy(out, &contacts[deepest], sizeof *out);
        free(hull.nodes);
        return 1;
    }

hull:
    first = VincHullAlloc(&hull, &contacts[0]);
    hull.head = first;
    first->next = first;
    first->prev = first;
    if (maxCount == 1) {
        if (request->faceNormalsFirst != 0)
            for (i = 1; i < count; ++i)
                if (VincContactIsFace(&contacts[i])) {
                    first->contact = &contacts[i];
                    break;
                }
    } else if (count != 1) {
        VincHullNode* second = VincHullAlloc(&hull, &contacts[1]);
        VincHullInsertAfter(first, second);
        if (maxCount != 2 && count > 2) {
            for (i = 2; i < count; ++i) {
                MeReal v = VincTriple(first->contact->position, second->contact->position, contacts[i].position,
                                      normal);
                if (VincSimplifyAreaTol < v) {
                    node = VincHullAlloc(&hull, &contacts[i]);
                    VincHullInsertAfter(second, node);
                } else if (v < -VincSimplifyAreaTol) {
                    node = VincHullAlloc(&hull, &contacts[i]);
                    VincHullInsertAfter(first, node);
                    v = -v;
                } else {
                    continue;
                }
                node->value = v;
                second->value = v;
                first->value = v;
                for (j = i + 1; j < count; ++j) {
                    VincHullAddPoint(&hull, &contacts[j], normal);
                    if (maxCount < hull.count)
                        VincHullRemoveSmallest(&hull, normal);
                }
                break;
            }
        }
    }

    /* drop (nearly) collinear middle points */
    if (0.0 < request->m08) {
        int changed = 1;
        while (hull.count >= 3 && changed) {
            MeReal tolSq = request->m08 * request->m08;
            VincHullNode* cur = hull.head;
            VincHullNode* nx;
            changed = 0;
            do {
                nx = cur->next;
                if (VincHullIsCollinear(cur->contact->position, nx->next->contact->position, nx->contact->position,
                                        tolSq)) {
                    VincHullRemove(&hull, nx);
                    if (hull.count > 2) {
                        cur->value = VincHullNodeArea(cur, normal);
                        cur->next->value = VincHullNodeArea(cur->next, normal);
                    }
                    changed = 1;
                    break;
                }
                cur = nx;
            } while (hull.head != nx);
        }
    }

    /* the hull's area */
    if (area != 0) {
        n = hull.count;
        *area = 0.0;
        node = hull.head;
        for (i = 0; i < n - 1; i += 2) {
            *area = *area + node->value;
            node = node->next->next;
        }
        if (n > 4) {
            VincHullNode* a = hull.head->next;
            if (n - 1 > 3) {
                VincHullNode* p = a->next->next;
                VincHullNode* q;
                i = 3;
                do {
                    q = n - i < 3 ? p->next : p->next->next;
                    *area = *area + VincTriple(a->contact->position, p->contact->position, q->contact->position, normal);
                    i += 2;
                    p = q;
                } while (i < n - 1);
            }
        }
        *area = *area * 0.5;
    }

    /* the deepest contact first, unless it is on the hull */
    k = 0;
    if (hull.count >= 1 && deepest >= 0) {
        node = hull.head;
        for (i = 0; &contacts[deepest] != node->contact; node = node->next)
            if (hull.count <= ++i)
                goto addDeepest;
    } else {
addDeepest:
        if (deepest >= 0) {
            memcpy(out, &contacts[deepest], sizeof *out);
            k = 1;
            if (hull.count == maxCount)
                VincHullRemoveSmallest(&hull, normal);
        }
    }

    /* face contacts next (only when no area is asked for) */
    n = hull.count;
    if (request->faceNormalsFirst != 0 && area == 0) {
        if (n < 1) {
            free(hull.nodes);
            return k;
        }
        node = hull.head;
        for (i = 0; i != n; ++i) {
            VincHullNode* nx = node;
            if (node != 0 && node->contact != 0) {
                if (VincContactIsFace(node->contact)) {
                    memcpy(&out[k], node->contact, sizeof *out);
                    ++k;
                    nx = node->next;
                    VincHullRemove(&hull, node);
                } else {
                    nx = node->next;
                }
            }
            node = nx;
        }
    }
    n = hull.count;
    if (n > 0) {
        node = hull.head;
        for (i = 0; i != n; ++i) {
            memcpy(&out[k + i], node->contact, sizeof *out);
            node = node->next;
        }
        k += n;
    }
    free(hull.nodes);
    return k;
}

/* group normal from the group's contacts (request->m28 modes 1 / 3) */
static void VincGroupNormalize(MeReal* g)
{
    MeReal x = g[0], y = g[1], z = g[2];
    MeReal len = x * x + y * y + z * z;
    if (len > 0.0) {
        MeReal inv = 1.0 / sqrt(len);
        g[0] = x * inv;
        g[1] = y * inv;
        g[2] = inv * z;
    } else {
        g[0] = 1.0;
        g[1] = 0.0;
        g[2] = 0.0;
    }
}

static void VincGroupMakeNormal(VincContactGroup* g, const VincContact* contacts, int mode)
{
    int i;
    if (mode == 2) {
        MeWarning(0, "Not implemented\n");
    } else if (mode == 3) {
        for (i = 0; i < g->count; ++i)
            if (contacts[i].separation < 0.0) {
                MeReal d = -contacts[i].separation;
                g->normal[0] = d * contacts[i].normal[0] + g->normal[0];
                g->normal[1] = d * contacts[i].normal[1] + g->normal[1];
                g->normal[2] = g->normal[2] + d * contacts[i].normal[2];
            }
        VincGroupNormalize(g->normal);
    } else if (mode == 1) {
        for (i = 0; i < g->count; ++i) {
            g->normal[0] = g->normal[0] + contacts[i].normal[0];
            g->normal[1] = g->normal[1] + contacts[i].normal[1];
            g->normal[2] = g->normal[2] + contacts[i].normal[2];
        }
        VincGroupNormalize(g->normal);
    }
}

#define VINC_MAX(a, b) ((a) > (b) ? (a) : (b))

/* Mac 0x54dffb (regparm: EDX = group, EAX = contacts, ECX = area, XMM0 = padding), Win 0x756b10:
 * the group's size measure (1 without an area request; one contact: the disc of its depth; two:
 * the rectangle of their distance and depth; more: the hull area; at least the padding) */
static void VincGroupMeasure(VincContactGroup* g, const VincContact* c, const MeReal* area, MeReal pad)
{
    if (area == 0) {
        g->m04 = 1.0;
    } else if (g->count == 1) {
        MeReal r;
        g->normal[0] = c[0].normal[0];
        g->normal[1] = c[0].normal[1];
        g->normal[2] = c[0].normal[2];
        r = VINC_MAX(pad, fabs(c[0].separation));
        g->m04 = r * r * 3.141592653589793;
    } else if (g->count == 2) {
        MeReal r, d, m;
        g->normal[0] = c[0].normal[0] + c[1].normal[0];
        g->normal[1] = c[0].normal[1] + c[1].normal[1];
        g->normal[2] = c[0].normal[2] + c[1].normal[2];
        VincGroupNormalize(g->normal);
        r = VINC_MAX(pad, fabs(c[0].separation));
        r = VINC_MAX(r, fabs(c[1].separation));
        d = sqrt(VincSumSq3(c[0].position[0] - c[1].position[0], c[0].position[1] - c[1].position[1],
                            c[0].position[2] - c[1].position[2]));
        m = VINC_MAX(pad, d);
        g->m04 = m * (r + r);
    } else if (g->count == 0) {
        g->m04 = 1.0;
    } else {
        g->m04 = VINC_MAX(*area, pad);
    }
}

/* Mac 0x54f6a8, Win 0x756dc0 */
int MEAPI VincContactSimplify(MeReal* normal, VincContact* contacts, int count, VincIntersectResult* result)
{
    VincRequest* request;
    VincContact* out;
    MeReal areaValue, pad;
    MeReal* area;
    int maxCount, groupMax, k, j, n, g;
    MeReal cosTol;
    int* groupOf;
    VincContact* sorted;
    int start[VINC_MAX_CONTACT_GROUPS + 1];

    if (result->pair == 0)
        MeFatalError(0, "VincContactSimplify: result requiquires a valid pair\n");
    request = result->pair->request;
    out = result->contacts;
    areaValue = 1.0;
    area = result == 0 || request->contactGroupMaxCount < 1 || request->m24 == 0 ? 0 : &areaValue;
    if (count == 0)
        return 0;
    maxCount = request->contactMaxCount;
    if (result->contactMaxCount <= maxCount)
        maxCount = result->contactMaxCount;
    if (result == 0)
        return VincContactSimplifyCore(normal, contacts, count, out, maxCount, request, area);

    if (request->contactGroupMaxCount < 2) {
        n = VincContactSimplifyCore(normal, contacts, count, out, maxCount, request, area);
        result->contactCount = n;
        if (result->pair->request->contactGroupMaxCount < 1 || n < 1) {
            result->contactGroupCount = 0;
        } else {
            result->contactGroupCount = 1;
            result->groups[0].m04 = areaValue;
            result->groups[0].count = n;
            result->groups[0].normal[0] = normal[0];
            result->groups[0].normal[1] = normal[1];
            result->groups[0].normal[2] = normal[2];
        }
        return n;
    }

    /* group the contacts by normal */
    groupMax = request->contactGroupMaxCount;
    if (groupMax > VINC_MAX_CONTACT_GROUPS)
        groupMax = VINC_MAX_CONTACT_GROUPS;
    cosTol = request->m1c;
    groupOf = (int*)malloc((size_t)count * sizeof(int)); /* alloca in the original */
    result->contactGroupCount = 0;
    for (k = 0; k < count; ++k) {
        const VincContact* c = &contacts[k];
        g = result->contactGroupCount;
        if (g >= 1) {
            for (j = 0; j < groupMax; ++j) {
                if (j != 0 && j == g)
                    break;
                if (cosTol < c->normal[0] * result->groups[j].normal[0] + c->normal[1] * result->groups[j].normal[1] +
                                 c->normal[2] * result->groups[j].normal[2]) {
                    ++result->groups[j].count;
                    groupOf[k] = j;
                    goto next;
                }
            }
            if (j == groupMax) {
                MeWarning(0, "VincContactSimplify: max group count (%d) reached with pair %s %s.\n", groupMax,
                          result->pair->model1->ma8, result->pair->model2->ma8);
                ++result->groups[groupMax - 1].count;
                groupOf[k] = groupMax - 1;
                goto next;
            }
        }
        /* a new group */
        groupOf[k] = g;
        g = result->contactGroupCount;
        result->groups[g].normal[0] = c->normal[0];
        result->groups[g].normal[1] = c->normal[1];
        result->groups[g].normal[2] = c->normal[2];
        result->groups[result->contactGroupCount].count = 1;
        ++result->contactGroupCount;
    next:;
    }
    pad = VincModelGetPadding(result->pair->model1);
    pad = pad + VincModelGetPadding(result->pair->model2);

    if (result->contactGroupCount == 1) {
        VincGroupMakeNormal(&result->groups[0], contacts, request->m28);
        result->groups[0].count = VincContactSimplifyCore(normal, contacts, count, out, maxCount, request, area);
        VincGroupMeasure(&result->groups[0], out, area, pad);
        free(groupOf);
        return result->groups[0].count;
    }

    /* sort the contacts by group, then simplify each group on its own */
    sorted = (VincContact*)malloc((size_t)count * sizeof(VincContact)); /* alloca in the original */
    start[0] = 0;
    for (g = 1; g < result->contactGroupCount; ++g)
        start[g] = start[g - 1] + result->groups[g - 1].count;
    for (k = 0; k < count; ++k) {
        g = groupOf[k];
        memcpy(&sorted[start[g]], &contacts[k], sizeof *sorted);
        ++start[g];
    }
    n = 0;
    k = 0;
    for (g = 0; g < result->contactGroupCount; ++g) {
        VincContactGroup* grp = &result->groups[g];
        int cnt;
        VincGroupMakeNormal(grp, &sorted[k], request->m28);
        cnt = grp->count;
        /* NOTE: every group may write maxCount contacts (not maxCount - n): more groups can write past
         * result->contactMaxCount, as in the original */
        grp->count = VincContactSimplifyCore(grp->normal, &sorted[k], cnt, &out[n], maxCount, request, area);
        VincGroupMeasure(grp, &out[n], area, pad);
        n += grp->count;
        k += cnt;
    }
    free(sorted);
    free(groupOf);
    return n;
}

/* Mac 0x550188, Win 0x757140 */
void MEAPI VincContactSimplifyNamed(MeReal* normal, VincContact* contacts, int count, VincIntersectResult* result,
                                    const char* pairName)
{
    (void)pairName;
    result->contactCount = VincContactSimplify(normal, contacts, count, result);
    result->normal[0] = normal[0];
    result->normal[1] = normal[1];
    result->normal[2] = normal[2];
}

/* Mac 0x5501e5, Win 0x757180 (Win: VincContactSimplifyNamed inlined) */
void MEAPI VincContactSimplifyResult(VincIntersectResult* result)
{
    int i;
    if (result->pair->request->contactMaxCount < result->contactCount) {
        MeVector3 normal;
        VincContact* tmp;
        normal[0] = result->normal[0];
        normal[1] = result->normal[1];
        normal[2] = result->normal[2];
        tmp = (VincContact*)malloc((size_t)result->contactCount * sizeof(VincContact)); /* alloca in the original */
        for (i = 0; i < result->contactCount; ++i)
            memcpy(&tmp[i], &result->contacts[i], sizeof *tmp);
        VincContactSimplifyNamed(normal, tmp, result->contactCount, result, "VincContactSimplifyResult");
        free(tmp);
    }
}
