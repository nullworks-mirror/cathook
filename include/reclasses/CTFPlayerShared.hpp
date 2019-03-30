#include "reclasses.hpp"

namespace re
{

class CTFPlayerShared
{
public:
    inline static bool IsDominatingPlayer(CTFPlayerShared *self, int ent_idx)
    {
        static auto signature = gSignatures.GetClientSignature("55 89 E5 56 53 83 EC ? 8B 5D ? E8 ? ? ? ? 89 C6 31 C0");
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
