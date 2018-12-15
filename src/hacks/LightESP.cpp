#include "config.h"
#if ENABLE_VISUALS
#include "common.hpp"
#include "glez/draw.hpp"

static settings::Bool enable{ "lightesp.enable", "false" };

namespace hacks::shared::lightesp
{

static Vector hitp[32];
static Vector minp[32];
static Vector maxp[32];
static bool drawEsp[32];

rgba_t LightESPColor(CachedEntity *ent)
{
    if (!playerlist::IsDefault(ent))
    {
        return playerlist::Color(ent);
    }
    return colors::green;
}

static void cm() {
    if (!*enable)
        return;
    for (int i = 1; i < g_IEngine->GetMaxClients(); i++)
    {
        if (g_pLocalPlayer->entity_idx == i)
            continue;
        CachedEntity *pEntity = ENTITY(i);
        if (CE_BAD(pEntity) || !pEntity->m_bAlivePlayer())
        {
            drawEsp[i] = false;
            continue;
        }
        if (pEntity->m_iTeam() == LOCAL_E->m_iTeam() && playerlist::IsDefault(pEntity))
        {
            drawEsp[i] = false;
            continue;
        }
        if (!pEntity->hitboxes.GetHitbox(0))
            continue;
        hitp[i]    = pEntity->hitboxes.GetHitbox(0)->center;
        minp[i]    = pEntity->hitboxes.GetHitbox(0)->min;
        maxp[i]    = pEntity->hitboxes.GetHitbox(0)->max;
        drawEsp[i] = true;
    }
}

void draw()
{
    if (!enable)
        return;
    for (int i = 1; i < g_IEngine->GetMaxClients(); i++)
    {
        if (!drawEsp[i])
            continue;
        CachedEntity *pEntity = ENTITY(i);
        if (CE_BAD(pEntity) || !pEntity->m_bAlivePlayer())
            continue;
        if (pEntity == LOCAL_E)
            continue;
        Vector out;
        if (draw::WorldToScreen(hitp[i], out))
        {
            float size = 0.0f;
            Vector pout, pout2;
            if (draw::WorldToScreen(minp[i], pout) && draw::WorldToScreen(maxp[i], pout2))
                size = fmaxf(fabsf(pout2.x - pout.x), fabsf(pout2.y - pout.y));

            float minsize = 20.0f;
            if (size < minsize)
                size = minsize;
            glez::draw::rect(out.x - fabsf(pout.x - pout2.x) / 4, out.y - fabsf(pout.y - pout2.y) / 4, fabsf(pout.x - pout2.x) / 2, fabsf(pout.y - pout2.y) / 2, hacks::shared::lightesp::LightESPColor(pEntity));
            glez::draw::rect(out.x - size / 8, out.y - size / 8, size / 4, size / 4, colors::red);
        }
    }
}

static InitRoutine init([](){
    EC::Register<EC::CreateMove>(cm, "cm_lightesp", EC::average);
    EC::Register<EC::Draw>(draw, "draw_lightesp", EC::average);
});

} // namespace hacks::shared::lightesp
#endif
