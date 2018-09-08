#include "HookTools.hpp"

std::vector<std::pair<int, std::function<void()>>> &GetCreateMoves()
{
    static std::vector<std::pair<int, std::function<void()>>> CreateMoves;
    return CreateMoves;
}

CreateMove::CreateMove(int priority, std::function<void()> func)
{
    auto &CreateMoves = GetCreateMoves();
    CreateMoves.emplace_back(priority, func);
}

// -----------------------------------------------------------

void HookTools::CreateMove()
{
    for (auto i : GetCreateMoves())
    {
        i.second();
    }
}

static InitRoutine init([]() {
    auto &CreateMoves = GetCreateMoves();
    std::sort(CreateMoves.begin(), CreateMoves.end(),
              [](std::pair<int, std::function<void()>> a,
                 std::pair<int, std::function<void()>> b) {
                  return a.first > b.first;
              });
    logging::Info("Sorted CreateMove functions: %i", CreateMoves.size());
});
