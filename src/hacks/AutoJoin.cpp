/*
 * AutoJoin.cpp
 *
 *  Created on: Jul 28, 2017
 *      Author: nullifiedcat
 */

#include "common.hpp"
#include "hack.hpp"

namespace hacks
{
namespace shared
{
namespace autojoin
{

/*
 * Credits to Blackfire for helping me with auto-requeue!
 */

static CatEnum classes_enum({ "DISABLED", "SCOUT", "SNIPER", "SOLDIER",
                              "DEMOMAN", "MEDIC", "HEAVY", "PYRO", "SPY",
                              "ENGINEER" });
static CatVar autojoin_team(CV_SWITCH, "autojoin_team", "0", "AutoJoin",
                            "Automatically joins a team");
static CatVar preferred_class(classes_enum, "autojoin_class", "0",
                              "AutoJoin class",
                              "You will pick a class automatically");

CatVar auto_queue(CV_SWITCH, "autoqueue", "0", "AutoQueue",
                  "Automatically queue in casual matches");

const std::string classnames[] = { "scout",   "sniper", "soldier",
                                   "demoman", "medic",  "heavyweapons",
                                   "pyro",    "spy",    "engineer" };

bool UnassignedTeam()
{
    return !g_pLocalPlayer->team or (g_pLocalPlayer->team == TEAM_SPEC);
}

bool UnassignedClass()
{
    return g_pLocalPlayer->clazz != int(preferred_class);
}

Timer autoqueue_timer{};
Timer req_timer{};
/*CatVar party_bypass(CV_SWITCH, "party_bypass", "0", "Party Bypass",
                   "Bypass Party restrictions");*/
void UpdateSearch()
{
    // segfaults for no reason
    /*static bool calld = false;
    if (party_bypass && !calld) {
        static unsigned char patch[] = { 0x90, 0x90, 0x90, 0x90, 0x90, 0x90};
        static uintptr_t party_addr1 = gSignatures.GetClientSignature(
             "0F 84 ? ? ? ? 8B 7B ? 8D 45");
        static unsigned char patch2[] = { 0x90, 0xE9};
        static uintptr_t party_addr2 = gSignatures.GetClientSignature(
            "0F 8E ? ? ? ? 31 DB 0F B6 BD");
        static unsigned char patch3[] = { 0x90, 0x90};
        static uintptr_t party_addr3 = gSignatures.GetClientSignature(
            "74 ? E8 ? ? ? ? 89 F1");
        static unsigned char patch4[] = { 0x90, 0xE9};
        static uintptr_t party_addr4 = gSignatures.GetClientSignature(
            "0F 85 ? ? ? ? E8 ? ? ? ? C7 04 24 ? ? ? ? 89 44 24");
        static unsigned char patch5[] = { 0x90, 0x90, 0x90, 0x90};
        static uintptr_t party_addr5 = gSignatures.GetClientSignature(
            "0F 84 ? ? ? ? 8B 45 ? 8B 70 ? 8B 78 ? 8D 45");
        Patch((void *) party_addr1, (void *) patch, sizeof(patch));
        Patch((void *) party_addr2, (void *) patch2, sizeof(patch2));
        Patch((void *) party_addr3, (void *) patch3, sizeof(patch3));
        Patch((void *) party_addr4, (void *) patch4, sizeof(patch4));
        Patch((void *) party_addr5, (void *) patch5, sizeof(patch5));
        calld = true;
    }*/
    if (!auto_queue)
        return;
    if (g_IEngine->IsInGame())
        return;

    re::CTFGCClientSystem *gc = re::CTFGCClientSystem::GTFGCClientSystem();
    if (g_pUserCmd && gc && gc->BConnectedToMatchServer(false))
        tfmm::queue_leave();
    if (autoqueue_timer.test_and_set(60000))
    {
        if (gc && !gc->BConnectedToMatchServer(false))
        {
            logging::Info("Starting queue");
            tfmm::queue_start();
        }
    }
    if (req_timer.test_and_set(1800000))
    {
        logging::Info("Starting queue");
        tfmm::queue_start();
    }
}

Timer timer{};
void Update()
{
    if (timer.test_and_set(500))
    {
        if (autojoin_team and UnassignedTeam())
        {
            hack::ExecuteCommand("jointeam auto");
        }
        else if (preferred_class and UnassignedClass())
        {
            if (int(preferred_class) < 10)
                g_IEngine->ExecuteClientCmd(
                    format("join_class ", classnames[int(preferred_class) - 1])
                        .c_str());
        }
    }
}
}
}
}
