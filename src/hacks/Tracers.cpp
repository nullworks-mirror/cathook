#include "common.hpp"
#include "PlayerTools.hpp"

namespace hacks::shared::tracers
{

settings::Bool enabled("tracers.enabled", "false");
settings::Float green_dist("tracers.green-distance", "1500");
settings::Float max_dist("tracers.max_dist", "0");

// 0 = don't, 1 = yes but only in enemy team, 2 = always
settings::Int draw_friendlies("tracers.draw-friends", "1");

// Extend a line to a certain border
// https://stackoverflow.com/a/45056039
static inline Vector2D toBorder(float x1, float y1, float x2, float y2, float left, float top, float right, float bottom)
{
    float dx, dy, py, vx, vy;
    vx = x2 - x1;
    vy = y2 - y1;
    dx = vx < 0 ? left : right;
    dy = py = vy < 0 ? top : bottom;
    if (vx == 0)
    {
        dx = x1;
    }
    else if (vy == 0)
    {
        dy = y1;
    }
    else
    {
        dy = y1 + (vy / vx) * (dx - x1);
        if (dy < top || dy > bottom)
        {
            dx = x1 + (vx / vy) * (py - y1);
            dy = py;
        }
    }
    return { dx, dy };
}

inline std::optional<rgba_t> getColor(CachedEntity *ent)
{
    auto state = playerlist::AccessData(ent->player_info.friendsID);
    if (state.state == playerlist::k_EState::DEFAULT)
    {
        if (!ent->m_bEnemy())
            return std::nullopt;
        float dist = ent->m_vecOrigin().DistTo(LOCAL_E->m_vecOrigin());
        if (*max_dist && dist > *max_dist)
            return std::nullopt;
        return colors::Health(std::min(dist, *green_dist), *green_dist);
    }
    if (!player_tools::shouldTargetSteamId(ent->player_info.friendsID))
    {
        if (*draw_friendlies == 1)
        {
            if (ent->m_bEnemy())
                return colors::blu;
        }
        else if (*draw_friendlies == 2)
            return colors::blu;
        return std::nullopt;
    }
    if (!ent->m_bEnemy())
        return std::nullopt;
    return playerlist::Color(ent->player_info.friendsID);
}

void draw()
{
    if (!enabled || CE_BAD(LOCAL_E) || !LOCAL_E->m_bAlivePlayer())
        return;
    // Loop all players
    for (int i = 1; i < g_IEngine->GetMaxClients(); i++)
    {
        // Get and check player
        auto ent = ENTITY(i);
        if (CE_BAD(ent) || !ent->m_bAlivePlayer())
            continue;
        if (ent == LOCAL_E)
            continue;
        auto color = getColor(ent);
        if (!color)
            continue;

        Vector out;
        if (!draw::WorldToScreen(ent->m_vecOrigin(), out))
        {
            // We need to flip on both x and y axis in case m_vecOrigin its not actually on screen
            out.x = draw::width - out.x;
            out.y = draw::height - out.y;

            auto extended = toBorder(draw::width / 2, draw::height / 2, out.x, out.y, 0, 0, draw::width, draw::height);
            out.x         = extended.x;
            out.y         = extended.y;
        }
        draw::Line(draw::width / 2, draw::height / 2, out.x - draw::width / 2, out.y - draw::height / 2, *color, 2);
    }
}

InitRoutine init([]() { EC::Register(EC::Draw, draw, "DRAW_tracers"); });
} // namespace hacks::shared::tracers
