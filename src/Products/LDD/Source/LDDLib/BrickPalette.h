// LEGO Digital Designer — restored source
// LEGO::BrickPalette — the bricks the user can place: bags (palettes, "*.paxml", models used as
// palettes) per brick set (BrickPaletteBuckets), each distinct brick of a set once (sorted by
// main group, volume, design ID, materials, decorations), the models saved as palettes and the
// material picker's colour grids. Notifies BrickPaletteObservers; follows the application's brand.
// LEGO::BrickPalette::Brick / ::Part — read-only brick descriptions (design ID, name, sort key,
// default materials / decorations / bone transforms per part); read by LDDBrick (TU 351),
// PrimitiveElementBrick (TU 184) and BrickDraw (TU 173).
// LEGO::BrickPalette::Bag — one bag: file, name, brand, version, models, flags, colour grid and
// the bricks with their counts.
// LEGO::MaterialGrid — the picker's colours in rows of a fixed width, in four groups (solid,
// transparent, special, obsolete).
//
// Mac TU 141 "BrickPalette.cpp" (modmap guess Path.cpp), 0x1f85f0-0x20372e; Win 0x63a5e9-0x641c11
// (scattered, much inlined by /LTCG). The bag loaders (BrickPaletteBagLoaderPAXML / Model,
// TU 057 / 440) fill Bags and Bricks; this TU only reads the brick database and
// MainGroupDividers/MainGroups.xml of db.lif.
//
// RTTI (Mac / Win):
//   class BrickPalette : LDDApplicationObserver (+0), NotifierT<BrickPalette, BrickPaletteObserver>
//       (+0x08 / Win +0x0c), virtual Object. Mac sizeof 0xbc (Object +0xb4), primary vtable
//       0x728e5c {D1 0x1ff2e0, D0 0x1ff420, OnFinalRelease 0x1ff590, LDDApplicationObserver
//       handlers, OnBrandChanged 0x1ff6d0}; Win Object at +0xdc (vftable 0x9d3fd0 {sdd 0x641ad5}),
//       handler vftable 0x9d3fe4 {…, OnBrandChanged 0x63af0b, …}.
//   class BrickPalette::Bag : Object (non-virtual). Mac sizeof 0x134, vtable 0x728e3c {D1
//       0x1fbbc0, D0 0x1fbbd0}; Win vftable 0x9d3fa4 {sdd 0x63fe9e}.
//   class BrickPalette::Brick : AbstractBrick (virtual Object). Mac sizeof 0x60 (Object +0x58),
//       vtable 0x728de4 {GetDesignID 0x1fa370, GetPartCount 0x1fa380, GetPart 0x1fa3a0, D1
//       0x1faac0, D0 0x1fab80}; Win vfptr +0, vbptr +4, members from +8, Object +0x44
//       (vftable 0x9d3e80 {sdd 0x63d8c6}).
//   class BrickPalette::Part : AbstractPart (virtual Object). Mac sizeof 0x40 (Object +0x38),
//       vtable 0x728d8c {0x1fa050, 0x1fa060, 0x1fa080, 0x1fa0c0, 0x1fa0f0, D1 0x1fa1a0, D0
//       0x1fa2b0, slot 7 0x1fa0a0}; Win vbptr +4, Object +0x3c (vftable 0x9d3e34 {sdd 0x63bc2c}).
//   struct BrickPalette::BrickLess, struct MaterialGrid, enum BrickPaletteBuckets (Win RTTI
//       template arguments U / W4).
// Every member and method name is inferred unless noted.
#pragma once
#include "LEGO/Types.h"
#include "Object.h"
#include "LEGO/String.h"
#include "BasicArray.h"
#include "BasicMap.h"
#include "Geometry.h"
#include "Brick.h"
#include "Part.h"
#include "Notifier.h"
#include "Path.h"
#include "Version.h"
#include "MaterialManual.h"
#include "BrickDatabase.h"
#include "Brand.h"
#include "LDDApplication.h"

namespace LEGO {

class BrickPalette;
class BrickPaletteObserver;
class PrimitiveElementBrick;
class XMLDOMNode;
class File;
struct StorageScene2;

// The brick sets of the palettes (GetBrandBrickSet, Brand.h): one palette content per set.
// (RTTI enum name BrickPaletteBuckets; enumerator names inferred)
enum BrickPaletteBuckets {
    kBrickSetLDD         = 0,
    kBrickSetLDDExtended = 1,
    kBrickSetMindstorms  = 2
};

// ===========================================================================================
// MaterialGrid — the materials of the colour picker, sorted by MaterialManual::GetMaterialOrder
// and laid out in rows of `width` (the last row padded with 0). Mac sizeof 0xb0. Implicit
// constructor / destructor (Win 0x63efcb / 0x63f087), assignment Mac 0x1fc880.
// ===========================================================================================
struct MaterialGrid {
    typedef BasicSet<UInt32, MaterialManual::OrderedMaterialLess> MaterialSet;
    typedef BasicArray<BasicArray<UInt32> >                       Rows;

    // Clears rows and sets. Mac 0x1f8a00
    void Clear();
    // Sorts `material` into the obsolete, transparent, solid (shiny / matte plastic) or special
    // set — in that order of precedence. Mac 0x1f8bd0
    void AddMaterial(UInt32 material, const BrickDatabase& database);
    // Appends the materials of every non-empty set as rows of `width` to its rows (NOTE: the
    // rows are not cleared first; Clear() does that). Mac 0x1f8c50
    void BuildRows(UInt32 width);
    // <Grid width="8"><Material id="…"/>…</Grid> children of `document` (default width 8; the
    // last Grid's width counts); Clear() first. Mac 0x1f85f0, Win 0x63e791
    void Load(const XMLDOMNode& document, const BrickDatabase& database);
    // Parses `file` as XML and Load()s it. Mac 0x1f8870, Win 0x63ed7d
    bool Load(File* file, const BrickDatabase& database);
    // Every non-zero material of every part of every brick of the palette's CURRENT brick set.
    // Mac 0x1f8910
    void Build(BrickPalette& palette, UInt32 width, const BrickDatabase& database);

    bool HasRows() const { return !m_solidRows.IsEmpty() || !m_transparentRows.IsEmpty() || !m_specialRows.IsEmpty(); }

    // ---- data (Mac offsets) ----
    Rows        m_solidRows;         // +0x00
    Rows        m_transparentRows;   // +0x10
    Rows        m_specialRows;       // +0x20
    Rows        m_obsoleteRows;      // +0x30
    MaterialSet m_solid;             // +0x40 MaterialType shiny / matte plastic
    MaterialSet m_transparent;       // +0x5c
    MaterialSet m_special;           // +0x78 the other material types (metal, glitter …)
    MaterialSet m_obsolete;          // +0x94 not in CurrentMaterials.xml
};

class BrickPalette : public LDDApplicationObserver,
                     public NotifierT<BrickPalette, BrickPaletteObserver> {
public:
    class Part;
    class Brick;
    class Bag;

    // Orders palette bricks by their sort key (String operator<).
    struct BrickLess {
        bool operator()(const RefPointer<const Brick>& a, const RefPointer<const Brick>& b) const;
    };

    typedef BasicSet<RefPointer<const Brick>, BrickLess>                        BrickSet;
    typedef BasicArray<RefPointer<const Bag> >                                   BagArray;
    typedef BasicMap<RefPointer<const StorageScene2>, Path>                      ModelMap;
    typedef BasicMap<BrickPaletteBuckets, BagArray, Less<BrickPaletteBuckets> >  BagMap;
    typedef BasicMap<BrickPaletteBuckets, BrickSet, Less<BrickPaletteBuckets> >  BrickSetMap;
    typedef BasicMap<BrickPaletteBuckets, MaterialGrid, Less<BrickPaletteBuckets> > GridMap;

    // -----------------------------------------------------------------------------------
    class Part : public AbstractPart {
    public:
        Part() : m_designID(0) {}                                                  // Mac 0x1fa110

        virtual UInt32 GetDesignID() const { return m_designID; }                  // Mac 0x1fa050, Win 0x639c26
        virtual UInt32 GetMaterialCount() const { return m_materials.GetSize(); }  // Mac 0x1fa060, Win 0x63a012
        // materials[index], or materials[0] when that is 0 (no range check).      Mac 0x1fa080, Win 0x63a01c
        virtual UInt32 GetMaterial(UInt32 index) const
        {
            return m_materials[index] != 0 ? m_materials[index] : m_materials[0];
        }
        // decorations[index] or 0.                                                 Mac 0x1fa0c0, Win 0x63a03d
        virtual UInt32 GetDecoration(UInt32 index) const
        {
            return index < m_decorations.GetSize() ? m_decorations[index] : 0;
        }
        virtual bool IsFlexible() const { return m_boneTransforms.GetSize() > 1; } // Mac 0x1fa0f0, Win 0x63a05b
        virtual ~Part() {}                                                         // Mac 0x1fa1a0 / 0x1fa2b0, Win sdd 0x63bc2c
        virtual UInt32 GetDecorationCount() const { return m_decorations.GetSize(); } // Mac 0x1fa0a0 (slot 7), Win 0x63a033

        // ---- data (Mac offsets / Win offsets) ----
        UInt32               m_designID;        // +0x04 / +0x08
        BasicArray<UInt32>   m_materials;       // +0x08 / +0x0c
        BasicArray<UInt32>   m_decorations;     // +0x18 / +0x1c
        BasicArray<Matrix3D> m_boneTransforms;  // +0x28 / +0x2c one per bone, in brick space
    };

    // -----------------------------------------------------------------------------------
    class Brick : public AbstractBrick {
    public:
        // Design ID 0xffffffff, everything else empty / 0. Mac 0x1fa4c0
        Brick() : m_designID(0xffffffff), m_mainGroupID(0), m_volume(0), m_mainGroup(0), m_mainGroupOrder(0) {}

        // --- AbstractBrick
        virtual UInt32 GetDesignID() const { return m_designID; }                  // Mac 0x1fa370, Win 0x639c26
        virtual UInt32 GetPartCount() const { return m_parts.GetSize(); }          // Mac 0x1fa380, Win 0x63a06e
        // The palette part (no range check).                                     Mac 0x1fa3a0, Win 0x63a5e9
        virtual AbstractPart* GetPart(UInt32 index) const;
        virtual ~Brick() {}                                                        // Mac 0x1faac0 / 0x1fab80, Win sdd 0x63d8c6

        // The database brick `designID` (Initialize below); false when the database has none.
        // Mac 0x1fa610
        bool Initialize(UInt32 designID, const BrickDatabase& database);
        // Design ID, "maingroupid" annotation, display main group and its order, and the volume
        // (sum of the bone bounding boxes of the parts' primitives, cached per design ID).
        // Does not touch name, parts or sort key. Always true. Mac 0x1fa650, Win 0x63ee09
        bool Initialize(const BrickDatabase::Brick& brick, const BrickDatabase& database);

        // --- non-virtual accessors
        const String& GetName() const       { return m_name; }                       // Mac 0x1fa3f0
        const String& GetSortKey() const    { return m_sortKey; }                    // Mac 0x1fa3e0
        UInt32        GetMainGroupID() const { return m_mainGroupID; }               // Mac 0x1fa3c0
        UInt32        GetMainGroup() const  { return m_mainGroup; }                  // Mac 0x1fa3d0
        Real32        GetVolume() const     { return m_volume; }
        UInt32        GetMainGroupOrder() const { return m_mainGroupOrder; }
        // Any part flexible (Part slot 4).                                        Mac 0x1fa400
        bool IsFlexible() const;
        const Part* GetPalettePart(UInt32 index) const { return m_parts[index]; }

        // ---- data (Mac offsets / Win offsets) ----
        UInt32                          m_designID;        // +0x04 / +0x08
        String                          m_name;            // +0x08 / +0x0c (LDDBrick::SetPaletteBrick copies it)
        BasicArray<RefPointer<Part> >   m_parts;           // +0x18 / +0x10
        UInt32                          m_mainGroupID;     // +0x28 / +0x20 "maingroupid" annotation
        Real32                          m_volume;          // +0x2c / +0x24
        UInt32                          m_mainGroup;       // +0x30 / +0x28 GetDisplayMainGroup(m_mainGroupID)
        UInt32                          m_mainGroupOrder;  // +0x34 / +0x2c GetMainGroupOrder(m_mainGroup)
        String                          m_sortKey;         // +0x38 / +0x30 MakeSortKey
        BasicArray<UInt32>              m_values48;        // +0x48 / +0x34 (not used in TU 141; meaning unknown)
    };

    // -----------------------------------------------------------------------------------
    class Bag : public Object {
    public:
        typedef BasicMap<RefPointer<const Brick>, UInt32, BrickLess> BrickCountMap;

        Bag();                                                                     // Mac 0x1fb2e0 (C1 0x1fb620)
        // Copies the description (path, name, text, brand, version, models, flags) — NOT the
        // colour grid nor the bricks (BrickPalette::AddBag fills those). Mac 0x1fb630
        Bag(const Bag& other);
        virtual ~Bag();                                                            // Mac 0x1fbbc0 / 0x1fbbd0 (body 0x1fb990), Win 0x63fd60 / sdd 0x63fe9e

        // Brick by sort key. Mac 0x1fae80
        bool   Contains(const Brick* brick) const;
        // Its count, 0 when absent. Mac 0x1faf90
        UInt32 GetCount(const Brick* brick) const;
        bool   IsCountable() const { return m_countable; }                         // Mac 0x1faf80
        bool   IsEmpty() const { return m_bricks.GetSize() == 0; }                 // Mac 0x1fb090
        void   Clear() { m_bricks.Clear(); }                                       // Mac 0x1fb0b0
        // Computes the brick's sort key (with materials unless the bag's brand is LDD
        // Extended) and adds `count` of it. Mac 0x1fb100
        void   AddBrick(Brick* brick, UInt32 count, const BrickDatabase& database);
        // Adds `count` (a new entry or the existing one's count + count). Mac 0x1fb150, Win 0x63ef20
        void   AddBrickCount(const Brick* brick, UInt32 count);
        BrickCountMap::ConstIterator Begin() const { return m_bricks.Begin(); }    // Mac 0x1fb2c0
        BrickCountMap::ConstIterator End() const   { return m_bricks.End(); }      // Mac 0x1fb2d0

        // ---- data (Mac offsets) ----
        Path                                   m_path;         // +0x08 the bag's file / folder
        String                                 m_name;         // +0x20 Info.baxml <Bag name>
        String                                 m_text;         // +0x30 (set to "" by the PAXML loader)
        UInt32                                 m_brand;        // +0x40 Brand, 0xffffffff by default
        Version                                m_version;      // +0x44 <Bag version>
        BasicArray<RefPointer<StorageScene2> > m_models;       // +0x54
        bool                                   m_countable;    // +0x64 <Bag countable>
        bool                                   m_brandFilter;  // +0x65 <Bag brandFilter>: the bag's colour grid replaces the set's
        MaterialGrid                           m_grid;         // +0x68
        BrickCountMap                          m_bricks;       // +0x118
    };

    // Attaches to LDDApplication::GetInstance() and takes the brick set of its brand.
    // Mac 0x1fbcd0 (Win: inlined into the LDDApplication constructor)
    BrickPalette();
    virtual ~BrickPalette() {}                                                     // Mac 0x1ff2e0 / 0x1ff420, Win sdd 0x641c11

    // ---- content -------------------------------------------------------------------------
    // A copy of `bag` (with the palette's own instance of each brick) for every brick set of
    // the bag's brand (GetBrandBrickSets). Mac 0x1fbf70
    void AddBag(const Bag& bag);
    // Remembers a model (by its file); notifies when its brand is the application's.
    // Mac 0x1fcb90, Win 0x63f188
    void AddModel(StorageScene2* model, const Path& path);
    // Forgets the model and DELETES its file. Mac 0x1fceb0
    void RemoveModel(StorageScene2* model);

    // The palette brick with the sort key of `brick` (materials included unless `set` is
    // LDD Extended — NOTE: the original tests IsBrandMindstorms(set)), or null.
    // Mac 0x1fd1e0 / 0x1fd3c0 (current set)
    const Brick* FindBrick(const Brick& brick, const BrickDatabase& database, BrickPaletteBuckets set);
    const Brick* FindBrick(const Brick& brick, const BrickDatabase& database);
    // The same for a scene brick snapshot. Mac 0x1fd3e0 / 0x1fd7f0 (current set)
    const Brick* FindBrick(const PrimitiveElementBrick& brick, const BrickDatabase& database, BrickPaletteBuckets set);
    const Brick* FindBrick(const PrimitiveElementBrick& brick, const BrickDatabase& database);

    // The set's colour grid (built from the current set's bricks with width 8 when no bag
    // supplied one). Mac 0x1fd810 / 0x1fda30 (current set)
    const MaterialGrid* GetMaterialGrid(BrickPaletteBuckets set, const BrickDatabase& database);
    const MaterialGrid* GetMaterialGrid(const BrickDatabase& database);

    // No notifications in between; EndUpdate notifies OnPaletteChanged. Mac 0x1fda50 / 0x1fda60
    void BeginUpdate() { m_updating = true; }
    void EndUpdate();

    // ---- access (current brick set; NOTE: map operator[], creates the entry) -------------
    BagArray::ConstIterator GetBagsBegin();                                        // Mac 0x1fdcd0
    BagArray::ConstIterator GetBagsEnd();                                          // Mac 0x1fdd00
    UInt32                  GetBagCount();                                         // Mac 0x1fdd30
    BrickSet::ConstIterator GetBricksBegin();                                      // Mac 0x1f8b70
    BrickSet::ConstIterator GetBricksEnd();                                        // Mac 0x1f8ba0
    ModelMap::ConstIterator GetModelsBegin() const { return m_models.Begin(); }    // Mac 0x1fdd60, Win 0x63b5ae
    ModelMap::ConstIterator GetModelsEnd() const   { return m_models.End(); }      // Mac 0x1fdd70
    BrickPaletteBuckets     GetCurrentSet() const  { return m_currentSet; }
    const BagMap&           GetBagMap() const      { return m_bags; }
    const BrickSetMap&      GetBrickSetMap() const { return m_bricks; }

    // ---- main groups and sort keys (static) ----------------------------------------------
    // MainGroupDividers/MainGroups.xml of the database directory: <Merge value merge="…"/>
    // entries (main group ids shown as `value`) and <OrderList order="…"/>. Adds to the static
    // tables (never cleared). False when the file is missing / malformed. Mac 0x1fdd80,
    // Win 0x63f24d
    static bool LoadMainGroups(const BrickDatabase& database);
    // Main group shown for a "maingroupid" (merged groups; 201 when main groups are off).
    // (Mac inlined, Win 0x63ccda)
    static UInt32 GetDisplayMainGroup(UInt32 mainGroupID);
    // Position in the order list, else min(order count + group, 999). (Mac inlined, Win 0x63a078)
    static UInt32 GetMainGroupOrder(UInt32 mainGroup);
    // Sort key: [order 3 digits][volume*10 6 digits] (unless main groups are off), design ID
    // (6), then for a one-part brick its materials' palette orders (3 each) and decorations
    // (6 each, as many as the primitive has faces) when `withMaterials`; for other bricks
    // every database part's design ID (6) and, with materials, the first palette part with
    // that design ID. Appended to `key`. Mac 0x1fac70 / 0x1fd5c0
    static void MakeSortKey(const Brick& brick, String& key, const BrickDatabase& database, bool withMaterials);
    static void MakeSortKey(const PrimitiveElementBrick& brick, String& key, const BrickDatabase& database, bool withMaterials);
    // `value` in decimal, zero padded to `width` digits. Mac 0x1fea40
    static void AppendNumber(String& key, UInt32 value, UInt32 width);
    // The material and decoration part of the sort key. Mac 0x1fead0 / 0x1fedd0
    static void AppendPartKey(String& key, const Part& part, const BrickDatabase& database);
    static void AppendPartKey(String& key, const PrimitiveElementBrick& brick, UInt32 part, const BrickDatabase& database);

    // The static tables (Mac 0x77de70 / 0x77de8c / 0x77ae90). (accessors for tests)
    static const BasicMap<UInt32, UInt32>& GetDisplayMainGroupMap() { return s_displayMainGroups; }
    static const BasicArray<UInt32>&       GetMainGroupOrderList()  { return s_mainGroupOrder; }
    // (helper, not in the original) Preference "SORTBYIDONLY": LDDApplication::LoadPalettes
    // (Mac 0x17b2a6) reads it straight into s_mainGroupsOff. (name inferred)
    static void SetMainGroupsOff(bool off) { s_mainGroupsOff = off; }

protected:
    // --- LDDApplicationObserver: switches to the brand's brick set, OnPaletteChanged.
    //     Mac 0x1ff6d0, Win 0x63af0b
    virtual void OnBrandChanged(Brand brand);
    // Detaches the observers, then from the application. Mac 0x1ff590, Win 0x63d8fd
    virtual void OnFinalRelease();

private:
    BrickPalette(const BrickPalette&);
    BrickPalette& operator=(const BrickPalette&);

    // AddBag for one brick set. Mac 0x1fc020
    void AddBag(const Bag& bag, BrickPaletteBuckets set);
    // The set's instance of `brick` (inserted when new; an existing one without a name takes
    // the brick's); notifies OnBrickAdded. Mac 0x1fc440
    const Brick* AddBrick(const Brick* brick, BrickPaletteBuckets set);
    const Brick* FindBrickByKey(const String& key, BrickPaletteBuckets set);

    // ---- data (Mac offsets; Win: notifier +0x0c, m_updating +0xc4, Object +0xdc) ----
    // LDDApplicationObserver +0x00 (subject +0x04), NotifierT +0x08 (observers +0x0c,
    // iterations +0x1c)
    BrickPaletteBuckets m_currentSet;  // +0x2c
    BagMap              m_bags;        // +0x30
    BrickSetMap         m_bricks;      // +0x4c
    ModelMap            m_models;      // +0x68
    GridMap             m_grids;       // +0x84
    bool                m_updating;    // +0xa0
    BasicArray<UInt32>  m_valuesA4;    // +0xa4 (not used in TU 141)
                                       // Object +0xb4

    static BasicMap<UInt32, UInt32> s_displayMainGroups;   // Mac 0x77de70 maingroupid -> shown group
    static BasicArray<UInt32>       s_mainGroupOrder;      // Mac 0x77de8c
    static BasicMap<UInt32, Real32> s_volumes;             // Mac 0x77ae90 design ID -> volume
    static bool                     s_mainGroupsOff;       // Mac 0x77ae8c ("SORTBYIDONLY", set by LDDApplication::LoadPalettes)
};

inline AbstractPart* BrickPalette::Brick::GetPart(UInt32 index) const
{
    return const_cast<Part*>(m_parts[index].Get());
}

inline bool BrickPalette::Brick::IsFlexible() const
{
    for (BasicArray<RefPointer<Part> >::ConstIterator it = m_parts.Begin(); it != m_parts.End(); ++it)
        if ((*it)->IsFlexible())
            return true;
    return false;
}

inline bool BrickPalette::BrickLess::operator()(const RefPointer<const Brick>& a, const RefPointer<const Brick>& b) const
{
    return a->m_sortKey < b->m_sortKey;
}

// ===========================================================================================
// BrickPaletteObserver — Observer<BrickPalette, BrickPaletteObserver>.
// Mac vtable 0x704e1c {D1 0x523c0, D0 0x523f0, OnFinalRelease 0x45010, OnAttached 0x52440,
// OnDetached 0x52450, handlers 0x52460-0x524a0 (empty)}; Win 0x9cf3f8 {OnAttached, OnDetached,
// 0x65a12a ×4 (ret 4), 0x5c8021 (ret)}. Observers: TemplateSelectorControl, BrickPaletteControl …
// Handler names inferred from the notifying BrickPalette code.
// ===========================================================================================
class BrickPaletteObserver : public Observer<BrickPalette, BrickPaletteObserver> {
public:
    virtual ~BrickPaletteObserver() {}                                         // Mac 0x523c0/0x523f0

    // (Mac slot / Win slot)
    virtual void OnBagAdded(const BrickPalette::Bag* /*bag*/) {}               // 5/2 Mac 0x52460 (AddBag)
    virtual void OnModelAdded(StorageScene2* /*model*/) {}                     // 6/3 Mac 0x52470 (AddModel)
    virtual void OnModelRemoved(StorageScene2* /*model*/) {}                   // 7/4 Mac 0x52480 (RemoveModel)
    virtual void OnBrickAdded(const BrickPalette::Brick* /*brick*/) {}         // 8/5 Mac 0x52490 (AddBag)
    virtual void OnPaletteChanged() {}                                         // 9/6 Mac 0x524a0 (EndUpdate, OnBrandChanged)
};

} // namespace LEGO
