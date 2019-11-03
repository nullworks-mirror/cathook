/*
 * Created on 3rd November 2019
 * Author: LightCat
 */
#include "reclasses.hpp"
#include "HookedMethods.hpp"

namespace hooked_methods
{

// Note that this is of the type CTFPlayerInventory *
DEFINE_HOOKED_METHOD(GetMaxItemCount, int, int *)
{
    // Max backpack slots ez gg
    return 3000;
}
} // namespace hooked_methods

static Timer hook_cooldown{};
namespace hacks::tf2::backpack_expander
{

void Paint()
{
    if (hook_cooldown.test_and_set(1000))
    {
        static auto invmng = re::CTFInventoryManager::GTFInventoryManager();
        if (invmng)
        {
            auto inv = invmng->GTFPlayerInventory();
            if (!hooks::inventory.IsHooked(inv))
            {
                hooks::inventory.Set(inv, 0);
                hooks::inventory.HookMethod(HOOK_ARGS(GetMaxItemCount));
                hooks::inventory.Apply();
            }
        }
    }
}

static InitRoutine init([]() {
#if !ENFORCE_STREAM_SAFETY
    EC::Register(EC::Paint, Paint, "bpexpander_paint");
    EC::Register(
        EC::Shutdown, []() { hooks::inventory.Release(); }, "backpack_expander_shutdown");
#endif
});
} // namespace hacks::tf2::backpack_expander
