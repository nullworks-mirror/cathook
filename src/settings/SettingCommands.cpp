#include <core/sdk.hpp>
#include <core/cvwrapper.hpp>
#include <settings/Manager.hpp>
#include <init.hpp>
#include <settings/SettingsIO.hpp>
#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <thread>

/*
  Created on 29.07.18.
*/

static void getAndSortAllConfigs();

static CatCommand cat("cat", "", [](const CCommand &args) {
    if (args.ArgC() < 3)
    {
        g_ICvar->ConsolePrintf("Usage: cat <set/get> <variable> [value]\n");
        return;
    }

    auto variable = settings::Manager::instance().lookup(args.Arg(2));
    if (variable == nullptr)
    {
        g_ICvar->ConsolePrintf("Variable not found: %s\n", args.Arg(2));
        return;
    }

    if (!strcmp(args.Arg(1), "set"))
    {
        if (args.ArgC() < 4)
        {
            g_ICvar->ConsolePrintf("Usage: cat <set> <variable> <value>\n");
            return;
        }
        variable->fromString(args.Arg(3));
        g_ICvar->ConsolePrintf("%s = \"%s\"\n", args.Arg(2),
                               variable->toString().c_str());
        return;
    }
    else if (!strcmp(args.Arg(1), "get"))
    {
        g_ICvar->ConsolePrintf("%s = \"%s\"\n", args.Arg(2),
                               variable->toString().c_str());
        return;
    }
    else
    {
        g_ICvar->ConsolePrintf("Usage: cat <set/get> <variable> <value>\n");
        return;
    }
});

void save_thread(const CCommand &args)
{
    std::this_thread::sleep_for(std::chrono_literals::operator""s(1));
    settings::SettingsWriter writer{ settings::Manager::instance() };

    DIR *config_directory = opendir(DATA_PATH "/configs");
    if (!config_directory)
    {
        logging::Info("Configs directory doesn't exist, creating one!");
        mkdir(DATA_PATH "/configs", S_IRWXU | S_IRWXG);
    }

    if (args.ArgC() == 1)
    {
        writer.saveTo(DATA_PATH "/configs/default.conf", false);
    }
    else
    {
        writer.saveTo(
            std::string(DATA_PATH "/configs/") + args.Arg(1) + ".conf", false);
    }
    logging::Info("cat_save: Sorting configs...");
    getAndSortAllConfigs();
    logging::Info("cat_save: Closing dir...");
    closedir(config_directory);
    logging::Info("cat_save: Enabeling cathook...");
    settings::RVarLock.store(false);
}

static CatCommand save("save", "", [](const CCommand &args) {
    if (!settings::RVarLock.load())
    {
        settings::RVarLock.store(true);
        std::thread loader(save_thread, args);
        loader.detach();
    }
});

void load_thread(const CCommand &args)
{
#if not ENABLE_VISUALS
    std::this_thread::sleep_for(std::chrono_literals::operator""s(1));
    settings::RVarLock.store(true);
#endif
    std::this_thread::sleep_for(std::chrono_literals::operator""s(1));
    settings::SettingsReader loader{ settings::Manager::instance() };
    if (args.ArgC() == 1)
    {
        loader.loadFrom(DATA_PATH "/configs/default.conf");
    }
    else
    {
#if ENABLE_VISUALS
        loader.loadFrom(std::string(DATA_PATH "/configs/") + args.Arg(1) +
                        ".conf");
#else
        for (int i = 0;; i++)
        {
            if (loader.loadFrom(std::string(DATA_PATH "/configs/") +
                                args.Arg(1) + ".conf"))
                break;
            if (i > 5)
            {
                logging::Info("cat_load: Force crash. Couldn't load config");
                std::terminate();
            }
            std::this_thread::sleep_for(std::chrono_literals::operator""s(3));
        }
#endif
    }
    settings::RVarLock.store(false);
}

static CatCommand load("load", "", [](const CCommand &args) {
    if (!settings::RVarLock.load())
    {
#if ENABLE_VISUALS
        settings::RVarLock.store(true);
#endif
        std::thread saver(load_thread, args);
        saver.detach();
    }
});

static std::vector<std::string> sortedVariables{};

static void getAndSortAllVariables()
{
    for (auto &v : settings::Manager::instance().registered)
    {
        sortedVariables.push_back(v.first);
    }

    std::sort(sortedVariables.begin(), sortedVariables.end());

    logging::Info("Sorted %u variables\n", sortedVariables.size());
}

static std::vector<std::string> sortedConfigs{};

static void getAndSortAllConfigs()
{
    DIR *config_directory = opendir(DATA_PATH "/configs");
    if (!config_directory)
    {
        logging::Info("Config directoy does not exist.");
        closedir(config_directory);
        return;
    }
    sortedConfigs.clear();

    struct dirent *ent;
    while ((ent = readdir(config_directory)))
    {
        std::string s(ent->d_name);
        s = s.substr(0, s.find_last_of("."));
        sortedConfigs.push_back(s);
    }
    std::sort(sortedConfigs.begin(), sortedConfigs.end());
    sortedConfigs.erase(sortedConfigs.begin());
    sortedConfigs.erase(sortedConfigs.begin());

    closedir(config_directory);
    logging::Info("Sorted %u config files\n", sortedConfigs.size());
}

static int cat_completionCallback(
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

    // "" -> cat [get, set]
    // "g" -> cat get
    // "get " -> cat get <variable>

    logging::Info("%s|%s", parts[0].c_str(), parts[1].c_str());

    if (parts[0].empty() ||
        parts[1].empty() && (!parts[0].empty() && partial.back() != ' '))
    {
        if (std::string("get").find(parts[0]) != std::string::npos)
            snprintf(commands[count++], COMMAND_COMPLETION_ITEM_LENGTH,
                     "cat get ");
        if (std::string("set").find(parts[0]) != std::string::npos)
            snprintf(commands[count++], COMMAND_COMPLETION_ITEM_LENGTH,
                     "cat set ");
        return count;
    }

    for (const auto &s : sortedVariables)
    {
        if (s.find(parts[1]) == 0)
        {
            auto variable = settings::Manager::instance().lookup(s);
            if (variable)
            {
                snprintf(commands[count++], COMMAND_COMPLETION_ITEM_LENGTH - 1,
                         "cat %s %s %s", parts[0].c_str(), s.c_str(),
                         variable->toString().c_str());
                if (count == COMMAND_COMPLETION_MAXITEMS)
                    break;
            }
        }
    }
    return count;
}

static int load_completionCallback(
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

    for (const auto &s : sortedConfigs)
    {
        if (s.find(parts[0]) == 0)
        {
            snprintf(commands[count++], COMMAND_COMPLETION_ITEM_LENGTH - 1,
                     "cat_load %s", s.c_str());
            if (count == COMMAND_COMPLETION_MAXITEMS)
                break;
        }
    }
    return count;
}

static InitRoutine init([]() {
    getAndSortAllVariables();
    getAndSortAllConfigs();
    cat.cmd->m_bHasCompletionCallback  = true;
    cat.cmd->m_fnCompletionCallback    = cat_completionCallback;
    load.cmd->m_bHasCompletionCallback = true;
    load.cmd->m_fnCompletionCallback   = load_completionCallback;
});
