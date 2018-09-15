/*
 * CTFPartyClient.hpp
 *
 *  Created on: Dec 7, 2017
 *      Author: nullifiedcat
 */

#pragma once
#include "reclasses.hpp"
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
    void RequestQueueForStandby();
    bool BInQueueForMatchGroup(int type);
    char RequestLeaveForMatch(int type);
    int BInvitePlayerToParty(CSteamID steamid);
    int BRequestJoinPlayer(CSteamID steamid);
    static bool BInQueue(CTFPartyClient *this_);
};
class ITFMatchGroupDescription
{
public:
    char pad0[4];
    int m_iID;
    char pad1[63];
    bool m_bForceCompetitiveSettings;
};

ITFMatchGroupDescription *GetMatchGroupDescription(int &idx);
} // namespace re
