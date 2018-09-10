#pragma once
#include <vector>
#include <functional>
#include <init.hpp>
#include "core/logging.hpp"
#include <string>
#include "core/profiler.hpp"
#include <string>
#include "config.h"

class HookedFunction;
namespace HookTools
{
std::vector<HookedFunction*> &GetHookedFunctions();
void CM();
} // namespace HookTools

enum HookedFunctions_types
{
    HF_CreateMove = 0,
    HF_Painttraverse

};

class HookedFunction
{
    std::string m_name;
    std::function<void()> m_func;
    HookedFunctions_types m_type;
    void init(HookedFunctions_types type, std::string name, int priority, std::function<void()> func)
    {
        switch (type)
        {
        case HF_CreateMove:
            m_name = "CM_";
            break;
        case HF_Painttraverse:
            m_name = "PT_";
            break;
        }
        m_name.append(name);
        m_priority = priority;
        m_func     = func;
        m_type     = type;
        HookTools::GetHookedFunctions().push_back(this);
    }
public:
    int m_priority;
    void run(HookedFunctions_types type)
    {
        if (m_type == type)
        {
#if ENABLE_PROFILER
            static ProfilerSection section(m_name);
            ProfilerNode node(section);
#endif
            m_func();
        }
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
