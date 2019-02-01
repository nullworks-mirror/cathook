/*
  Created by Jenny White on 29.04.18.
  Copyright (c) 2018 nullworks. All rights reserved.
*/

#include "MiscTemporary.hpp"
std::array<Timer, 32> timers{};
std::array<int, 32> bruteint{};

int spectator_target;
CLC_VoiceData *voicecrash{};
bool firstcm = false;
Timer DelayTimer{};
float prevflow            = 0.0f;
int prevflowticks         = 0;
bool calculated_can_shoot = false;

bool *bSendPackets{ nullptr };

settings::Bool crypt_chat{ "chat.crypto", "true" };
settings::Bool clean_screenshots{ "visual.clean-screenshots", "false" };
settings::Bool nolerp{ "misc.no-lerp", "false" };
settings::Bool no_zoom{ "remove.scope", "false" };
settings::Bool disable_visuals{ "visual.disable", "false" };
settings::Int print_r{ "print.rgb.r", "183" };
settings::Int print_g{ "print.rgb.b", "27" };
settings::Int print_b{ "print.rgb.g", "139" };
