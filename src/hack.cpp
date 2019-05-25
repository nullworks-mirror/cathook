/*
 * hack.cpp
 *
 *  Created on: Oct 3, 2016
 *      Author: nullifiedcat
 */

#define __USE_GNU
#include <execinfo.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <dlfcn.h>
#include <boost/stacktrace.hpp>
#include <cxxabi.h>
#include <visual/SDLHooks.hpp>
#include "hack.hpp"
#include "common.hpp"
#include "MiscTemporary.hpp"

#include <hacks/hacklist.hpp>

#define STRINGIFY(x) #x
#define TO_STRING(x) STRINGIFY(x)

#include "copypasted/CDumper.hpp"
#include "version.h"
#include <cxxabi.h>

/*
 *  Credits to josh33901 aka F1ssi0N for butifel F1Public and Darkstorm 2015
 * Linux
 */

// game_shutdown = Is full game shutdown or just detach
bool hack::game_shutdown = true;
bool hack::shutdown      = false;
bool hack::initialized   = false;

const std::string &hack::GetVersion()
{
    static std::string version("Unknown Version");
    static bool version_set = false;
    if (version_set)
        return version;
#if defined(GIT_COMMIT_HASH) && defined(GIT_COMMITTER_DATE)
    version = "Version: #" GIT_COMMIT_HASH " " GIT_COMMITTER_DATE;
#endif
    version_set = true;
    return version;
}

const std::string &hack::GetType()
{
    static std::string version("Unknown Type");
    static bool version_set = false;
    if (version_set)
        return version;
    version = "";
#if not ENABLE_IPC
    version += " NOIPC";
#endif
#if not ENABLE_GUI
    version += " NOGUI";
#else
    version += " GUI";
#endif

#ifndef DYNAMIC_CLASSES

#ifdef GAME_SPECIFIC
    version += " GAME " TO_STRING(GAME);
#else
    version += " UNIVERSAL";
#endif

#else
    version += " DYNAMIC";
#endif

#if not ENABLE_VISUALS
    version += " NOVISUALS";
#endif

    version     = version.substr(1);
    version_set = true;
    return version;
}

std::mutex hack::command_stack_mutex;
std::stack<std::string> &hack::command_stack()
{
    static std::stack<std::string> stack;
    return stack;
}

void hack::ExecuteCommand(const std::string &command)
{
    std::lock_guard<std::mutex> guard(hack::command_stack_mutex);
    hack::command_stack().push(command);
}

#if ENABLE_LOGGING
void critical_error_handler(int signum)
{
    namespace st = boost::stacktrace;
    ::signal(signum, SIG_DFL);
    passwd *pwd = getpwuid(getuid());
    std::ofstream out(strfmt("/tmp/cathook-%s-%d-segfault.log", pwd->pw_name, getpid()).get());

    Dl_info info;
    if (!dladdr(reinterpret_cast<void *>(hack::ExecuteCommand), &info))
        return;
    unsigned int baseaddr = (unsigned int) info.dli_fbase - 1;

    for (auto i : st::stacktrace())
    {
        unsigned int offset = (unsigned int) (i.address()) - baseaddr;
        Dl_info info2;
        out << (void *) offset;
        if (!dladdr(i.address(), &info2))
        {
            out << std::endl;
            continue;
        }
        out << " " << info2.dli_fname << ": ";
        if (info2.dli_sname)
        {
            int status     = -4;
            char *realname = abi::__cxa_demangle(info2.dli_sname, nullptr, nullptr, &status);
            if (status == 0)
            {
                out << realname << std::endl;
                free(realname);
            }
            else
                out << info2.dli_sname << std::endl;
        }
        else
            out << "No Symbol" << std ::endl;
    }

    out.close();
    ::raise(SIGABRT);
}
#endif

static bool blacklist_file(const char *filename)
{
    const static char *blacklist[] = { ".vtx", ".vtf", ".pcf", ".mdl" };
    if (!filename || !std::strcmp(filename, "models/error.mdl") || !std::strncmp(filename, "models/buildables", 17) || !std::strcmp(filename, "models/vgui/competitive_badge.mdl") || !std::strcmp(filename, "models/vgui/12v12_badge.mdl") || !std::strncmp(filename, "models/player/", 14) || !std::strncmp(filename, "models/weapons/", 15))
        return false;

    std::size_t len = std::strlen(filename);
    if (len > 3)
    {
        auto ext_p = filename + len - 4;
        for (int i = 0; i < sizeof(blacklist) / sizeof(blacklist[0]); ++i)
            if (!std::strcmp(ext_p, blacklist[i]))
                return true;
    }
    return false;
}

static bool (*FSorig_ReadFile)(void *, const char *, const char *, void *, int, int, void *);
static bool FSHook_ReadFile(void *this_, const char *pFileName, const char *pPath, void *buf, int nMaxBytes, int nStartingByte, void *pfnAlloc)
{
    // fprintf(stderr, "ReadFile: %s\n", pFileName);
    if (blacklist_file(pFileName))
        return false;
    return FSorig_ReadFile(this_, pFileName, pPath, buf, nMaxBytes, nStartingByte, pfnAlloc);
}

static hooks::VMTHook /*fs_hook,*/ fs_hook2;
static void ReduceRamUsage()
{
    fs_hook2.Set(reinterpret_cast<void *>(g_IFileSystem), 4);
    fs_hook2.HookMethod(FSHook_ReadFile, 14, &FSorig_ReadFile);
    fs_hook2.Apply();

    /* ERROR: Must be called from texture thread */
    // g_IMaterialSystem->ReloadTextures();
    g_IBaseClient->InvalidateMdlCache();
}

static void UnHookFs()
{
    fs_hook2.Release();
    g_IBaseClient->InvalidateMdlCache();
}

static void InitRandom()
{
    int rand_seed;
    FILE *rnd = fopen("/dev/urandom", "rb");
    if (!rnd || fread(&rand_seed, sizeof(rand_seed), 1, rnd) < 1)
    {
        logging::Info("Warning!!! Failed read from /dev/urandom (%s). Randomness is going to be weak", strerror(errno));
        timespec t;
        clock_gettime(CLOCK_MONOTONIC, &t);
        rand_seed = t.tv_nsec ^ t.tv_sec & getpid();
    }
    srand(rand_seed);
    if (rnd)
        fclose(rnd);
}

void hack::Initialize()
{
#if ENABLE_LOGGING
    ::signal(SIGSEGV, &critical_error_handler);
    ::signal(SIGABRT, &critical_error_handler);
#endif
    time_injected = time(nullptr);
/*passwd *pwd   = getpwuid(getuid());
char *logname = strfmt("/tmp/cathook-game-stdout-%s-%u.log", pwd->pw_name,
time_injected);
freopen(logname, "w", stdout);
free(logname);
logname = strfmt("/tmp/cathook-game-stderr-%s-%u.log", pwd->pw_name,
time_injected);
freopen(logname, "w", stderr);
free(logname);*/
// Essential files must always exist, except when the game is running in text
// mode.
#if ENABLE_VISUALS

    {
        std::vector<std::string> essential = { "fonts/tf2build.ttf" };
        for (const auto &s : essential)
        {
            std::ifstream exists(DATA_PATH "/" + s, std::ios::in);
            if (not exists)
            {
                Error(("Missing essential file: " + s +
                       "/%s\nYou MUST run install-data script to finish "
                       "installation")
                          .c_str(),
                      s.c_str());
            }
        }
    }

#endif /* TEXTMODE */
    logging::Info("Initializing...");
    InitRandom();
    sharedobj::LoadAllSharedObjects();
    CreateInterfaces();
    CDumper dumper;
    null_graphics.installChangeCallback([](settings::VariableBase<bool> &, bool after) {
        if (after)
            ReduceRamUsage();
        else
            UnHookFs();
    });
    dumper.SaveDump();
    logging::Info("Is TF2? %d", IsTF2());
    logging::Info("Is TF2C? %d", IsTF2C());
    logging::Info("Is HL2DM? %d", IsHL2DM());
    logging::Info("Is CSS? %d", IsCSS());
    logging::Info("Is TF? %d", IsTF());
    InitClassTable();

    BeginConVars();
    g_Settings.Init();
    EndConVars();

#if ENABLE_VISUALS
    draw::Initialize();
#if ENABLE_GUI
// FIXME put gui here
#endif

#endif /* TEXTMODE */

    gNetvars.init();
    InitNetVars();
    g_pLocalPlayer    = new LocalPlayer();
    g_pPlayerResource = new TFPlayerResource();

    uintptr_t *clientMode = 0;
    // Bad way to get clientmode.
    // FIXME [MP]?
    while (!(clientMode = **(uintptr_t ***) ((uintptr_t)((*(void ***) g_IBaseClient)[10]) + 1)))
    {
        usleep(10000);
    }
    hooks::clientmode.Set((void *) clientMode);
    hooks::clientmode.HookMethod(HOOK_ARGS(CreateMove));
#if ENABLE_VISUALS
    hooks::clientmode.HookMethod(HOOK_ARGS(OverrideView));
#endif
    hooks::clientmode.HookMethod(HOOK_ARGS(LevelInit));
    hooks::clientmode.HookMethod(HOOK_ARGS(LevelShutdown));
    hooks::clientmode.Apply();

    hooks::clientmode4.Set((void *) (clientMode), 4);
    hooks::clientmode4.HookMethod(HOOK_ARGS(FireGameEvent));
    hooks::clientmode4.Apply();

    hooks::client.Set(g_IBaseClient);
    hooks::client.HookMethod(HOOK_ARGS(DispatchUserMessage));
#if ENABLE_VISUALS
    hooks::client.HookMethod(HOOK_ARGS(FrameStageNotify));
    hooks::client.HookMethod(HOOK_ARGS(IN_KeyEvent));
#endif
    hooks::client.Apply();

#if ENABLE_VISUALS
    hooks::vstd.Set((void *) g_pUniformStream);
    hooks::vstd.HookMethod(HOOK_ARGS(RandomInt));
    hooks::vstd.Apply();

    hooks::panel.Set(g_IPanel);
    hooks::panel.HookMethod(hooked_methods::methods::PaintTraverse, offsets::PaintTraverse(), &hooked_methods::original::PaintTraverse);
    hooks::panel.Apply();
#endif

    hooks::input.Set(g_IInput);
    hooks::input.HookMethod(HOOK_ARGS(GetUserCmd));
    hooks::input.Apply();

    hooks::modelrender.Set(g_IVModelRender);
    hooks::modelrender.HookMethod(HOOK_ARGS(DrawModelExecute));
    hooks::modelrender.Apply();

    hooks::enginevgui.Set(g_IEngineVGui);
    hooks::enginevgui.HookMethod(HOOK_ARGS(Paint));
    hooks::enginevgui.Apply();

    hooks::engine.Set(g_IEngine);
    hooks::engine.HookMethod(HOOK_ARGS(IsPlayingTimeDemo));
    hooks::engine.Apply();

    hooks::eventmanager2.Set(g_IEventManager2);
    hooks::eventmanager2.HookMethod(HOOK_ARGS(FireEvent));
    hooks::eventmanager2.HookMethod(HOOK_ARGS(FireEventClientSide));
    hooks::eventmanager2.Apply();

    hooks::steamfriends.Set(g_ISteamFriends);
    hooks::steamfriends.HookMethod(HOOK_ARGS(GetFriendPersonaName));
    hooks::steamfriends.Apply();

    // FIXME [MP]
    logging::Info("Hooked!");
    velocity::Init();
    playerlist::Load();

#if ENABLE_VISUALS

    InitStrings();
#ifndef FEATURE_EFFECTS_DISABLED
    if (g_ppScreenSpaceRegistrationHead && g_pScreenSpaceEffects)
    {
        effect_chams::g_pEffectChams = new CScreenSpaceEffectRegistration("_cathook_chams", &effect_chams::g_EffectChams);
        g_pScreenSpaceEffects->EnableScreenSpaceEffect("_cathook_chams");
        effect_glow::g_pEffectGlow = new CScreenSpaceEffectRegistration("_cathook_glow", &effect_glow::g_EffectGlow);
        g_pScreenSpaceEffects->EnableScreenSpaceEffect("_cathook_glow");
    }
    logging::Info("SSE enabled..");
#endif
    sdl_hooks::applySdlHooks();
    logging::Info("SDL hooking done");

#endif /* TEXTMODE */
#if ENABLE_VISUALS
#ifndef FEATURE_FIDGET_SPINNER_ENABLED
    InitSpinner();
    logging::Info("Initialized Fidget Spinner");
#endif
#endif
    logging::Info("Clearing initializer stack");
    while (!init_stack().empty())
    {
        init_stack().top()();
        init_stack().pop();
    }
    logging::Info("Initializer stack done");
#if not ENABLE_VISUALS
    hack::command_stack().push("exec cat_autoexec_textmode");
#else
    hack::command_stack().push("exec cat_autoexec");
#endif
    auto extra_exec = std::getenv("CH_EXEC");
    if (extra_exec)
        hack::command_stack().push(extra_exec);

    hack::initialized = true;
    for (int i = 0; i < 12; i++)
    {
        re::ITFMatchGroupDescription *desc = re::GetMatchGroupDescription(i);
        if (!desc || desc->m_iID > 9) // ID's over 9 are invalid
            continue;
        if (desc->m_bForceCompetitiveSettings)
        {
            desc->m_bForceCompetitiveSettings = false;
            logging::Info("Bypassed force competitive cvars!");
        }
    }
}

void hack::Think()
{
    usleep(250000);
}

void hack::Shutdown()
{
    if (hack::shutdown)
        return;
    hack::shutdown = true;
    // Stop cathook stuff
    settings::cathook_disabled.store(true);
    playerlist::Save();
#if ENABLE_VISUALS
    sdl_hooks::cleanSdlHooks();
#endif
    logging::Info("Unregistering convars..");
    ConVar_Unregister();
    logging::Info("Shutting down killsay...");
    if (!hack::game_shutdown)
    {
        EC::run(EC::Shutdown);
#if ENABLE_VISUALS
        g_pScreenSpaceEffects->DisableScreenSpaceEffect("_cathook_glow");
        g_pScreenSpaceEffects->DisableScreenSpaceEffect("_cathook_chams");
#endif
    }
    logging::Info("Success..");
}
