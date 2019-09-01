#include "common.hpp"
#include "HookedMethods.hpp"

namespace hooked_methods
{
DEFINE_HOOKED_METHOD(EmitSound1, void, void *_this, IRecipientFilter &filter, int iEntIndex, int iChannel, const char *pSample, float flVolume, float flAttenuation, int iFlags, int iPitch, int iSpecialDSP, const Vector *pOrigin, const Vector *pDirection, CUtlVector<Vector> *pUtlVecOrigins, bool bUpdatePositions, float soundtime, int speakerentity)
{
    logging::Info("Sound Emitted1");
    return original::EmitSound1(_this, filter, iEntIndex, iChannel, pSample, flVolume, flAttenuation, iFlags, iPitch, iSpecialDSP, pOrigin, pDirection, pUtlVecOrigins, bUpdatePositions, soundtime, speakerentity);
}
DEFINE_HOOKED_METHOD(EmitSound2, void, void *_this, IRecipientFilter &filter, int iEntIndex, int iChannel, const char *pSample, float flVolume, soundlevel_t iSoundlevel, int iFlags, int iPitch, int iSpecialDSP, const Vector *pOrigin, const Vector *pDirection, CUtlVector<Vector> *pUtlVecOrigins, bool bUpdatePositions, float soundtime, int speakerentity)
{
    logging::Info("Sound Emitted2");
    return original::EmitSound2(_this, filter, iEntIndex, iChannel, pSample, flVolume, iSoundlevel, iFlags, iPitch, iSpecialDSP, pOrigin, pDirection, pUtlVecOrigins, bUpdatePositions, soundtime, speakerentity);
}
DEFINE_HOOKED_METHOD(EmitSound3, void, void *_this, IRecipientFilter &filter, int iEntIndex, int iChannel, int iSentenceIndex, float flVolume, soundlevel_t iSoundlevel, int iFlags, int iPitch, int iSpecialDSP, const Vector *pOrigin, const Vector *pDirection, CUtlVector<Vector> *pUtlVecOrigins, bool bUpdatePositions, float soundtime, int speakerentity)
{
    logging::Info("Sound Emitted3");
    return original::EmitSound3(_this, filter, iEntIndex, iChannel, iSentenceIndex, flVolume, iSoundlevel, iFlags, iPitch, iSpecialDSP, pOrigin, pDirection, pUtlVecOrigins, bUpdatePositions, soundtime, speakerentity);
}
} // namespace hooked_methods
