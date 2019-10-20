/*
 * Created on 29.07.18.
 */

#include "common.hpp"
#include "hacks/AntiAntiAim.hpp"
#include "sdk/dt_recv_redef.h"
#include "sdk.hpp"

namespace hacks::shared::anti_anti_aim
{
static settings::Boolean enable{ "anti-anti-aim.enable", "false" };
static settings::Boolean debug{ "anti-anti-aim.debug.force-rotate", "false" };

std::unordered_map<unsigned, brutedata> resolver_map;

void frameStageNotify(ClientFrameStage_t stage)
{
    if (!enable || !g_IEngine->IsInGame())
        return;
    if (stage == FRAME_NET_UPDATE_POSTDATAUPDATE_START)
    {
        for (int i = 1; i <= g_IEngine->GetMaxClients(); i++)
        {
            auto player = ENTITY(i);
            if (CE_BAD(player) || !player->m_bAlivePlayer() || !player->m_bEnemy() || !player->player_info.friendsID)
                continue;
            auto &data  = resolver_map[player->player_info.friendsID];
            auto &angle = CE_VECTOR(player, netvar.m_angEyeAngles);
            angle.x = data.new_angle.x;
            angle.y = data.new_angle.y;
        }
    }
}

std::array<float, 5> yaw_resolves{ 0.0f, 180.0f, 90.0f, -90.0f, -180.0f };

Vector resolveAngle(Vector angles, brutedata &brute)
{
    if (brute.brutenum % 2)
    {
        // Pitch resolver
        if (angles.x >= 90)
            angles.x = -89;
        if (angles.x <= -90)
            angles.x = 89;
    }
    while (angles.y > 180)
        angles.y -= 360;

    while (angles.y < -180)
        angles.y += 360;

    // Yaw Resolving
    // Find out which angle we should try
    int entry = (int) std::floor((brute.brutenum / 2.0f)) % yaw_resolves.size();
    angles.y += yaw_resolves[entry];

    while (angles.y > 180)
        angles.y -= 360;

    while (angles.y < -180)
        angles.y += 360;

    if (debug)
        angles.y = 100;
    return angles;
}

float resolveAngleYaw(float angle, brutedata &brute)
{
    brute.original_angle.y = angle;
    while (angle > 180)
        angle -= 360;

    while (angle < -180)
        angle += 360;

    // Yaw Resolving
    // Find out which angle we should try
    int entry = (int) std::floor((brute.brutenum / 2.0f)) % yaw_resolves.size();
    angle += yaw_resolves[entry];

    while (angle > 180)
        angle -= 360;

    while (angle < -180)
        angle += 360;

    if (debug)
        angle = 100;
    brute.new_angle.y = angle;
    return angle;
}

float resolveAnglePitch(float angle, brutedata &brute)
{
    brute.original_angle.x = angle;
    if (brute.brutenum % 2)
    {
        // Pitch resolver
        if (angle >= 90)
            angle = -89;
        if (angle <= -90)
            angle = 89;
    }
    brute.new_angle.x = angle;
    return angle;
}

void increaseBruteNum(int idx)
{
    auto ent        = ENTITY(idx);
    if (CE_BAD(ent) || !ent->player_info.friendsID)
        return;
    auto &data = hacks::shared::anti_anti_aim::resolver_map[ent->player_info.friendsID];
    if (data.hits_in_a_row >= 4)
        data.hits_in_a_row = 2;
    else if (data.hits_in_a_row >= 2)
        data.hits_in_a_row = 0;
    else
    {
        logging::Info("Brutenum for entity %i increased to %i", idx, data.brutenum++ + 1);
        data.hits_in_a_row = 0;
        auto &angle = CE_VECTOR(ent, netvar.m_angEyeAngles);
        angle.x = resolveAnglePitch(data.original_angle.x, data);
        angle.y = resolveAngleYaw(data.original_angle.y, data);
        data.new_angle.x = angle.x;
        data.new_angle.y = angle.y;
    }
}


// gay garbage
/*void resolveEnt(int IDX, IClientEntity *entity)
{
    if (IDX == g_IEngine->GetLocalPlayer())
        return;
    entity = g_IEntityList->GetClientEntity(IDX);
    if (entity && !entity->IsDormant() && !NET_BYTE(entity, netvar.iLifeState))
    {
        float quotat = 0;
        float quotaf = 0;
        if (!g_Settings.brute.choke[IDX].empty())
            for (auto it : g_Settings.brute.choke[IDX])
            {
                if (it)
                    quotat++;
                else
                    quotaf++;
            }
        float quota            = quotat / quotaf;
        Vector &netangles      = NET_VECTOR(entity, netvar.m_angEyeAngles);
        Vector angles          = QAngleToVector(entity->GetAbsAngles());
        static bool brutepitch = false;
        if (g_Settings.brute.brutenum[IDX] > 5)
        {
            g_Settings.brute.brutenum[IDX] = 0;
            brutepitch                     = !brutepitch;
        }
        if (quota > 0.8f)
            brutepitch = true;
        angles.y = fmod(angles.y + 180.0f, 360.0f);
        if (angles.y < 0)
            angles.y += 360.0f;
        angles.y -= 180.0f;
        if (angles.x >= 90)
            angles.x = -89;
        if (angles.x <= -90)
            angles.x = 89;
        if (quota < 0.8f)
            switch (g_Settings.brute.brutenum[IDX])
            {
            case 0:
                break;
            case 1:
                angles.y += 180.0f;
                break;
            case 2:
                angles.y -= 90.0f;
                break;
            case 3:
                angles.y += 90.0f;
                break;
            case 4:
                angles.y -= 180.0f;
                break;
            case 5:
                angles.y = 0.0f;
                break;
            }
        if (brutepitch)
            switch (g_Settings.brute.brutenum[IDX] % 4)
            {
            case 0:
                break;
            case 1:
                angles.x = -89.0f;
                break;
            case 2:
                angles.x = 89.0f;
                break;
            case 3:
                angles.x = 0.0f;
                break;
            }
        const_cast<QAngle &>(entity->GetAbsAngles()) = VectorToQAngle(angles);
        netangles                                    = angles;
    }
}*/

void ResetPlayer(unsigned steamid)
{
    if (resolver_map.find(steamid) != resolver_map.end())
        resolver_map.erase(steamid);
}

void ResetPlayer(int idx)
{
    CachedEntity *ent = ENTITY(idx);
    if (!ent || CE_INVALID(ent) || !ent->player_info.friendsID)
        return;
    ResetPlayer(ent->player_info.friendsID);
}

/*
class ResolverListener : public IGameEventListener
{
public:
    virtual void FireGameEvent(KeyValues *event)
    {
        if (!enable)
            return;
        std::string name(event->GetName());
        if (name == "player_activate")
        {
            int uid    = event->GetInt("userid");
            int entity = g_IEngine->GetPlayerForUserID(uid);
            ResetPlayer(entity);
        }
        else if (name == "player_disconnect")
        {
            int uid    = event->GetInt("userid");
            int entity = g_IEngine->GetPlayerForUserID(uid);
            ResetPlayer(entity);
        }
    }
};

static ResolverListener listener;
static InitRoutine init([]() { g_IGameEventManager->AddListener(&listener, false); });*/
void PitchHook(const CRecvProxyData *pData, void *pStruct, void *pOut)
{
    logging::Info("Pitch hook called");
    float *ang = (float *) pOut;
    *ang       = pData->m_Value.m_Float;

    auto client_ent   = (IClientEntity *) (pStruct);
    CachedEntity *ent = ENTITY(client_ent->entindex());
    if (CE_GOOD(ent))
        *ang = resolveAnglePitch(pData->m_Value.m_Float, resolver_map[ent->player_info.friendsID]);
}

void YawHook(const CRecvProxyData *pData, void *pStruct, void *pOut)
{
    logging::Info("Yaw hook called");
    float flYaw = pData->m_Value.m_Float;

    float *flYaw_out = (float *) ((unsigned) pOut);

    auto client_ent = (IClientEntity *) (pStruct);

    CachedEntity *ent = ENTITY(client_ent->entindex());
    if (CE_GOOD(ent))
        *flYaw_out = resolveAngleYaw(flYaw, resolver_map[ent->player_info.friendsID]);
}

static InitRoutine init([]() {
    auto pClass = g_IBaseClient->GetAllClasses();
    while (pClass)
    {
        const char *pszName = pClass->m_pRecvTable->m_pNetTableName;
        // "DT_TFPlayer", "tfnonlocaldata"
        if (!strcmp(pszName, "DT_TFPlayer"))
        {
            for (int i = 0; i < pClass->m_pRecvTable->m_nProps; i++)
            {
                RecvPropRedef *pProp1 = (RecvPropRedef *) &(pClass->m_pRecvTable->m_pProps[i]);
                if (!pProp1)
                    continue;
                const char *pszName2 = pProp1->m_pVarName;
                if (!strcmp(pszName2, "tfnonlocaldata"))
                    for (int j = 0; j < pProp1->m_pDataTable->m_nProps; j++)
                    {
                        RecvPropRedef *pProp2 = (RecvPropRedef *) &(pProp1->m_pDataTable->m_pProps[j]);
                        if (!pProp2)
                            continue;
                        const char *name = pProp2->m_pVarName;
                        logging::Info("tableing: %s %d", name, ((RecvProp *)pProp1)->GetNumElements());

                        // Pitch Fix
                        if (!strcmp(name, "m_angEyeAngles[0]"))
                        {
                            pProp2->m_ProxyFn = PitchHook;
                        }

                        // Yaw Fix
                        if (!strcmp(name, "m_angEyeAngles[1]"))
                        {
                            logging::Info("Yaw Fix Applied");
                            pProp2->m_ProxyFn = YawHook;
                        }
                    }
            }
        }
        pClass = pClass->m_pNext;
    }
});
} // namespace hacks::shared::anti_anti_aim
