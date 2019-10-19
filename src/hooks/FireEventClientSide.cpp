/*
  Created by Jenny White on 29.04.18.
  Copyright (c) 2018 nullworks. All rights reserved.
*/

#include "HookedMethods.hpp"

namespace hacks::tf2::killstreak
{
extern void fire_event(IGameEvent *event);
}
namespace hooked_methods
{

#if !ENFORCE_STREAM_SAFETY
static settings::Boolean enable_antispam{ "chat.party.anticrash.enabled", "true" };
#else
static settings::Boolean enable_antispam{ "chat.party.anticrash.enabled", "false" };
#endif

// max messages in 5 seconds before being flagged
static settings::Int spam_rate{ "chat.party.anticrash.limit-rate", "12" };

int messages_since_reset{ 0 };
Timer spam_timer{};

DEFINE_HOOKED_METHOD(FireEventClientSide, bool, IGameEventManager2 *this_, IGameEvent *event)
{
    if (enable_antispam)
        if (!strcmp(event->GetName(), "party_chat"))
        {
            // You have violated the law, please sit in timeout for your sins
            if (messages_since_reset >= *spam_rate)
            {
                // Way to reuse same timer
                if (messages_since_reset == *spam_rate)
                {
                    spam_timer.test_and_set(0);
                    messages_since_reset++;
                }
                // You're free, now flee
                if (spam_timer.check(15000))
                    messages_since_reset = 0;
                // How about we don't send that anywhere m'lad
                else
                    return true;
            }

            // Not banned yet
            if (messages_since_reset < 12)
            {
                // Way to reset timer every 5 seconds
                if (spam_timer.check(5000))
                {
                    spam_timer.test_and_set(0);
                    messages_since_reset = 0;
                }
                // Increment spam counter
                messages_since_reset++;
            }
        }
    hacks::tf2::killstreak::fire_event(event);
    return original::FireEventClientSide(this_, event);
}
} // namespace hooked_methods
