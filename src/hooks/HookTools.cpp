#include "common.hpp"
#include "HookTools.hpp"

namespace EC
{
// Ordered set to always keep priorities correct
std::multiset<EventCallbackData<CreateMove>> createmoves;
#if ENABLE_VISUALS
std::multiset<EventCallbackData<Draw>> draws;
#endif
std::multiset<EventCallbackData<Paint>> paints;
std::multiset<EventCallbackData<LevelInit>> levelinits;

template <typename t> inline void run(t &set)
{
    for (auto &i : set)
    {
#if ENABLE_PROFILER
        ProfilerNode node(i.section);
#endif
        i.function();
    }
}

void RunCreateMove()
{
    run(createmoves);
}
#if ENABLE_VISUALS
void RunDraw()
{
    run(draws);
}
#endif
void RunPaint()
{
    run(paints);
}

void RunLevelInit()
{
    run(levelinits);
}

} // namespace EC
