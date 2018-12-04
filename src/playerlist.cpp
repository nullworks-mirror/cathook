/*
 * playerlist.cpp
 *
 *  Created on: Apr 11, 2017
 *      Author: nullifiedcat
 */

#include "playerlist.hpp"
#include "common.hpp"

#include <stdint.h>
#include <dirent.h>
#include <sys/stat.h>
#include <boost/algorithm/string.hpp>

namespace playerlist
{

std::unordered_map<unsigned, userdata> data{};

const userdata null_data{};
#if ENABLE_VISUALS
rgba_t k_Colors[] = { colors::empty, colors::FromRGBA8(99, 226, 161, 255),
                      colors::FromRGBA8(226, 204, 99, 255),
                      colors::FromRGBA8(232, 134, 6, 255), colors::empty };
#endif
bool ShouldSave(const userdata &data)
{
#if ENABLE_VISUALS
    return data.color || (data.state != k_EState::DEFAULT);
#endif
    return (data.state != k_EState::DEFAULT);
}

void Save()
{
    DIR *cathook_directory = opendir(DATA_PATH);
    if (!cathook_directory)
    {
        logging::Info("[ERROR] cathook data directory doesn't exist! How did "
                      "the cheat even get injected?");
        return;
    }
    else
        closedir(cathook_directory);
    try
    {
        std::ofstream file(DATA_PATH "/plist",
                           std::ios::out | std::ios::binary);
        file.write(reinterpret_cast<const char *>(&SERIALIZE_VERSION),
                   sizeof(SERIALIZE_VERSION));
        int size = 0;
        for (const auto &item : data)
        {
            if (ShouldSave(item.second))
                size++;
        }
        file.write(reinterpret_cast<const char *>(&size), sizeof(size));
        for (const auto &item : data)
        {
            if (!ShouldSave(item.second))
                continue;
            file.write(reinterpret_cast<const char *>(&item.first),
                       sizeof(item.first));
            file.write(reinterpret_cast<const char *>(&item.second),
                       sizeof(item.second));
        }
        file.close();
        logging::Info("Writing successful");
    }
    catch (std::exception &e)
    {
        logging::Info("Writing unsuccessful: %s", e.what());
    }
}

void Load()
{
    data.clear();
    DIR *cathook_directory = opendir(DATA_PATH);
    if (!cathook_directory)
    {
        logging::Info("[ERROR] cathook data directory doesn't exist! How did "
                      "the cheat even get injected?");
        return;
    }
    else
        closedir(cathook_directory);
    try
    {
        std::ifstream file(DATA_PATH "/plist", std::ios::in | std::ios::binary);
        int file_serialize = 0;
        file.read(reinterpret_cast<char *>(&file_serialize),
                  sizeof(file_serialize));
        if (file_serialize != SERIALIZE_VERSION)
        {
            logging::Info(
                "Outdated/corrupted playerlist file! Cannot load this.");
            file.close();
            return;
        }
        int count = 0;
        file.read(reinterpret_cast<char *>(&count), sizeof(count));
        logging::Info("Reading %i entries...", count);
        for (int i = 0; i < count; i++)
        {
            int steamid;
            userdata udata;
            file.read(reinterpret_cast<char *>(&steamid), sizeof(steamid));
            file.read(reinterpret_cast<char *>(&udata), sizeof(udata));
            data.emplace(steamid, udata);
        }
        file.close();
        logging::Info("Reading successful!");
    }
    catch (std::exception &e)
    {
        logging::Info("Reading unsuccessful: %s", e.what());
    }
}
#if ENABLE_VISUALS
rgba_t Color(unsigned steamid)
{
    if (AccessData(steamid).state == k_EState::DEVELOPER)
        return colors::RainbowCurrent();
    if (AccessData(steamid).state == k_EState::CAT)
        return colors::RainbowCurrent();
    if (AccessData(steamid).color.a)
    {
        return AccessData(steamid).color;
    }
    else
    {
        return k_Colors[static_cast<int>(AccessData(steamid).state)];
    }
}

rgba_t Color(CachedEntity *player)
{
    if (CE_GOOD(player))
        return Color(player->player_info.friendsID);
    return colors::empty;
}
#endif
userdata &AccessData(unsigned steamid)
{
    return data[steamid];
}

// Assume player is non-null
userdata &AccessData(CachedEntity *player)
{
    if (CE_GOOD(player))
        return AccessData(player->player_info.friendsID);
    return AccessData(0U);
}

bool IsDefault(unsigned steamid)
{
    const userdata &data = AccessData(steamid);
#if ENABLE_VISUALS
    return data.state == k_EState::DEFAULT && !data.color.a;
#endif
    return data.state == k_EState ::DEFAULT;
}

bool IsDefault(CachedEntity *entity)
{
    if (CE_GOOD(entity))
        return IsDefault(entity->player_info.friendsID);
    return true;
}

CatCommand pl_save("pl_save", "Save playerlist", Save);
CatCommand pl_load("pl_load", "Load playerlist", Load);

CatCommand pl_set_state(
    "pl_set_state",
    "cat_pl_set_state [playername] [state] (Tab to autocomplete)",
    [](const CCommand &args) {
        if (args.ArgC() != 3)
        {
            logging::Info("Invalid call");
            return;
        }
        auto name = args.Arg(1);
        int id    = -1;
        for (int i = 0; i < g_IEngine->GetMaxClients(); i++)
        {
            player_info_s info;
            if (!g_IEngine->GetPlayerInfo(i, &info))
                continue;
            std::string currname(info.name);
            std::replace(currname.begin(), currname.end(), ' ', '-');
            std::replace_if(currname.begin(), currname.end(),
                            [](char x) { return !isprint(x); }, '*');
            if (currname.find(name) != 0)
                continue;
            id = i;
            break;
        }
        if (id == -1)
        {
            logging::Info("Unknown Player Name. (Use tab for autocomplete)");
            return;
        }
        std::string state = args.Arg(2);
        boost::to_upper(state);
        player_info_s info;
        g_IEngine->GetPlayerInfo(id, &info);

        if (k_Names[0] == state)
            AccessData(info.friendsID).state = k_EState::DEFAULT;
        else if (k_Names[1] == state)
            AccessData(info.friendsID).state = k_EState::FRIEND;
        else if (k_Names[2] == state)
            AccessData(info.friendsID).state = k_EState::RAGE;
        else if (k_Names[3] == state)
            AccessData(info.friendsID).state = k_EState::IPC;
        else if (k_Names[4] == state)
            AccessData(info.friendsID).state = k_EState::DEVELOPER;
        else
            logging::Info("Unknown State. (Use tab for autocomplete)");
    });

static int cat_pl_set_state_completionCallback(
    const char *c_partial,
    char commands[COMMAND_COMPLETION_MAXITEMS][COMMAND_COMPLETION_ITEM_LENGTH])
{
    std::string partial = c_partial;
    std::string parts[2]{};
    auto j    = 0u;
    auto f    = false;
    int count = 0;

    for (auto i = 0u; i < partial.size() && j < 3; ++i)
    {
        auto space = (bool) isspace(partial[i]);
        if (!space)
        {
            if (j)
                parts[j - 1].push_back(partial[i]);
            f = true;
        }

        if (i == partial.size() - 1 || (f && space))
        {
            if (space)
                ++j;
            f = false;
        }
    }

    std::vector<std::string> names;

    for (int i = 0; i < g_IEngine->GetMaxClients(); i++)
    {
        player_info_s info;
        if (!g_IEngine->GetPlayerInfo(i, &info))
            continue;
        std::string name(info.name);
        std::replace(name.begin(), name.end(), ' ', '-');
        std::replace_if(name.begin(), name.end(),
                        [](char x) { return !isprint(x); }, '*');
        names.push_back(name);
    }
    std::sort(names.begin(), names.end());

    if (parts[0].empty() ||
        parts[1].empty() && (!parts[0].empty() && partial.back() != ' '))
    {
        boost::to_lower(parts[0]);
        for (const auto &s : names)
        {
            // if (s.find(parts[0]) == 0)
            if (boost::to_lower_copy(s).find(parts[0]) == 0)
            {
                snprintf(commands[count++], COMMAND_COMPLETION_ITEM_LENGTH - 1,
                         "cat_pl_set_state %s", s.c_str());
            }
        }
        return count;
    }
    boost::to_lower(parts[1]);
    for (const auto &s : k_Names)
    {
        if (boost::to_lower_copy(s).find(parts[1]) == 0)
        {
            snprintf(commands[count++], COMMAND_COMPLETION_ITEM_LENGTH - 1,
                     "cat_pl_set_state %s %s", parts[0].c_str(), s.c_str());
            if (count == COMMAND_COMPLETION_MAXITEMS)
                break;
        }
    }
    return count;
}

#if ENABLE_VISUALS
CatCommand pl_set_color("pl_set_color", "pl_set_color uniqueid r g b",
                        [](const CCommand &args) {
                            if (args.ArgC() < 5)
                            {
                                logging::Info("Invalid call");
                                return;
                            }
                            unsigned steamid =
                                strtoul(args.Arg(1), nullptr, 10);
                            int r                     = strtol(args.Arg(2), nullptr, 10);
                            int g                     = strtol(args.Arg(3), nullptr, 10);
                            int b                     = strtol(args.Arg(4), nullptr, 10);
                            rgba_t color              = colors::FromRGBA8(r, g, b, 255);
                            AccessData(steamid).color = color;
                            logging::Info("Changed %d's color", steamid);
                        });
#endif
CatCommand pl_info("pl_info", "pl_info uniqueid", [](const CCommand &args) {
    if (args.ArgC() < 2)
    {
        logging::Info("Invalid call");
        return;
    }
    unsigned steamid;
    try
    {
        steamid = strtoul(args.Arg(1), nullptr, 10);
    }
    catch (std::invalid_argument)
    {
        return;
    }
    logging::Info("Data for %i: ", steamid);
    logging::Info("   State: %i", AccessData(steamid).state);
    /*int clr = AccessData(steamid).color;
    if (clr) {
        ConColorMsg(*reinterpret_cast<::Color*>(&clr), "[CUSTOM COLOR]\n");
    }*/
});

static InitRoutine init([]() {
    pl_set_state.cmd->m_bHasCompletionCallback = true;
    pl_set_state.cmd->m_fnCompletionCallback =
        cat_pl_set_state_completionCallback;
});
} // namespace playerlist
