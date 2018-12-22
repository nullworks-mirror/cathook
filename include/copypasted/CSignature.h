#pragma once

// Copypasted from Darkstorm 2015 linux base

//#include "SDK.h"
#include <stdint.h>

class CSignature
{
public:
    static uintptr_t dwFindPattern(uintptr_t dwAddress, uintptr_t dwLength, const char *szPattern);
    static void *GetModuleHandleSafe(const char *pszModuleName);
    static uintptr_t GetClientSignature(const char *chPattern);
    static uintptr_t GetEngineSignature(const char *chPattern);
    static uintptr_t GetSteamAPISignature(const char *chPattern);
    static uintptr_t GetVstdSignature(const char *chPattern);
};

extern CSignature gSignatures;
