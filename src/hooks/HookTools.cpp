#include "common.hpp"
#include "HookTools.hpp"

namespace EC
{

struct EventCallbackData
{
    explicit EventCallbackData(const EventFunction &function, std::string name, enum ec_priority priority) : function{ function }, priority{ int(priority) }, event_name{ name }, section{ name }
    {
    }
    EventFunction function;
    int priority;
    mutable ProfilerSection section;
    std::string event_name;
    bool operator<(const EventCallbackData &other) const
    {
        return priority < other.priority;
    }
};
// Ordered set to always keep priorities correct
static std::multiset<EventCallbackData> events[ec_types::EcTypesSize];

void Register(enum ec_types type, const EventFunction &function, const std::string &name, enum ec_priority priority)
{
    events[type].insert(EventCallbackData(function, name, priority));
}

void Unregister(enum ec_types type, const std::string &name)
{
    auto &e = events[type];
    for (auto it = e.begin(); it != e.end(); ++it)
        if (it->event_name == name)
        {
            e.erase(it);
            break;
        }
}

void run(ec_types type)
{
    const auto &set = events[type];
    for (auto &i : set)
    {
#if ENABLE_PROFILER
        ProfilerNode node(i.section);
#endif
        i.function();
    }
}

} // namespace EC
