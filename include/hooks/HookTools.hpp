#pragma once
#include <vector>
#include <functional>
#include <init.hpp>
#include "core/logging.hpp"
#include <string>

class HookedFunction;
namespace HookTools
{
std::vector<HookedFunction*> &GetHookedFunctions();
enum types
{
    CreateMove = 0,
    Painttraverse

};
void CM();

// struct HookedBase
//{
//    int m_priority;
//    std::string m_name;
//    std::function<void()> m_func;
//    HookTools::types m_type;
//};
} // namespace HookTools

class HookedFunction
{
    std::string m_name;
    std::function<void()> m_func;
    HookTools::types m_type;
    void init(HookTools::types type, std::string name, int priority, std::function<void()> func)
    {
        switch (type)
        {
        case HookTools::CreateMove:
            m_name = "CM_";
            break;
        case HookTools::Painttraverse:
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
    void run(HookTools::types type)
    {
        if (m_type == type)
        {
            m_func();
        }
    }
    HookedFunction(HookTools::types type, std::string name, int priority, std::function<void()> func)
    {
        init(type, name, priority, func);
    }
    HookedFunction(HookTools::types type, int priority, std::function<void()> func)
    {
        std::string name("UNNAMED_FUNCTION");
        init(type, name, priority, func);
    }
    HookedFunction(HookTools::types type, std::string name, std::function<void()> func)
    {
        int priority = 5;
        init(type, name, priority, func);
    }
    HookedFunction(HookTools::types type, std::function<void()> func)
    {
        std::string name("UNNAMED_FUNCTION");
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
