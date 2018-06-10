#include "common.hpp"

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
        if (pEntity->m_iTeam() == LOCAL_E->m_iTeam())
            continue;
        if (pEntity->m_Type() != ENTITY_PLAYER)
            continue;
        if (!pEntity->hitboxes.GetHitbox(0))
            continue;
        Vector hitboxpos = pEntity->hitboxes.GetHitbox(0)->center;
        Vector min       = pEntity->hitboxes.GetHitbox(0)->min;
        Vector max       = pEntity->hitboxes.GetHitbox(0)->max;
        hitp[i] = hitboxpos;
        minp[i] = min;
        maxp[i] = max;
    }
}
void draw() {
    #if ENABLE_VISUALS
    if (!enable)
        return;
    for (int i = 1; i < g_IEngine->GetMaxClients(); i++) {
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
                                    colors::green);
        }
    }
#endif
}

}}}
