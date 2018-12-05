#include "config.h"
#if ENABLE_IRC
#include <string>

namespace IRC
{
bool sendmsg(std::string &msg, bool loopback = false);
void auth(bool reply = false);
} // namespace IRC
#endif
