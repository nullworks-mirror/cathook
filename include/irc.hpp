#include <string>

namespace IRC
{
bool sendmsg(std::string &msg, bool loopback = false);
void auth(bool reply = false);
}
