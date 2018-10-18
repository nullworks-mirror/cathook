/*
 * Achievement.cpp
 *
 *  Created on: Jan 20, 2017
 *      Author: nullifiedcat
 */

#include <settings/Bool.hpp>
#include "common.hpp"

static settings::Bool safety{ "achievement.safety", "true" };

namespace hacks::tf2::achievement
{

void Lock()
{
    if (safety)
    {
        ConColorMsg(
            { 255, 0, 0, 255 },
            "Switch " CON_PREFIX
            "achievement_safety to 0 before using any achievement commands!\n");
        return;
    }
    g_ISteamUserStats->RequestCurrentStats();
    for (int i = 0; i < g_IAchievementMgr->GetAchievementCount(); i++)
    {
        g_ISteamUserStats->ClearAchievement(
            g_IAchievementMgr->GetAchievementByIndex(i)->GetName());
    }
    g_ISteamUserStats->StoreStats();
    g_ISteamUserStats->RequestCurrentStats();
}

void Unlock()
{
    /*auto Invmng = re::CTFInventoryManager::GTFInventoryManager();
    auto Inv = re::CTFPlayerInventory::GTFPlayerInventory();
    auto Item = Inv->GetFirstItemOfItemDef(59);
    Invmng->EquipItemInLoadout(0, 0, (unsigned long long
    int)Item->uniqueid());*/
    if (safety)
    {
        ConColorMsg(
            { 255, 0, 0, 255 },
            "Switch " CON_PREFIX
            "achievement_safety to 0 before using any achievement commands!\n");
        return;
    }
    for (int i = 0; i < g_IAchievementMgr->GetAchievementCount(); i++)
    {
        g_IAchievementMgr->AwardAchievement(
            g_IAchievementMgr->GetAchievementByIndex(i)->GetAchievementID());
    }
}

CatCommand dump_achievement(
    "achievement_dump", "Dump achievements to file (development)", []() {
        std::ofstream out("/tmp/cathook_achievements.txt", std::ios::out);
        if (out.bad())
            return;
        for (int i = 0; i < g_IAchievementMgr->GetAchievementCount(); i++)
        {
            out << '[' << i << "] "
                << g_IAchievementMgr->GetAchievementByIndex(i)->GetName() << ' '
                << g_IAchievementMgr->GetAchievementByIndex(i)
                       ->GetAchievementID()
                << "\n";
        }
        out.close();
    });
CatCommand unlock_single("achievement_unlock_single",
                         "Unlocks single achievement by ID",
                         [](const CCommand &args) {
                             char *out = nullptr;
                             int id    = strtol(args.Arg(1), &out, 10);
                             if (out == args.Arg(1))
                             {
                                 logging::Info("NaN achievement ID!");
                                 return;
                             }
                             IAchievement *ach =
                                 reinterpret_cast<IAchievement *>(
                                     g_IAchievementMgr->GetAchievementByID(id));
                             if (ach)
                             {
                                 g_IAchievementMgr->AwardAchievement(id);
                             }
                         });
// For some reason it SEGV's when I try to GetAchievementByID();
CatCommand lock_single(
    "achievement_lock_single", "Locks single achievement by INDEX!",
    [](const CCommand &args) {
        if (args.ArgC() < 1)
        {
            logging::Info("Actually provide an index");
            return;
        }
        int id;
        try
        {
            id = std::stoi(args.Arg(1));
        }
        catch (std::invalid_argument)
        {
            logging::Info("NaN achievement ID!");
            return;
        }
        IAchievement *ach = reinterpret_cast<IAchievement *>(
            g_IAchievementMgr->GetAchievementByID(id));

        int index = -1;
        if (ach)
            for (int i = 0; i < g_IAchievementMgr->GetAchievementCount(); i++)
            {
                auto ach2 = g_IAchievementMgr->GetAchievementByIndex(i);
                if (ach2->GetAchievementID() == id)
                {
                    index = i;
                    break;
                }
            }
        if (ach && index != -1)
        {
            g_ISteamUserStats->RequestCurrentStats();
            auto ach = g_IAchievementMgr->GetAchievementByIndex(index);
            g_ISteamUserStats->ClearAchievement(ach->GetName());
            g_ISteamUserStats->StoreStats();
            g_ISteamUserStats->RequestCurrentStats();
        }
    });
CatCommand lock("achievement_lock", "Lock all achievements", Lock);
CatCommand unlock("achievement_unlock", "Unlock all achievements", Unlock);
} // namespace hacks::tf2::achievement
