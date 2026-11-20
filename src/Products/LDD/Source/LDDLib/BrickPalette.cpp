// LEGO Digital Designer — restored source
// BrickPalette, BrickPalette::Bag / ::Brick, MaterialGrid — see BrickPalette.h.
// Mac TU 141 (0x1f85f0-0x20372e), Win 0x63a5e9-0x641c11 (scattered).
#include "BrickPalette.h"
#include "PrimitiveElementBrick.h"
#include "LDDUtilities.h"
#include "LDDApplication.h"
#include "XMLDOMNode.h"
#include "StringStream.h"
#include "Primitive.h"
#include "Directory.h"

namespace LEGO {

// ===========================================================================================
// statics (Mac static initialiser 0x2035c0)
// ===========================================================================================
BasicMap<UInt32, UInt32> BrickPalette::s_displayMainGroups;   // Mac 0x77de70
BasicArray<UInt32>       BrickPalette::s_mainGroupOrder;      // Mac 0x77de8c
BasicMap<UInt32, Real32> BrickPalette::s_volumes;             // Mac 0x77ae90
bool                     BrickPalette::s_mainGroupsOff = false;   // Mac 0x77ae8c (written by LDDApplication::LoadPalettes)

// ===========================================================================================
// MaterialGrid
// ===========================================================================================

// Mac 0x1f8a00
void MaterialGrid::Clear()
{
    m_solidRows.Clear();
    m_transparentRows.Clear();
    m_specialRows.Clear();
    m_obsoleteRows.Clear();
    m_solid.Clear();
    m_transparent.Clear();
    m_special.Clear();
    m_obsolete.Clear();
}

// Mac 0x1f8bd0
void MaterialGrid::AddMaterial(UInt32 material, const BrickDatabase& database)
{
    MaterialSet* set;
    if (database.IsMaterialObsolete(material)) {
        set = &m_obsolete;
    } else if (database.IsMaterialTransparent(material)) {
        set = &m_transparent;
    } else {
        // NOTE: the original leaves `type` uninitialised when the material is unknown
        // (GetMaterialType's result is ignored); initialised here.
        MaterialManual::MaterialType type = MaterialManual::kShinyPlastic;
        database.GetMaterialType(material, type);
        set = ((UInt32)type | 1) == 1 ? &m_solid : &m_special;
    }
    set->Insert(material);
}

// The rows of one set (one inlined block per set in Mac 0x1f8c50).
static void BuildMaterialRows(MaterialGrid::Rows& rows, const MaterialGrid::MaterialSet& set, UInt32 width)
{
    if (set.IsEmpty())
        return;
    UInt32 column = 0;
    for (MaterialGrid::MaterialSet::ConstIterator it = set.Begin(); it != set.End(); ++it) {
        if (column == 0)
            rows.PushBack(BasicArray<UInt32>());
        rows.Back().PushBack(*it);
        column = (column + 1) % width;
    }
    rows.Back().Resize(width, 0);
}

// Mac 0x1f8c50
void MaterialGrid::BuildRows(UInt32 width)
{
    // NOTE: the original divides by `width` (column = (column + 1) % width, Mac 0x1f8c50), so a
    // palette's <Grid width="0"> is an integer division by zero; fixed: at least one column (M12).
    if (width == 0)
        width = 1;
    BuildMaterialRows(m_solidRows, m_solid, width);
    BuildMaterialRows(m_transparentRows, m_transparent, width);
    BuildMaterialRows(m_specialRows, m_special, width);
    BuildMaterialRows(m_obsoleteRows, m_obsolete, width);
}

// Mac 0x1f85f0, Win 0x63e791
void MaterialGrid::Load(const XMLDOMNode& document, const BrickDatabase& database)
{
    Clear();
    UInt32 width = 8;
    const XMLDOMNode::NodeArray& grids = document.GetChildren();
    for (XMLDOMNode::NodeArray::ConstIterator g = grids.Begin(); g != grids.End(); ++g) {
        if (!((*g)->GetName() == String("Grid")))
            continue;
        String value;
        if ((*g)->GetAttribute(String("width"), value))
            FromString(width, value);
        const XMLDOMNode::NodeArray& materials = (*g)->GetChildren();
        for (XMLDOMNode::NodeArray::ConstIterator m = materials.Begin(); m != materials.End(); ++m) {
            if (!((*m)->GetName() == String("Material")))
                continue;
            UInt32 id = 0;
            String text;
            if ((*m)->GetAttribute(String("id"), text))
                FromString(id, text);
            if (id != 0)
                AddMaterial(id, database);
        }
    }
    BuildRows(width);
}

// Mac 0x1f8870, Win 0x63ed7d
bool MaterialGrid::Load(File* file, const BrickDatabase& database)
{
    RefPointer<XMLDOM> document = new XMLDOM;
    bool ok = document->Parse(file);
    if (ok)
        Load(*document, database);
    return ok;
}

// Mac 0x1f8910
void MaterialGrid::Build(BrickPalette& palette, UInt32 width, const BrickDatabase& database)
{
    Clear();
    for (BrickPalette::BrickSet::ConstIterator b = palette.GetBricksBegin(); b != palette.GetBricksEnd(); ++b) {
        const BasicArray<RefPointer<BrickPalette::Part> >& parts = (*b)->m_parts;
        for (BasicArray<RefPointer<BrickPalette::Part> >::ConstIterator p = parts.Begin(); p != parts.End(); ++p) {
            const BasicArray<UInt32>& materials = (*p)->m_materials;
            for (BasicArray<UInt32>::ConstIterator m = materials.Begin(); m != materials.End(); ++m)
                if (*m != 0)
                    AddMaterial(*m, database);
        }
    }
    BuildRows(width);
}

// ===========================================================================================
// BrickPalette::Brick
// ===========================================================================================

// Mac 0x1fa610
bool BrickPalette::Brick::Initialize(UInt32 designID, const BrickDatabase& database)
{
    const BrickDatabase::Brick* brick = database.GetBrick(designID);
    if (brick)
        Initialize(*brick, database);
    return brick != 0;
}

// Mac 0x1fa650, Win 0x63ee09
bool BrickPalette::Brick::Initialize(const BrickDatabase::Brick& brick, const BrickDatabase& database)
{
    m_designID = brick.m_designID;
    {
        String key("maingroupid");
        BrickDatabase::AnnotationMap::ConstIterator it = brick.m_annotations.Find(key);
        if (it != brick.m_annotations.End())
            FromString(m_mainGroupID, it->second);
    }
    m_mainGroup      = BrickPalette::GetDisplayMainGroup(m_mainGroupID);
    m_mainGroupOrder = BrickPalette::GetMainGroupOrder(m_mainGroup);

    BasicMap<UInt32, Real32>::Iterator cached = s_volumes.Find(m_designID);
    if (cached != s_volumes.End()) {
        m_volume = cached->second;
        return true;
    }
    m_volume = 0;
    for (BasicArray<BrickDatabase::Part>::ConstIterator p = brick.m_parts.Begin(); p != brick.m_parts.End(); ++p) {
        // NOTE: no check for a missing primitive (both builds)
        const Primitive* primitive = database.GetPrimitive(p->m_designID);
        const BasicArray<Primitive::Bone>& bones = primitive->GetBones();
        for (BasicArray<Primitive::Bone>::ConstIterator bone = bones.Begin(); bone != bones.End(); ++bone)
            m_volume = bone->m_bounding.GetVolume() + m_volume;
    }
    s_volumes[m_designID] = m_volume;
    return true;
}

// ===========================================================================================
// BrickPalette::Bag
// ===========================================================================================

// Mac 0x1fb2e0
BrickPalette::Bag::Bag()
    : m_brand(0xffffffff), m_countable(false), m_brandFilter(false)
{
}

// Mac 0x1fb630
BrickPalette::Bag::Bag(const Bag& other)
    : Object(),
      m_name(other.m_name), m_text(other.m_text), m_brand(other.m_brand), m_version(other.m_version),
      m_models(other.m_models), m_countable(other.m_countable), m_brandFilter(other.m_brandFilter)
{
    m_path.Assign(other.m_path);
}

// Mac 0x1fbbc0 (body 0x1fb990), Win 0x63fd60
BrickPalette::Bag::~Bag()
{
}

// Mac 0x1fae80
bool BrickPalette::Bag::Contains(const Brick* brick) const
{
    return m_bricks.Find(RefPointer<const Brick>(brick)) != m_bricks.End();
}

// Mac 0x1faf90
UInt32 BrickPalette::Bag::GetCount(const Brick* brick) const
{
    BrickCountMap::ConstIterator it = m_bricks.Find(RefPointer<const Brick>(brick));
    return it == m_bricks.End() ? 0 : it->second;
}

// Mac 0x1fb100
void BrickPalette::Bag::AddBrick(Brick* brick, UInt32 count, const BrickDatabase& database)
{
    bool extended = IsBrandLDDExtended((Brand)m_brand);
    MakeSortKey(*brick, brick->m_sortKey, database, !extended);
    AddBrickCount(brick, count);
}

// Mac 0x1fb150, Win 0x63ef20
void BrickPalette::Bag::AddBrickCount(const Brick* brick, UInt32 count)
{
    BrickCountMap::Iterator it = m_bricks.Find(RefPointer<const Brick>(brick));
    if (it == m_bricks.End())
        m_bricks[RefPointer<const Brick>(brick)] = count;
    else
        it->second += count;
}

// ===========================================================================================
// BrickPalette
// ===========================================================================================

// Mac 0x1fbcd0
BrickPalette::BrickPalette()
    : m_currentSet(kBrickSetLDD), m_updating(false)
{
    Attach(LDDApplication::GetInstance());
    m_currentSet = (BrickPaletteBuckets)LDDApplication::GetInstance()->GetBrandBrickSet();   // Mac 0x178640
}

// Mac 0x1ff590, Win 0x63d8fd
void BrickPalette::OnFinalRelease()
{
    NotifierT<BrickPalette, BrickPaletteObserver>::OnFinalRelease();
    LDDApplicationObserver::OnFinalRelease();
}

// Mac 0x1ff6d0, Win 0x63af0b
void BrickPalette::OnBrandChanged(Brand brand)
{
    m_currentSet = (BrickPaletteBuckets)GetBrandBrickSet(brand);
    NotifierT<BrickPalette, BrickPaletteObserver>::Iterator it(*this);
    while (BrickPaletteObserver* observer = it.Next(m_observers))
        observer->OnPaletteChanged();
}

// Mac 0x1fbf70
void BrickPalette::AddBag(const Bag& bag)
{
    BasicArray<UInt32> sets;
    GetBrandBrickSets((Brand)bag.m_brand, sets);
    for (BasicArray<UInt32>::ConstIterator it = sets.Begin(); it != sets.End(); ++it)
        AddBag(bag, (BrickPaletteBuckets)*it);
}

// Mac 0x1fc020
void BrickPalette::AddBag(const Bag& bag, BrickPaletteBuckets set)
{
    RefPointer<Bag> copy = new Bag(bag);
    for (Bag::BrickCountMap::ConstIterator it = bag.m_bricks.Begin(); it != bag.m_bricks.End(); ++it)
        copy->AddBrickCount(AddBrick(it->first, set), it->second);
    m_bags[set].PushBack(RefPointer<const Bag>(copy.Get()));
    if (bag.m_brandFilter && bag.m_grid.HasRows())
        m_grids[set] = bag.m_grid;
    if (m_updating || set != m_currentSet)
        return;
    NotifierT<BrickPalette, BrickPaletteObserver>::Iterator it(*this);
    while (BrickPaletteObserver* observer = it.Next(m_observers))
        observer->OnBagAdded(copy);
}

// Mac 0x1fc440
const BrickPalette::Brick* BrickPalette::AddBrick(const Brick* brick, BrickPaletteBuckets set)
{
    BrickSet& bricks = m_bricks[set];
    BrickSet::Iterator found = bricks.Find(RefPointer<const Brick>(brick));
    const Brick* result;
    if (found == m_bricks[set].End()) {
        m_bricks[set].Insert(RefPointer<const Brick>(brick));
        result = brick;
    } else {
        result = *found;
        // the palette's instance takes the first name it is given
        if (result->m_name.IsEmpty())
            const_cast<Brick*>(result)->m_name = brick->m_name;
    }
    if (m_updating || set != m_currentSet)
        return result;
    NotifierT<BrickPalette, BrickPaletteObserver>::Iterator it(*this);
    while (BrickPaletteObserver* observer = it.Next(m_observers))
        observer->OnBrickAdded(result);
    return result;
}

// Mac 0x1fcb90, Win 0x63f188
void BrickPalette::AddModel(StorageScene2* model, const Path& path)
{
    Brand brand;
    if (!LDDUtilities::GetInstance()->GetModelBrand(model, brand))
        return;
    m_models[RefPointer<const StorageScene2>(model)].Assign(path);
    if (m_updating)
        return;
    if (brand != LDDApplication::GetInstance()->GetBrand())
        return;
    NotifierT<BrickPalette, BrickPaletteObserver>::Iterator it(*this);
    while (BrickPaletteObserver* observer = it.Next(m_observers))
        observer->OnModelAdded(model);
}

// Mac 0x1fceb0
void BrickPalette::RemoveModel(StorageScene2* model)
{
    ModelMap::Iterator found = m_models.Find(RefPointer<const StorageScene2>(model));
    if (found == m_models.End())
        return;
    found->second.Delete(false);
    m_models.Erase(found);
    // NOTE: no m_updating check here (both builds)
    NotifierT<BrickPalette, BrickPaletteObserver>::Iterator it(*this);
    while (BrickPaletteObserver* observer = it.Next(m_observers))
        observer->OnModelRemoved(model);
}

// The lookup of Mac 0x1fd1e0 / 0x1fd3e0: a temporary brick carries the key.
const BrickPalette::Brick* BrickPalette::FindBrickByKey(const String& key, BrickPaletteBuckets set)
{
    RefPointer<Brick> probe = new Brick;
    probe->m_sortKey = key;
    BrickSet::Iterator found = m_bricks[set].Find(RefPointer<const Brick>(probe.Get()));
    return found == m_bricks[set].End() ? 0 : found->Get();
}

// Mac 0x1fd1e0
const BrickPalette::Brick* BrickPalette::FindBrick(const Brick& brick, const BrickDatabase& database, BrickPaletteBuckets set)
{
    String key;
    // NOTE: IsBrandMindstorms of a brick set (= LDD Extended's set number 1 is Mindstorms'
    // brand number) — both builds.
    MakeSortKey(brick, key, database, !IsBrandMindstorms((Brand)set));
    return FindBrickByKey(key, set);
}

// Mac 0x1fd3c0
const BrickPalette::Brick* BrickPalette::FindBrick(const Brick& brick, const BrickDatabase& database)
{
    return FindBrick(brick, database, m_currentSet);
}

// Mac 0x1fd3e0
const BrickPalette::Brick* BrickPalette::FindBrick(const PrimitiveElementBrick& brick, const BrickDatabase& database,
                                                   BrickPaletteBuckets set)
{
    String key;
    MakeSortKey(brick, key, database, !IsBrandMindstorms((Brand)set));
    return FindBrickByKey(key, set);
}

// Mac 0x1fd7f0
const BrickPalette::Brick* BrickPalette::FindBrick(const PrimitiveElementBrick& brick, const BrickDatabase& database)
{
    return FindBrick(brick, database, m_currentSet);
}

// Mac 0x1fd810
const MaterialGrid* BrickPalette::GetMaterialGrid(BrickPaletteBuckets set, const BrickDatabase& database)
{
    GridMap::Iterator found = m_grids.Find(set);
    if (found != m_grids.End())
        return &found->second;
    MaterialGrid grid;
    // NOTE: built from the CURRENT set's bricks, stored for `set` (both builds)
    grid.Build(*this, 8, database);
    m_grids[set] = grid;
    return &m_grids[set];
}

// Mac 0x1fda30
const MaterialGrid* BrickPalette::GetMaterialGrid(const BrickDatabase& database)
{
    return GetMaterialGrid(m_currentSet, database);
}

// Mac 0x1fda60
void BrickPalette::EndUpdate()
{
    m_updating = false;
    NotifierT<BrickPalette, BrickPaletteObserver>::Iterator it(*this);
    while (BrickPaletteObserver* observer = it.Next(m_observers))
        observer->OnPaletteChanged();
}

BrickPalette::BagArray::ConstIterator BrickPalette::GetBagsBegin() { return m_bags[m_currentSet].Begin(); }       // Mac 0x1fdcd0
BrickPalette::BagArray::ConstIterator BrickPalette::GetBagsEnd()   { return m_bags[m_currentSet].End(); }         // Mac 0x1fdd00
UInt32 BrickPalette::GetBagCount()                                 { return m_bags[m_currentSet].GetSize(); }     // Mac 0x1fdd30
BrickPalette::BrickSet::ConstIterator BrickPalette::GetBricksBegin() { return m_bricks[m_currentSet].Begin(); }   // Mac 0x1f8b70
BrickPalette::BrickSet::ConstIterator BrickPalette::GetBricksEnd()   { return m_bricks[m_currentSet].End(); }     // Mac 0x1f8ba0

// ---- main groups -----------------------------------------------------------------------------

// (Mac inlined in 0x1fa650 / 0x1fac70 / 0x1fd5c0, Win 0x63ccda)
UInt32 BrickPalette::GetDisplayMainGroup(UInt32 mainGroupID)
{
    if (s_mainGroupsOff)
        return 201;
    BasicMap<UInt32, UInt32>::ConstIterator it = s_displayMainGroups.Find(mainGroupID);
    return it != s_displayMainGroups.End() ? it->second : mainGroupID;
}

// (Mac inlined, Win 0x63a078)
UInt32 BrickPalette::GetMainGroupOrder(UInt32 mainGroup)
{
    UInt32 count = s_mainGroupOrder.GetSize();
    for (UInt32 i = 0; i < count; ++i)
        if (s_mainGroupOrder[i] == mainGroup)
            return i;
    return count + mainGroup < 999 ? count + mainGroup : 999;
}

// Mac 0x1fdd80, Win 0x63f24d
bool BrickPalette::LoadMainGroups(const BrickDatabase& database)
{
    Directory* directory = database.GetDirectory();
    if (!directory)
        return false;
    Path path;
    path.SetElement(directory);
    if (!path.Append(String("MainGroupDividers/MainGroups.xml")))
        return false;

    RefPointer<XMLDOM> document = new XMLDOM;
    if (!document->Parse(path))
        return false;

    RefPointer<XMLDOMNode> node;
    const XMLDOMNode::NodeArray& children = document->GetChildren();
    for (XMLDOMNode::NodeArray::ConstIterator c = children.Begin(); c != children.End(); ++c) {
        node = *c;
        if (!(node->GetName() == String("Merge")))
            continue;
        String merge;
        // NOTE: `value` stays uninitialised when the attribute is missing (both builds);
        // initialised to 0 here, which rejects the file like a 0 value does.
        UInt32 value = 0;
        bool ok;
        {
            String text;
            ok = node->GetAttribute(String("value"), text) && FromString(value, text);
        }
        bool empty = true;
        if (value != 0 && ok)
            empty = !node->GetAttribute(String("merge"), merge) || merge.IsEmpty();
        if (empty)
            return false;
        UInt32 position = 0;
        UInt32 id;
        while (position < merge.GetLength() && FromString(id, merge, position)) {
            if (id == 0)
                continue;
            if (s_displayMainGroups.Contains(id)) {
                // NOTE: the message is built and dropped (both builds)
                String message("DisplayMainGroupMap maps ");
                message << id << " twice.";
            } else {
                s_displayMainGroups[id] = value;
            }
        }
        s_displayMainGroups[value] = value;
    }

    for (BasicMap<UInt32, UInt32>::ConstIterator it = s_displayMainGroups.Begin(); it != s_displayMainGroups.End(); ++it) {
        // NOTE: the original reads end()->second when `value` is no key (garbage compare);
        // the result is only a dropped message either way.
        BasicMap<UInt32, UInt32>::ConstIterator target = s_displayMainGroups.Find(it->second);
        if (target == s_displayMainGroups.End() || target->second != it->second) {
            String message("DisplayMainGroupMap value ");
            message << it->second << " does not map to itself.";
        }
    }

    String order;
    node = document->FindChild(String("OrderList"));
    if (node.IsNull())
        return false;
    if (!node->GetAttribute(String("order"), order) || order.IsEmpty())
        return false;
    UInt32 position = 0;
    UInt32 id;
    while (position < order.GetLength() && FromString(id, order, position)) {
        if (id == 0)
            continue;
        bool known = false;
        for (BasicArray<UInt32>::ConstIterator it = s_mainGroupOrder.Begin(); it != s_mainGroupOrder.End(); ++it)
            if (*it == id) {
                known = true;
                break;
            }
        if (known) {
            String message("MainGroupOrderList contains ");
            message << id << " twice.";
            continue;
        }
        s_mainGroupOrder.PushBack(id);
    }
    return true;
}

// ---- sort keys -------------------------------------------------------------------------------

// Mac 0x1fea40
void BrickPalette::AppendNumber(String& key, UInt32 value, UInt32 width)
{
    String digits;
    digits << value;
    // NOTE: no check for more digits than `width` (count wraps; both builds)
    key.Append(width - digits.GetLength(), (UniChar16)'0');
    key.Append(digits);
}

// The decorations, padded / cut to the primitive's face count (shared tail of Mac 0x1fead0 /
// 0x1fedd0).
static void AppendDecorationKey(String& key, const BasicArray<UInt32>& decorations, const Primitive* primitive)
{
    BasicArray<UInt32> faces(decorations);
    // NOTE: no check for a missing primitive (both builds)
    faces.Resize(primitive->GetDecorationFaceCount(), 0);
    for (BasicArray<UInt32>::ConstIterator it = faces.Begin(); it != faces.End(); ++it)
        BrickPalette::AppendNumber(key, *it, 6);
}

// Mac 0x1fead0
void BrickPalette::AppendPartKey(String& key, const Part& part, const BrickDatabase& database)
{
    for (UInt32 i = 0; i < part.m_materials.GetSize(); ++i) {
        UInt32 material = part.m_materials[i] != 0 ? part.m_materials[i] : part.m_materials[0];
        AppendNumber(key, MaterialManual::GetMaterialOrder(material), 3);
    }
    AppendDecorationKey(key, part.m_decorations, database.GetPrimitive(part.m_designID));
}

// Mac 0x1fedd0
void BrickPalette::AppendPartKey(String& key, const PrimitiveElementBrick& brick, UInt32 part, const BrickDatabase& database)
{
    const BasicArray<UInt32>& materials = brick.GetMaterials(part);
    for (UInt32 i = 0; i < materials.GetSize(); ++i) {
        UInt32 material = materials[i] != 0 ? materials[i] : materials[0];
        AppendNumber(key, MaterialManual::GetMaterialOrder(material), 3);
    }
    const Primitive* primitive = database.GetPrimitive(brick.GetPalettePart(part)->GetDesignID());
    AppendDecorationKey(key, brick.GetDecorations(part), primitive);
}

// The main group / volume / design ID prefix (start of Mac 0x1fac70 / 0x1fd5c0).
static void AppendBrickKey(String& key, const BrickPalette::Brick& brick, bool mainGroupsOff)
{
    UInt32 order = BrickPalette::GetMainGroupOrder(BrickPalette::GetDisplayMainGroup(brick.m_mainGroupID));
    if (!mainGroupsOff) {
        BrickPalette::AppendNumber(key, order, 3);
#if defined(_WIN32)
        // Win 0x63cd43: x87 product with the double 10.0 (exact), truncated (_ftol2)
        BrickPalette::AppendNumber(key, (UInt32)(Int64)((Real64)brick.m_volume * 10.0), 6);
#else
        // Mac 0x1fad43: float product (mulss), truncated (fisttp)
        Real32 volume = brick.m_volume * 10.0f;
        BrickPalette::AppendNumber(key, (UInt32)(Int64)volume, 6);
#endif
    }
    BrickPalette::AppendNumber(key, brick.m_designID, 6);
}

// Mac 0x1fac70
void BrickPalette::MakeSortKey(const Brick& brick, String& key, const BrickDatabase& database, bool withMaterials)
{
    AppendBrickKey(key, brick, s_mainGroupsOff);
    if (brick.m_parts.GetSize() == 1) {
        if (withMaterials)
            AppendPartKey(key, *brick.m_parts[0], database);
        return;
    }
    // NOTE: no check for a design ID missing from the database (both builds)
    const BrickDatabase::Brick* entry = database.GetBrick(brick.m_designID);
    for (BasicArray<BrickDatabase::Part>::ConstIterator p = entry->m_parts.Begin(); p != entry->m_parts.End(); ++p) {
        AppendNumber(key, p->m_designID, 6);
        if (!withMaterials)
            continue;
        for (BasicArray<RefPointer<Part> >::ConstIterator q = brick.m_parts.Begin(); q != brick.m_parts.End(); ++q)
            if ((*q)->m_designID == p->m_designID) {
                AppendPartKey(key, **q, database);
                break;
            }
    }
}

// Mac 0x1fd5c0
void BrickPalette::MakeSortKey(const PrimitiveElementBrick& brick, String& key, const BrickDatabase& database,
                               bool withMaterials)
{
    const Brick& palette = *brick.GetBrick();
    AppendBrickKey(key, palette, s_mainGroupsOff);
    if (palette.m_parts.GetSize() == 1) {
        if (withMaterials)
            AppendPartKey(key, brick, 0, database);
        return;
    }
    const BrickDatabase::Brick* entry = database.GetBrick(palette.m_designID);
    for (BasicArray<BrickDatabase::Part>::ConstIterator p = entry->m_parts.Begin(); p != entry->m_parts.End(); ++p) {
        AppendNumber(key, p->m_designID, 6);
        if (!withMaterials)
            continue;
        for (UInt32 i = 0; i < palette.m_parts.GetSize(); ++i)
            if (brick.GetPalettePart(i)->m_designID == p->m_designID) {
                AppendPartKey(key, brick, i, database);
                break;
            }
    }
}

} // namespace LEGO
