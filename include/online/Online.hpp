/*
  Created on 23.06.18.
*/

#pragma once

#include <string>
#include <colors.hpp>
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
    std::vector<std::string> shown_roles{};
    std::string software_name{};
    bool has_software{ false };
#if ENABLE_VISUALS
    bool has_color{ false };
    colors::rgba_t color{};
    bool rainbow{ false };
#endif
};

/*
 * Identify unidentified users, send online status, etc
 */
void update();

user_data *getUserData(unsigned steamId);

}