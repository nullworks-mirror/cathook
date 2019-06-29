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
Timer LookAtPathTimer{};
float prevflow            = 0.0f;
int prevflowticks         = 0;
int stored_buttons        = 0;
bool calculated_can_shoot = false;
bool ignoredc             = false;

bool *bSendPackets{ nullptr };
bool ignoreKeys{ false };
settings::Boolean clean_chat{ "chat.clean", "false" };

settings::Boolean crypt_chat{ "chat.crypto", "true" };
settings::Boolean clean_screenshots{ "visual.clean-screenshots", "false" };
settings::Boolean nolerp{ "misc.no-lerp", "false" };
settings::Boolean no_zoom{ "remove.scope", "false" };
settings::Boolean disable_visuals{ "visual.disable", "false" };
settings::Int print_r{ "print.rgb.r", "183" };
settings::Int print_g{ "print.rgb.b", "27" };
settings::Int print_b{ "print.rgb.g", "139" };
settings::Boolean null_graphics("hack.nullgraphics", "false");
