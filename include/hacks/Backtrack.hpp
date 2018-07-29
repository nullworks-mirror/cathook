/*
 * Backtrack.hpp
 *
 *  Created on: May 15, 2018
 *      Author: bencat07
 */

#pragma once
#include "common.hpp"
#include <boost/circular_buffer.hpp>

namespace hacks::shared::backtrack
{
struct BacktrackData
{
    int tickcount{ 0 };
    Vector hitboxpos{ 0.0f, 0.0f, 0.0f };
    Vector min{ 0.0f, 0.0f, 0.0f };
    Vector max{ 0.0f, 0.0f, 0.0f };
    Vector origin{ 0.0f, 0.0f, 0.0f };
    float viewangles{ 0.0f };
    float simtime{ 0.0f };
    Vector entorigin{ 0.0f, 0.0f, 0.0f };
};
struct BestTickData
{
    int tickcount{ 0 };
    int tick{ 0 };
    bool operator<(const BestTickData &rhs) const
    {
        return tickcount < rhs.tickcount;
    }
};
void Init();
void Run();
void Draw();
void AddLatencyToNetchan(INetChannel *, float);
void UpdateIncomingSequences();
bool shouldBacktrack();
extern int lastincomingsequencenumber;
extern int BestTick;
extern int iBestTarget;
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
typedef boost::circular_buffer_space_optimized<CIncomingSequence> circular_buf;
extern circular_buf sequences;
extern BacktrackData headPositions[32][66];
extern BestTickData sorted_ticks[66];

bool isBacktrackEnabled();
float getLatency();
int getTicks();
// FIXME
int getTicks2();
}
