/*
 * CTFPartyClient.hpp
 *
 *  Created on: Dec 7, 2017
 *      Author: nullifiedcat
 */

#pragma once

namespace re
{

class CTFPartyClient
{
public:
    static CTFPartyClient *GTFPartyClient();

    static int SendPartyChat(CTFPartyClient *client, const char *message);
    int LoadSavedCasualCriteria();
    static ITFGroupMatchCriteria *MutLocalGroupCriteria(CTFPartyClient *client);
    static bool BCanQueueForStandby(CTFPartyClient *this_);
    char RequestQueueForMatch(int type);
    static bool BInQueue(CTFPartyClient *this_);
};
}
