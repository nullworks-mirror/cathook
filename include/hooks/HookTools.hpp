#pragma once
#include <vector>
#include <functional>
#include <string>
#include "core/profiler.hpp"
#include <string>
#include "config.h"
#include "memory"

class HookedFunction;
namespace HookTools
{
std::vector<HookedFunction *> &GetHookedFunctions();
void CM();
void DRAW();
void PAINT();
} // namespace HookTools

enum HookedFunctions_types
{
    // Use CreateMove to run functions that need to run while ingame.
    HF_CreateMove = 0,
    // Use Draw to draw on screen
    HF_Draw,
    // Use Paint to run functions everywhere (including main menu)
    HF_Paint
};

class HookedFunction
{
    std::function<void()> m_func;
    int m_priority;
    std::string m_name;
#if ENABLE_PROFILER
    ProfilerSection section = ProfilerSection("UNNAMED_FUNCTIONS");
#endif
    void init(HookedFunctions_types type, std::string name, int priority, std::function<void()> func)
    {
        switch (type)
        {
        case HF_CreateMove:
            m_name = "CM_";
            break;
        case HF_Draw:
            m_name = "DRAW_";
            break;
        case HF_Paint:
            m_name = "PAINT_";
            break;
        default:
            m_name = "UNDEFINED_";
            break;
        }
        m_name.append(name);
        m_priority = priority;
        m_func     = func;
        m_type     = type;
#if ENABLE_PROFILER
        section.m_name = m_name;
#endif
        HookTools::GetHookedFunctions().push_back(this);
    }

public:
    HookedFunctions_types m_type;
    bool run(HookedFunctions_types type)
    {
        if (m_type == type)
        {
#if ENABLE_PROFILER
            ProfilerNode node(section);
#endif
            m_func();
            return true;
        }
        return false;
    }
    bool operator>(HookedFunction const &other)
    {
        if (this->m_type < other.m_type)
            return true;
        return this->m_priority > other.m_priority;
    }
    HookedFunction(HookedFunctions_types type, std::string name, int priority, std::function<void()> func)
    {
        init(type, name, priority, func);
    }
    HookedFunction(HookedFunctions_types type, int priority, std::function<void()> func)
    {
        static const std::string name("UNNAMED_FUNCTIONS");
        init(type, name, priority, func);
    }
    HookedFunction(HookedFunctions_types type, std::string name, std::function<void()> func)
    {
        int priority = 5;
        init(type, name, priority, func);
    }
    HookedFunction(HookedFunctions_types type, std::function<void()> func)
    {
        static const std::string name("UNNAMED_FUNCTIONS");
        int priority = 5;
        init(type, name, priority, func);
    }
};

// struct CreateMove
//{
//    int priority = 0;
//    CreateMove(int priority, std::function<void()> func);
//    CreateMove(std::function<void()> func);
//};
