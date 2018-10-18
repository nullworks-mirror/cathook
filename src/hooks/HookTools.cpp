#include "HookTools.hpp"
#include "common.hpp"

std::vector<HookedFunction *> &HookTools::GetHookedFunctions()
{
    static std::vector<HookedFunction *> CreateMoves{};
    return CreateMoves;
}

// -----------------------------------------------------------

void RunHookedFunctions(HookedFunctions_types type)
{
    auto &HookedFunctions = HookTools::GetHookedFunctions();
    for (auto &i : HookedFunctions)
        i->run(type);
}

void HookTools::CM()
{
    RunHookedFunctions(HF_CreateMove);
}

void HookTools::DRAW()
{
    RunHookedFunctions(HF_Draw);
}

void HookTools::PAINT()
{
    RunHookedFunctions(HF_Paint);
}

static InitRoutine init([]() {
    auto &HookedFunctions = HookTools::GetHookedFunctions();
    logging::Info("Hooked Functions amount: %i", HookedFunctions.size());
    std::sort(HookedFunctions.begin(), HookedFunctions.end(),
              [](HookedFunction *a, HookedFunction *b) { return *a > *b; });
    logging::Info("Sorted Hooked Functions: %i", HookedFunctions.size());
});

static CatCommand print("debug_print_hookedfunctions",
                        "Print hooked functions (CreateMove, Draw, Paint)",
                        []() {

                        });
