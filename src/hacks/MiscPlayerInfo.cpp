#include "common.hpp"

static settings::Bool draw_kda{ "misc.playerinfo.draw-kda", "false" };

namespace hacks::tf2::miscplayerinfo
{
#if ENABLE_VISUALS
std::array<float, 32> death_timer;
void Paint()
{
    if (!*draw_kda)
        return;
    if (draw_kda)
    {
        for (int i = 0; i < g_IEngine->GetMaxClients(); i++)
        {
            CachedEntity *ent = ENTITY(i);
            if (CE_BAD(ent))
                continue;
            if (ent->m_Type() != ENTITY_PLAYER)
                continue;
            if (g_pPlayerResource->isAlive(i))
                death_timer[i] = g_GlobalVars->curtime;
            auto collidable = RAW_ENT(ent)->GetCollideable();
            Vector draw_at  = ent->m_vecOrigin();
            draw_at.z += collidable->OBBMaxs().z + 30.0f;
            Vector out;

            rgba_t color = colors::white;
            int kills    = g_pPlayerResource->GetKills(i);
            int deaths   = g_pPlayerResource->GetDeaths(i);
            float KDA    = deaths ? (float) kills / (float) deaths : 0;
            if (KDA > 1.0f)
                color = colors::green;
            else if (KDA < 1.0f && deaths)
                color = colors::orange;
            color.g -= (float) (g_GlobalVars->curtime - death_timer[i]) / (3.0f);
            color.b -= (float) (g_GlobalVars->curtime - death_timer[i]) / (3.0f);
            color.r += (float) (g_GlobalVars->curtime - death_timer[i]) / (3.0f);
            color.g = fmaxf(0.0f, color.g);
            color.b = fmaxf(0.0f, color.b);
            color.r = fminf(1.0f, color.r);
            if (color.b > 0.0f || color.g > 0.0f)
                if (draw::WorldToScreen(draw_at, out))
                {

                    draw::String(out.x, out.y, color, format("KDA: ", KDA, " (", kills, "/", deaths, ")").c_str(), *fonts::esp);
                }
        }
    }
}
static InitRoutine init([]() { EC::Register(EC::Draw, Paint, "DRAW_Miscplayerinfo", EC::average); });
#endif
}; // namespace hacks::tf2::miscplayerinfo
