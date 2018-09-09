#include "HookTools.hpp"

std::vector<HookedFunction*> &HookTools::GetHookedFunctions()
{
    static std::vector<HookedFunction*> CreateMoves{};
    return CreateMoves;
}

//CreateMove::CreateMove(int priority, std::function<void()> func)
//{
//    auto &CreateMoves = GetCreateMoves();
//    CreateMoves.emplace_back(priority, func);
//}



// -----------------------------------------------------------

void HookTools::CM()
{
    for (auto i : GetHookedFunctions())
    {
        i->run(HookTools::CreateMove);
    }
}

static InitRoutine init([]() {
    auto &HookedFunctions = HookTools::GetHookedFunctions();
    std::sort(HookedFunctions.begin(), HookedFunctions.end(),
              [](HookedFunction *a, HookedFunction *b){
                return a->m_priority > b->m_priority;
    });
    logging::Info("Sorted Hooked Functions: %i", HookedFunctions.size());
});
