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
    LevelInit,
    LevelShutdown,
    Shutdown,
    /* Append new event above this line. Line below declares amount of events */
    EcTypesSize
};

enum priority : int8_t
{
    very_early = -2,
    early,
    average,
    late,
    very_late
};

typedef std::function<void()> EventFunction;
void Register(enum ec_types type, const EventFunction &function, const std::string &name, enum ec_priority priority = average);
void Unregister(enum ec_types type, const std::string &name);
void run(enum ec_types type);
} // namespace EC
