/*
 * Achievement.cpp
 *
 *  Created on: Jan 20, 2017
 *      Author: nullifiedcat
 */

#include <settings/Bool.hpp>
#include "Misc.hpp"
#include "common.hpp"

static settings::Bool safety{ "achievement.safety", "true" };

namespace hacks::tf2::achievement
{

void Lock()
{
    if (safety)
    {
        ConColorMsg({ 255, 0, 0, 255 }, "Switch `cat set achievement.safety` to true before using any achievement commands!\n");
        return;
    }
    g_ISteamUserStats->RequestCurrentStats();
    for (int i = 0; i < g_IAchievementMgr->GetAchievementCount(); i++)
    {
        g_ISteamUserStats->ClearAchievement(g_IAchievementMgr->GetAchievementByIndex(i)->GetName());
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
        ConColorMsg({ 255, 0, 0, 255 }, "Switch `cat set achievement.safety` to true before using any achievement commands!\n");
        return;
    }
    for (int i = 0; i < g_IAchievementMgr->GetAchievementCount(); i++)
    {
        g_IAchievementMgr->AwardAchievement(g_IAchievementMgr->GetAchievementByIndex(i)->GetAchievementID());
    }
}

CatCommand dump_achievement("achievement_dump", "Dump achievements to file (development)", []() {
    std::ofstream out("/tmp/cathook_achievements.txt", std::ios::out);
    if (out.bad())
        return;
    for (int i = 0; i < g_IAchievementMgr->GetAchievementCount(); i++)
    {
        out << '[' << i << "] " << g_IAchievementMgr->GetAchievementByIndex(i)->GetName() << ' ' << g_IAchievementMgr->GetAchievementByIndex(i)->GetAchievementID() << "\n";
    }
    out.close();
});
CatCommand unlock_single("achievement_unlock_single", "Unlocks single achievement by ID", [](const CCommand &args) {
    char *out = nullptr;
    int id    = strtol(args.Arg(1), &out, 10);
    if (out == args.Arg(1))
    {
        logging::Info("NaN achievement ID!");
        return;
    }
    IAchievement *ach = reinterpret_cast<IAchievement *>(g_IAchievementMgr->GetAchievementByID(id));
    if (ach)
    {
        g_IAchievementMgr->AwardAchievement(id);
    }
});
// For some reason it SEGV's when I try to GetAchievementByID();
CatCommand lock_single("achievement_lock_single", "Locks single achievement by INDEX!", [](const CCommand &args) {
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
    IAchievement *ach = reinterpret_cast<IAchievement *>(g_IAchievementMgr->GetAchievementByID(id));

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

static bool accept_notifs;
static bool equip;

void unlock_achievements_and_accept(std::vector<int> items)
{
    for (auto id : items)
    {
        IAchievement *ach = reinterpret_cast<IAchievement *>(g_IAchievementMgr->GetAchievementByID(id));
        if (ach)
        {
            g_IAchievementMgr->AwardAchievement(id);
        }
    }
    accept_notifs = true;
}
static CatCommand get_sniper_items("achievement_sniper", "Get all sniper achievement items", []() {
    static std::vector<int> sniper_items = { 1136, 1137, 1138 };
    unlock_achievements_and_accept(sniper_items);
});
static Timer accept_time{};
static Timer cooldowm{};
static CatCommand get_best_hats("achievement_cathats", "Get and equip the bencat hats", []() {
    static std::vector<int> bencat_hats = { 1902, 1912, 2006 };
    unlock_achievements_and_accept(bencat_hats);
    hacks::shared::misc::Schema_Reload();
    equip = true;
});
bool equip_on_all(int hat1, int hat2, int hat3)
{
    auto invmng     = re::CTFInventoryManager::GTFInventoryManager();
    auto inv        = invmng->GTFPlayerInventory();
    auto item_view1 = inv->GetFirstItemOfItemDef(hat1);
    auto item_view2 = inv->GetFirstItemOfItemDef(hat2);
    auto item_view3 = inv->GetFirstItemOfItemDef(hat3);
    for (int i = tf_scout; i < tf_engineer; i++)
    {
        bool success1 = invmng->EquipItemInLoadout(i, 7, item_view1->UUID());
        bool success2 = invmng->EquipItemInLoadout(i, 8, item_view2->UUID());
        bool success3 = invmng->EquipItemInLoadout(i, 10, item_view3->UUID());
        if (!(success1 && success2 && success3))
            return false;
    }
    return true;
}
static InitRoutine init([]() {
    EC::Register(
        EC::Paint,
        []() {
            if (accept_notifs)
            {
                accept_time.update();
                accept_notifs = false;
            }
            if (!accept_time.check(5000) && cooldowm.test_and_set(500))
                g_IEngine->ClientCmd_Unrestricted("cl_trigger_first_notification");
            if (equip)
            {
                if (accept_time.check(5000) && !accept_time.check(10000) && cooldowm.test_and_set(500))
                {
                    auto invmng = re::CTFInventoryManager::GTFInventoryManager();
                    auto inv    = invmng->GTFPlayerInventory();
                    // Frontline field recorder
                    auto item_view1 = inv->GetFirstItemOfItemDef(302);
                    // Gibus
                    auto item_view2 = inv->GetFirstItemOfItemDef(940);
                    // Skull Island Tropper
                    auto item_view3 = inv->GetFirstItemOfItemDef(941);
                    if (item_view1 && item_view2 && item_view3)
                    {
                        bool success = equip_on_all(302, 940, 941);
                        if (success)
                        {
                            logging::Info("Equipped hats");
                            equip = false;
                        }
                    }
                }
                else if (accept_time.check(20000))
                    equip = false;
            }
        },
        "achievement_autounlock");
});
} // namespace hacks::tf2::achievement
