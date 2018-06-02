/*
  Created by Jenny White on 29.04.18.
  Copyright (c) 2018 nullworks. All rights reserved.
*/

#pragma once

#include "common.hpp"

// This is a temporary file to put code that needs moving/refactoring in.
extern bool *bSendPackets;
extern CatVar no_zoom;
extern CatVar clean_screenshots;
extern CatVar disable_visuals;
extern CatVar disconnect_reason;
extern CatVar crypt_chat;
extern CatVar minigun_jump;
extern CatVar nolerp;
extern CatVar joinclass;
extern CatVar jointeam;
extern CatVar fakelag_amount;
extern CatVar serverlag_amount;
extern CatVar serverlag_string;
extern CatVar servercrash;
extern CatVar debug_projectiles;
extern CatVar semiauto;
extern CatVar engine_pred;
extern Timer DelayTimer;
extern bool firstcm;
extern CatVar delay;
extern CatVar adjust;
extern float prevflow;
extern int prevflowticks;
#if ENABLE_VISUALS
extern int spectator_target;
extern CLC_VoiceData *voicecrash;
#endif
