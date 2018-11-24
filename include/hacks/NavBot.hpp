#pragma once

#include "common.hpp"
namespace hacks::tf2::NavBot
{
bool init(bool first_cm);
namespace task
{
enum task : uint8_t
{
    none = 0,
    sniper_spot,
    stay_near,
    health,
    ammo
};
}
struct bot_class_config
{
    float min;
    float preferred;
    float max;
};
} // namespace hacks::tf2::NavBot
