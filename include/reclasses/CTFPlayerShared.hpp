#include "reclasses.hpp"
#include "e8call.hpp"

namespace re
{

class CTFPlayerShared
{
public:
    inline static bool IsDominatingPlayer(CTFPlayerShared *self, int ent_idx)
    {
        static auto signature = e8call_direct(gSignatures.GetClientSignature("E8 ? ? ? ? 84 C0 74 8F"));
        typedef bool (*IsDominatingPlayer_t)(CTFPlayerShared *, int);
        static IsDominatingPlayer_t IsDominatingPlayer_fn = IsDominatingPlayer_t(signature);
        return IsDominatingPlayer_fn(self, ent_idx);
    }
    inline static float GetCritMult(CTFPlayerShared *self)
    {
        return ((fminf(fmaxf(*(float *) (unsigned(self) + 672) * 0.0039215689f, 0.0f), 1.0f) * 3.0f) + 1.0f);
    }
    inline static bool IsCritBoosted(CTFPlayerShared *self)
    {
        // TODO signature
        return false;
    }
};
} // namespace re
