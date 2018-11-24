#pragma once

#include "common.hpp"
namespace hacks::tf2::NavBot
{
bool init(bool first_cm);
namespace task
{
enum task : u_int8_t
{
    none = 0,
    sniper_spot,
    stay_near,
};
}
} // namespace hacks::tf2::NavBot
