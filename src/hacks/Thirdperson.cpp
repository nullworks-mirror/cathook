/*
  Created on 29.07.18.
*/

#include <hacks/Thirdperson.hpp>
#include <settings/Bool.hpp>
#include <localplayer.hpp>
#include <entitycache.hpp>
#include <core/sdk.hpp>

namespace hacks::tf::thirdperson
{
static settings::Boolean enable{ "visual.thirdperson.enable", "false" };
static settings::Boolean real_angles{ "visual.thirdperson.real-angles", "false" };
static bool was_enabled{ false };

void frameStageNotify()
{
    if (CE_BAD(LOCAL_E))
        return;

    if (enable)
    {
        // Add thirdperson
        if (!g_pLocalPlayer->life_state)
            CE_INT(LOCAL_E, netvar.nForceTauntCam) = 1;
        was_enabled = true;
    }
    if (!enable && was_enabled)
    {
        // Remove thirdperson
        CE_INT(LOCAL_E, netvar.nForceTauntCam) = 0;
        was_enabled                            = false;
    }
    if (real_angles && g_IInput->CAM_IsThirdPerson())
    {
        CE_FLOAT(LOCAL_E, netvar.deadflag + 4) = LOCAL_E->m_vecAngle().x;
        CE_FLOAT(LOCAL_E, netvar.deadflag + 8) = LOCAL_E->m_vecAngle().y;
    }
}
} // namespace hacks::tf::thirdperson
