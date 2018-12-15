#pragma once

#include "core/profiler.hpp"
#include "functional"
#include <set>

namespace EC
{

enum ec_types : int8_t
{
    CreateMove = 0,
#if ENABLE_VISUALS
    Draw,
#endif
    Paint,
    LevelInit
};

enum priority : int8_t
{
    very_early = -2,
    early,
    average,
    late,
    very_late
};

template <ec_types T> struct EventCallbackData
{
    explicit EventCallbackData(std::function<void()> function, std::string name, int8_t priority) : function{ function }, priority{ priority }, section{ name }
    {
    }
    std::function<void()> function;
    int8_t priority;
    mutable ProfilerSection section;
    bool operator<(const EventCallbackData &other) const
    {
        return priority < other.priority;
    }
};

extern std::multiset<EventCallbackData<CreateMove>> createmoves;
#if ENABLE_VISUALS
extern std::multiset<EventCallbackData<Draw>> draws;
#endif
extern std::multiset<EventCallbackData<Paint>> paints;
extern std::multiset<EventCallbackData<LevelInit>> levelinits;

template <ec_types T> void Register(std::function<void()> function, std::string name, int8_t priority)
{
    switch (T)
    {
    case CreateMove:
    {
        EventCallbackData<CreateMove> data(function, name, priority);
        createmoves.insert(data);
        break;
    }
#if ENABLE_VISUALS
    case Draw:
    {
        EventCallbackData<Draw> data(function, name, priority);
        draws.insert(data);
        break;
    }
#endif
    case Paint:
    {
        EventCallbackData<Paint> data(function, name, priority);
        paints.insert(data);
        break;
    }
    case LevelInit:
    {
        EventCallbackData<LevelInit> data(function, name, priority);
        levelinits.insert(data);
        break;
    }
    default:
        throw(std::invalid_argument("Unknown event"));
        break;
    }
}

void RunCreateMove();
#if ENABLE_VISUALS
void RunDraw();
#endif
void RunPaint();

void RunLevelInit();
} // namespace EC
