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
    std::string username{};
    std::vector<std::string> shown_roles{};
#if ENABLE_VISUALS
    colors::rgba_t color{};
    bool rainbow{ false };
#endif
};

/*
 * Initialize everything, login if key is saved
 */
void init();

/*
 * Identify unidentified users, send online status, etc
 */
void update();

user_data *getUserData(unsigned steamId);

}