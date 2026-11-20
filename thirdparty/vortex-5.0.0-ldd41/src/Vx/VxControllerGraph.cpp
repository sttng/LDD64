// Vortex 5.0.0 build 800 "vortexldd41" - VxControllerGraph, VxControllerGraphVisitor
// (restored). Mac TU 447 (0x41d6c0-0x42051f), Win 0x7fb450-0x7fd570.
#include "Vx/VxControllerGraph.h"
#include "Vx/VxCompositeController.h"
#include "Vx/VxAssemblyBase.h"
#include "Vx/VxFrame.h"
#include "Vx/VxConnection.h"

namespace Vx {

// Mac 0x779194, Win 0xafa24c
VxControllerGraph* VxControllerGraph::sInstance = 0;

// --------------------------------------------------------------------------------------
// VxControllerGraphVisitor

// Mac D1 0x41d8e0 (body 0x41d8b6) / D0 0x41d87e, Win 0x7fc340
VxControllerGraphVisitor::~VxControllerGraphVisitor()
{
}

// Mac 0x41e81e, Win 0x7fb8c0: every assembly contributes its embedded composite
int VxControllerGraphVisitor::visitAssembly(VxAssemblyBase* a)
{
    if (mGraph != 0)
        addComposite(&a->mController);
    return 0;
}

// Mac 0x41e4ea, Win 0x7fb810
void VxControllerGraphVisitor::addComposite(VxCompositeController* c)
{
    mGraph->mControllers.push_back(c);
    for (VxConnection** i = c->mConnections.begin(); i != c->mConnections.end(); ++i)
        mGraph->mConnections.push_back(*i);
    for (VxControllerBase** i = c->mControllers.begin(); i != c->mControllers.end(); ++i) {
        VxControllerBase* child = *i;
        VxCompositeController* cc = child != 0 ? dynamic_cast<VxCompositeController*>(child) : 0;
        if (cc != 0)
            addComposite(cc);
        else
            mGraph->mControllers.push_back(child);
    }
}

// --------------------------------------------------------------------------------------
// VxControllerGraph

// Mac 0x41d86a / 0x41d853
void* VxControllerGraph::operator new(size_t size)
{
    return vxAlloc(size);
}

void VxControllerGraph::operator delete(void* p)
{
    if (p != 0)
        vxFree(p);
}

// Mac 0x41d6c0 (0x181), 0x41d6ca (isOfClassType), 0x41dade (types), 0x41de66 (isOfType);
// Win 0x7fb5c0, 0x7fb6c0
VX_IMPLEMENT_CLASS_TYPES(VxControllerGraph, VxStepperBase, 0x181)

// Mac C2 0x41d948 / C1 0x41da70 (the time step stays VxStepperBase's (double)0.02f; the five
// stepper parameters are 3)
VxControllerGraph::VxControllerGraph()
    : VxStepperBase(), mValid(false), mRegistered(false), mControllers(), mConnections(), mPredecessors(),
      mSuccessors(), mPredecessorCount(), mRoots(), mListener(0)
{
    mStepperParam0 = 3;
    mStepperParam1 = 3;
    mStepperParam2 = 3;
    mStepperParam3 = 3;
    mStepperParam4 = 3;
}

// Mac D1 0x41e1be (body 0x41dff8) / D0 0x41e1d2, Win 0x7fd570
VxControllerGraph::~VxControllerGraph()
{
    sInstance = 0;
}

// Mac 0x41faf6, Win 0x7fd550
const char* VxControllerGraph::getClassName() const
{
    return "VxControllerGraph";
}

// Mac 0x41fb08 / 0x41fb0e / 0x41fb14 / 0x41fb1a, Win 0x7076a0
void VxControllerGraph::stepPhase1()
{
}

void VxControllerGraph::stepDynamics()
{
}

void VxControllerGraph::stepPhase3()
{
}

void VxControllerGraph::endStep()
{
}

// Mac 0x41fb20 / 0x41fb28, Win 0x7fd560
VxReal VxControllerGraph::getTime1() const
{
    return 0;
}

VxReal VxControllerGraph::getTime2() const
{
    return 0;
}

// Mac 0x41da84, Win 0x7fc160
VxControllerGraph* VxControllerGraph::instance()
{
    if (sInstance == 0)
        sInstance = new VxControllerGraph();
    return sInstance;
}

// Mac 0x41d8f4, Win 0x7fb450. NOTE: like the original it registers the singleton, whatever
// `this` is (it is only ever called on the singleton).
void VxControllerGraph::registerWithFrame()
{
    VxControllerGraph* g = sInstance;
    if (!VxFrame::instance()->hasStepper(g)) {
        VxFrame::instance()->addStepper(sInstance);
        mRegistered = true;
    }
}

// Mac 0x41deac
VxArray<VxConnection*> VxControllerGraph::getConnectionsFrom(VxControllerBase* c) const
{
    VxArray<VxConnection*> r;
    for (VxConnection* const* i = mConnections.begin(); i != mConnections.end(); ++i)
        if ((*i)->mInputController == c)
            r.push_back(*i);
    return r;
}

// Mac 0x41e39e
VxArray<VxConnection*> VxControllerGraph::getConnectionsTo(VxControllerBase* c) const
{
    VxArray<VxConnection*> r;
    for (VxConnection* const* i = mConnections.begin(); i != mConnections.end(); ++i)
        if ((*i)->mOutputController == c)
            r.push_back(*i);
    return r;
}

// Mac 0x41e846, Win 0x7fb9b0
void VxControllerGraph::rebuild()
{
    mRoots.clear();
    mPredecessors.clear();
    mSuccessors.clear();
    mControllers.clear();
    mConnections.clear();

    VxControllerGraphVisitor visitor(this);
    VxFrame::instance()->accept(&visitor);

    int n = mControllers.size();
    if (n != 0) {
        // notifying the inputs runs the connections, which flag their destinations; the flags
        // are put back afterwards
        VxArray<bool> changed(n);
        for (int i = 0; i < n; ++i)
            changed.begin()[i] = mControllers.begin()[i]->getInputsChanged();
        VxControllerBase** e = mControllers.end();
        for (VxControllerBase** i = mControllers.begin(); i != e; ++i)
            (*i)->notifyInputs();
        for (int i = 0; i < mControllers.size(); ++i)
            mControllers.begin()[i]->mInputsChanged = changed.begin()[i];

        for (VxControllerBase** ci = mControllers.begin(); ci != mControllers.end(); ++ci) {
            VxControllerBase* c = *ci;
            VxArray<VxConnection*> in = getConnectionsTo(c);
            VxArray<VxConnection*> out = getConnectionsFrom(c);

            ControllerList preds;
            for (VxConnection** i = in.begin(); i != in.end(); ++i)
                if (!(*i)->mNotDependency)
                    preds.pushBackUnique((*i)->mInputController);
            if (c->mParent != 0) {
                VxCompositeController* parent = dynamic_cast<VxCompositeController*>(c->mParent);
                if (parent != 0)
                    preds.pushBackUnique(parent);
            }
            mPredecessors[c] = preds;
            mPredecessorCount[c] = preds.size();

            ControllerList succs;
            for (VxConnection** i = out.begin(); i != out.end(); ++i)
                succs.pushBackUnique((*i)->mOutputController);
            if (c != 0) {
                VxCompositeController* comp = dynamic_cast<VxCompositeController*>(c);
                if (comp != 0) {
                    VxControllerBase** ce = comp->mControllers.end();
                    for (VxControllerBase** i = comp->mControllers.begin(); i != ce; ++i)
                        succs.pushBackUnique(*i);
                }
            }
            mSuccessors[c] = succs;

            if (preds.size() == 0)
                mRoots.push_back(c);
        }
    }
    mValid = true;
}

// Mac 0x41f5ba, Win 0x7fbee0: Kahn's algorithm from the roots (first in, first out), then
// every controller of the order executes.
void VxControllerGraph::beginStep()
{
    if (!mValid)
        rebuild();
    if (mListener != 0 && mListener->skipStep() != 0)
        return;

    ControllerList order;
    CountMap count(mPredecessorCount);
    ControllerList queue(mRoots);
    while (queue.begin() != queue.end()) {
        VxControllerBase* c = *queue.begin();
        ControllerMap::iterator it = mSuccessors.find(c);
        if (it != mSuccessors.end()) {
            for (VxControllerBase** s = it->second.begin(); s != it->second.end(); ++s) {
                if (--count[*s] == 0)
                    queue.push_back(*s);
            }
        }
        order.push_back(c);
        for (VxControllerBase** j = queue.begin() + 1; j != queue.end(); ++j)
            *(j - 1) = *j;
        queue.pop_back();
    }

    if (mListener != 0)
        mListener->beginStep(&order);
    for (VxControllerBase** i = order.begin(); i != order.end(); ++i) {
        VxControllerBase* c = *i;
        if (c != 0 && (mListener == 0 || mListener->skipController(c) == 0)) {
            c->execute();
            if (mListener != 0)
                mListener->controllerExecuted(c);
        }
    }
    if (mListener != 0)
        mListener->endStep();
}

} // namespace Vx
