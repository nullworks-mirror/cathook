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

CatEnum classes_enum({ "DISABLED", "SCOUT", "SNIPER", "SOLDIER", "DEMOMAN",
                       "MEDIC", "HEAVY", "PYRO", "SPY", "ENGINEER" });
CatVar autojoin_team(CV_SWITCH, "autojoin_team", "0", "AutoJoin",
                     "Automatically joins a team");
CatVar preferred_class(classes_enum, "autojoin_class", "0", "AutoJoin class",
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

void UpdateSearch()
{
    if (!auto_queue)
        return;
    if (g_IEngine->IsInGame())
        return;
    static Timer autoqueue_timer{};

    if (autoqueue_timer.test_and_set(5000))
    {
        re::CTFParty *party = re::CTFParty::GetParty();
        if (!party || re::CTFParty::state_(party) == 0)
        {
            logging::Info("Starting queue");
            tfmm::queue_start();
        }
    }
}

void Update()
{
    static Timer timer;

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
