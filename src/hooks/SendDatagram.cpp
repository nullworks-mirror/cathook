/*
 * SendDatagram.cpp
 *
 *  Created on: May 21, 2018
 *      Author: bencat07
 */
#include "HookedMethods.hpp"
#include "Backtrack.hpp"
namespace hooked_methods
{
float latency2 = 0.0f;
DEFINE_HOOKED_METHOD(SendDatagram, int, INetChannel *ch, bf_write *buf)
{
#if not LAGBOT_MODE
    if ((float) hacks::shared::backtrack::latency > latency2)
        latency2 += 1.0f;
    else if ((float) hacks::shared::backtrack::latency < latency2)
        latency2 -= 1.0f;
    if (latency2 + 1.0f > (float) hacks::shared::backtrack::latency &&
        latency2 - 1.0f < (float) hacks::shared::backtrack::latency)
        latency2 = (float) hacks::shared::backtrack::latency;
    if (!hacks::shared::backtrack::enable || latency2 <= 0.001f)
        return original::SendDatagram(ch, buf);
    int in     = ch->m_nInSequenceNr;
    auto state = ch->m_nInReliableState;

    float latencysend = round(
        (round((latency2 - 0.5f) / 15.1515151515f) - 0.5f) * 15.1515151515f);
    hacks::shared::backtrack::AddLatencyToNetchan(ch, latencysend);
#endif

    int ret = original::SendDatagram(ch, buf);
#if not LAGBOT_MODE

    ch->m_nInSequenceNr    = in;
    ch->m_nInReliableState = state;

#endif
    return ret;
}
}
