#pragma once

#include "common.hpp"
namespace hacks::tf2::NavBot
{
bool init(bool first_cm);
namespace task
{
enum task : u_int8_t
{
    sniper_spot = 0,
    stay_near = 0,
    none
};
}
} // namespace hacks::tf2::NavBot
