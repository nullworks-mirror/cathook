/*
  Created by Jenny White on 29.04.18.
  Copyright (c) 2018 nullworks. All rights reserved.
*/

#include "MiscTemporary.hpp"

CatVar minigun_jump(CV_SWITCH, "minigun_jump", "0", "TF2C minigun jump",
                    "Allows jumping while shooting with minigun");

CatVar jointeam(CV_SWITCH, "fb_autoteam", "1",
                "Joins player team automatically (NYI)");
CatVar joinclass(CV_STRING, "fb_autoclass", "spy",
                 "Class that will be picked after joining a team (NYI)");

CatVar nolerp(CV_SWITCH, "nolerp", "0", "NoLerp mode (experimental)");

CatVar engine_pred(CV_SWITCH, "engine_prediction", "0", "Engine Prediction");
CatVar debug_projectiles(CV_SWITCH, "debug_projectiles", "0",
                         "Debug Projectiles");

CatVar fakelag_amount(CV_INT, "fakelag", "0", "Bad Fakelag");
CatVar serverlag_amount(
    CV_INT, "serverlag", "0", "serverlag",
    "Lag the server by spamming this many voicecommands per tick");
CatVar semiauto(CV_INT, "semiauto", "0", "Semiauto");
CatVar servercrash(CV_SWITCH, "servercrash", "0", "crash servers",
                   "Crash servers by spamming signon net messages");
bool *bSendPackets;


CatVar crypt_chat(
        CV_SWITCH, "chat_crypto", "1", "Crypto chat",
        "Start message with !! and it will be only visible to cathook users");

int spectator_target;