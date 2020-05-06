/*
 * Remade on May the 3rd 2020
 * Author: BenCat07
 *
 */
#include "Backtrack.hpp"

namespace hacks::tf2::backtrack
{
// Apply Backtrack
void Backtrack::ApplyBacktrack()
{
    if (!isBacktrackEnabled)
        return;
    if (bt_ent)
    {
        current_user_cmd->tick_count                = (*bt_data).tickcount;
        CE_FLOAT(bt_ent, netvar.m_angEyeAngles)     = (*bt_data).m_vecAngles.x;
        CE_FLOAT(bt_ent, netvar.m_angEyeAngles + 4) = (*bt_data).m_vecAngles.y;
        CE_FLOAT(bt_ent, netvar.m_flSimulationTime) = (*bt_data).m_flSimulationTime;
    }
}

// Update tick to apply
void Backtrack::SetBacktrackData(CachedEntity *ent, BacktrackData tick)
{
    bt_ent  = ent;
    bt_data = tick;
}

// Get Best tick for Backtrack (crosshair/fov based)
bool Backtrack::getBestInternalTick(CachedEntity *, BacktrackData &data, std::optional<BacktrackData> &best_tick)
{
    // Best Score
    float bestScore = FLT_MAX;

    // Are we using a melee weapon?
    bool is_melee = false;
    if (g_pLocalPlayer->weapon_mode == weapon_melee)
        is_melee = true;

    // Get the FOV of the best tick if available
    if (best_tick)
        bestScore = GetFov(g_pLocalPlayer->v_OrigViewangles, g_pLocalPlayer->v_Eye, (*best_tick).hitboxes.at(head).center);

    float FOVDistance = GetFov(g_pLocalPlayer->v_OrigViewangles, g_pLocalPlayer->v_Eye, data.hitboxes.at(head).center);
    if (FOVDistance >= bestScore)
        return false;

    // Found new best fov, now vischeck
    if (FOVDistance < bestScore)
    {
        // Vischeck check
        if (!is_melee && !IsVectorVisible(g_pLocalPlayer->v_Eye, data.hitboxes.at(head).center, false))
            return false;
        return true;
    }
    // Should never be called but gcc likes to complain anyways
    return false;
}
// Is backtrack enabled?
bool Backtrack::isEnabled()
{
    if (!*enabled)
        return false;
    CachedEntity *wep = LOCAL_W;
    if (CE_BAD(wep))
        return false;
    if (*bt_slots == 0)
        return true;
    int slot = re::C_BaseCombatWeapon::GetSlot(RAW_ENT(wep));
    switch ((int) bt_slots)
    {
    case 1:
        if (slot == 0)
            return true;
        break;
    case 2:
        if (slot == 1)
            return true;
        break;
    case 3:
        if (slot == 2)
            return true;
        break;
    case 4:
        if (slot == 0 || slot == 1)
            return true;
        break;
    case 5:
        if (slot == 0 || slot == 2)
            return true;
        break;
    case 6:
        if (slot == 1 || slot == 2)
            return true;
        break;
    }
    return false;
}

// Main Tracking logic
void Backtrack::CreateMove()
{
    // Update enabled status
    isBacktrackEnabled = isEnabled();
    if (!isBacktrackEnabled)
    {
        latency_rampup = 0.0f;
        return;
    }

    if (CE_BAD(LOCAL_E) || CE_BAD(LOCAL_W) || !LOCAL_E->m_bAlivePlayer())
    {
        latency_rampup = 0.0f;
        return;
    }

    // Make Latency not instantly spike
    latency_rampup += 1.0f / 66.0f;
    latency_rampup = std::min(latency_rampup, 1.0f);

    updateDatagram();

    // Clear data
    bt_ent = nullptr;
    bt_data.reset();

    current_tickcount = current_user_cmd->tick_count;

    // the "Base" Backtracking
    std::optional<BacktrackData> best_data;
    CachedEntity *best_ent = nullptr;

    for (int i = 0; i <= g_IEngine->GetMaxClients(); i++)
    {
        if (i == g_pLocalPlayer->entity_idx)
        {
            resetData(i);
            continue;
        }
        CachedEntity *ent = ENTITY(i);
        if (CE_BAD(ent) || !ent->m_bAlivePlayer() || ent->m_Type() != ENTITY_PLAYER || !ent->m_bEnemy())
        {
            resetData(i);
            continue;
        }
        if (!ent->hitboxes.GetHitbox(0))
        {
            resetData(i);
            continue;
        }
        auto &backtrack_ent = backtrack_data.at(i);

        // Have no data, create it
        if (!backtrack_ent)
            backtrack_ent.reset(new std::array<BacktrackData, 67>);

        int current_index = current_user_cmd->tick_count % getTicks();

        // Our current tick
        auto &current_tick = (*backtrack_ent).at(current_index);

        // Previous tick
        int last_index = current_index - 1;
        if (last_index < 0)
            last_index = getTicks() - 1;

        auto &previous_tick = (*backtrack_ent).at(last_index);

        // Update basics
        current_tick.tickcount = current_user_cmd->tick_count;

        current_tick.m_vecAngles.x = CE_FLOAT(ent, netvar.m_angEyeAngles);
        current_tick.m_vecAngles.y = CE_FLOAT(ent, netvar.m_angEyeAngles + 4);

        current_tick.m_vecOrigin = ent->m_vecOrigin();
        current_tick.m_vecMins   = RAW_ENT(ent)->GetCollideable()->OBBMins();
        current_tick.m_vecMaxs   = RAW_ENT(ent)->GetCollideable()->OBBMaxs();

        current_tick.m_flSimulationTime = CE_FLOAT(ent, netvar.m_flSimulationTime);

        // Update hitboxes
        ent->hitboxes.InvalidateCache();
        for (size_t i = 0; i < 18; i++)
        {
            current_tick.hitboxes[i].center = ent->hitboxes.GetHitbox(i)->center;
            current_tick.hitboxes[i].min    = ent->hitboxes.GetHitbox(i)->min;
            current_tick.hitboxes[i].max    = ent->hitboxes.GetHitbox(i)->max;
        }

        // Copy bones (for chams/glow)
        memcpy((void *) current_tick.bones, (void *) ent->hitboxes.bones, sizeof(matrix3x4_t) * 128);

        // Check if tick updated or not (fakelag)
        current_tick.has_updated = !previous_tick.m_flSimulationTime || previous_tick.m_flSimulationTime != current_tick.m_flSimulationTime;

        // Get best tick for this ent
        std::optional<BacktrackData> data = getBestTick(ent, std::bind(&Backtrack::getBestInternalTick, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
        // Check if actually the best tick we have in total
        if (data && (!best_data || getBestInternalTick(ent, *data, best_data)))
        {
            best_data = data;
            best_ent  = ent;
        }
    }
    if (best_data && best_ent)
        SetBacktrackData(best_ent, *best_data);
}

void Backtrack::CreateMoveLate()
{
    ApplyBacktrack();
}

#if ENABLE_VISUALS
// Drawing
void Backtrack::Draw()
{
    if (!isBacktrackEnabled || !draw)
        return;

    if (CE_BAD(LOCAL_E))
        return;

    for (int i = 0; i <= g_IEngine->GetMaxClients(); i++)
    {
        auto data = getGoodTicks(i);
        if (data.empty())
            continue;
        for (auto &tick : data)
        {
            auto hbpos = tick.hitboxes.at(head).center;
            auto min   = tick.hitboxes.at(head).min;
            auto max   = tick.hitboxes.at(head).max;
            if (!hbpos.x && !hbpos.y && !hbpos.z)
                continue;
            Vector out;
            if (draw::WorldToScreen(hbpos, out))
            {
                float size = 0.0f;
                if (abs(max.x - min.x) > abs(max.y - min.y))
                    size = abs(max.x - min.x);
                else
                    size = abs(max.y - min.y);

                rgba_t draw_color = colors::green;
                // Found our target tick
                if (bt_ent && tick.tickcount == (*bt_data).tickcount && i == bt_ent->m_IDX)
                    draw_color = colors::red;
                draw::Rectangle(out.x, out.y, size / 4, size / 4, draw_color);
            }
        }
    }
}
#endif

// Reset things
void Backtrack::LevelShutdown()
{
    lastincomingsequence = 0;
    sequences.clear();
    for (int i = 0; i < PLAYER_ARRAY_SIZE; i++)
        resetData(i);
}

// Change Datagram data
void Backtrack::adjustPing(INetChannel *ch)
{
    if (!isBacktrackEnabled)
        return;
    for (auto &seq : sequences)
    {
        if (g_GlobalVars->realtime - seq.curtime >= getLatency() / 1000.0f)
        {
            ch->m_nInReliableState = seq.inreliablestate;
            ch->m_nInSequenceNr    = seq.sequencenr;
            break;
        }
    }
}

// Latency to add for backtrack
float Backtrack::getLatency()
{
    INetChannel *ch = (INetChannel *) g_IEngine->GetNetChannelInfo();
    // Track what actual latency we have
    float real_latency = 0.0f;

    // If we have a netchannel (just in case) set real latency to it
    if (ch)
        real_latency = ch->GetLatency(FLOW_OUTGOING) * 1000.0f;

    // Fix the latency
    float backtrack_latency = *latency - real_latency;

    // Clamp and apply rampup
    backtrack_latency = latency_rampup * std::clamp(backtrack_latency, 0.0f, std::max(800.0f - real_latency, 0.0f));

    return backtrack_latency;
}

// Update our sequences
void Backtrack::updateDatagram()
{
    INetChannel *ch = (INetChannel *) g_IEngine->GetNetChannelInfo();
    if (ch)
    {
        int m_nInSequenceNr = ch->m_nInSequenceNr;
        int instate         = ch->m_nInReliableState;
        if (m_nInSequenceNr > lastincomingsequence)
        {
            lastincomingsequence = m_nInSequenceNr;
            sequences.insert(sequences.begin(), CIncomingSequence(instate, m_nInSequenceNr, g_GlobalVars->realtime));
        }
        if (sequences.size() > 2048)
            sequences.pop_back();
    }
}

// Get How many ticks we should Store and use
int Backtrack::getTicks()
{
    float max_lat = getLatency() + 200.0f;

    // Clamp
    max_lat = std::min(1000.0f, max_lat);

    // Get difference
    int ticks = TIME_TO_TICKS(max_lat / 1000.0f);
    return ticks;
};

void Backtrack::resetData(int entidx)
{
    // Clear everything
    backtrack_data[entidx].reset();
}

bool Backtrack::isGoodTick(BacktrackData &tick)
{
    // This tick hasn't updated since the last one, Entity might be dropping packets
    if (!tick.has_updated)
        return false;
    // How big a difference is between the ping we fake and the tick passed?
    int delta_ticks = current_tickcount - TIME_TO_TICKS(getLatency() / 1000.0f) - tick.tickcount;

    // Difference may not be greater than 200ms, shave off a few ticks just to be safe.
    if (fabsf(TICKS_TO_TIME(delta_ticks)) <= 0.2f - TICKS_TO_TIME(3))
        return true;

    return false;
}

// Read only vector of good ticks
std::vector<BacktrackData> Backtrack::getGoodTicks(int entidx)
{
    std::vector<BacktrackData> to_return;
    // Invalid
    if (!backtrack_data.at(entidx))
        return to_return;

    // Check all ticks
    for (auto &tick : *backtrack_data.at(entidx))
        if (isGoodTick(tick))
            to_return.push_back(tick);

    // Sort so that oldest ticks come first
    std::sort(to_return.begin(), to_return.end(), [](BacktrackData &a, BacktrackData &b) { return a.tickcount < b.tickcount; });

    return to_return;
}

// This function is so other files can Easily get the best tick matching their criteria
std::optional<BacktrackData> Backtrack::getBestTick(CachedEntity *ent, std::function<bool(CachedEntity *ent, BacktrackData &data, std::optional<BacktrackData> &best_tick)> callback)
{
    std::optional<BacktrackData> best_tick;

    // No data recorded
    if (!backtrack_data.at(ent->m_IDX))
        return best_tick;

    // Let the callback do the lifting
    for (auto &tick : getGoodTicks(ent->m_IDX))
        // Call the callback, and if we have a new best tick assign it
        if (callback(ent, tick, best_tick))
            best_tick = tick;

    // Return best result
    return best_tick;
}

// Default filter method. Checks for vischeck on Hitscan weapons.
bool Backtrack::defaultTickFilter(CachedEntity *ent, BacktrackData tick)
{
    // Not hitscan, no vischeck needed
    if (g_pLocalPlayer->weapon_mode != weapon_hitscan)
        return true;
    // Return visibility
    return IsEntityVectorVisible(ent, tick.hitboxes.at(head).center, MASK_SHOT);
}

bool Backtrack::defaultEntFilter(CachedEntity *ent)
{
    // Dormant
    if (CE_BAD(ent))
        return false;
    // Friend check
    if (playerlist::IsFriend(ent))
        return false;
    return true;
}

// Get Closest tick of a specific entity
std::optional<BacktrackData> Backtrack::getClosestEntTick(CachedEntity *ent, Vector vec, std::function<bool(CachedEntity *, BacktrackData)> tick_filter)
{
    std::optional<BacktrackData> return_value;
    // No entry
    if (!backtrack_data.at(ent->m_IDX))
        return return_value;

    float distance = FLT_MAX;

    // Go through all Good ticks
    for (auto &tick : getGoodTicks(ent->m_IDX))
    {
        // Found Closer tick
        if (tick.m_vecOrigin.DistTo(vec) < distance)
        {
            // Does the tick pass the filter
            if (tick_filter(ent, tick))
            {
                return_value = tick;
                distance     = tick.m_vecOrigin.DistTo(vec);
            }
        }
    }
    return return_value;
}

// Get Closest tick of any (enemy) entity, Second Parameter is to allow custom filters for entity criteria, third for ticks. We provide defaults for vischecks + melee for the second one
std::optional<std::pair<CachedEntity *, BacktrackData>> Backtrack::getClosestTick(Vector vec, std::function<bool(CachedEntity *)> ent_filter, std::function<bool(CachedEntity *, BacktrackData)> tick_filter)
{
    float distance         = FLT_MAX;
    CachedEntity *best_ent = nullptr;
    BacktrackData best_data;

    std::optional<std::pair<CachedEntity *, BacktrackData>> return_val;

    for (int i = 0; i <= g_IEngine->GetMaxClients(); i++)
    {
        CachedEntity *ent = ENTITY(i);
        // These checks are always present
        if (CE_INVALID(ent) || !ent->m_bAlivePlayer() || !ent->m_bEnemy())
            continue;
        // true = passes check
        if (!ent_filter(ent))
            continue;
        auto closest_entdata = getClosestEntTick(ent, vec, tick_filter);
        // Closer than the stuff we have
        if (closest_entdata && (*closest_entdata).m_vecOrigin.DistTo(vec) <= distance)
        {
            distance  = (*closest_entdata).m_vecOrigin.DistTo(vec);
            best_data = *closest_entdata;
            best_ent  = ent;
        }
    }
    if (best_ent)
        return_val = std::pair<CachedEntity *, BacktrackData>(best_ent, best_data);
    return return_val;
}

static InitRoutine init([]() {
    EC::Register(EC::CreateMove, std::bind(&Backtrack::CreateMove, &hacks::tf2::backtrack::backtrack), "backtrack_cm", EC::early);
    EC::Register(EC::CreateMove, std::bind(&Backtrack::CreateMoveLate, &hacks::tf2::backtrack::backtrack), "backtrack_cmlate", EC::very_late);
#if ENABLE_VISUALS
    EC::Register(EC::Draw, std::bind(&Backtrack::Draw, &hacks::tf2::backtrack::backtrack), "backtrack_draw");
#endif
    EC::Register(EC::LevelShutdown, std::bind(&Backtrack::LevelShutdown, &hacks::tf2::backtrack::backtrack), "backtrack_levelshutdown");
});
Backtrack backtrack;
} // namespace hacks::tf2::backtrack
// Global interface
