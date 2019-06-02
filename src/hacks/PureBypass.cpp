#include <settings/Bool.hpp>
#include "common.hpp"

namespace hacks::shared::purebypass
{
hooks::VMTHook svpurehook{};
static void (*orig_RegisterFileWhilelist)(void *, void *, void *);
static void RegisterFileWhitelist(void *_this, void *a, void *b)
{
    logging::Info("git gud sv_pure !");
}

static InitRoutine init([] {
    svpurehook.Set(g_IFileSystem);
    svpurehook.HookMethod(RegisterFileWhitelist, offsets::RegisterFileWhitelist(), &orig_RegisterFileWhilelist);
    svpurehook.Apply();
});
} // namespace hacks::shared::purebypass
