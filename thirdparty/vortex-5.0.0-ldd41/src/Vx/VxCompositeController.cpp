// Vortex 5.0.0 build 800 "vortexldd41" - VxCompositeController (restored).
// Mac SUB-TU 442e (0x4083ac-0x40aa90; the first Vortex file of the Mac link order),
// Win 0x78b450-0x78d230. Child / template management survives only in the Win build.
#include "Vx/VxCompositeController.h"
#include "Vx/VxControllerGraph.h"
#include "Vx/VxUniqueNameList.h"
#include "Vx/VxMessage.h"

namespace Vx {

namespace {

VxConnection::FieldInfo makeFieldInfo(VxData::FieldBase* field, int kind)
{
    VxConnection::FieldInfo r;
    r.field = field;
    r.kind = kind;
    return r;
}

template <class T>
T* find(VxArray<T>& a, const T& x)
{
    T* i = a.begin();
    for (; i != a.end(); ++i)
        if (*i == x)
            break;
    return i;
}

template <class T>
void erase(VxArray<T>& a, T* i)
{
    for (T* j = i + 1; j != a.end(); ++j)
        *(j - 1) = *j;
    a.pop_back();
}

} // namespace

// Mac C2 0x408510 / C1 0x4085d2, Win 0x78b5a0
VxCompositeController::VxCompositeController()
    : VxSteppingController(), mControllers(), mSteppingControllers(), mConnections(), mTemplates(),
      mEnabledControllers()
{
}

// Mac D1 0x409066 (body 0x408de2) / D0 0x40907a, Win 0x78d230 (body 0x78b8e0).
// The composite owns its connections and its child controllers.
VxCompositeController::~VxCompositeController()
{
    mEnabledControllers.mEnd = mEnabledControllers.mBegin; // (no element destructors)
    while (mConnections.begin() != mConnections.end())
        deleteConnection(*(mConnections.end() - 1));
    VxCompositeController::setUniverse(0);
    for (VxControllerBase** i = mControllers.begin(); i != mControllers.end(); ++i)
        if (*i != 0)
            delete *i;
}

// Mac 0x40a41c, Win 0x78ce60
const char* VxCompositeController::getClassName() const
{
    return "VxCompositeController";
}

// Mac 0x4083ac, Win 0x78b450
void VxCompositeController::update()
{
}

// Mac 0x4083e3, Win 0x78b680
void VxCompositeController::setChildEnabled(VxControllerBase* child, bool enabled)
{
    child->mEnable.setValue(enabled);
    child->mInputsChanged = true;
}

// Mac 0x408758, Win 0x78b810: switch on the stepping-enabled children
void VxCompositeController::onActivate()
{
    VxControllerBase** e = mEnabledControllers.end();
    for (VxControllerBase** i = mEnabledControllers.begin(); i != e; ++i)
        setChildEnabled(*i, true);
}

// Mac 0x408700, Win 0x78b840: switch off every child
void VxCompositeController::onDeactivate()
{
    VxControllerBase** e = mControllers.end();
    for (VxControllerBase** i = mControllers.begin(); i != e; ++i)
        setChildEnabled(*i, false);
}

// Mac 0x4087b0, Win 0x78b870
void VxCompositeController::onReset()
{
    VxControllerBase** e = mControllers.end();
    for (VxControllerBase** i = mControllers.begin(); i != e; ++i)
        setChildEnabled(*i, false);
    e = mControllers.end();
    for (VxControllerBase** i = mControllers.begin(); i != e; ++i)
        (*i)->reset();
    if (mState == kStateActive && sControllersEnabled)
        onActivate();
}

// Mac 0x408aa6 / 0x40898c / 0x4085e6, Win 0x78ba80 / 0x78bac0 / 0x78bb00:
// the active stepping children, in order
void VxCompositeController::preStep()
{
    VxSteppingController** e = mSteppingControllers.end();
    for (VxSteppingController** i = mSteppingControllers.begin(); i != e; ++i)
        if ((*i)->isActive())
            (*i)->preStep();
}

void VxCompositeController::step()
{
    VxSteppingController** e = mSteppingControllers.end();
    for (VxSteppingController** i = mSteppingControllers.begin(); i != e; ++i)
        if ((*i)->isActive())
            (*i)->step();
}

void VxCompositeController::postStep()
{
    VxSteppingController** e = mSteppingControllers.end();
    for (VxSteppingController** i = mSteppingControllers.begin(); i != e; ++i)
        if ((*i)->isActive())
            (*i)->postStep();
}

// Mac 0x40845c, Win 0x78b7c0
void VxCompositeController::setUniverse(VxUniverse* u)
{
    VxSteppingController::setUniverse(u);
    VxSteppingController** e = mSteppingControllers.end();
    for (VxSteppingController** i = mSteppingControllers.begin(); i != e; ++i)
        (*i)->setUniverse(u);
    VxControllerGraph::invalidateInstance();
}

// Mac 0x408436, Win 0x78b4b0
void VxCompositeController::onDoneChanged(bool done)
{
    if (done) {
        setDone();
        onDeactivate();
    }
}

// Mac 0x4083b2, Win 0x78b4f0
VxControllerBase* VxCompositeController::getController(int i) const
{
    if (i < 0 || i >= mControllers.size())
        return 0;
    return mControllers.begin()[i];
}

// Win 0x78bc10
void VxCompositeController::addController(VxControllerBase* c)
{
    if (find(mControllers, c) == mControllers.end()) {
        mControllers.push_back(c);
        c->setParent(this);
    }
    VxSteppingController* s = dynamic_cast<VxSteppingController*>(c);
    if (s != 0 && find(mSteppingControllers, s) == mSteppingControllers.end()) {
        mSteppingControllers.push_back(s);
        s->setUniverse(mUniverse);
    }
    VxControllerGraph::invalidateInstance();
    updateSteppingSet(c);
}

// Win 0x78b6a0
void VxCompositeController::removeController(VxControllerBase* c)
{
    VxControllerBase** i = find(mControllers, c);
    if (i != mControllers.end()) {
        erase(mControllers, i);
        c->setParent(0);
    }
    VxSteppingController* s = dynamic_cast<VxSteppingController*>(c);
    if (s != 0) {
        s->setUniverse(0);
        VxSteppingController** j = find(mSteppingControllers, s);
        if (j != mSteppingControllers.end())
            erase(mSteppingControllers, j);
    }
    VxControllerGraph::invalidateInstance();
    VxControllerBase** k = find(mEnabledControllers, c);
    if (k != mEnabledControllers.end())
        erase(mEnabledControllers, k);
}

// Mac 0x40980e, Win 0x78bbb0
void VxCompositeController::updateSteppingSet(VxControllerBase* child)
{
    if (child == 0)
        return;
    if (!child->mSteppingEnabled) {
        VxControllerBase** i = find(mEnabledControllers, child);
        if (i != mEnabledControllers.end())
            erase(mEnabledControllers, i);
    } else {
        if (find(mEnabledControllers, child) == mEnabledControllers.end())
            mEnabledControllers.push_back(child);
        child->mEnable.setValue(mEnable.mValue);
    }
}

// Win 0x78b520
VxConnection* VxCompositeController::getConnection(int i) const
{
    if (i < 0 || i >= mConnections.size())
        return 0;
    return mConnections.begin()[i];
}

// Mac 0x408bc0, Win 0x78bb40
bool VxCompositeController::addConnection(VxConnection* c)
{
    if (find(mConnections, c) != mConnections.end())
        return false;
    c->setContainer(this);
    mConnections.push_back(c);
    VxControllerGraph::invalidateInstance();
    return true;
}

// Mac 0x40888e, Win 0x78b610
bool VxCompositeController::removeConnection(VxConnection* c)
{
    VxConnection** i = find(mConnections, c);
    if (i == mConnections.end())
        return false;
    c->setContainer(0);
    erase(mConnections, i);
    VxControllerGraph::invalidateInstance();
    return true;
}

// Mac 0x40893c, Win 0x78b780
void VxCompositeController::deleteConnection(VxConnection* c)
{
    removeConnection(c);
    if (c != 0)
        delete c;
    VxControllerGraph::invalidateInstance();
}

// Mac 0x408d3e, Win 0x78bd20 (plain operator new(0x1c))
VxConnection* VxCompositeController::createConnection(const FieldInfo& from, const FieldInfo& to, int type)
{
    InternalConnection* c = new InternalConnection();
    if (type == 2)
        c->mNotDependency = true;
    c->connect(from, to);
    c->setType(type);
    addConnection(c);
    return c;
}

// The body of Mac 0x4099c0 / 0x409ec2 / 0x409304 (Win 0x78bda0 / 0x78c060 / 0x78c310).
// `exposedKind` is 1 (input), 2 (output) or 3 (parameter): the connection runs from the new
// field to the original one, except for outputs.
void VxCompositeController::expose(VxData::Container& container, const VxID& name, VxData::FieldBase* field,
                                   int kind, bool makeUnique, int exposedKind)
{
    VxID id(name);
    VxData::FieldBase* exposed;
    bool found = container.findField(name) != container.mFields.end();
    if (found && !makeUnique) {
        // reuse the existing field of that name
        VxData::Container::Fields::iterator i = container.findField(id);
        exposed = i == container.mFields.end() ? &VxData::FieldBase::getInvalidField() : *i;
    } else {
        if (makeUnique) {
            VxUniqueNameList names;
            VxArray<VxData::FieldBase*> fields(container.getFields());
            for (VxData::FieldBase** i = fields.begin(); i != fields.end(); ++i)
                names.add(std::string((*i)->getName().c_str()));
            id = VxID(names.addUnique(std::string(id.c_str())));
        }
        exposed = field->clone(&mFields);
        exposed->setName(id);
    }
    if (exposed != 0) {
        if (exposedKind == kFieldOutput)
            createConnection(makeFieldInfo(field, kind), makeFieldInfo(exposed, kFieldOutput), kFieldOutput);
        else
            createConnection(makeFieldInfo(exposed, exposedKind), makeFieldInfo(field, kind), exposedKind);
    }
}

// Mac 0x4099c0, Win 0x78bda0
void VxCompositeController::exposeInput(const VxID& name, VxData::FieldBase* field, int kind, bool makeUnique)
{
    expose(mFields.mInputs, name, field, kind, makeUnique, kFieldInput);
}

// Mac 0x409ec2, Win 0x78c060
void VxCompositeController::exposeOutput(const VxID& name, VxData::FieldBase* field, int kind, bool makeUnique)
{
    expose(mFields.mOutputs, name, field, kind, makeUnique, kFieldOutput);
}

// Mac 0x409304, Win 0x78c310
void VxCompositeController::exposeParameter(const VxID& name, VxData::FieldBase* field, int kind, bool makeUnique)
{
    expose(mFields.mParameters, name, field, kind, makeUnique, kFieldParameter);
}

// Win 0x78b550
VxControllerTemplate* VxCompositeController::getTemplate(int i)
{
    if (i < 0 || i >= mTemplates.size())
        return 0;
    return mTemplates.begin() + i;
}

namespace {
VxControllerTemplate* findTemplate(VxArray<VxControllerTemplate>& a, const VxID& name)
{
    VxControllerTemplate* i = a.begin();
    for (; i != a.end(); ++i)
        if (i->getName() == name)
            break;
    return i;
}
} // namespace

// Win 0x78c8a0
void VxCompositeController::addTemplate(const VxControllerTemplate& t)
{
    if (findTemplate(mTemplates, t.getName()) == mTemplates.end()) {
        mTemplates.push_back(t);
        return;
    }
    VxWarning(0, "VxCompositeController::addTemplate: Template with name %s already exists. \n", t.getName().c_str());
}

// Win 0x78c980
void VxCompositeController::removeTemplate(const VxControllerTemplate& t)
{
    VxControllerTemplate* i = findTemplate(mTemplates, t.getName());
    if (i != mTemplates.end()) {
        for (VxControllerTemplate* j = i + 1; j != mTemplates.end(); ++j)
            *(j - 1) = *j;
        mTemplates.pop_back();
        return;
    }
    VxWarning(0, "VxCompositeController::removeTemplate: Template with name %s doesn't exist. \n", t.getName().c_str());
}

} // namespace Vx
