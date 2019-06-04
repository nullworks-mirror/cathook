#pragma once
#include "timer.hpp"
#include "map"
#include "public/mathlib/vector.h"
struct CSndInfo_t
{
    Vector m_pOrigin;
};

struct SoundStruct
{
    CSndInfo_t sound;
    Timer last_update;
};
extern std::map<int, SoundStruct> sound_cache;
