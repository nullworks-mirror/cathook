/*
  Created on 23.06.18.
*/

#pragma once
#include "config.h"
#if ENABLE_ONLINE
#include <string>
#if ENABLE_VISUALS
#include <colors.hpp>
#endif
#include <config.h>
#include <vector>

namespace online
{

struct user_data
{
    bool is_anonymous{ false };
    bool is_using_friendly_software{ false };
    bool is_steamid_verified{ false };
    std::string username{};
    std::vector<std::string> shown_groups{};
    std::string software_name{};
    bool has_software{ false };
    bool no_target{ false };
    bool is_developer{};
    bool has_color{ false };
#if ENABLE_VISUALS
    colors::rgba_t color{};
#endif
    bool rainbow{ false };
};

/*
 * Identify unidentified users, send online status, etc
 */
void update();

user_data *getUserData(unsigned steamId);
} // namespace online
#endif
