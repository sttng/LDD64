// LEGO Digital Designer — restored source
// LEGO::DisconnectElement. See DisconnectElement.h.
//
// Original object file: unknown (Mac TU 353 0x334640-0x334c1f).
#include "DisconnectElement.h"
#include "BasicArray.h"
#include "PrimitiveConnection.h"
#include "PrimitiveElement.h"
#include "Scene.h"

namespace LEGO {

// Mac 0x334640
void DisconnectElement(PrimitiveElement* element)
{
    BasicArray<PrimitiveConnection*> connections;
    const PrimitiveElement::ConnectionMap& map = element->GetConnections();
    for (PrimitiveElement::ConnectionMap::ConstIterator it = map.Begin(); it != map.End(); ++it) {
        PrimitiveConnection* connection = it->second;
        if (it->first->GetPart() == element->GetPart()) {
            const BasicArray<PrimitiveConnection::Connection>& fields = connection->m_connections;
            for (UInt32 i = 0; i < fields.GetSize(); ++i) {
                if (fields[i].m_field->GetKind() < 0x79f2c)
                    connections.PushBack(connection);
            }
        } else {
            connections.PushBack(connection);
        }
    }
    element->GetScene()->Disconnect(connections);
}

} // namespace LEGO
