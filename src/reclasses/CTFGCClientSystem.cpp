/*
 * CTFGCClientSystem.cpp
 *
 *  Created on: Dec 7, 2017
 *      Author: nullifiedcat
 */

#include "common.hpp"
#include "e8call.hpp"

re::CTFGCClientSystem *re::CTFGCClientSystem::GTFGCClientSystem()
{
    typedef re::CTFGCClientSystem *(*GTFGCClientSystem_t)();
    static uintptr_t addr1 = gSignatures.GetClientSignature(
        "E8 ? ? ? ? 84 C0 0F 85 7B 02 00 00 E8 ? ? ? ? BE 01 00 00 00 89 04 24 "
        "E8 ? ? ? ? 85 C0 0F 84 E5 02 00 00");
    static GTFGCClientSystem_t GTFGCClientSystem_fn =
        GTFGCClientSystem_t(e8call((void *) (addr1 + 14)));

    return GTFGCClientSystem_fn();
}

void re::CTFGCClientSystem::AbandonCurrentMatch(re::CTFGCClientSystem *this_)
{
    typedef void *(*AbandonCurrentMatch_t)(re::CTFGCClientSystem *);
    static uintptr_t addr1 = gSignatures.GetClientSignature(
        "55 89 E5 57 56 8D 75 C8 53 81 EC 8C 00 00 00 C7 04 24 ? ? ? ? 8B 5D "
        "08 E8 ? ? ? ? C7 44 24 04 91 18 00 00 89 34 24 E8 ? ? ? ? A1 ? ? ? ? "
        "C7 45 C8 ? ? ? ?");
    static AbandonCurrentMatch_t AbandonCurrentMatch_fn =
        AbandonCurrentMatch_t(addr1);

    AbandonCurrentMatch_fn(this_);
}
