#include "LightESP.hpp"

namespace hacks
{
namespace shared
{
namespace lightesp
{


CatVar enable(CV_SWITCH, "lightesp", "0", "Enable LightESP",
              "Lightweight ESP. Only shows head.");
Vector hitp[32];
Vector minp[32];
Vector maxp[32];

void run() {
    for (int i = 1; i < g_IEngine->GetMaxClients(); i++)
    {
        CachedEntity *pEntity = ENTITY(i);
        if (CE_BAD(pEntity) || !pEntity->m_bAlivePlayer()) {
            hitp[i] = {0, 0, 0};
            continue;
        }
        if (pEntity->m_iTeam() == LOCAL_E->m_iTeam() && playerlist::IsDefault(pEntity)) {
            hitp[i] = {0, 0, 0};
            continue;
        }
        if (!pEntity->hitboxes.GetHitbox(0))
            continue;
        hitp[i] = pEntity->hitboxes.GetHitbox(0)->center;
        minp[i] = pEntity->hitboxes.GetHitbox(0)->min;
        maxp[i] = pEntity->hitboxes.GetHitbox(0)->max;
    }
}
void draw() {
    #if ENABLE_VISUALS
    if (!enable)
        return;
    for (int i = 1; i < g_IEngine->GetMaxClients(); i++) {
        CachedEntity *pEntity = ENTITY(i);
        if (CE_BAD(pEntity) || !pEntity->m_bAlivePlayer())
            continue;
        auto hitboxpos = hitp[i];
        auto min = minp[i];
        auto max = maxp[i];

        if (!hitboxpos.x && !hitboxpos.y && !hitboxpos.z)
            continue;
        Vector out;
        if (draw::WorldToScreen(hitboxpos, out))
        {
            float size = 0.0f;
            if (abs(max.x - min.x) > abs(max.y - min.y))
                size = abs(max.x - min.x);
            else
                size = abs(max.y - min.y);

                draw_api::draw_rect(out.x, out.y, size / 4, size / 4,
                                    hacks::shared::lightesp::LightESPColor(pEntity)); //hacks::shared::lightesp::LightESPColor(pEntity)
        }
    }
#endif
}

rgba_t LightESPColor(CachedEntity *ent)
{
    if (!playerlist::IsDefault(ent)) {
        return playerlist::Color(ent);
    }
    return colors::green;
}
}}}
