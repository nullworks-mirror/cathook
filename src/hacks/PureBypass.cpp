#include <settings/Bool.hpp>
#include "common.hpp"

static settings::Bool enabled{ "pure-bypass.enable", "false" };
static void *pure_orig, **pure_addr;

static void toggle(bool on)
{
    if (on)
    {
        if (!pure_addr)
        {
            pure_addr = *reinterpret_cast<void ***>(gSignatures.GetEngineSignature("A1 ? ? ? ? 85 C0 74 ? C7 44 24 ? ? ? ? ? 89 04 24") + 1);
            if (!pure_addr)
            {
                logging::Info("Pure bypass broken, failed to find signature");
                return;
            }
        }
        if (*pure_addr)
            pure_orig = *pure_addr;

        *pure_addr = nullptr;
    }
    else if (pure_orig)
    {
        *pure_addr = pure_orig;
        pure_orig  = nullptr;
    }
}

static InitRoutine init([] {
    toggle(*enabled);
    enabled.installChangeCallback([](settings::VariableBase<bool> &, bool on) { toggle(on); });
});
