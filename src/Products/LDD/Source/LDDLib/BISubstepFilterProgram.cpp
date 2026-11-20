// LEGO Digital Designer — restored source
// TU 302 "BISubstepFilterProgram.cpp" (Mac 0x2af8c0-0x2aff3f): LEGO::BISubstepFilterProgram.
// Template code of the TU: BasicArray<RefPointer<BISubstepFilter> > (PushBack 0x2af8c0,
// destructor 0x2afa00).
#include "BISubstepFilterProgram.h"
#include "BISubstepFilterBaseplate.h"
#include "BGSubstepFilterMinifig.h"
#include "BIBrick.h"

namespace LEGO {

// Mac 0x2afa70
BISubstepFilterProgram::BISubstepFilterProgram()
{
    m_filters.PushBack(RefPointer<BISubstepFilter>(new BISubstepFilterBaseplate));
    m_filters.PushBack(RefPointer<BISubstepFilter>(new BGSubstepFilterMinifig));
}

// Mac 0x2afd30 / 0x2afe00 (thunk 0x2afde0), Win 0x592b90 / sdd 0x592b50
BISubstepFilterProgram::~BISubstepFilterProgram()
{
}

// Mac 0x2afc40, Win 0x592bc0
UInt32 BISubstepFilterProgram::Apply(BasicSet<BIBrick*>& bricks)
{
    for (BasicArray<RefPointer<BISubstepFilter> >::Iterator f = m_filters.Begin(); f != m_filters.End(); ++f)
        (*f)->Begin();
    UInt32 removed = 0;
    BasicSet<BIBrick*>::Iterator it = bricks.Begin();
    while (it != bricks.End()) {
        BIBrick* brick = *it;
        bool rejected = false;
        if (brick->GetType() == BIBrick::TypeSubstep) {
            for (BasicArray<RefPointer<BISubstepFilter> >::Iterator f = m_filters.Begin(); f != m_filters.End(); ++f) {
                if (!(*f)->Accept(brick)) {
                    rejected = true;
                    break;
                }
            }
        }
        if (rejected) {
            bricks.Erase(it++);
            ++removed;
        } else {
            ++it;
        }
    }
    return removed;
}

} // namespace LEGO
