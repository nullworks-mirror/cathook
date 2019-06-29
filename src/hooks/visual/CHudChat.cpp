#include "HookedMethods.hpp"
#include "MiscTemporary.hpp"

namespace hooked_methods
{
DEFINE_HOOKED_METHOD(StartMessageMode, int, CHudBaseChat *_this, int mode)
{
    ignoreKeys = true;
    return original::StartMessageMode(_this, mode);
}
DEFINE_HOOKED_METHOD(StopMessageMode, void *, CHudBaseChat *_this)
{
    ignoreKeys = false;
    return original::StopMessageMode(_this);
}
DEFINE_HOOKED_METHOD(ChatPrintf, void, CHudBaseChat *_this, int player_idx, int iFilter, const char *str, ...)
{
    auto buf = std::make_unique<char[]>(1024);
    va_list list;
    va_start(list, str);
    vsprintf(buf.get(), str, list);
    va_end(list);

    if (*clean_chat && isHackActive())
    {
        std::string result = buf.get();

        ReplaceString(result, "\n", "");
        ReplaceString(result, "\r", "");
        return original::ChatPrintf(_this, player_idx, iFilter, "%s", result.c_str());
    }
    return original::ChatPrintf(_this, player_idx, iFilter, "%s", buf.get());
}
} // namespace hooked_methods
