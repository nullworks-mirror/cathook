/*
 * SendDatagram.cpp
 *
 *  Created on: May 21, 2018
 *      Author: bencat07
 */
#include "HookedMethods.hpp"
#include "hacks/Backtrack.hpp"
namespace hooked_methods
{
DEFINE_HOOKED_METHOD(SendDatagram, int, INetChannel *ch, bf_write *buf)
{
    if (!round(*hacks::shared::backtrack::latency) || !isHackActive())
        return original::SendDatagram(ch, buf);
    int in    = 0;
    int state = 0;
    if (CE_GOOD(LOCAL_E) && ch)
    {
        in    = ch->m_nInSequenceNr;
        state = ch->m_nInReliableState;
        hacks::shared::backtrack::AddLatencyToNetchan(ch);
    }

    int ret = original::SendDatagram(ch, buf);
    if (CE_GOOD(LOCAL_E) && ch)
    {
        ch->m_nInSequenceNr    = in;
        ch->m_nInReliableState = state;
    }
    return ret;
}
} // namespace hooked_methods
