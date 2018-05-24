/*
 * Backtrack.hpp
 *
 *  Created on: May 15, 2018
 *      Author: bencat07
 */

#pragma once
#include "common.hpp"
#include <boost/circular_buffer.hpp>

namespace hacks
{
namespace shared
{
namespace backtrack
{
struct BacktrackData
{
    int tickcount;
    Vector hitboxpos;
    Vector min;
    Vector max;
    Vector origin;
};
void Init();
void Run();
void Draw();
void AddLatencyToNetchan(INetChannel *, float);
void UpdateIncomingSequences();
extern int lastincomingsequencenumber;
extern int BestTick;
struct CIncomingSequence
{
    CIncomingSequence(int instate, int seqnr, float time)
    {
        inreliablestate = instate;
        sequencenr      = seqnr;
        curtime         = time;
    }
    int inreliablestate;
    int sequencenr;
    float curtime;
};
typedef boost::circular_buffer<CIncomingSequence> circular_buf;
extern circular_buf sequences;
extern CatVar latency;
extern CatVar enable;
extern BacktrackData headPositions[32][66];
}
}
}
