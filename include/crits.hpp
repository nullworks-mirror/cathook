#pragma once
#include "settings/Bool.hpp"
namespace criticals
{
extern settings::Boolean enabled;
extern settings::Boolean melee;
extern std::vector<int> crit_cmds;
extern int current_index;
extern bool isEnabled();
extern settings::Boolean old_mode;
} // namespace criticals
