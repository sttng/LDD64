// LEGO Digital Designer — restored source
// ConceptBrick.cpp — LEGO::ConceptBrick (see ConceptBrick.h). TU 116: Mac 0x1d2090-0x1d2baf,
// Win 0x68d7d1-0x68d914.
#include "ConceptBrick.h"

#include <iostream>

#include "BasicMap.h"
#include "Scene.h"
#include "Part.h"
#include "Brick.h"

namespace LEGO {

// Mac 0x1d2090, Win 0x68d7d1
ConceptBrick::ConceptBrick(PrimitiveElement* element)
{
    m_elements.PushBack(RefPointer<PrimitiveElement>(element));
}

// Mac 0x1d26c0 / 0x1d2760, Win 0x68d83f (sdd 0x68d879)
ConceptBrick::~ConceptBrick()
{
}

// Mac 0x1d21c0
void ConceptBrick::CreateConceptBricks(BasicArray<RefPointer<ConceptBrick> >& conceptBricks,
                                       BasicArray<RefPointer<PrimitiveElement> > elements)
{
    // The concept brick of each brick (not referenced: `conceptBricks` holds them).
    BasicMap<Brick*, ConceptBrick*, Less<Brick*> > byBrick;
    for (BasicArray<RefPointer<PrimitiveElement> >::Iterator it = elements.Begin(); it != elements.End(); ++it) {
        PrimitiveElement* element = *it;
        Brick* brick = element->GetPart()->GetBrick();
        BasicMap<Brick*, ConceptBrick*, Less<Brick*> >::Iterator found = byBrick.Find(brick);
        if (found == byBrick.End()) {
            ConceptBrick* conceptBrick = new ConceptBrick(element);
            byBrick[brick] = conceptBrick;
            conceptBricks.PushBack(RefPointer<ConceptBrick>(conceptBrick));
        } else {
            found->second->AddElement(element);
        }
    }
}

// Mac 0x1d2580
void ConceptBrick::CreateConceptBricks(BasicArray<RefPointer<ConceptBrick> >& conceptBricks, Scene* scene)
{
    CreateConceptBricks(conceptBricks, scene->GetPrimitiveElements());
}

} // namespace LEGO
