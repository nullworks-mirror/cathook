#pragma once

#include "reclasses.hpp"
#include "e8call.hpp"

namespace re
{

class CTFPlayerShared
{
public:
    // Convert IClientEntity to CTFPlayerShared
    inline static CTFPlayerShared *GetPlayerShared(IClientEntity *ent)
    {
        return (CTFPlayerShared *) (((uintptr_t) ent) + 0x17cc);
    }
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
    // Get max charge turning (yaw) to prevent glitchy movement
    inline static float CalculateChargeCap(CTFPlayerShared *self)
    {
        static auto signature = gSignatures.GetClientSignature("55 89 E5 57 56 53 83 EC 6C C7 45 ? 00 00 00 00 8B 45 ? C7 45 ? 00 00 00 00 8B 98");
        typedef float (*CalculateChargeCap_t)(re::CTFPlayerShared *);
        static CalculateChargeCap_t CalculateChargeCap_fn = CalculateChargeCap_t(signature);
        return CalculateChargeCap_fn(self);
    }
};
} // namespace re
