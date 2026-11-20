// Vortex 5.0.0 build 800 "vortexldd41" - CxSmallSort, the sweep-and-prune broad phase behind a
// VincSpace (restored; Mac 558d 0x53bae0-0x540d84 + the small slots at the end of
// 558c 0x53b3de-0x53ba64; Win 0x78dc10-0x790570). Data: include/Cx/CxSmallSort.h. All names are
// inferred; the original's template containers (out of line on Win, inlined by GCC) are static
// helpers here.
#include "Cx/CxSmallSort.h"
#include "Me/MeMemory.h"
#include "Me/MeMessage.h"

#include <new>
#include <stdio.h>
#include <string.h>
#include <algorithm>
#include <limits>
#include <math.h>
#include <vector>

#if defined(__i386__) || defined(_M_IX86)
#define CX_SIZE_IS(n) 1   // members appended after the original's (their offsets are checked)
#if defined(_WIN32)
typedef char CxSmallSortSizeCheck[CX_SIZE_IS(0x188) && offsetof(CxSmallSort, minSentinel) == 0x90 &&
                                   offsetof(CxSmallSort, axisList) == 0x120 && offsetof(CxSmallSort, boundsFn) == 0x174 &&
                                   sizeof(CxEntry) == 0xa0 ? 1 : -1];
#else
typedef char CxSmallSortSizeCheck[CX_SIZE_IS(0x180) && offsetof(CxSmallSort, minSentinel) == 0x8c &&
                                   offsetof(CxSmallSort, axisList) == 0x11c && offsetof(CxSmallSort, boundsFn) == 0x170 &&
                                   sizeof(CxEntry) == 0x9c ? 1 : -1];
#endif
typedef char CxPairNodeSizeCheck[sizeof(CxPairNode) == 0x50 && sizeof(CxBlockList) == 0x328 &&
                                 offsetof(CxSmallSort, pairs) == 0x54 && offsetof(CxSmallSort, states) == 0x7c ? 1 : -1];
#endif

namespace {

const MeReal kMaxReal = 1.7976931348623157e308; // 0x7fefffffffffffff

// ---- memory (MeMemoryAPI; arrays are constructed back to front like the original's loops) ----
template <class T>
T* cxNewArray(unsigned n)
{
    T* p = (T*)MeMemoryAPI.create(n * sizeof(T));
    if (p != 0)
        for (T* q = p + n; q > p;)
            new (--q) T();
    return p;
}

void cxFree(void* p)
{
    if (p != 0)
        MeMemoryAPI.destroy(p);
}

// ---- links ----
inline void linkInit(CxLink* l)
{
    l->next = l;
    l->prev = l;
}

inline void linkUnlink(CxLink* l)
{
    l->next->prev = l->prev;
    l->prev->next = l->next;
}

inline void linkUnlinkSelf(CxLink* l)
{
    linkUnlink(l);
    linkInit(l);
}

// insert l before `at`
inline void linkInsertBefore(CxLink* l, CxLink* at)
{
    l->next = at;
    l->prev = at->prev;
    at->prev->next = l;
    l->next->prev = l;
}

// insert l after `at`
inline void linkInsertAfter(CxLink* l, CxLink* at)
{
    l->prev = at;
    l->next = at->next;
    at->next = l;
    l->next->prev = l;
}

// Win 0x78dcc0: unlink l and append it to `list`
inline void linkMoveToBack(CxLink* l, CxLink* list)
{
    linkUnlink(l);
    linkInsertBefore(l, list);
}

// ---- block list (Win 0x78f6c0 add, 0x78fd80 clear) ----
CxBlockList* blockListCreate(CxBlockList* next)
{
    CxBlockList* b = (CxBlockList*)MeMemoryAPI.create(sizeof(CxBlockList));
    if (b != 0) {
        b->count = 0;
        b->next = next;
    }
    return b;
}

void blockListAdd(CxBlockList** head, void* items, int count)
{
    CxBlockList* b = *head;
    if (b->count > 99) {
        CxBlockList* nb = blockListCreate(b);
        *head = nb;
    }
    b = *head;
    b->chunks[b->count].items = items;
    b->chunks[b->count].count = count;
    b->count++;
}

// pair chunks: the CxPair links are unlinked (Mac 0x53fcdb, Win 0x78f9a0)
void freePairChunk(void* items, int count)
{
    if (items != 0) {
        CxPairNode* nodes = (CxPairNode*)items;
        for (CxPairNode* n = nodes + count; n > nodes;) {
            --n;
            linkUnlinkSelf(&n->value.link);
        }
        MeMemoryAPI.destroy(items);
    }
}

// Mac 0x53fe86 / 0x540246 (unrolled), Win 0x78fd80
void blockListClear(CxBlockList* b, bool pairChunks)
{
    while (b->count != 0) {
        b->count--;
        if (pairChunks)
            freePairChunk(b->chunks[b->count].items, b->chunks[b->count].count);
        else
            cxFree(b->chunks[b->count].items);
    }
    if (b->next != 0) {
        blockListClear(b->next, pairChunks);
        MeMemoryAPI.destroy(b->next);
        b->next = 0;
    }
}

void blockListDestroy(CxBlockList** head, bool pairChunks)
{
    if (*head != 0) {
        blockListClear(*head, pairChunks);
        MeMemoryAPI.destroy(*head);
        *head = 0;
    }
}

// ---- entry table (Win 0x78fde0 ctor, 0x78f620 reserve, 0x78fbb0 allocate, 0x78f720 dtor) ----
CxEntry* newEntry(CxEntry* e)
{
    e->handle = -1;
    e->object = 0;
    e->active = true;
    for (int a = 0; a < 3; ++a)
        for (int k = 0; k < 2; ++k) {
            CxEndpoint* p = &e->ends[a][k];
            linkInit(p);
            p->value = 0;
            p->entry = 0;
            p->kind = 0;
        }
    return e;
}

bool entryTableReserve(CxEntryTable* t, unsigned n)
{
    if (t->capacity < n) {
        CxEntry** d = cxNewArray<CxEntry*>(n);
        if (d == 0)
            return false;
        for (unsigned i = t->size; i != 0; --i)
            d[i - 1] = t->data[i - 1];
        cxFree(t->data);
        t->data = d;
        t->capacity = n;
    }
    return true;
}

// entries for the handles lo .. hi (one chunk)
bool entryTableAllocate(CxEntryTable* t, unsigned lo, unsigned hi)
{
    if (lo <= hi) {
        unsigned n = hi - lo + 1;
        CxEntry* chunk = (CxEntry*)MeMemoryAPI.create(n * sizeof(CxEntry));
        if (chunk != 0)
            for (CxEntry* q = chunk + n; q > chunk;)
                newEntry(--q);
        if (chunk == 0)
            return false;
        blockListAdd(&t->blocks, chunk, (int)n);
        for (; lo <= hi; ++lo)
            t->data[lo] = chunk++;
    }
    return true;
}

void entryTableInit(CxEntryTable* t, unsigned n)
{
    t->size = 0;
    t->capacity = n;
    t->initialSize = n;
    t->data = cxNewArray<CxEntry*>(n);
    t->blocks = blockListCreate(0);
    if (t->capacity >= n || entryTableReserve(t, n))
        t->size = n;
    entryTableAllocate(t, 0, n - 1);
}

// ---- bit array (Win 0x78dcf0 ctor, 0x78dd50 clearUnused, 0x78f2b0 reserve) ----
void bitArrayClearUnused(CxBitArray* b)
{
    unsigned w = b->bitCount >> 5;
    if ((b->bitCount & 31) != 0) {
        b->data[w] &= ~(~0u << (b->bitCount & 31));
        ++w;
    }
    for (; w < b->capacity; ++w)
        b->data[w] = 0;
}

void bitArrayInit(CxBitArray* b, unsigned n)
{
    b->bitCount = n;
    b->wordCount = ((int)(n - 1) >> 5) + 1;
    b->capacity = b->wordCount;
    b->initialWords = b->wordCount;
    b->data = cxNewArray<unsigned>(b->capacity);
    for (unsigned* p = b->data; p < b->data + b->wordCount; ++p)
        *p = 0;
}

bool bitArrayReserve(CxBitArray* b, unsigned words)
{
    unsigned old = b->capacity;
    if (old < words) {
        unsigned* od = b->data;
        b->capacity = words;
        b->data = cxNewArray<unsigned>(words);
        if (b->data == 0)
            return false;
        bitArrayClearUnused(b);
        for (unsigned i = old; i != 0; --i)
            b->data[i - 1] = od[i - 1];
        cxFree(od);
    }
    return true;
}

void bitArrayResize(CxBitArray* b, unsigned n)
{
    unsigned words = ((int)(n - 1) >> 5) + 1;
    if (words <= b->capacity || bitArrayReserve(b, words)) {
        b->bitCount = n;
        b->wordCount = words;
    }
}

inline bool bitTest(const CxBitArray* b, int i)
{
    return (b->data[i >> 5] & (1u << (i & 31))) != 0;
}

inline void bitAssign(CxBitArray* b, int i, bool v)
{
    unsigned* w = &b->data[i >> 5];
    unsigned bit = 1u << (i & 31);
    *w = (*w & ~bit) | ((unsigned)v << (i & 31));
}

// ---- pair states: see "Pair states without the table" at the end of the file ----
typedef CxSmallSort::StateIndex StateIndexT;

// The pair whose state the original's index stateIndex(lo, hi, n) addresses. lo < hi: the pair
// itself; (h, h) indexes (h - 1, n - 1) as the original's triangle does. (0, 0) and handle -1 lie
// outside the original's table: reads give the default, writes are dropped.
inline StateIndexT stateIndex(int lo, int hi, int n)
{
    StateIndexT i;
    i.lo = lo;
    i.hi = hi;
    if (lo < 0 || hi < 0 || (lo == hi && lo == 0)) {
        i.lo = -1;
        i.hi = -1;
    } else if (lo == hi) {
        i.lo = lo - 1;
        i.hi = n - 1;
    }
    return i;
}

inline bool stateIndexValid(const StateIndexT& i)
{
    return i.lo >= 0;
}

// no table: the original's sizes are kept for nothing but the fill state
void statesInit(CxPairStates* s, unsigned, int n, unsigned char state)
{
    s->objectCount = n;
    s->fill = (unsigned char)(state << 4 | state);
    s->byteCount = 0;
    s->data = 0;
}

void statesResize(CxPairStates* s, int n)
{
    s->objectCount = n;
}

const unsigned long long kRecordEmpty = ~0ull;
const unsigned long long kLabelMax = ~0ull;   // the max sentinels' label (min sentinels: 0)

inline unsigned long long mix64(unsigned long long k)
{
    k ^= k >> 33;
    k *= 0xff51afd7ed558ccdull;
    k ^= k >> 33;
    k *= 0xc4ceb9fe1a85ec53ull;
    k ^= k >> 33;
    return k;
}

inline unsigned long long recordKey(int lo, int hi)
{
    return (unsigned long long)(unsigned)lo << 32 | (unsigned)hi;
}


// ---- pair table (Win 0x790180 ctor, 0x78fca0 grow, 0x78ff40 rehash, 0x790100 / 0x790280 alloc,
// 0x78f560 / 0x78f900 find, 0x78f5d0 / 0x78f920 release, 0x78f530 deferred, 0x78fe80 dtor) ----
inline CxLink* bucketOf(CxPairTable* t, unsigned key)
{
    return &t->buckets[key % t->mask];
}

// A pair's key. Above handle 0xffff the original's key lo << 16 | hi collides, so such pairs get a
// mixed key (pairKey) and their lookups compare the handles; below it the key is the original's.
struct CxPairKey {
    unsigned key;
    int lo, hi;
    bool wide;
};

inline unsigned keyValue(const CxPairKey& k)
{
    return k.key;
}

inline bool keyMatches(const CxPairNode* n, const CxPairKey& k)
{
    return n->key == k.key && (!k.wide || (n->value.handle1 == k.lo && n->value.handle2 == k.hi));
}

bool pairTableGrow(CxPairTable* t, int n)
{
    if (n < 1)
        return false;
    CxPairNode* nodes = (CxPairNode*)MeMemoryAPI.create(n * sizeof(CxPairNode));
    if (nodes != 0)
        for (CxPairNode* q = nodes + n; q > nodes;) {
            --q;
            linkInit(&q->hashLink);
            linkInit(&q->value.link);
        }
    if (nodes == 0)
        return false;
    blockListAdd(&t->blocks, nodes, n);
    t->allocated += n;
    CxPairNode** stack = cxNewArray<CxPairNode*>(t->allocated);
    if (stack == 0)
        return false;
    cxFree(t->freeBase);
    t->freeBase = stack;
    t->freeTop = stack;
    for (CxPairNode* q = nodes; q < nodes + n; ++q)
        *t->freeTop++ = q;
    return true;
}

// the warm-up of pairTableInit: the node pool's first block, one node out and straight back
void pairTableWarmUp(CxPairTable* t)
{
    if (t->freeTop != t->freeBase || pairTableGrow(t, t->growBy)) {
        --t->freeTop;
        if (*t->freeTop != 0)
            ++t->freeTop;
    }
}

bool pairTableRehash(CxPairTable* t, int capacity)
{
    MeInfo(0, "Resizing geometry pair table size to %d pairs.\n", capacity);
    pairTableGrow(t, capacity - t->allocated);
    CxLink* old = t->buckets;
    unsigned oldMask = t->mask;
    t->capacity = capacity;
    unsigned char bits = 0;
    for (unsigned v = capacity - 1; v != 0; v >>= 1)
        ++bits;
    t->mask = 1u << bits;
    t->buckets = (CxLink*)MeMemoryAPI.create(t->mask * sizeof(CxLink));
    if (t->buckets != 0)
        for (CxLink* q = t->buckets + t->mask; q > t->buckets;)
            linkInit(--q);
    if (t->buckets == 0)
        return false;
    t->mask -= 1;
    for (unsigned i = 0; i != oldMask + 1; ++i) {
        CxLink* b = &old[i];
        while (b->prev != b) {
            CxPairNode* n = (CxPairNode*)b->prev;
            linkUnlink(&n->hashLink);
            linkInsertAfter(&n->hashLink, bucketOf(t, n->key));
        }
    }
    cxFree(old);
    return true;
}

// a new node for key (front of its bucket), 0 when out of memory
CxPair* pairTableAlloc(CxPairTable* t, CxPairKey key)
{
    if (t->allocated == 0)
        pairTableWarmUp(t);   // pairTableInit's warm-up, deferred to the first pair
    if (t->freeTop == t->freeBase)
        pairTableRehash(t, t->capacity * 2);
    CxLink* b = bucketOf(t, keyValue(key));
    if (t->freeTop != t->freeBase || pairTableGrow(t, t->growBy)) {
        CxPairNode* n = *--t->freeTop;
        if (n != 0) {
            linkInsertAfter(&n->hashLink, b);
            n->key = keyValue(key);
            return &n->value;
        }
    }
    return 0;
}

// lookup; a hit that is not first in its bucket becomes first (the head moves)
CxPair* pairTableFind(CxPairTable* t, CxPairKey key)
{
    CxLink* b = bucketOf(t, keyValue(key));
    CxLink* l = b->next;
    if (l != b && keyMatches((CxPairNode*)l, key))
        return &((CxPairNode*)l)->value;
    for (l = l->next;; l = l->next) {
        if (l == b)
            return 0;
        if (keyMatches((CxPairNode*)l, key))
            break;
    }
    linkUnlink(b);
    linkInsertBefore(b, l);
    return &((CxPairNode*)l)->value;
}

// lookup + back to the free stack (the value's link is left alone)
CxPair* pairTableRelease(CxPairTable* t, CxPairKey key)
{
    CxLink* b = bucketOf(t, keyValue(key));
    for (CxLink* l = b->next; l != b; l = l->next) {
        CxPairNode* n = (CxPairNode*)l;
        if (keyMatches(n, key)) {
            linkUnlinkSelf(&n->hashLink);
            *t->freeTop++ = n;
            return &n->value;
        }
    }
    return 0;
}

// lookup + onto the deferred list
void pairTableDefer(CxPairTable* t, CxPairKey key)
{
    CxLink* b = bucketOf(t, keyValue(key));
    for (CxLink* l = b->next; l != b; l = l->next) {
        CxPairNode* n = (CxPairNode*)l;
        if (keyMatches(n, key)) {
            linkUnlink(&n->hashLink);
            linkInsertAfter(&n->hashLink, &t->deferred);
            return;
        }
    }
}

void pairTableFlushDeferred(CxPairTable* t)
{
    while (t->deferred.next != &t->deferred) {
        CxLink* l = t->deferred.next;
        linkUnlinkSelf(l);
        *t->freeTop++ = (CxPairNode*)l;
    }
}

void pairTableInit(CxPairTable* t, int capacity)
{
    t->growBy = 100;
    t->allocated = 0;
    t->freeBase = 0;
    t->freeTop = 0;
    t->blocks = blockListCreate(0);
    if (capacity > 0)
        t->growBy = capacity;
    linkInit(&t->deferred);
    t->capacity = capacity;
    // LDD64: no node until the first pair (pairTableAlloc)
    unsigned char bits = 0;
    for (unsigned v = capacity - 1; v != 0; v >>= 1)
        ++bits;
    t->mask = 1u << bits;
    t->buckets = (CxLink*)MeMemoryAPI.create(t->mask * sizeof(CxLink));
    for (CxLink* q = t->buckets + t->mask; q > t->buckets;)
        linkInit(--q);
    t->mask -= 1;
}

// Mac 0x540d84, Win 0x78fe80 / 0x78fc30 / 0x78f940
void pairTableDestroy(CxPairTable* t)
{
    for (unsigned i = 0; i < t->mask + 1; ++i)
        while (t->buckets[i].next != &t->buckets[i]) {
            CxLink* l = t->buckets[i].next;
            linkUnlinkSelf(l);
            *t->freeTop++ = (CxPairNode*)l;
        }
    cxFree(t->buckets);
    blockListDestroy(&t->blocks, true);
    t->blocks = blockListCreate(0);
    cxFree(t->freeBase);
    t->freeBase = 0;
    t->freeTop = 0;
    t->allocated = 0;
    blockListDestroy(&t->blocks, true);
}

inline CxPairKey pairKey(int lo, int hi)
{
    CxPairKey k;
    k.lo = lo;
    k.hi = hi;
    k.wide = !((unsigned)lo <= 0xffff && (unsigned)hi <= 0xffff);
    if (!k.wide) {
        k.key = (unsigned)lo << 16 | (unsigned)hi;
    } else {
        unsigned long long m = mix64(recordKey(lo, hi));
        k.key = (unsigned)(m ^ (m >> 32));
    }
    return k;
}

inline void orderHandles(int& a, int& b)
{
    if (b < a) {
        int t = a;
        a = b;
        b = t;
    }
}

void initEndpoint(CxEndpoint* p)
{
    linkInit(p);
    p->value = 0;
    p->entry = 0;
    p->kind = 0;
}

} // namespace

// the original's nibble operations, on the pair a StateIndex names (members below)
#define stateGet(s, i) stateRead(i)
#define stateAnd(s, i, m) stateAndOr(i, (m), 0)
#define stateOr(s, i, m) stateAndOr(i, 0xf, (m))

void* CxSmallSort::operator new(size_t size)
{
    return MeMemoryAPI.create(size);
}

void CxSmallSort::operator delete(void* p)
{
    MeMemoryAPI.destroy(p);
}

// Mac 0x53e81c (complete ctor 0x53f93a), Win 0x78e770
CxSmallSort::CxSmallSort(int maxObjectCount, int maxPairCount, bool pairsDisabled)
{
    labels = 0;
    partners = 0;
    disabled = 0;
    linkedAxes = 0;
    unorderedAxes = 0;
    handleCapacity = 0;
    records = 0;
    recordMask = 0;
    recordCount = 0;
    scratch = 0;
    scratchCount = 0;
    scratchCapacity = 0;
    tree = 0;
    for (int a = 0; a < 3; ++a)
        treeRoot[a] = -1;
    takingOut = false;
    relocateCrossings = 0x7fffffff;
    groupOf = 0;
    groupPos = 0;
    groupCount = 0;
    groupMembers = 0;
    handlesResize((unsigned)maxObjectCount);
    entryTableInit(&entries, (unsigned)maxObjectCount);
    bitArrayInit(&active, (unsigned)maxObjectCount);
    bitArrayInit(&used, (unsigned)maxObjectCount);
    maxHandle = 0;
    freeHandles = 0;
    pairTableInit(&pairs, maxPairCount);
    unsigned char state = pairsDisabled ? 4 : 0;
    statesInit(&states, (unsigned)((MeReal)maxObjectCount * 0.5 * (MeReal)(maxObjectCount - 1)), maxObjectCount, state);
    for (int a = 0; a < 3; ++a)
        initEndpoint(&minSentinel[a]);
    for (int a = 0; a < 3; ++a)
        initEndpoint(&maxSentinel[a]);
    for (int a = 0; a < 3; ++a)
        linkInit(&axisList[a]);
    linkInit(&hello);
    linkInit(&staying);
    linkInit(&goodbye);
    linkInit(&pendingHello);
    linkInit(&pendingGoodbye);
    linkInit(&removed);
    changing = 1;
    m164 = 0;
    m168 = 0;
    m16c = 0;
    boundsFn = 0;
    userData = 0;
    defaultState = state;
}

struct CxSmallSort::GroupMembers {
    std::vector<std::vector<int> > members;   // [group - 1]
};

// Mac 0x53e59c (complete dtor 0x53e7d2), Win 0x78e550
CxSmallSort::~CxSmallSort()
{
    for (unsigned i = 0; i < handleCapacity; ++i) {
        cxFree(partners[i].data);
        if (disabled[i].r != disabled[i].inl)
            cxFree(disabled[i].r);
    }
    cxFree(partners);
    cxFree(disabled);
    cxFree(linkedAxes);
    cxFree(labels);
    cxFree(records);
    cxFree(scratch);
    cxFree(tree);
    cxFree(groupOf);
    cxFree(groupPos);
    delete groupMembers;
    cxFree(freeHandles);
    cxFree(states.data);
    pairTableDestroy(&pairs);
    cxFree(used.data);
    cxFree(active.data);
    blockListDestroy(&entries.blocks, false);
    cxFree(entries.data);
}

// Mac 0x53f962, Win 0x78e940
CxSmallSort* CxSmallSort::create(unsigned char axisMask, int maxObjectCount, int maxPairCount, bool pairsDisabled)
{
    if ((unsigned char)(axisMask - 1) >= 7 || maxObjectCount <= 0 || maxPairCount <= 0)
        return 0;
    CxSmallSort* s = new CxSmallSort(maxObjectCount, maxPairCount, pairsDisabled);
    if (s == 0)
        return 0;
    s->freeHandleCapacity = maxObjectCount;
    s->freeHandles = (int*)MeMemoryAPI.create(maxObjectCount * sizeof(int));
    s->axisMask = axisMask;
    s->axisCount = 0;
    unsigned char m = axisMask;
    unsigned char c = 0;
    do {
        m &= (unsigned char)(m - 1);
        ++c;
    } while (m != 0);
    s->axisCount = c;
    for (int a = 0; a < 3; ++a) {
        s->minSentinel[a].value = -kMaxReal;
        s->maxSentinel[a].value = kMaxReal;
        s->minSentinel[a].kind = -1;
        s->maxSentinel[a].kind = -1;
        linkInsertAfter(&s->minSentinel[a], &s->axisList[a]);
        linkInsertBefore(&s->maxSentinel[a], &s->axisList[a]);
    }
    for (int i = 0; i < maxObjectCount; ++i)
        s->freeHandles[i] = maxObjectCount - 1 - i;
    s->freeCount = maxObjectCount;
    bitArrayClearUnused(&s->active);
    bitArrayClearUnused(&s->used);
    s->maxHandle = 0;
    return s;
}

// Mac 0x53e7e6, Win 0x78e750
void CxSmallSort::destroy()
{
    delete this;
}

// Mac 0x53b638, Win 0x78e010
int CxSmallSort::getMaxObjectCount()
{
    return (int)entries.size;
}

// Mac 0x53b62c, Win 0x78e000
int CxSmallSort::getMaxPairCount()
{
    return pairs.capacity;
}

// Mac 0x53b3de, Win 0x78dea0
int CxSmallSort::getFreeCount()
{
    return freeCount;
}

// Mac 0x53bae0, Win 0x78e600
bool CxSmallSort::resize(unsigned n)
{
    char text[512];
    memset(text, 0, sizeof text);
    sprintf(text, "Resizing geometry table size to %d geometries\n", n);
    MeInfo(0, text); // NOTE: the text is passed as the format
    unsigned oldCapacity = entries.capacity;
    bool ok = true;
    if (n > oldCapacity)
        ok = entryTableReserve(&entries, n);
    if (ok) {
        entries.size = n;
        entryTableAllocate(&entries, oldCapacity, entries.capacity - 1);
    }
    bitArrayResize(&active, n);
    bitArrayResize(&used, n);
    int* handles = (int*)MeMemoryAPI.create(n * sizeof(int));
    cxFree(freeHandles);
    freeHandles = handles;
    freeCount = (int)n - freeHandleCapacity;
    for (int i = 0; i < freeCount; ++i)
        freeHandles[i] = freeHandleCapacity - 1 + freeCount - i;
    statesResize(&states, (int)n);
    handlesResize(n);
    freeHandleCapacity = (int)n;
    return true;
}

// Mac 0x53cfa2, Win 0x78f150
int CxSmallSort::insert(void* object, bool unsorted)
{
    if (freeCount < 1 && !resize((unsigned)(getMaxObjectCount() * 2)))
        return 0;
    int h = freeHandles[--freeCount];
    bitAssign(&used, h, true);
    if (maxHandle < h)
        maxHandle = h;
    CxEntry* e = entries.data[h];
    e->handle = h;
    e->object = object;
    setActive(h, true);
    for (int a = 0; a < 3; ++a) {
        e->ends[a][0].entry = e;
        e->ends[a][0].kind = 0;
        e->ends[a][1].entry = e;
        e->ends[a][1].kind = 1;
    }
    if (!unsorted)
        sortIn(e);
    return h;
}

// Mac 0x53b8f6, Win 0x78df80
int CxSmallSort::getEnabledPairCount()
{
    int count = 0;
    for (int i = 0; i <= maxHandle; ++i)
        if (bitTest(&used, i) && i > 0)
            for (int j = 0; j != i; ++j)
                if (bitTest(&used, j))
                    count += (unsigned char)isPairEnabled(j, i);
    return count;
}

// Mac 0x53c628, Win 0x78e410
bool CxSmallSort::remove(int h)
{
    if (!isValid(h))
        return true;
    setActive(h, false);
    bitAssign(&used, h, false);
    CxEntry* e = entries.data[h];
    sortOut(e, false);
    if (groupOf[h] != 0)
        groupLeave(h);   // not in the lists (sortOut returned at once): its pairs keep their states
    freeHandles[freeCount++] = h;
    e->handle = -1;
    e->object = 0;
    int m = maxHandle;
    if (m > 0)
        while (!bitTest(&used, m)) {
            --m;
            maxHandle = m;
            if (m == 0)
                break;
        }
    return true;
}

// Mac 0x53cf66, Win 0x78f200
void CxSmallSort::setUnsorted(int h, bool unsorted)
{
    CxEntry* e = entries.data[h];
    if (unsorted)
        sortOut(e, true);
    else
        sortIn(e);
}

// Mac 0x53b3ea, Win 0x78df00
void CxSmallSort::setUserData(void* data)
{
    userData = data;
}

// Mac 0x53fbce, Win 0x790050
void* CxSmallSort::getObject(int h)
{
    return entries.data[h]->object;
}

// Mac 0x53b3fc, Win 0x78df10
void* CxSmallSort::getUserData()
{
    return userData;
}

// Mac 0x53ba64, Win 0x78e020
bool CxSmallSort::setActive(int h, bool on)
{
    if (!isValid(h))
        return false;
    bitAssign(&active, h, on);
    entries.data[h]->active = on;
    return true;
}

// Mac 0x53b98e, Win 0x78deb0
bool CxSmallSort::isActive(int h)
{
    if (!isValid(h))
        return false;
    return bitTest(&active, h);
}

// Mac 0x53b442, Win 0x78df60
void CxSmallSort::setBoundsFn(CxBoundsFn fn)
{
    boundsFn = fn;
}

// Mac 0x53b454, Win 0x78df70
void CxSmallSort::setSphereFn(CxSphereFn)
{
}

// Mac 0x53d350, Win 0x78eab0
bool CxSmallSort::updateNow(int h, MeReal dt)
{
    if (boundsFn == 0)
        return false;
    beginChanges();
    CxEntry* e = entries.data[h];
    MeReal b[6];
    boundsFn(e->object, b, b + 3, dt);
    if (!relocate(e, b))
    updateEntry(e, b);
    endChanges();
    return true;
}

// LDD64: see the header. update() opens a change period first; so does this.
bool CxSmallSort::takeOut(const int* handles, int count)
{
    // (update moves only the axes of axisMask, sortIn all three: the same only with all three)
    if (tree == 0 || unorderedAxes != 0 || boundsFn == 0 || (axisMask & 7) != 7)
        return false;
    for (int i = 0; i < count; ++i)
        if (!isValid(handles[i]))
            return false;
    beginChanges();
    takingOut = true;
    for (int i = 0; i < count; ++i)
        sortOut(entries.data[handles[i]], true);
    takingOut = false;
    return true;
}

void CxSmallSort::putBack(const int* handles, int count)
{
    for (int i = 0; i < count; ++i) {
        CxEntry* e = entries.data[handles[i]];
        if (e->ends[0][0].next == &e->ends[0][0])
            sortIn(e);
    }
}

// Mac 0x53b644, Win 0x78e4c0 (body 0x78fa00)
bool CxSmallSort::beginChanges()
{
    if (changing != 0)
        return false;
    while (hello.next != &hello)
        linkMoveToBack(hello.next, &staying);
    while (goodbye.next != &goodbye) {
        CxPair* p = (CxPair*)goodbye.next;
        linkUnlinkSelf(&p->link);
        if (p->handle1 != -1) {
            if (pairTableRelease(&pairs, pairKey(p->handle1, p->handle2)) != 0)
                nodeDetach(p->handle1, p->handle2);
        }
    }
    while (pendingGoodbye.next != &pendingGoodbye)
        linkMoveToBack(pendingGoodbye.next, &goodbye);
    int n = (int)entries.size;
    while (pendingHello.next != &pendingHello) {
        CxPair* p = (CxPair*)pendingHello.next;
        linkMoveToBack(&p->link, &hello);
        stateAnd(&states, stateIndex(p->handle1, p->handle2, n), 7);
    }
    pairTableFlushDeferred(&pairs);
    changing = 1;
    return true;
}

// Mac 0x53b45a, Win 0x78e0c0 (body 0x78f7a0)
bool CxSmallSort::endChanges()
{
    int n = (int)entries.size;
    for (CxLink* l = hello.next; l != &hello; l = l->next)
        stateAnd(&states, stateIndex(((CxPair*)l)->handle1, ((CxPair*)l)->handle2, n), 7);
    for (CxLink* l = goodbye.next; l != &goodbye; l = l->next) {
        // NOTE: a second endChanges without beginChanges finds the removed pairs (handle1 -1) in the
        // goodbye list: their index is negative and the original clears a bit in the heap before
        // the table (guarded here)
        StateIndexT idx = stateIndex(((CxPair*)l)->handle1, ((CxPair*)l)->handle2, n);
        if (stateIndexValid(idx))
            stateAnd(&states, idx, 7);
    }
    for (CxLink* l = removed.next; l != &removed; l = l->next)
        ((CxPair*)l)->handle1 = -1;
    // the removed pairs go to the front of the goodbye list
    CxLink* first = removed.next;
    if (first != &removed) {
        first->prev = removed.prev;
        removed.prev->next = first;
        removed.prev = &removed;
        removed.next = &removed;
        CxLink* last = first->prev;
        last->next = goodbye.next;
        goodbye.next->prev = last;
        first->prev = &goodbye;
        goodbye.next = first;
    }
    changing = 0;
    return true;
}

// Mac 0x53d072, Win 0x78eb20
bool CxSmallSort::update(int h, MeReal dt, bool force)
{
    beginChanges();
    CxEntry* e = entries.data[h];
    if (e->active || force) {
        MeReal b[6];
        boundsFn(e->object, b, b + 3, dt);
        if (!relocate(e, b))
        updateEntry(e, b);
    }
    return true;
}

// Mac 0x53c700, Win 0x78eb80 (no beginChanges here)
bool CxSmallSort::updateAll(MeReal dt)
{
    if (boundsFn == 0)
        return false;
    const unsigned* end = active.data + (maxHandle >> 5) + 1;
    int base = 0;
    for (const unsigned* w = active.data; w < end; ++w, base += 32)
        for (unsigned bits = *w, h = (unsigned)base; bits != 0; bits >>= 1, ++h)
            if (bits & 1) {
                CxEntry* e = entries.data[h];
                MeReal b[6];
                boundsFn(e->object, b, b + 3, dt);
                updateEntry(e, b);
            }
    return true;
}

// Mac 0x53b8b6, Win 0x78e080
bool CxSmallSort::isValid(int h)
{
    if (h < 0 || h >= getMaxObjectCount())
        return false;
    return entries.data[h]->handle == h;
}

// Mac 0x53b40a / 0x53b418 / 0x53b426 / 0x53b434, Win 0x78df20 / 0x78df30 / 0x78df40 / 0x78df50
CxLink* CxSmallSort::getHelloList()
{
    return &hello;
}

CxLink* CxSmallSort::getStayingList()
{
    return &staying;
}

CxLink* CxSmallSort::getGoodbyeList()
{
    return &goodbye;
}

CxLink* CxSmallSort::getRemovedList()
{
    return &removed;
}

CxSmallSort::StateIndex CxSmallSort::pairIndex(int a, int b) const
{
    orderHandles(a, b);
    return stateIndex(a, b, (int)entries.size);
}

// Mac 0x53d634, Win 0x78ec30
bool CxSmallSort::setPairEnabled(int h1, int h2, bool enable)
{
    if (h1 == h2)
        return false;
    int lo = h1, hi = h2;
    orderHandles(lo, hi);
    StateIndexT idx = stateIndex(lo, hi, (int)entries.size);
    unsigned s = stateGet(&states, idx);
    if ((s & 3) != axisCount) {
        if (enable)
            stateAnd(&states, idx, 0xb);
        else
            stateOr(&states, idx, 4);
        return true;
    }
    CxPairKey key = pairKey(lo, hi);
    bool wasEnabled = (s & 4) == 0;
    if (changing == 0) {
        if (s & 8)
            return false;
        if (!enable) {
            if (wasEnabled) {
                stateOr(&states, idx, 4);
                stateOr(&states, idx, 8);
                CxPair* p = pairTableFind(&pairs, key);
                linkMoveToBack(&p->link, &pendingGoodbye);
            }
            return true;
        }
        if (wasEnabled)
            return true;
        CxPair* p = pairTableAlloc(&pairs, key);
        if (p == 0) {
            VincError(VincErrorTable, 0x18, "");
            return true;
        }
        p->handle1 = lo;
        p->handle2 = hi;
        nodeAttach(lo, hi);
        VincModelPairReset(&p->pair, (VincModelID)entries.data[lo]->object, (VincModelID)entries.data[hi]->object);
        linkInsertBefore(&p->link, &pendingHello);
        stateAnd(&states, idx, 0xb);
        stateOr(&states, idx, 8);
        return true;
    }
    bool listed = (s & 8) != 0;
    if (!enable) {
        if (!wasEnabled)
            return true;
        if (listed) {
            // a hello of this period: gone again
            stateAnd(&states, idx, 7);
            stateOr(&states, idx, 4);
            CxPair* p = pairTableRelease(&pairs, key);
            if (p != 0)
                nodeDetach(lo, hi);
            linkUnlinkSelf(&p->link);
            return true;
        }
        stateOr(&states, idx, 4);
        stateOr(&states, idx, 8);
        CxPair* p = pairTableFind(&pairs, key);
        linkMoveToBack(&p->link, &goodbye);
        return true;
    }
    if (wasEnabled)
        return true;
    if (listed) {
        // a goodbye of this period: back to staying
        CxPair* p = pairTableFind(&pairs, key);
        stateAnd(&states, idx, 7);
        stateAnd(&states, idx, 0xb);
        linkMoveToBack(&p->link, &staying);
        return true;
    }
    CxPair* p = pairTableAlloc(&pairs, key);
    if (p == 0) {
        VincError(VincErrorTable, 0x18, "");
        return true;
    }
    p->handle1 = lo;
    p->handle2 = hi;
    nodeAttach(lo, hi);
    VincModelPairReset(&p->pair, (VincModelID)entries.data[lo]->object, (VincModelID)entries.data[hi]->object);
    linkInsertBefore(&p->link, &hello);
    stateAnd(&states, idx, 0xb);
    stateOr(&states, idx, 8);
    return true;
}

// Mac 0x53b9e2, Win 0x78e0e0
bool CxSmallSort::setPairEnabled(int h, bool enable)
{
    const unsigned* end = used.data + (maxHandle >> 5) + 1;
    int base = 0;
    for (const unsigned* w = used.data; w < end; ++w, base += 32)
        for (unsigned bits = *w, j = (unsigned)base; bits != 0; bits >>= 1, ++j)
            if (bits & 1)
                setPairEnabled(h, (int)j, enable);
    return true;
}

// Mac 0x53fdde, Win 0x790070 (no h1 != h2 check)
bool CxSmallSort::isPairEnabled(int h1, int h2)
{
    // bit 2 only: the record, without the derived count
    StateIndex i = pairIndex(h1, h2);
    return ((stateIndexValid(i) ? recordGet(i.lo, i.hi) : defaultState) & 4) == 0;
}

// Mac 0x53fd34, Win 0x7900b0
bool CxSmallSort::isPairOverlapping(int h1, int h2)
{
    return (stateGet(&states, pairIndex(h1, h2)) & 3) == axisCount;
}

// Mac 0x53c27e, Win 0x78e4f0
McdModelPair* CxSmallSort::getPair(int h1, int h2)
{
    if (h1 == h2)
        return 0;
    orderHandles(h1, h2);
    CxPair* p = pairTableFind(&pairs, pairKey(h1, h2));
    return p != 0 ? &p->pair : 0;
}

// Mac 0x53fbe2, Win 0x790040
bool CxSmallSort::isChanging()
{
    return changing != 0;
}

// Mac 0x53ca84, Win 0x78efb0
void CxSmallSort::sortIn(CxEntry* e)
{
    MeReal b[6];
    boundsFn(e->object, b, b + 3, 0);
    sortIn(e, b);
}

// LDD64: see the header (relocate).
bool CxSmallSort::relocate(CxEntry* e, const MeReal* b)
{
    if (tree == 0 || unorderedAxes != 0 || (axisMask & 7) != 7 || e->ends[0][0].next == &e->ends[0][0])
        return false;
    long long crossed = 0;
    for (int a = 0; a < 3; ++a) {
        if (!(b[a] <= b[a + 3]))
            return false;   // (an inverted interval: the walk, which marks the axis unordered)
        for (int k = 0; k < 2; ++k) {
            const MeReal v = b[a + 3 * k];
            int mins0, maxes0, mins1, maxes1;
            treeLowerBound(a, e->ends[a][k].value, &mins0, &maxes0);
            treeLowerBound(a, v, &mins1, &maxes1);
            const int d = (mins1 + maxes1) - (mins0 + maxes0);
            crossed += d < 0 ? -d : d;
            // Equal values: the walk moves an end up to before the first end >= its new value
            // (as sortIn places it) and down to after the last end <= it; only an end of the
            // other kind at that value tells the difference, so with one there an end that
            // moves down or stays is walked.
            if (!(v > e->ends[a][k].value)) {
                int minsUp, maxesUp;
                treeLowerBound(a, nextafter(v, std::numeric_limits<MeReal>::infinity()), &minsUp, &maxesUp);
                int other = k == 0 ? maxesUp - maxes1 : minsUp - mins1;
                if (e->ends[a][1 - k].value == v)
                    --other;   // e's own other end (taken out with it)
                if (other > 0)
                    return false;
            }
        }
    }
    if (crossed < relocateCrossings)
        return false;
    takingOut = true;
    sortOut(e, true);
    takingOut = false;
    sortIn(e, b);
    return true;
}

// LDD64: see the header. Only the disabled bit changes: an enabled pair without a record does not
// overlap and a disabled one keeps its state, so its bit is set in the ranges as recordSet does.
// Pairs with a record (h's partners) go through setPairEnabled.
void CxSmallSort::disablePairs(int h, const int* others, int count)
{
    std::vector<int> withRecord;
    if (h >= 0 && (unsigned)h < handleCapacity) {
        const Partners& p = partners[h];
        withRecord.assign(p.data, p.data + p.count);
        std::sort(withRecord.begin(), withRecord.end());
    }
    const bool off = ((4u ^ defaultState) & 4) != 0;   // the disabled bit is a range entry
    for (int k = 0; k < count; ++k) {
        const int j = others[k];
        int lo = h, hi = j;
        orderHandles(lo, hi);
        if (h == j || h < 0 || j < 0 || (unsigned)h >= handleCapacity || (unsigned)j >= handleCapacity ||
            !stateIndexValid(stateIndex(lo, hi, (int)entries.size)) ||
            std::binary_search(withRecord.begin(), withRecord.end(), j)) {
            setPairEnabled(h, j, false);
            continue;
        }
        if (disabledTest(lo, hi) != off) {
            if (off) {
                disabledAdd(lo, hi);
                disabledAdd(hi, lo);
            } else {
                disabledRemove(lo, hi);
                disabledRemove(hi, lo);
            }
        }
    }
}

// sortIn at the bounds b
void CxSmallSort::sortIn(CxEntry* e, const MeReal* b)
{
    // an inverted (or NaN) interval can put list order and values apart (sortInOffsets)
    for (int a = 0; a < 3; ++a)
        if (!(b[a] <= b[a + 3]))
            unorderedAxes |= (unsigned char)(1 << a);
    if (unorderedAxes != 0 && tree != 0)
        treeDrop();
    if (sortInFast(e, b))
        return;
    bool fresh = false;
    for (int a = 0; a < 3; ++a) {
        CxEndpoint* lo = &e->ends[a][0];
        CxEndpoint* hi = &e->ends[a][1];
        lo->value = b[a];
        hi->value = b[a + 3];
        bool walked = (unorderedAxes & (1 << a)) != 0;
        CxLink* head = &axisList[a];
        CxLink* first = head->next != head ? head->next : 0;
        CxEndpoint* n = (CxEndpoint*)first->next;
        if (n->entry == 0) {
            // only the sentinels: both ends at the top, moved down by updateEntry below
            lo->value = kMaxReal;
            hi->value = kMaxReal;
            CxLink* last = head->prev != head ? head->prev : 0;
            linkInsertBefore(lo, last);
            labelInserted(lo);
            treeLink(lo);
            last = head->prev != head ? head->prev : 0;
            linkInsertBefore(hi, last);
            labelInserted(hi);
            treeLink(hi);
            linkedAxes[e->handle] |= (unsigned char)(1 << a);
            fresh = true;
        } else {
            scratchCount = 0;   // the walked endpoints, for sortInOffsets (only while values and list
                                // order may disagree on this axis: else they agree with the counts)
            MeReal v = b[a];
            while (n->value < v) {
                if (walked)
                    scratchPush(n);
                if (n->kind == 0 && v <= n[1].value)
                    updatePair(e, n->entry, 1);
                n = (CxEndpoint*)n->next;
            }
            linkInsertBefore(lo, n);
            labelInserted(lo);
            treeLink(lo);
            v = b[a + 3];
            while (n->value < v) {
                if (walked)
                    scratchPush(n);
                if (n->kind == 0)
                    updatePair(e, n->entry, 1);
                n = (CxEndpoint*)n->next;
            }
            linkInsertBefore(hi, n);
            labelInserted(hi);
            treeLink(hi);
            linkedAxes[e->handle] |= (unsigned char)(1 << a);
            if (walked)
                sortInOffsets(e, a, b[a]);
        }
    }
    if (fresh)
        updateEntry(e, b);
}

// Mac 0x53c330, Win 0x78e150. The original unlinks e and visits every handle j: a pair (e, j) whose
// count equals the axis count loses its node, and its state is reset. Only pairs with a record can
// change, so e's partners (and the (h, h) index when in use) are visited in ascending order; their
// counts are read before e leaves the lists and written for after (derived 0).
void CxSmallSort::sortOut(CxEntry* e, bool keepDisabled)
{
    if (e->ends[0][0].next == &e->ends[0][0])
        return;
    int n = (int)entries.size;
    int h = e->handle;
    // the disabled ranges come ascending; the record partners (unordered, few) are sorted and merged in
    scratchCount = 0;
    const Partners& ps = partners[h];
    for (int i = 0; i < ps.count; ++i)
        scratchPush((void*)(size_t)(unsigned)ps.data[i]);
    if (bitTest(&used, h))
        scratchPush((void*)(size_t)(unsigned)h);
    for (unsigned i = 1; i < scratchCount; ++i) {
        void* v = scratch[i];
        unsigned k = i;
        for (; k > 0 && (size_t)scratch[k - 1] > (size_t)v; --k)
            scratch[k] = scratch[k - 1];
        scratch[k] = v;
    }
    unsigned recordPartners = scratchCount;
    // keepDisabled: a range partner without a record has nothing to change; only a removed object's
    // ranges are cleared
    const Ranges& ds = disabled[h];
    if (!keepDisabled)
        for (int i = 0; i < ds.count; ++i)
            for (int x = ds.r[2 * i]; x < ds.r[2 * i + 1]; ++x)
                scratchPush((void*)(size_t)(unsigned)x);
    // merge the ascending runs [0, recordPartners) and [recordPartners, scratchCount), no
    // duplicates
    unsigned end1 = recordPartners, end2 = scratchCount;
    for (unsigned a = 0, b = recordPartners; a < end1 || b < end2;) {
        size_t v;
        if (b >= end2 || (a < end1 && (size_t)scratch[a] <= (size_t)scratch[b]))
            v = (size_t)scratch[a++];
        else
            v = (size_t)scratch[b++];
        if (scratchCount == end2 || (size_t)scratch[scratchCount - 1] != v)
            scratchPush((void*)v);
    }
    for (unsigned i = end2; i < scratchCount; ++i) {
        int j = (int)(size_t)scratch[i];
        if (!bitTest(&used, j))
            continue;
        int lo = h, hi = j;
        orderHandles(lo, hi);
        StateIndex idx = stateIndex(lo, hi, n);
        if (!stateIndexValid(idx))
            continue;
        // e's pairs before / after e leaves the lists. (h, h) = (h - 1, n - 1) is e's pair (h - 1,
        // h) only when h = n - 1, already reset at j = h - 1.
        bool ofE = idx.lo == h || idx.hi == h;
        unsigned dBefore = j == h && ofE ? 0 : derivedCount(idx.lo, idx.hi);
        unsigned dAfter = ofE ? 0 : dBefore;
        unsigned r = recordGet(idx.lo, idx.hi);
        unsigned s = ((dBefore + r) & 3) | (r & 0xc);
        if (takingOut) {
            // takeOut: e's pairs as update's walk would leave them when e moves away from every
            // box: an overlapping enabled pair goes to goodbye, listed and disabled bits stay,
            // count 0. putBack's sortIn restores what overlaps at the new place. The (h, h) index's
            // pair (h - 1, n - 1) is left.
            if (!ofE)
                continue;
            unsigned upper = s & 0xc;
            if ((s & 3) == axisCount && (s & 4) == 0) {
                CxPairKey key = pairKey(lo, hi);
                if ((s & 8) == 0) {
                    CxPair* pr = pairTableFind(&pairs, key);
                    if (pr != 0)
                        linkMoveToBack(&pr->link, &goodbye);
                } else {
                    CxPair* pr = pairTableRelease(&pairs, key);
                    if (pr != 0) {
                        nodeDetach(lo, hi);
                        linkUnlinkSelf(&pr->link);
                    }
                }
                upper ^= 8;
            }
            recordSet(idx.lo, idx.hi, upper, 0);
            continue;
        }
        if ((s & 3) == axisCount) {
            CxPairKey key = pairKey(lo, hi);
            CxPair* pr = pairTableFind(&pairs, key);
            if (pr != 0) {
                pairTableDefer(&pairs, key);
                nodeDetach(lo, hi);
                linkMoveToBack(&pr->link, &removed);
            }
        }
        unsigned ns = keepDisabled ? (s & 4) : defaultState;
        recordSet(idx.lo, idx.hi, ((ns - dAfter) & 3) | (ns & 0xc), 0);
    }
    // a group member: its pairs with the members are reset like the others, so it leaves the group
    if (!keepDisabled && groupOf[h] != 0)
        groupDetach(h);
    for (int a = 0; a < 3; ++a) {
        treeUnlink(&e->ends[a][0]);
        treeUnlink(&e->ends[a][1]);
        linkUnlinkSelf(&e->ends[a][0]);
        linkUnlinkSelf(&e->ends[a][1]);
    }
    linkedAxes[h] = 0;
}

// Win 0x790570 (inlined on Mac)
void CxSmallSort::updateEntry(CxEntry* e, const MeReal* b)
{
    if (e->ends[0][0].next == &e->ends[0][0])
        return;
    for (int a = 0; a < 3; ++a) {
        if ((axisMask & (1 << a)) == 0)
            continue;
        if (!(b[a] <= b[a + 3])) {
            unorderedAxes |= (unsigned char)(1 << a);
            if (tree != 0)
                treeDrop();
        }
        for (int k = 0; k < 2; ++k) {
            CxEndpoint* n = &e->ends[a][k];
            MeReal v = b[a + 3 * k];
            if (n->value <= v) {
                if (n->value < v)
                    moveUp(n, v);
            } else {
                moveDown(n, v);
            }
        }
    }
}

// Win 0x7904e0
void CxSmallSort::moveUp(CxEndpoint* n, MeReal v)
{
    CxEndpoint* next = (CxEndpoint*)n->next;
    if (next->value < v && next->kind != -1) {
        do {
            signed char d = (signed char)(n->kind - next->kind);
            if (d != 0)
                updatePair(n->entry, next->entry, d);
            {
                unsigned long long* x = labelSlot(n);
                unsigned long long* y = labelSlot(next);
                unsigned long long l = *x;
                *x = *y;
                *y = l;
            }
            next = (CxEndpoint*)next->next;
        } while (next->value < v && next->kind != -1);
        treeUnlink(n);
        linkUnlink(n);
        linkInsertBefore(n, next);
        treeLink(n);
    }
    n->value = v;
    treeValueChanged(n);
}

// Win 0x790450
void CxSmallSort::moveDown(CxEndpoint* n, MeReal v)
{
    CxEndpoint* prev = (CxEndpoint*)n->prev;
    if (v < prev->value && prev->kind != -1) {
        do {
            signed char d = (signed char)(prev->kind - n->kind);
            if (d != 0)
                updatePair(n->entry, prev->entry, d);
            {
                unsigned long long* x = labelSlot(n);
                unsigned long long* y = labelSlot(prev);
                unsigned long long l = *x;
                *x = *y;
                *y = l;
            }
            prev = (CxEndpoint*)prev->prev;
        } while (v < prev->value && prev->kind != -1);
        treeUnlink(n);
        linkUnlink(n);
        linkInsertAfter(n, prev);
        treeLink(n);
    }
    n->value = v;
    treeValueChanged(n);
}

// Mac 0x540606, Win 0x7902a0. The count follows the pass by itself (derived); only the flags are
// written.
void CxSmallSort::updatePair(CxEntry* a, CxEntry* b, int delta)
{
    int ha = a->handle, hb = b->handle;
    if (ha == hb)
        return;
    int lo = ha, hi = hb;
    orderHandles(lo, hi);
    unsigned r = recordGet(lo, hi);
    unsigned s = ((derivedCount(lo, hi) + r) & 3) | (r & 0xc);
    unsigned upper = s & 0xc;
    unsigned count = s & 3;
    unsigned newCount = (count + (unsigned)delta) & 3;
    if (count == axisCount && (s & 4) == 0) {
        // the pair stops overlapping
        CxPairKey key = pairKey(lo, hi);
        if ((s & 8) == 0) {
            CxPair* pr = pairTableFind(&pairs, key);
            linkMoveToBack(&pr->link, &goodbye);
        } else {
            // a hello of this period: gone again
            CxPair* pr = pairTableRelease(&pairs, key);
            if (pr != 0)
                nodeDetach(lo, hi);
            linkUnlinkSelf(&pr->link);
        }
        upper ^= 8;
    } else if (newCount == axisCount && (s & 4) == 0) {
        // the pair starts overlapping
        CxPairKey key = pairKey(lo, hi);
        if ((s & 8) == 0) {
            CxPair* pr = pairTableAlloc(&pairs, key);
            if (pr == 0) {
                VincError(VincErrorTable, 0x18, "");
            } else {
                pr->handle1 = lo;
                pr->handle2 = hi;
                nodeAttach(lo, hi);
                VincModelPairReset(&pr->pair, (VincModelID)a->object, (VincModelID)b->object);
                linkInsertBefore(&pr->link, &hello);
            }
        } else {
            // a goodbye of this period: staying again
            CxPair* pr = pairTableFind(&pairs, key);
            linkMoveToBack(&pr->link, &staying);
        }
        upper ^= 8;
    }
    if (upper != (s & 0xc))
        recordSet(lo, hi, (r & 3) | upper, 0);
}

// =================================================================================================
// Pair states without the table (LDD64; include/Cx/CxSmallSort.h)
//
// The original stores a nibble per pair: bits 0-1 the overlap count (mod 4), bit 2 disabled, bit 3
// listed in this change period. Here it is derived:
//  * Count: every endpoint has an order label, increasing along its axis list (new endpoints take
//    the middle of their neighbours'; passing endpoints swap labels). Over the axes where both
//    objects are in the list, [A.min < B.max] + [B.min < A.max] - 1 by label (mod 4) changes on
//    each pass exactly as updatePair's delta does, so the count needs no storage.
//  * Offset: where the original's stored count departs from the derived one (sortIn counts by
//    value, sortOut writes counts), a 2-bit offset in the pair's record.
//  * Disabled bit: sorted ranges of partner handles per handle (a sensor's geometries have
//    consecutive handles, so its k (k - 1) / 2 disabled pairs are a few ranges).
//  * Record: offset, listed bit and node count, only for pairs that are not (0, 0, no node), in an
//    open-addressing table; each handle lists its record partners.
// A state reads ((derived + offset) & 3) | disabled | listed: the original's nibble.
// Memory per handle: 80 bytes plus 8 per range, 48 per record, and 240 of search tree (sortInFast).
// =================================================================================================

// a node of the search tree over one axis list (sortInFast): in-order = list order
struct CxSmallSort::TreeNode {
    int l, r, p;                     // children, parent (-1: none; p -2: not in the tree)
    int count;                       // nodes in the subtree
    int countMin;                    // min endpoints in the subtree
    MeReal key;                      // the endpoint's value
    MeReal sub;                      // the subtree's largest max value of a min endpoint's interval
};

struct CxSmallSort::Record {
    unsigned long long key;          // lo << 32 | hi; kRecordEmpty: free slot
    int posLo;                       // place in partners[lo]
    int posHi;                       // place in partners[hi]
    unsigned char state;             // offset (bits 0-1), flags (bits 2-3)
    unsigned char nodes;             // hash nodes of the pair in the buckets
};

CxSmallSort::Record* CxSmallSort::recordFind(int lo, int hi) const
{
    if (records == 0)
        return 0;
    {
        // most pairs have no record: the shorter partner list says so without the hash probe
        const Partners& a = partners[lo];
        const Partners& b = partners[hi];
        const Partners& s = a.count <= b.count ? a : b;
        if (s.count <= 16) {
            int other = &s == &a ? hi : lo;
            int k = 0;
            while (k < s.count && s.data[k] != other)
                ++k;
            if (k == s.count)
                return 0;
        }
    }
    unsigned long long key = recordKey(lo, hi);
    for (unsigned i = (unsigned)mix64(key) & recordMask;; i = (i + 1) & recordMask) {
        Record* r = &records[i];
        if (r->key == key)
            return r;
        if (r->key == kRecordEmpty)
            return 0;
    }
}

// offset (bits 0-1) and flags (bits 2-3): disabled from the ranges, the rest from the record
unsigned CxSmallSort::recordGet(int lo, int hi) const
{
    const Record* r = recordFind(lo, hi);
    unsigned d = (defaultState & 4) ^ (disabledTest(lo, hi) ? 4u : 0u);
    return (r != 0 ? r->state : 0u) | d;
}

void CxSmallSort::recordGrow(unsigned capacity)
{
    Record* old = records;
    unsigned oldSlots = old != 0 ? recordMask + 1 : 0;
    records = (Record*)MeMemoryAPI.create(capacity * sizeof(Record));
    recordMask = capacity - 1;
    for (unsigned i = 0; i < capacity; ++i)
        records[i].key = kRecordEmpty;
    for (unsigned i = 0; i < oldSlots; ++i) {
        if (old[i].key == kRecordEmpty)
            continue;
        unsigned k = (unsigned)mix64(old[i].key) & recordMask;
        while (records[k].key != kRecordEmpty)
            k = (k + 1) & recordMask;
        records[k] = old[i];
    }
    cxFree(old);
}

// state (offset | flags) of the pair, and a change of its node count
void CxSmallSort::recordSet(int lo, int hi, unsigned state, int nodeDelta)
{
    bool off = ((state ^ defaultState) & 4) != 0;
    if (off != disabledTest(lo, hi)) {
        if (off) {
            disabledAdd(lo, hi);
            disabledAdd(hi, lo);
        } else {
            disabledRemove(lo, hi);
            disabledRemove(hi, lo);
        }
    }
    state &= 0xb;   // the record: offset and listed bit
    Record* r = recordFind(lo, hi);
    if (r == 0) {
        if (state == 0 && nodeDelta <= 0)
            return;
        if (records == 0 || (recordCount + 1) * 2 > recordMask + 1)
            recordGrow(records == 0 ? 64 : (recordMask + 1) * 2);
        unsigned long long key = recordKey(lo, hi);
        unsigned k = (unsigned)mix64(key) & recordMask;
        while (records[k].key != kRecordEmpty)
            k = (k + 1) & recordMask;
        r = &records[k];
        r->key = key;
        r->state = (unsigned char)state;
        r->nodes = (unsigned char)(nodeDelta > 0 ? nodeDelta : 0);
        ++recordCount;
        partnerAdd(lo, hi, &r->posLo);
        partnerAdd(hi, lo, &r->posHi);
        return;
    }
    r->state = (unsigned char)state;
    r->nodes = (unsigned char)(r->nodes + nodeDelta);
    if (r->state == 0 && r->nodes == 0)
        recordErase(r);
}

// the disabled ranges: partner in disabled[handle] (binary search for the first range ending after it)
static int rangeFind(const CxSmallSort::Ranges& d, int x)
{
    int lo = 0, hi = d.count;
    while (lo < hi) {
        int mid = (lo + hi) / 2;
        if (d.r[2 * mid + 1] <= x)
            lo = mid + 1;
        else
            hi = mid;
    }
    return lo;
}

static void rangeInsert(CxSmallSort::Ranges& d, int i, int a, int b)
{
    if (d.capacity == 0) {
        d.r = d.inl;   // two ranges inline
        d.capacity = 2;
    } else if (d.count == d.capacity) {
        int capacity = d.capacity * 2;
        int* r = (int*)MeMemoryAPI.create((size_t)capacity * 2 * sizeof(int));
        memcpy(r, d.r, (size_t)d.count * 2 * sizeof(int));
        if (d.r != d.inl)
            cxFree(d.r);
        d.r = r;
        d.capacity = capacity;
    }
    memmove(d.r + 2 * i + 2, d.r + 2 * i, (size_t)(d.count - i) * 2 * sizeof(int));
    d.r[2 * i] = a;
    d.r[2 * i + 1] = b;
    ++d.count;
}

static void rangeErase(CxSmallSort::Ranges& d, int i)
{
    memmove(d.r + 2 * i, d.r + 2 * i + 2, (size_t)(d.count - i - 1) * 2 * sizeof(int));
    if (--d.count == 0) {
        if (d.r != d.inl)
            cxFree(d.r);
        d.r = 0;
        d.capacity = 0;
    }
}

// the pair's disabled bit (not the default's): the range entry, flipped inside a group
bool CxSmallSort::disabledTest(int handle, int partner) const
{
    if (disabled == 0 || (unsigned)handle >= handleCapacity)
        return false;
    const Ranges& d = disabled[handle];
    bool entry = false;
    if (d.count != 0) {
        int i = rangeFind(d, partner);
        entry = i < d.count && d.r[2 * i] <= partner;
    }
    return inGroup(handle, partner) ? !entry : entry;
}

bool CxSmallSort::inGroup(int handle, int partner) const
{
    if (groupOf == 0 || (unsigned)handle >= handleCapacity || (unsigned)partner >= handleCapacity ||
        handle == partner)
        return false;
    int g = groupOf[handle];
    return g != 0 && groupOf[partner] == g;
}

// handle's side of the pair's bit set / cleared (each side is set by its own call)
void CxSmallSort::disabledAdd(int handle, int partner)
{
    if (inGroup(handle, partner))
        rangesRemove(handle, partner);
    else
        rangesAdd(handle, partner);
}

void CxSmallSort::disabledRemove(int handle, int partner)
{
    if (inGroup(handle, partner))
        rangesAdd(handle, partner);
    else
        rangesRemove(handle, partner);
}

// the range entry of partner in disabled[handle]
void CxSmallSort::rangesAdd(int handle, int partner)
{
    Ranges& d = disabled[handle];
    int i = rangeFind(d, partner);
    if (i < d.count && d.r[2 * i] <= partner)
        return;
    bool withPrevious = i > 0 && d.r[2 * i - 1] == partner;
    bool withNext = i < d.count && d.r[2 * i] == partner + 1;
    if (withPrevious && withNext) {
        d.r[2 * i - 1] = d.r[2 * i + 1];
        rangeErase(d, i);
    } else if (withPrevious) {
        d.r[2 * i - 1] = partner + 1;
    } else if (withNext) {
        d.r[2 * i] = partner;
    } else {
        rangeInsert(d, i, partner, partner + 1);
    }
}

void CxSmallSort::rangesRemove(int handle, int partner)
{
    Ranges& d = disabled[handle];
    int i = rangeFind(d, partner);
    if (!(i < d.count && d.r[2 * i] <= partner))
        return;
    int a = d.r[2 * i], b = d.r[2 * i + 1];
    if (a == partner && b == partner + 1) {
        rangeErase(d, i);
    } else if (a == partner) {
        d.r[2 * i] = partner + 1;
    } else if (b == partner + 1) {
        d.r[2 * i + 1] = partner;
    } else {
        d.r[2 * i + 1] = partner;
        rangeInsert(d, i + 1, partner + 1, b);
    }
}

// ---- pair groups (see the header; GroupMembers is defined before the destructor) ----

int CxSmallSort::groupNew()
{
    if ((defaultState & 4) != 0)
        return 0;   // pairs start disabled: a group would enable them
    if (groupMembers == 0)
        groupMembers = new GroupMembers;
    groupMembers->members.push_back(std::vector<int>());
    return ++groupCount;
}

// the members of `group` h has a record with
void CxSmallSort::groupRecordPartners(int h, int group, std::vector<int>& out) const
{
    out.clear();
    if (group == 0 || (unsigned)h >= handleCapacity)
        return;
    const Partners& p = partners[h];
    for (int i = 0; i < p.count; ++i)
        if (p.data[i] != h && groupOf[p.data[i]] == group)
            out.push_back(p.data[i]);
}

// h joins `group`: pairs with members that have no record become disabled (only the bit, as
// setPairEnabled(false) leaves them); pairs with a record keep the state setPairEnabled gave them.
// Inside a group a range entry flips the bit, so entries are removed or added to match.
void CxSmallSort::groupJoin(int h, int group)
{
    if (group == 0 || group > groupCount || (unsigned)h >= handleCapacity || groupOf[h] != 0)
        return;
    std::vector<int> withRecord;
    groupRecordPartners(h, group, withRecord);
    std::sort(withRecord.begin(), withRecord.end());
    const Ranges& d = disabled[h];
    std::vector<int> entries;
    for (int i = 0; i < d.count; ++i)
        for (int x = d.r[2 * i]; x < d.r[2 * i + 1]; ++x)
            if (x != h && (unsigned)x < handleCapacity && groupOf[x] == group &&
                !std::binary_search(withRecord.begin(), withRecord.end(), x))
                entries.push_back(x);
    for (size_t i = 0; i < entries.size(); ++i) {
        rangesRemove(h, entries[i]);
        rangesRemove(entries[i], h);
    }
    for (size_t i = 0; i < withRecord.size(); ++i) {
        int x = withRecord[i];
        const Ranges& dh = disabled[h];
        int k = rangeFind(dh, x);
        if (k < dh.count && dh.r[2 * k] <= x) {
            rangesRemove(h, x);   // disabled: the group's bit
            rangesRemove(x, h);
        } else {
            rangesAdd(h, x);      // enabled: an entry against the group's bit
            rangesAdd(x, h);
        }
    }
    std::vector<int>& m = groupMembers->members[group - 1];
    groupOf[h] = group;
    groupPos[h] = (int)m.size();
    m.push_back(h);
}

// h leaves its group, every pair keeping its state (entries flipped back to plain ranges)
void CxSmallSort::groupLeave(int h)
{
    int g = (unsigned)h < handleCapacity ? groupOf[h] : 0;
    if (g == 0)
        return;
    const std::vector<int> m = groupMembers->members[g - 1];
    for (size_t i = 0; i < m.size(); ++i) {
        int x = m[i];
        if (x == h)
            continue;
        const Ranges& d = disabled[h];
        int k = rangeFind(d, x);
        if (k < d.count && d.r[2 * k] <= x) {
            rangesRemove(h, x);
            rangesRemove(x, h);
        } else {
            rangesAdd(h, x);
            rangesAdd(x, h);
        }
    }
    groupUnlink(h);
}

// sortOut of a member leaving the space: its pairs with the members return to enabled
void CxSmallSort::groupDetach(int h)
{
    int g = groupOf[h];
    const Ranges& d = disabled[h];
    std::vector<int> entries;
    for (int i = 0; i < d.count; ++i)
        for (int x = d.r[2 * i]; x < d.r[2 * i + 1]; ++x)
            if (x != h && (unsigned)x < handleCapacity && groupOf[x] == g)
                entries.push_back(x);
    for (size_t i = 0; i < entries.size(); ++i) {
        rangesRemove(h, entries[i]);
        rangesRemove(entries[i], h);
    }
    groupUnlink(h);
}

// h out of its group's members
void CxSmallSort::groupUnlink(int h)
{
    int g = groupOf[h];
    std::vector<int>& m = groupMembers->members[g - 1];
    int pos = groupPos[h];
    int last = m.back();
    m[pos] = last;
    groupPos[last] = pos;
    m.pop_back();
    groupOf[h] = 0;
    groupPos[h] = 0;
}

void CxSmallSort::recordErase(Record* r)
{
    int lo = (int)(r->key >> 32), hi = (int)(unsigned)r->key;
    partnerRemove(lo, r->posLo);
    partnerRemove(hi, r->posHi);
    // backward-shift deletion (linear probing)
    unsigned i = (unsigned)(r - records);
    for (unsigned j = (i + 1) & recordMask; records[j].key != kRecordEmpty; j = (j + 1) & recordMask) {
        unsigned home = (unsigned)mix64(records[j].key) & recordMask;
        bool stays = i <= j ? (i < home && home <= j) : (i < home || home <= j);
        if (!stays) {
            records[i] = records[j];
            i = j;
        }
    }
    records[i].key = kRecordEmpty;
    --recordCount;
    if (recordMask + 1 > 1024 && recordCount * 8 < recordMask + 1)
        recordGrow((recordMask + 1) / 2);
}

void CxSmallSort::partnerAdd(int handle, int partner, int* position)
{
    Partners& p = partners[handle];
    if (p.count == p.capacity) {
        int capacity = p.capacity != 0 ? p.capacity * 2 : 4;
        int* d = (int*)MeMemoryAPI.create((size_t)capacity * sizeof(int));
        if (p.count != 0)
            memcpy(d, p.data, (size_t)p.count * sizeof(int));
        cxFree(p.data);
        p.data = d;
        p.capacity = capacity;
    }
    *position = p.count;
    p.data[p.count++] = partner;
}

void CxSmallSort::partnerRemove(int handle, int position)
{
    Partners& p = partners[handle];
    int last = p.data[--p.count];
    if (position != p.count) {
        p.data[position] = last;
        Record* m = handle < last ? recordFind(handle, last) : recordFind(last, handle);
        if (handle < last)
            m->posLo = position;
        else
            m->posHi = position;
    }
    if (p.count == 0) {
        cxFree(p.data);
        p.data = 0;
        p.capacity = 0;
    }
}

void CxSmallSort::nodeAttach(int lo, int hi)
{
    recordSet(lo, hi, recordGet(lo, hi), 1);
}

void CxSmallSort::nodeDetach(int lo, int hi)
{
    recordSet(lo, hi, recordGet(lo, hi), -1);
}

unsigned CxSmallSort::derivedCount(int lo, int hi) const
{
    if (lo < 0 || hi < 0 || (unsigned)lo >= entries.size || (unsigned)hi >= entries.size)
        return 0;
    unsigned both = linkedAxes[lo] & linkedAxes[hi];
    if (both == 0)
        return 0;
    unsigned c = 0;
    for (int ax = 0; ax < 3; ++ax) {
        if ((both & (1u << ax)) == 0)
            continue;
        const unsigned long long* la = &labels[(size_t)lo * 6 + (size_t)ax * 2];
        const unsigned long long* lb = &labels[(size_t)hi * 6 + (size_t)ax * 2];
        c += (unsigned)(la[0] < lb[1]) + (unsigned)(lb[0] < la[1]) + 3;   // + 3: - 1 (mod 4)
    }
    return c & 3;
}

unsigned CxSmallSort::stateRead(StateIndex i) const
{
    if (!stateIndexValid(i))
        return defaultState;
    unsigned r = recordGet(i.lo, i.hi);
    return ((derivedCount(i.lo, i.hi) + r) & 3) | (r & 0xc);
}

void CxSmallSort::stateWrite(StateIndex i, unsigned state)
{
    if (!stateIndexValid(i))
        return;
    unsigned d = derivedCount(i.lo, i.hi);
    recordSet(i.lo, i.hi, ((state - d) & 3) | (state & 0xc), 0);
}

// the original's stateAnd (andMask) / stateOr (orMask): only ever on bits 2-3
void CxSmallSort::stateAndOr(StateIndex i, unsigned andMask, unsigned orMask)
{
    if (!stateIndexValid(i))
        return;
    unsigned r = recordGet(i.lo, i.hi);
    unsigned flags = ((r & andMask) | orMask) & 0xc;
    if (flags != (r & 0xc))
        recordSet(i.lo, i.hi, (r & 3) | flags, 0);
}

unsigned long long* CxSmallSort::labelSlot(const CxEndpoint* p) const
{
    return &labels[(size_t)p->entry->handle * 6 + (size_t)(p - &p->entry->ends[0][0])];
}

unsigned long long CxSmallSort::labelOf(const CxEndpoint* p) const
{
    if (p->kind == -1)
        return p->value < 0 ? 0 : kLabelMax;
    return *labelSlot(p);
}

// p was just linked: a label between its neighbours'. If none is free, the labels of a doubling
// window around p are spread evenly (64-bit labels always leave room).
void CxSmallSort::labelInserted(CxEndpoint* p)
{
    unsigned long long lp = labelOf((CxEndpoint*)p->prev);
    unsigned long long ln = labelOf((CxEndpoint*)p->next);
    if (ln - lp >= 2) {
        *labelSlot(p) = lp + (ln - lp) / 2;
        return;
    }
    for (unsigned j = 2;; j *= 2) {
        CxEndpoint* left = (CxEndpoint*)p->prev;
        CxEndpoint* right = (CxEndpoint*)p->next;
        unsigned long long m = 1;
        for (unsigned k = 1; k < j && left->kind != -1; ++k, ++m)
            left = (CxEndpoint*)left->prev;
        for (unsigned k = 1; k < j && right->kind != -1; ++k, ++m)
            right = (CxEndpoint*)right->next;
        unsigned long long l = labelOf(left);
        unsigned long long gap = labelOf(right) - l;
        bool whole = left->kind == -1 && right->kind == -1;
        if (gap / (m + 1) >= m + 1 || whole) {
            unsigned long long step = gap / (m + 1);
            for (CxEndpoint* q = (CxEndpoint*)left->next; q != right; q = (CxEndpoint*)q->next) {
                l += step;
                *labelSlot(q) = l;
            }
            return;
        }
    }
}

void CxSmallSort::scratchPush(void* p)
{
    if (scratchCount == scratchCapacity) {
        unsigned capacity = scratchCapacity != 0 ? scratchCapacity * 2 : 256;
        void** d = (void**)MeMemoryAPI.create((size_t)capacity * sizeof(void*));
        if (scratchCount != 0)
            memcpy(d, scratch, (size_t)scratchCount * sizeof(void*));
        cxFree(scratch);
        scratch = d;
        scratchCapacity = capacity;
    }
    scratch[scratchCount++] = p;
}

// labels and partner lists for handles 0 .. n - 1
void CxSmallSort::handlesResize(unsigned n)
{
    if (n <= handleCapacity)
        return;
    unsigned long long* l = (unsigned long long*)MeMemoryAPI.create((size_t)n * 6 * sizeof(unsigned long long));
    Partners* p = (Partners*)MeMemoryAPI.create((size_t)n * sizeof(Partners));
    Ranges* d = (Ranges*)MeMemoryAPI.create((size_t)n * sizeof(Ranges));
    unsigned char* x = (unsigned char*)MeMemoryAPI.create(n);
    int* go = (int*)MeMemoryAPI.create((size_t)n * sizeof(int));
    int* gp = (int*)MeMemoryAPI.create((size_t)n * sizeof(int));
    // the search trees: allocated until an axis turns unordered (treeDrop)
    TreeNode* tr = handleCapacity == 0 || tree != 0 ? (TreeNode*)MeMemoryAPI.create((size_t)n * 6 * sizeof(TreeNode)) : 0;
    if (tr != 0) {
        if (tree != 0)
            memcpy(tr, tree, (size_t)handleCapacity * 6 * sizeof(TreeNode));
        for (size_t i = (size_t)handleCapacity * 6; i < (size_t)n * 6; ++i)
            tr[i].p = -2;
    }
    if (handleCapacity != 0) {
        memcpy(l, labels, (size_t)handleCapacity * 6 * sizeof(unsigned long long));
        memcpy(p, partners, (size_t)handleCapacity * sizeof(Partners));
        memcpy(d, disabled, (size_t)handleCapacity * sizeof(Ranges));
        memcpy(x, linkedAxes, handleCapacity);
        memcpy(go, groupOf, (size_t)handleCapacity * sizeof(int));
        memcpy(gp, groupPos, (size_t)handleCapacity * sizeof(int));
        for (unsigned i = 0; i < handleCapacity; ++i)
            if (disabled[i].r == disabled[i].inl)
                d[i].r = d[i].inl;   // the inline ranges moved
    }
    memset(x + handleCapacity, 0, n - handleCapacity);
    memset(go + handleCapacity, 0, (size_t)(n - handleCapacity) * sizeof(int));
    memset(gp + handleCapacity, 0, (size_t)(n - handleCapacity) * sizeof(int));
    memset(l + (size_t)handleCapacity * 6, 0, (size_t)(n - handleCapacity) * 6 * sizeof(unsigned long long));
    for (unsigned i = handleCapacity; i < n; ++i) {
        p[i].data = 0;
        p[i].count = 0;
        p[i].capacity = 0;
        d[i].r = 0;
        d[i].count = 0;
        d[i].capacity = 0;
    }
    cxFree(labels);
    cxFree(partners);
    cxFree(disabled);
    cxFree(linkedAxes);
    cxFree(groupOf);
    cxFree(groupPos);
    groupOf = go;
    groupPos = gp;
    cxFree(tree);
    tree = tr;
    labels = l;
    partners = p;
    disabled = d;
    linkedAxes = x;
    handleCapacity = n;
}

// After sortIn: the original's count counts the mins it walked (the first walk only where e.min <=
// B.max by value); the derived count compares labels. They agree while the list is in value order;
// otherwise the difference goes into the offset. With no interval ever inverted (unorderedAxes 0,
// always for LDD's boxes), sortIn skips this.
void CxSmallSort::sortInOffsets(CxEntry* e, int axis, MeReal minValue)
{
    unsigned long long eMin = labelOf(&e->ends[axis][0]);
    unsigned long long eMax = labelOf(&e->ends[axis][1]);
    for (unsigned i = 0; i < scratchCount; ++i) {
        const CxEndpoint* q = (const CxEndpoint*)scratch[i];
        CxEntry* b = q->entry;
        if (b == 0 || b == e)
            continue;
        const CxEndpoint* bMin = &b->ends[axis][0];
        const CxEndpoint* bMax = &b->ends[axis][1];
        unsigned long long lMin = labelOf(bMin), lMax = labelOf(bMax);
        unsigned added;
        if (q->kind == 0) {
            added = lMin < eMin ? (minValue <= bMax->value ? 1u : 0u) : 1u;
        } else {
            if (lMin < eMax)
                continue;   // B's min was walked too: counted there
            added = 0;
        }
        unsigned derived = (unsigned)(eMin < lMax) + (unsigned)(lMin < eMax) + 3;
        unsigned diff = (added - derived) & 3;
        if (diff == 0)
            continue;
        int lo = e->handle, hi = b->handle;
        orderHandles(lo, hi);
        unsigned r = recordGet(lo, hi);
        recordSet(lo, hi, (r & 0xc) | ((r + diff) & 3), 0);
    }
}

// -------------------------------------------------------------------------------------------------
// sortIn without the walk (LDD64)
//
// The original's sortIn walks each axis list from its start to the new min's and max's places,
// calling updatePair at each min it passes: O(n) per insert, O(n^2) for a space built object by
// object (70,000 loose bricks: 9 * 10^9 steps).
//
// While every list is in value order (unorderedAxes 0), a treap per axis (endpoints in list order,
// keyed by value, with subtree counts and the largest max of each subtree's mins) finds both places
// by binary search and lists the overlapping B in list order. Only calls that can change a state
// are made, in the walk's order: pairs whose count can reach axisCount (found from the smallest
// overlap list of the earlier axes) and pairs with a record. When a count of 0 matters (axisCount
// 0 or 1), every overlapping B is called.
// -------------------------------------------------------------------------------------------------

namespace {

inline unsigned treePriority(int id)
{
    unsigned x = (unsigned)id;
    x ^= x >> 16;
    x *= 0x7feb352du;
    x ^= x >> 15;
    x *= 0x846ca68bu;
    x ^= x >> 16;
    return x;
}

const MeReal kTreeNone = -std::numeric_limits<MeReal>::infinity();   // treeOwn of a max endpoint

} // namespace

int CxSmallSort::treeIdOf(const CxEndpoint* p) const
{
    return p->entry->handle * 6 + (int)(p - &p->entry->ends[0][0]);
}

CxEndpoint* CxSmallSort::treeEndpoint(int id) const
{
    return &entries.data[id / 6]->ends[0][0] + id % 6;
}

// a min endpoint: its interval's max value (the walk's v <= max test: NaN never passes)
MeReal CxSmallSort::treeOwn(int id) const
{
    if ((id & 1) != 0)
        return kTreeNone;
    MeReal m = tree[id + 1].key;
    return m == m ? m : kTreeNone;
}

void CxSmallSort::treePull(int x)
{
    TreeNode& n = tree[x];
    int count = 1, countMin = (x & 1) == 0 ? 1 : 0;
    MeReal sub = treeOwn(x);
    if (n.l >= 0) {
        const TreeNode& c = tree[n.l];
        count += c.count;
        countMin += c.countMin;
        if (c.sub > sub)
            sub = c.sub;
    }
    if (n.r >= 0) {
        const TreeNode& c = tree[n.r];
        count += c.count;
        countMin += c.countMin;
        if (c.sub > sub)
            sub = c.sub;
    }
    n.count = count;
    n.countMin = countMin;
    n.sub = sub;
}

// x's own value changed: its subtree sums up to the root (until one does not change)
void CxSmallSort::treeRefresh(int x)
{
    for (; x >= 0; x = tree[x].p) {
        MeReal old = tree[x].sub;
        treePull(x);
        if (tree[x].sub == old)
            return;
    }
}

void CxSmallSort::treeRotateUp(int axis, int x)
{
    int p = tree[x].p, g = tree[p].p;
    if (tree[p].l == x) {
        tree[p].l = tree[x].r;
        if (tree[x].r >= 0)
            tree[tree[x].r].p = p;
        tree[x].r = p;
    } else {
        tree[p].r = tree[x].l;
        if (tree[x].l >= 0)
            tree[tree[x].l].p = p;
        tree[x].l = p;
    }
    tree[p].p = x;
    tree[x].p = g;
    if (g < 0)
        treeRoot[axis] = x;
    else if (tree[g].l == p)
        tree[g].l = x;
    else
        tree[g].r = x;
    treePull(p);
    treePull(x);
}

// x (key set) goes in front of y (-1: at the end)
void CxSmallSort::treeInsertBefore(int axis, int x, int y)
{
    TreeNode& n = tree[x];
    n.l = n.r = -1;
    if (treeRoot[axis] < 0) {
        n.p = -1;
        treeRoot[axis] = x;
        treePull(x);
        return;
    }
    int t;
    if (y < 0) {
        for (t = treeRoot[axis]; tree[t].r >= 0; t = tree[t].r) {
        }
        tree[t].r = x;
    } else if (tree[y].l < 0) {
        t = y;
        tree[t].l = x;
    } else {
        for (t = tree[y].l; tree[t].r >= 0; t = tree[t].r) {
        }
        tree[t].r = x;
    }
    n.p = t;
    treePull(x);
    for (; t >= 0; t = tree[t].p)
        treePull(t);
    unsigned px = treePriority(x);
    while (tree[x].p >= 0 && px > treePriority(tree[x].p))
        treeRotateUp(axis, x);
}

// x (key set) goes after y (-1: at the start)
void CxSmallSort::treeInsertAfter(int axis, int x, int y)
{
    TreeNode& n = tree[x];
    n.l = n.r = -1;
    if (treeRoot[axis] < 0) {
        n.p = -1;
        treeRoot[axis] = x;
        treePull(x);
        return;
    }
    int t;
    if (y < 0) {
        for (t = treeRoot[axis]; tree[t].l >= 0; t = tree[t].l) {
        }
        tree[t].l = x;
    } else if (tree[y].r < 0) {
        t = y;
        tree[t].r = x;
    } else {
        for (t = tree[y].r; tree[t].l >= 0; t = tree[t].l) {
        }
        tree[t].l = x;
    }
    n.p = t;
    treePull(x);
    for (; t >= 0; t = tree[t].p)
        treePull(t);
    unsigned px = treePriority(x);
    while (tree[x].p >= 0 && px > treePriority(tree[x].p))
        treeRotateUp(axis, x);
}

void CxSmallSort::treeRemove(int axis, int x)
{
    for (;;) {
        int l = tree[x].l, r = tree[x].r;
        if (l < 0 && r < 0)
            break;
        int c = l < 0 ? r : r < 0 ? l : treePriority(l) > treePriority(r) ? l : r;
        treeRotateUp(axis, c);
    }
    int p = tree[x].p;
    if (p < 0)
        treeRoot[axis] = -1;
    else if (tree[p].l == x)
        tree[p].l = -1;
    else
        tree[p].r = -1;
    tree[x].p = -2;
    for (; p >= 0; p = tree[p].p)
        treePull(p);
}

// p was just linked into its axis list (with its value): its node goes to the same place
void CxSmallSort::treeLink(CxEndpoint* p)
{
    if (tree == 0)
        return;
    int x = treeIdOf(p);
    int axis = (x % 6) / 2;
    tree[x].key = p->value;
    if ((x & 1) == 0)
        tree[x + 1].key = p[1].value;   // the interval's max (linked after it)
    CxEndpoint* next = (CxEndpoint*)p->next;
    if (next->kind != -1) {
        treeInsertBefore(axis, x, treeIdOf(next));
    } else {
        CxEndpoint* prev = (CxEndpoint*)p->prev;
        treeInsertAfter(axis, x, prev->kind != -1 ? treeIdOf(prev) : -1);
    }
}

void CxSmallSort::treeUnlink(CxEndpoint* p)
{
    if (tree == 0)
        return;
    int x = treeIdOf(p);
    if (tree[x].p != -2)
        treeRemove((x % 6) / 2, x);
}

void CxSmallSort::treeValueChanged(CxEndpoint* p)
{
    if (tree == 0)
        return;
    int x = treeIdOf(p);
    if (tree[x].p == -2)
        return;
    tree[x].key = p->value;
    if ((x & 1) != 0 && tree[x - 1].p != -2)
        treeRefresh(x - 1);   // the interval's max: the min's tree value
}

// the first node whose key is not below v (-1: none), with the min and max endpoints before it
int CxSmallSort::treeLowerBound(int axis, MeReal v, int* minsBefore, int* maxesBefore) const
{
    int before = 0, beforeMin = 0, found = -1;
    for (int x = treeRoot[axis]; x >= 0;) {
        const TreeNode& n = tree[x];
        if (n.key < v) {
            int l = n.l;
            before += 1 + (l >= 0 ? tree[l].count : 0);
            beforeMin += ((x & 1) == 0 ? 1 : 0) + (l >= 0 ? tree[l].countMin : 0);
            x = n.r;
        } else {
            found = x;
            x = n.l;
        }
    }
    *minsBefore = beforeMin;
    *maxesBefore = before - beforeMin;
    return found;
}

// the intervals [vlo, vhi) overlaps in subtree x (min below vhi, max >= vlo), in list order, onto
// scratch; false: the end of the range was reached
bool CxSmallSort::treeCollect(int x, MeReal vlo, MeReal vhi, int skipHandle)
{
    while (x >= 0) {
        const TreeNode& n = tree[x];
        if (!(n.sub >= vlo))
            return true;
        if (!treeCollect(n.l, vlo, vhi, skipHandle))
            return false;
        if (!(n.key < vhi))
            return false;
        if ((x & 1) == 0 && treeOwn(x) >= vlo && x / 6 != skipHandle)
            scratchPush((void*)(size_t)(unsigned)(x / 6));
        x = n.r;
    }
    return true;
}

// an axis turned unordered: the walks are the original's from now on
void CxSmallSort::treeDrop()
{
    cxFree(tree);
    tree = 0;
    for (int a = 0; a < 3; ++a)
        treeRoot[a] = -1;
}

namespace {
struct LabelLess {
    const unsigned long long* labels;
    int axis;
    bool operator()(void* a, void* b) const
    {
        return labels[(size_t)a * 6 + (size_t)axis * 2] < labels[(size_t)b * 6 + (size_t)axis * 2];
    }
};
} // namespace

// sortIn's three axes for e with bounds b, while every list is in value order and has endpoints;
// false (nothing changed): the original's walks
bool CxSmallSort::sortInFast(CxEntry* e, const MeReal* b)
{
    if (tree == 0 || unorderedAxes != 0)
        return false;
    for (int a = 0; a < 3; ++a)
        if (treeRoot[a] < 0 || !(b[a] <= b[a + 3]) || !(b[a + 3] <= kMaxReal))
            return false;   // an empty list (the first object), or a walk past the max sentinel
    // the walks' stops, and how many intervals each axis' walk calls
    int stop1[3], stop2[3], overlaps[3];
    for (int a = 0; a < 3; ++a) {
        int mins1, maxes1, mins2, maxes2;
        stop1[a] = treeLowerBound(a, b[a], &mins1, &maxes1);
        stop2[a] = treeLowerBound(a, b[a + 3], &mins2, &maxes2);
        overlaps[a] = mins2 - maxes1;
    }
    unsigned need0 = axisCount & 3u, need1 = (axisCount + 3u) & 3u;
    int h = e->handle;
    for (int a = 0; a < 3; ++a) {
        CxEndpoint* lo = &e->ends[a][0];
        CxEndpoint* hi = &e->ends[a][1];
        lo->value = b[a];
        hi->value = b[a + 3];
        int idLo = h * 6 + a * 2, idHi = idLo + 1;
        tree[idLo].key = b[a];
        tree[idHi].key = b[a + 3];
        linkInsertBefore(lo, stop1[a] >= 0 ? treeEndpoint(stop1[a]) : &maxSentinel[a]);
        labelInserted(lo);
        treeInsertBefore(a, idLo, stop1[a]);
        linkInsertBefore(hi, stop2[a] >= 0 ? treeEndpoint(stop2[a]) : &maxSentinel[a]);
        labelInserted(hi);
        treeInsertBefore(a, idHi, stop2[a]);
        // the calls (updatePair reads no axis-a labels: e is not linked on axis a for it yet)
        scratchCount = 0;
        if (need0 == 0 || need1 == 0) {
            // a pair overlapping on no earlier axis can matter: every interval the walk passes
            treeCollect(treeRoot[a], b[a], b[a + 3], h);
        } else {
            // the counts a pair without a record can have here: 0 .. a (overlaps on earlier axes)
            int kmin = 4;
            for (int k = a; k >= 1; --k)
                if ((unsigned)k == need0 || (unsigned)k == need1)
                    kmin = k;
            if (kmin <= a) {
                // kmin == a: such a pair overlaps e on every axis so far; any of the lists will do
                int s = a;
                if (kmin == a)
                    for (int k = 0; k < a; ++k)
                        if (overlaps[k] < overlaps[s])
                            s = k;
                treeCollect(treeRoot[s], b[s], b[s + 3], h);
            }
            const Partners& ps = partners[h];
            for (int i = 0; i < ps.count; ++i)
                scratchPush((void*)(size_t)(unsigned)ps.data[i]);
            // only those the walk calls (overlapping on axis a), in its order (list order of the mins)
            const unsigned long long* le = &labels[(size_t)h * 6 + (size_t)a * 2];
            unsigned k = 0;
            for (unsigned i = 0; i < scratchCount; ++i) {
                int j = (int)(size_t)scratch[i];
                if (j == h || (unsigned)j >= entries.size || (linkedAxes[j] & (1u << a)) == 0)
                    continue;
                const unsigned long long* lj = &labels[(size_t)j * 6 + (size_t)a * 2];
                if (le[0] < lj[1] && lj[0] < le[1])
                    scratch[k++] = scratch[i];
            }
            scratchCount = k;
            LabelLess less = {labels, a};
            std::sort(scratch, scratch + scratchCount, less);
            k = 0;
            for (unsigned i = 0; i < scratchCount; ++i)
                if (k == 0 || scratch[k - 1] != scratch[i])
                    scratch[k++] = scratch[i];
            scratchCount = k;
        }
        for (unsigned i = 0; i < scratchCount; ++i)
            updatePair(e, entries.data[(size_t)scratch[i]], 1);
        linkedAxes[h] |= (unsigned char)(1 << a);
    }
    return true;
}
