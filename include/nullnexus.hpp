#pragma once
#include "config.h"
#if ENABLE_NULLNEXUS
#include <string>

namespace nullnexus
{
bool sendmsg(std::string &msg);
void auth(bool reply = false);
} // namespace NullNexus

#endif
