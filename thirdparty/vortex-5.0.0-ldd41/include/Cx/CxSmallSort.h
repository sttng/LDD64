// Vortex 5.0.0 build 800 "vortexldd41" - CxSmallSort: sweep-and-prune broad phase (the only
// CxSpace; VincSpaceAxisSortCreate) (restored; src/Cx/CxSmallSort.cpp,
// Mac 558d 0x53b3de-0x540d84 (the small slots sit at the end of 558c), Win 0x78dc10-0x790570).
//
// Objects get small integer handles. Each sorted object has a min and a max endpoint per axis in
// three axis lists sorted by value (sentinels at -DBL_MAX / DBL_MAX). Moving an endpoint past
// another object's endpoint changes the pair's overlap count; a pair whose count reaches the number
// of enabled axes gets a CxPair (with its VincModelPair) from a hash table keyed (lo << 16 | hi) and
// enters the hello list. The per-pair state is one nibble of a triangular table:
//   bits 0-1  number of overlapping axes (mod 4)
//   bit 2     pair disabled (setPairEnabled)
//   bit 3     the pair changed list in this change period (hello / goodbye / pending)
// Lists: hello (new pairs), staying, goodbye (ended pairs), pending hello / goodbye (pairs enabled /
// disabled outside a change period), removed (pairs of removed / unsorted objects; moved to the
// front of goodbye at endChanges with handle1 = -1). beginChanges() starts a change period: hellos
// become stayings, goodbyes are freed, pending pairs become hellos / goodbyes.
//
// Layout natural (doubles: Mac 4-, MSVC 8-aligned): Mac 0x180 bytes, Win 0x188. Names inferred
// except the RTTI class names. The containers are template code in the binaries (out of line on
// Win: hash table 0x78f560-0x790280, block list 0x78f6c0 / 0x78fd80, bit array 0x78dcf0 / 0x78dd50 /
// 0x78f2b0, entry table 0x78f260 / 0x78f620 / 0x78fbb0, state table 0x78dd90 / 0x78de00 / 0x78de40
// / 0x78f350); here plain structs with inline members.
#ifndef CX_CXSMALLSORT_H
#define CX_CXSMALLSORT_H

// LDD64 memory: a space allocates its pair-state table and pair-node pool when it first needs them
// (same contents), so an empty space costs nothing (LDD makes four; three stay empty, 40 MB each).

// LDD64 large models: no per-pair state table (CxSmallSort.cpp, "Pair states without the table").
// The original keeps 4 bits per pair (2 GB at 131,072 objects) and works up to 65,536 objects.
// Here overlap counts come from the endpoints' order and only differing states are stored. Every
// state, pair list and return value is the original's.

#include "Cx/CxSpace.h"
#include "Vinc/VincIntersect.h"

#include <stddef.h>
#include <vector>

struct CxEntry;

// Chunk registry of a pool: 100 chunks per block, blocks chained (0x328 bytes). (name inferred)
struct CxBlockList {
    struct Chunk {
        void* items;                 // +0x00
        int count;                   // +0x04
    };
    Chunk chunks[100];               // +0x000
    unsigned count;                  // +0x320
    CxBlockList* next;               // +0x324 older block
};

// One end of an object's interval on an axis (0x18 bytes on both).
struct CxEndpoint : CxLink {
    MeReal value;                    // +0x08
    CxEntry* entry;                  // +0x10
    signed char kind;                // +0x14 0 min, 1 max, -1 list sentinel
};

// One object (Mac 0x9c, Win 0xa0 bytes).
struct CxEntry {
    int handle;                      // +0x00 -1: free
    void* object;                    // +0x04 the VincModel
    bool active;                     // +0x08 setActive (also in CxSmallSort::active)
    CxEndpoint ends[3][2];           // +0x0c / +0x10 [axis][min, max]
};

// A broad-phase pair (0x44 bytes): the list link, the two handles (lo, hi) and the VincModelPair.
struct CxPair {
    CxLink link;                     // +0x00 in one of the pair lists
    int handle1;                     // +0x08 lo (-1: removed, hash node already released)
    int handle2;                     // +0x0c hi
    McdModelPair pair;               // +0x10 (link + 0x10 is what VincSpaceGetPairs hands out)
    int unused40;                    // +0x40
};

// Hash table node (0x50 bytes).
struct CxPairNode {
    CxLink hashLink;                 // +0x00 bucket chain
    unsigned key;                    // +0x08 lo << 16 | hi
    CxPair value;                    // +0x0c
};

// Array of entry pointers + their chunk registry (0x14 bytes). (name inferred)
struct CxEntryTable {
    CxEntry** data;                  // +0x00
    unsigned size;                   // +0x04 = max object count
    unsigned capacity;               // +0x08
    unsigned initialSize;            // +0x0c
    CxBlockList* blocks;             // +0x10
};

// Bit array (0x14 bytes). (name inferred)
struct CxBitArray {
    unsigned* data;                  // +0x00
    unsigned bitCount;               // +0x04
    unsigned wordCount;              // +0x08
    unsigned capacity;               // +0x0c words
    unsigned initialWords;           // +0x10
};

// Pooled hash table of CxPairs (0x28 bytes). The bucket of a key is key % mask (mask = 2^k - 1:
// the last bucket is never used). (name inferred)
struct CxPairTable {
    int growBy;                      // +0x00 nodes per pool growth (100, or the pair count)
    int allocated;                   // +0x04 nodes made so far
    CxPairNode** freeBase;           // +0x08 free-node stack
    CxPairNode** freeTop;            // +0x0c
    CxBlockList* blocks;             // +0x10
    CxLink deferred;                 // +0x14 nodes released at the next beginChanges
    unsigned mask;                   // +0x1c
    CxLink* buckets;                 // +0x20 mask + 1 bucket heads
    int capacity;                    // +0x24 nominal pair count (doubled on demand)
};

// Triangular table of 4-bit pair states (0x10 bytes). (name inferred)
struct CxPairStates {
    unsigned char* data;             // +0x00
    int byteCount;                   // +0x04
    int objectCount;                 // +0x08
    unsigned char fill;              // +0x0c initial byte (state | state << 4)
};

class CxSmallSort : public CxSpace {
public:
    // Mac 0x53f962, Win 0x78e940: axisMask bits 0-2 = the axes that are sorted (1..7)
    static CxSmallSort* create(unsigned char axisMask, int maxObjectCount, int maxPairCount,
                               bool pairsDisabled);
    static void* operator new(size_t size);   // MeMemoryAPI.create
    static void operator delete(void* p);     // MeMemoryAPI.destroy

    void destroy();
    int getMaxObjectCount();
    int getMaxPairCount();
    int getFreeCount();
    int insert(void* object, bool unsorted);
    int getEnabledPairCount();
    bool remove(int handle);
    void setUnsorted(int handle, bool unsorted);
    void setUserData(void* data);
    void* getObject(int handle);
    void* getUserData();
    bool setActive(int handle, bool active);
    bool isActive(int handle);
    void setBoundsFn(CxBoundsFn fn);
    void setSphereFn(CxSphereFn fn);
    bool updateNow(int handle, MeReal dt);
    // LDD64 (large carried groups): objects that jump leave the axis lists and are sorted back in
    // at their new bounds instead of update's walk past every endpoint between. The pairs end as
    // update leaves them. True only while the search trees exist and all axes are sorted; else
    // nothing changes and the caller updates as before.
    bool takeOut(const int* handles, int count);
    void putBack(const int* handles, int count);
    // LDD64: update / updateNow relocate (sortOut + sortIn) an object whose ends would pass at
    // least relocateCrossings endpoints, as takeOut / putBack do: the same pair states, in another
    // order within a list. INT_MAX (default): always walk; VincSpaceSetRelocateCrossings sets it.
    // disablePairs: setPairEnabled(handle, others[k], false) for each k in order; a pair without a
    // record only gets its disabled bit.
    void disablePairs(int handle, const int* others, int count);
    // LDD64: pair groups. Two members of one group read as disabled (a range entry inside a group
    // means enabled), so joining (groupJoin) or leaving costs O(the object's records) instead of a
    // pair per member, which made a part of n geometries O(n^2). Only where pairs start enabled
    // (else groupNew returns 0). groupJoin(h, g): h's pairs with g's members become disabled (the
    // caller has already disabled those with a record); groupRecordPartners: the members h has a
    // record with; groupLeave: h leaves, every pair keeping its state.
    int groupNew();
    void groupJoin(int handle, int group);
    void groupRecordPartners(int handle, int group, std::vector<int>& out) const;
    void groupLeave(int handle);
    bool beginChanges();
    bool endChanges();
    bool update(int handle, MeReal dt, bool force);
    bool updateAll(MeReal dt);
    bool isValid(int handle);
    CxLink* getHelloList();
    CxLink* getStayingList();
    CxLink* getGoodbyeList();
    CxLink* getRemovedList();
    bool setPairEnabled(int handle1, int handle2, bool enable);
    bool setPairEnabled(int handle, bool enable);
    bool isPairEnabled(int handle1, int handle2);
    bool isPairOverlapping(int handle1, int handle2);
    McdModelPair* getPair(int handle1, int handle2);
    bool isChanging();

    // object is active (not frozen): read directly by VincSpaceGetPairs (Mac 0x541c40)
    bool activeBit(int handle) const { return (active.data[handle >> 5] & (1u << (handle & 31))) != 0; }

    CxSmallSort(int maxObjectCount, int maxPairCount, bool pairsDisabled);   // Mac 0x53e81c, Win 0x78e770
    ~CxSmallSort();                                                          // Mac 0x53e59c, Win 0x78e550

    // Mac 0x53bae0, Win 0x78e600: grow the object tables to n handles
    bool resize(unsigned n);
    // Mac 0x53ca84, Win 0x78efb0: put an object into the axis lists
    void sortIn(CxEntry* e);
    void sortIn(CxEntry* e, const MeReal* bounds);   // at these bounds (relocate)
    bool relocate(CxEntry* e, const MeReal* bounds);
    // Mac 0x53c330, Win 0x78e150: take it out (its overlapping pairs go to the removed list)
    void sortOut(CxEntry* e, bool keepDisabled);
    // Win 0x790570 (inlined on Mac): move the endpoints to bounds[0..2] (min) / [3..5] (max)
    void updateEntry(CxEntry* e, const MeReal* bounds);
    // Win 0x7904e0 / 0x790450 (inlined on Mac): move one endpoint up / down to v
    void moveUp(CxEndpoint* n, MeReal v);
    void moveDown(CxEndpoint* n, MeReal v);
    // Mac 0x540606, Win 0x7902a0: the overlap count of (a, b) changes by delta
    void updatePair(CxEntry* a, CxEntry* b, int delta);
    // pair index / state nibble (Win 0x78dc10)
    struct StateIndex {
        int lo, hi;                  // the pair whose state the original's index addresses (lo -1: none)
    };
    StateIndex pairIndex(int handle1, int handle2) const;

    // pair states without the table (CxSmallSort.cpp)
    unsigned derivedCount(int lo, int hi) const;
    unsigned stateRead(StateIndex i) const;
    void stateWrite(StateIndex i, unsigned state);
    void stateAndOr(StateIndex i, unsigned andMask, unsigned orMask);
    struct Record;
    Record* recordFind(int lo, int hi) const;
    unsigned recordGet(int lo, int hi) const;
    void recordSet(int lo, int hi, unsigned record, int nodeDelta);
    void recordErase(Record* r);
    void recordGrow(unsigned capacity);
    void partnerAdd(int handle, int partner, int* position);
    void partnerRemove(int handle, int position);
    unsigned long long* labelSlot(const CxEndpoint* p) const;
    unsigned long long labelOf(const CxEndpoint* p) const;
    void labelInserted(CxEndpoint* p);
    void scratchPush(void* p);
    void handlesResize(unsigned n);
    void sortInOffsets(CxEntry* e, int axis, MeReal minValue);
    void nodeAttach(int lo, int hi);
    void nodeDetach(int lo, int hi);
    bool disabledTest(int handle, int partner) const;
    void disabledAdd(int handle, int partner);
    void disabledRemove(int handle, int partner);
    bool inGroup(int handle, int partner) const;
    void rangesAdd(int handle, int partner);
    void rangesRemove(int handle, int partner);
    void groupDetach(int handle);
    void groupUnlink(int handle);
    // search trees over the axis lists (sortIn without the walk)
    struct TreeNode;
    bool sortInFast(CxEntry* e, const MeReal* b);
    int treeIdOf(const CxEndpoint* p) const;
    CxEndpoint* treeEndpoint(int id) const;
    MeReal treeOwn(int id) const;
    void treePull(int x);
    void treeRefresh(int x);
    void treeRotateUp(int axis, int x);
    void treeInsertBefore(int axis, int x, int y);
    void treeInsertAfter(int axis, int x, int y);
    void treeRemove(int axis, int x);
    void treeLink(CxEndpoint* p);
    void treeUnlink(CxEndpoint* p);
    void treeValueChanged(CxEndpoint* p);
    int treeLowerBound(int axis, MeReal v, int* minsBefore, int* maxesBefore) const;
    bool treeCollect(int x, MeReal vlo, MeReal vhi, int skipHandle);
    void treeDrop();

    unsigned char axisCount;         // +0x04 number of bits in axisMask
    unsigned char axisMask;          // +0x05
    CxEntryTable entries;            // +0x08
    CxBitArray active;               // +0x1c not frozen
    CxBitArray used;                 // +0x30 handle in use
    int maxHandle;                   // +0x44 highest handle in use (or 0)
    int* freeHandles;                // +0x48 stack
    int freeHandleCapacity;          // +0x4c
    int freeCount;                   // +0x50
    CxPairTable pairs;               // +0x54
    CxPairStates states;             // +0x7c
    CxEndpoint minSentinel[3];       // +0x8c / +0x90
    CxEndpoint maxSentinel[3];       // +0xd4 / +0xd8
    CxLink axisList[3];              // +0x11c / +0x120
    CxLink hello;                    // +0x134 / +0x138
    CxLink staying;                  // +0x13c / +0x140
    CxLink goodbye;                  // +0x144 / +0x148
    CxLink pendingHello;             // +0x14c / +0x150
    CxLink pendingGoodbye;           // +0x154 / +0x158
    CxLink removed;                  // +0x15c / +0x160
    int m164;                        // +0x164 / +0x168 (0, never read)
    int m168;                        // +0x168 / +0x16c (0, never read)
    int m16c;                        // +0x16c / +0x170 (0, never read)
    CxBoundsFn boundsFn;             // +0x170 / +0x174
    int changing;                    // +0x174 / +0x178 1 inside begin/endChanges (initially 1)
    void* userData;                  // +0x178 / +0x17c
    unsigned char defaultState;      // +0x17c / +0x180 4 when pairs start disabled
    // (after the original's members, which keep their offsets)
    struct Partners {                // the handles a handle has records with (unordered)
        int* data;
        int count;
        int capacity;
    };
    struct Ranges {                  // sorted half-open handle ranges [r[2i], r[2i + 1])
        int* r;                      // inline (two ranges) or allocated
        int count;                   // ranges
        int capacity;
        int inl[4];
    };
    unsigned long long* labels;      // endpoint order labels: [handle * 6 + axis * 2 + kind]
    Partners* partners;              // [handle]
    Ranges* disabled;                // [handle]: partners whose disabled bit is not defaultState's
    unsigned char* linkedAxes;       // [handle]: bit a = both endpoints on axis a are in the list
    unsigned char unorderedAxes;     // bit a: an interval was inverted (or NaN) on axis a once
    unsigned handleCapacity;         // handles the two arrays hold
    Record* records;                 // open addressing, recordMask + 1 slots (0: none)
    unsigned recordMask;
    unsigned recordCount;
    void** scratch;                  // sortIn's walked endpoints / sortOut's partners
    unsigned scratchCount;
    unsigned scratchCapacity;
    TreeNode* tree;                  // [handle * 6 + axis * 2 + kind]; 0 once an axis is unordered
    bool takingOut;                  // in takeOut (sortOut leaves e's pairs as update would)
    int treeRoot[3];                 // per axis list (-1: empty)
    int relocateCrossings;           // update / updateNow relocate from this many crossed ends
    int* groupOf;                    // [handle]: its pair group (0: none)
    int* groupPos;                   // [handle]: its index in the group's members
    int groupCount;                  // groups handed out
    struct GroupMembers;
    GroupMembers* groupMembers;      // group -> members (CxSmallSort.cpp)
};

#endif
