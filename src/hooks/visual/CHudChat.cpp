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
} // namespace hooked_methods
