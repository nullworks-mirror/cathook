#pragma once
#include "common.hpp"
struct CSndInfo_t
{
    Vector m_pOrigin;
    int m_nSoundSource;
};

struct SoundStruct
{
    CSndInfo_t sound;
    Timer last_update;
};
extern std::map<int, SoundStruct> sound_cache;
