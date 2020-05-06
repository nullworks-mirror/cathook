#pragma once
#include "common.hpp"

namespace hacks::tf2::backtrack
{
class CIncomingSequence
{
public:
    int inreliablestate;
    int sequencenr;
    float curtime;

    CIncomingSequence(int instate, int seqnr, float time)
    {
        inreliablestate = instate;
        sequencenr      = seqnr;
        curtime         = time;
    }
};

class hitboxData
{
public:
    Vector center{ 0.0f, 0.0f, 0.0f };
    Vector min{ 0.0f, 0.0f, 0.0f };
    Vector max{ 0.0f, 0.0f, 0.0f };
};

class BacktrackData
{
public:
    int tickcount{};

    std::array<hitboxData, 18> hitboxes{};
    Vector m_vecOrigin{};
    Vector m_vecAngles{};

    Vector m_vecMins{};
    Vector m_vecMaxs{};

    float m_flSimulationTime{};
    bool has_updated{};

    matrix3x4_t bones[128]{};
};

// Let's try OOP for once
class Backtrack
{
    // Internal rvars
    settings::Boolean enabled{ "backtrack.enabled", "false" };
    settings::Boolean draw{ "backtrack.draw", "false" };

    std::vector<CIncomingSequence> sequences;
    int current_tickcount;
    std::array<std::unique_ptr<std::array<BacktrackData, 67>>, PLAYER_ARRAY_SIZE> backtrack_data;
    int lastincomingsequence{ 0 };
    // Used to make transition smooth(er)
    float latency_rampup = 0.0f;

    // Which data to apply in the late CreateMove
    CachedEntity *bt_ent;
    std::optional<BacktrackData> bt_data;

    bool isEnabled();
    float getLatency();
    int getTicks();
    bool getBestInternalTick(CachedEntity *, BacktrackData &, std::optional<BacktrackData> &);
    void ApplyBacktrack();

    // Stuff that has to be accessible from outside, mostly functions
public:
    settings::Float latency{ "backtrack.latency", "0" };
    settings::Int bt_slots{ "backtrack.slots", "0" };
    settings::Boolean chams{ "backtrack.chams", "false" };
    settings::Int chams_ticks{ "backtrack.chams.ticks", "1" };
    settings::Boolean team_color{ "backtrack.chams.team-color", "true" };

    // Check if backtrack is enabled
    bool isBacktrackEnabled;
    // Event callbacks
    void CreateMove();
    void CreateMoveLate();
#if ENABLE_VISUALS
    void Draw();
#endif
    void LevelShutdown();

    void adjustPing(INetChannel *);
    void updateDatagram();
    void resetData(int);
    bool isGoodTick(BacktrackData &);
    bool defaultTickFilter(CachedEntity *, BacktrackData);
    bool defaultEntFilter(CachedEntity *);

    // Various functions for getting backtrack ticks
    std::vector<BacktrackData> getGoodTicks(int);
    std::optional<BacktrackData> getBestTick(CachedEntity *, std::function<bool(CachedEntity *, BacktrackData &, std::optional<BacktrackData> &)>);
    std::optional<BacktrackData> getClosestEntTick(CachedEntity *, Vector, std::function<bool(CachedEntity *, BacktrackData)>);
    std::optional<std::pair<CachedEntity *, BacktrackData>> getClosestTick(Vector, std::function<bool(CachedEntity *)>, std::function<bool(CachedEntity *, BacktrackData)>);

    void SetBacktrackData(CachedEntity *ent, BacktrackData);
};
extern hacks::tf2::backtrack::Backtrack backtrack;
} // namespace hacks::tf2::backtrack
