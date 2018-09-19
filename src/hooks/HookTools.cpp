#include "HookTools.hpp"

std::vector<HookedFunction *> &HookTools::GetHookedFunctions()
{
    static std::vector<HookedFunction *> CreateMoves{};
    return CreateMoves;
}

// -----------------------------------------------------------

void HookTools::CM()
{
    for (auto i : GetHookedFunctions())
    {
        i->run(HF_CreateMove);
    }
}

void HookTools::PT()
{
    for (auto i : GetHookedFunctions())
    {
        i->run(HF_Draw);
    }
}

static InitRoutine init([]() {
    auto &HookedFunctions = HookTools::GetHookedFunctions();
    std::sort(HookedFunctions.begin(), HookedFunctions.end(),
              [](HookedFunction *a, HookedFunction *b) {
                  return a->m_priority > b->m_priority;
              });
    logging::Info("Sorted Hooked Functions: %i", HookedFunctions.size());
});
