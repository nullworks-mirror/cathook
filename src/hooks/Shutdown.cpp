/*
  Created by Jenny White on 29.04.18.
  Copyright (c) 2018 nullworks. All rights reserved.
*/

#include <hacks/hacklist.hpp>
#include <settings/Bool.hpp>
#include "HookedMethods.hpp"

settings::Bool die_if_vac{ "misc.die-if-vac", "false" };
static settings::Bool autoabandon{ "misc.auto-abandon", "false" };
static settings::String custom_disconnect_reason{ "misc.disconnect-reason", "" };
settings::Bool random_name{ "misc.random-name", "false" };
extern settings::String force_name;

namespace hooked_methods
{

DEFINE_HOOKED_METHOD(Shutdown, void, INetChannel *this_, const char *reason)
{
    g_Settings.bInvalid = true;
    logging::Info("Disconnect: %s", reason);
    if (strstr(reason, "banned") || (strstr(reason, "Generic_Kicked") && tfmm::isMMBanned()))
    {
        if (*die_if_vac)
        {
            logging::Info("VAC/Matchmaking banned");
            *(int *) nullptr = 0;
            exit(1);
        }
    }
#if ENABLE_IPC
    ipc::UpdateServerAddress(true);
#endif
    if (isHackActive() && (custom_disconnect_reason.toString().size() > 3) && strstr(reason, "user"))
    {
        original::Shutdown(this_, custom_disconnect_reason.toString().c_str());
    }
    else
    {
        original::Shutdown(this_, reason);
    }
    if (autoabandon)
        tfmm::disconnectAndAbandon();
    hacks::shared::autojoin::onShutdown();
    if (*random_name)
    {
        static TextFile file;
        if (file.TryLoad("names.txt"))
        {
            force_name = file.lines.at(rand() % file.lines.size());
        }
    }
}
} // namespace hooked_methods
