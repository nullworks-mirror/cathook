/*
  Created by Jenny White on 29.04.18.
  Copyright (c) 2018 nullworks. All rights reserved.
*/

#include <hacks/hacklist.hpp>
#include <settings/Bool.hpp>
#include "HookedMethods.hpp"

static settings::Bool die_if_vac{ "misc.die-if-vac", "false" };
static settings::Bool autoabandon{ "misc.auto-abandon", "false" };
static settings::String custom_disconnect_reason{ "misc.disconnect-reason",
                                                  "" };

namespace hooked_methods
{
Timer t{};
DEFINE_HOOKED_METHOD(Shutdown, void, INetChannel *this_, const char *reason)
{
    g_Settings.bInvalid = true;
    // This is a INetChannel hook - it SHOULDN'T be static because netchannel
    // changes.
    logging::Info("Disconnect: %s", reason);
    if (strstr(reason, "Generic_Kicked"))
    {
        if (*die_if_vac)
        {
            static uintptr_t addr = gSignatures.GetClientSignature(
                "C7 04 24 ? ? ? ? 8D 7D ? 31 F6");
            static uintptr_t offset0 = uintptr_t(*(uintptr_t *) (addr + 0x3));
            static uintptr_t offset1 = gSignatures.GetClientSignature(
                "55 89 E5 83 EC ? 8B 45 ? 8B 80 ? ? ? ? 85 C0 74 ? C7 44 24 ? "
                "? ? ? ? "
                "89 04 24 E8 ? ? ? ? 85 C0 74 ? 8B 40");
            typedef int (*GetPendingInvites_t)(uintptr_t);
            GetPendingInvites_t GetPendingInvites =
                GetPendingInvites_t(offset1);
            int invites = GetPendingInvites(offset0);

            re::CTFGCClientSystem *gc =
                re::CTFGCClientSystem::GTFGCClientSystem();
            re::CTFPartyClient *pc = re::CTFPartyClient::GTFPartyClient();
            if (gc && !gc->BConnectedToMatchServer(false) &&
                !gc->BHaveLiveMatch() && !invites)
                if (!(pc && pc->BInQueueForMatchGroup(tfmm::getQueue())))
                {
                    logging::Info("VAC/Matchmaking banned");
                    *(int *) 0 = 0;
                    exit(1);
                }
        }
    }
    if (strstr(reason, "banned"))
    {
        if (*die_if_vac)
        {
            logging::Info("VAC/Matchmaking banned");
            *(int *) 0 = 0;
            exit(1);
        }
    }
#if ENABLE_IPC
    ipc::UpdateServerAddress(true);
#endif
    if (isHackActive() && (custom_disconnect_reason.toString().size() > 3) &&
        strstr(reason, "user"))
    {
        original::Shutdown(this_, custom_disconnect_reason.toString().c_str());
    }
    else
    {
        original::Shutdown(this_, reason);
    }

    if (autoabandon)
    {
        t.update();
        tfmm::disconnectAndAbandon();
    }
    hacks::shared::autojoin::onShutdown();
}
} // namespace hooked_methods
