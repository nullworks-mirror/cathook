#include "HookedMethods.hpp"

namespace hooked_methods
{
int last_tick = 0;

// Credits to blackfire for telling me to do this :)
DEFINE_HOOKED_METHOD(RunCommand, void, IPrediction *prediction, IClientEntity *entity, CUserCmd *usercmd, IMoveHelper *move)
{
    if (CE_GOOD(LOCAL_E) && CE_GOOD(LOCAL_W) && usercmd && usercmd->command_number)
    {
        criticals::weapon_info info(RAW_ENT(LOCAL_W));
        original::RunCommand(prediction, entity, usercmd, move);

        if (usercmd->tick_count == last_tick)
            info.restore_data(RAW_ENT(LOCAL_W));

        last_tick = usercmd->tick_count;
    }
    else
        return original::RunCommand(prediction, entity, usercmd, move);
}
} // namespace hooked_methods
