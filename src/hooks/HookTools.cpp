#include "HookTools.hpp"
#include "common.hpp"

std::vector<HookedFunction *> &HookTools::GetHookedFunctions()
{
    static std::vector<HookedFunction *> CreateMoves{};
    return CreateMoves;
}

// -----------------------------------------------------------

static std::array<int, 3> bounds{};

void RunHookedFunctions(HookedFunctions_types type)
{
    auto &HookedFunctions = HookTools::GetHookedFunctions();
    for (int i = bounds.at(type); i < HookedFunctions.size(); i++)
    {
        if (!HookedFunctions.at(i)->run(type))
            break;
    }
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
    std::sort(HookedFunctions.begin(), HookedFunctions.end(),
              [](HookedFunction *a, HookedFunction *b) { return *a > *b; });
    logging::Info("Sorted Hooked Functions: %i", HookedFunctions.size());
    for (int i = 0; i < bounds.size(); i++)
    {
        for (int j = 0; j < HookedFunctions.size(); j++)
        {
            if (HookedFunctions.at(j)->m_type == i)
            {
                bounds.at(i) = j;
                break;
            }
        }
    }
    logging::Info(
        "Initialized HookedFunction bounds: CM: %i, Draw: %i, Paint: %i",
        bounds.at(0), bounds.at(1), bounds.at(2));
});

static CatCommand print("debug_print_hookedfunctions",
                        "Print hooked functions (CreateMove, Draw, Paint)",
                        []() {

                        });
