/*
 * AutoJoin.cpp
 *
 *  Created on: Jul 28, 2017
 *      Author: nullifiedcat
 */

#include <settings/Int.hpp>
#include "HookTools.hpp"
#include <hacks/AutoJoin.hpp>

#include "common.hpp"
#include "hack.hpp"

static settings::Bool autojoin_team{ "autojoin.team", "false" };
static settings::Int autojoin_class{ "autojoin.class", "0" };
static settings::Bool auto_queue{ "autojoin.auto-queue", "false" };
static settings::Bool auto_requeue{ "autojoin.auto-requeue", "false" };

namespace hacks::shared::autojoin
{

/*
 * Credits to Blackfire for helping me with auto-requeue!
 */

const std::string classnames[] = { "scout", "sniper", "soldier", "demoman", "medic", "heavyweapons", "pyro", "spy", "engineer" };

bool UnassignedTeam()
{
    return !g_pLocalPlayer->team or (g_pLocalPlayer->team == TEAM_SPEC);
}

bool UnassignedClass()
{
    return g_pLocalPlayer->clazz != *autojoin_class;
}

static Timer autoteam_timer{};
static Timer startqueue_timer{};
#if not ENABLE_VISUALS
static Timer queue_time{};
#endif
void updateSearch()
{
    if (!auto_queue && !auto_requeue)
    {
#if not ENABLE_VISUALS
        queue_time.update();
#endif
        return;
    }
    if (g_IEngine->IsInGame())
    {
#if not ENABLE_VISUALS
        queue_time.update();
#endif
        return;
    }

    static uintptr_t addr    = gSignatures.GetClientSignature("C7 04 24 ? ? ? ? 8D 7D ? 31 F6");
    static uintptr_t offset0 = uintptr_t(*(uintptr_t *) (addr + 0x3));
    static uintptr_t offset1 = gSignatures.GetClientSignature("55 89 E5 83 EC ? 8B 45 ? 8B 80 ? ? ? ? 85 C0 74 ? C7 44 24 ? ? ? ? ? "
                                                              "89 04 24 E8 ? ? ? ? 85 C0 74 ? 8B 40");
    typedef int (*GetPendingInvites_t)(uintptr_t);
    GetPendingInvites_t GetPendingInvites = GetPendingInvites_t(offset1);
    int invites                           = GetPendingInvites(offset0);

    re::CTFGCClientSystem *gc = re::CTFGCClientSystem::GTFGCClientSystem();
    re::CTFPartyClient *pc    = re::CTFPartyClient::GTFPartyClient();

    if (current_user_cmd && gc && gc->BConnectedToMatchServer(false) && gc->BHaveLiveMatch())
    {
#if not ENABLE_VISUALS
        queue_time.update();
#endif
        tfmm::leaveQueue();
    }
    //    if (gc && !gc->BConnectedToMatchServer(false) &&
    //            queuetime.test_and_set(10 * 1000 * 60) &&
    //            !gc->BHaveLiveMatch())
    //        tfmm::leaveQueue();

    if (auto_requeue)
    {
        if (startqueue_timer.check(5000) && gc && !gc->BConnectedToMatchServer(false) && !gc->BHaveLiveMatch() && !invites)
            if (pc && !(pc->BInQueueForMatchGroup(tfmm::getQueue()) || pc->BInQueueForStandby()))
            {
                logging::Info("Starting queue for standby, Invites %d", invites);
                tfmm::startQueueStandby();
            }
    }

    if (auto_queue)
    {
        if (startqueue_timer.check(5000) && gc && !gc->BConnectedToMatchServer(false) && !gc->BHaveLiveMatch() && !invites)
            if (pc && !(pc->BInQueueForMatchGroup(tfmm::getQueue()) || pc->BInQueueForStandby()))
            {
                logging::Info("Starting queue, Invites %d", invites);
                tfmm::startQueue();
            }
    }
    startqueue_timer.test_and_set(5000);
#if not ENABLE_VISUALS
    if (queue_time.test_and_set(1200000))
    {
        *(int *) nullptr = 0;
    }
#endif
}
static void update()
{
    if (autoteam_timer.test_and_set(500))
    {
        if (autojoin_team and UnassignedTeam())
        {
            hack::ExecuteCommand("autoteam");
        }
        else if (autojoin_class and UnassignedClass())
        {
            if (int(autojoin_class) < 10)
                g_IEngine->ExecuteClientCmd(format("join_class ", classnames[int(autojoin_class) - 1]).c_str());
        }
    }
}

void onShutdown()
{
    if (auto_queue)
        tfmm::startQueue();
}

static InitRoutine init([]() { EC::Register<EC::CreateMove>(update, "cm_autojoin", EC::average); });
} // namespace hacks::shared::autojoin
