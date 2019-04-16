#include "common.hpp"
#include "NavBot.hpp"
#include "navparser.hpp"
#include "MiscTemporary.hpp"
#include "hack.hpp"

namespace hacks::tf2::OutOfBounds
{
struct Posinfo
{
    float x;
    float y;
    float z;
    float pitch;
    float yaw;
    bool usepitch;
    bool active = false;
};
struct MapPosinfo
{
    Posinfo spot;
    std::string lvlname;
};

static Posinfo to_path{};
static std::vector<MapPosinfo> oob_list;

Posinfo AutoOOB()
{
    std::string lvlname = g_IEngine->GetLevelName();
    std::vector<Posinfo> potential_spots{};
    for (auto &i : oob_list)
    {
        if (lvlname.find(i.lvlname) != lvlname.npos)
            potential_spots.push_back(i.spot);
    }
    Posinfo best_spot{};
    float best_score = FLT_MAX;
    for (auto &i : potential_spots)
    {
        Vector pos  = { i.x, i.y, 0.0f };
        float score = pos.AsVector2D().DistTo(LOCAL_E->m_vecOrigin().AsVector2D());
        if (score < best_score)
        {
            if (IsVectorVisible(g_pLocalPlayer->v_Eye, { pos.x, pos.y, g_pLocalPlayer->v_Eye.z }, true))
            {
                best_spot  = i;
                best_score = score;
            }
        }
    }
    return best_spot;
}

int getCarriedBuilding()
{
    if (CE_BYTE(LOCAL_E, netvar.m_bCarryingObject))
        return HandleToIDX(CE_INT(LOCAL_E, netvar.m_hCarriedObject));
    for (int i = 1; i < MAX_ENTITIES; i++)
    {
        auto ent = ENTITY(i);
        if (CE_BAD(ent) || ent->m_Type() != ENTITY_BUILDING)
            continue;
        if (HandleToIDX(CE_INT(ent, netvar.m_hBuilder)) != LOCAL_E->m_IDX)
            continue;
        if (!CE_BYTE(ent, netvar.m_bPlacing))
            continue;
        return i;
    }
    return -1;
}

void EquipTele()
{
    if (re::C_BaseCombatWeapon::GetSlot(RAW_ENT(LOCAL_W)) != 5)
        hack::command_stack().push("build 3");
}

void NavOOB()
{
    std::string lvlname = g_IEngine->GetLevelName();
    logging::Info("Going out of bounds on %s!", lvlname.c_str());
    std::vector<Posinfo> potential_spots{};
    for (auto &i : oob_list)
    {
        if (lvlname.find(i.lvlname) != lvlname.npos)
            potential_spots.push_back(i.spot);
    }
    Posinfo best_spot{};
    float best_score = FLT_MAX;
    for (auto &i : potential_spots)
    {
        Vector pos  = { i.x, i.y, 0.0f };
        float score = pos.DistTo(LOCAL_E->m_vecOrigin());
        if (score < best_score)
        {
            best_spot  = i;
            best_score = score;
        }
    }
    if (!best_spot.active)
    {
        logging::Info("No valid spots found nearby!");
        return;
    }
    to_path                    = best_spot;
    NavBot::task::current_task = NavBot::task::outofbounds;
    bool success               = nav::navTo(Vector{ to_path.x, to_path.y, to_path.z }, 8, true, true);
    if (!success)
    {
        logging::Info("No valid spots found nearby!");
        return;
    }
    else
        EquipTele();
}

void OutOfBoundsCommand(const CCommand &args)
{
    if (CE_BAD(LOCAL_E) || !LOCAL_E->m_bAlivePlayer())
        return;
    if (g_pLocalPlayer->clazz != tf_engineer)
    {
        g_ICvar->ConsoleColorPrintf(Color(*print_r, *print_g, *print_b, 255), "CAT: You are not playing as Engineer.");
        return;
    }

    // Need atleast 3 arguments (x, y, yaw)
    if (args.ArgC() < 2)
    {
        if (nav::prepare())
        {
            // TODO: Make navoob work with custom locations
            NavOOB();
            return;
        }
        else
        {
            auto loc = AutoOOB();
            if (!loc.active)
                logging::Info("No valid spots found nearby!");
            else
            {
                to_path = loc;
                EquipTele();
            }
            return;
        }
    }
    if (args.ArgC() < 4)
    {
        logging::Info("Usage:");
        logging::Info("cat_outofbounds x y Yaw (example: cat_outofbounds 511.943848 2783.968750 7.6229) or");
        logging::Info("cat_outofbounds x y Pitch Yaw (example: cat_outofbounds 511.943848 2783.968750 7.6229 89.936729)");
        return;
    }
    bool usepitch = false;
    // Use pitch too
    if (args.ArgC() > 4)
        usepitch = true;
    float x, y, yaw, pitch;
    // Failsafe
    try
    {
        x   = std::stof(args.Arg(1));
        y   = std::stof(args.Arg(2));
        yaw = std::stof(args.Arg(3));
        if (usepitch)
        {
            pitch = std::stof(args.Arg(3));
            yaw   = std::stof(args.Arg(4));
        }
    }
    catch (std::invalid_argument)
    {
        logging::Info("Invalid argument! (Malformed input?)\n");
        logging::Info("Usage:");
        logging::Info("cat_outofbounds x y Yaw (example: cat_outofbounds 511.943848 2783.968750 7.6229) or");
        logging::Info("cat_outofbounds x y Yaw Pitch (example: cat_outofbounds 511.943848 2783.968750 7.6229 89.936729)");
        return;
    }
    // Assign all the values
    to_path.x        = x;
    to_path.y        = y;
    to_path.yaw      = yaw;
    to_path.pitch    = pitch;
    to_path.usepitch = usepitch;
    to_path.active   = true;
}

static CatCommand auto_outofbounds{ "outofbounds", "Out of bounds", OutOfBoundsCommand };

static Timer timeout{};
static float yaw_offset = 0.0f;
bool failed             = false;
uint8_t fails           = 0;
void oobcm()
{
    if (CE_GOOD(LOCAL_E) && LOCAL_E->m_bAlivePlayer())
    {
        if (NavBot::task::current_task == NavBot::task::outofbounds)
        {
            if (!to_path.active)
            {
                if (failed)
                {
                    failed = false;
                    fails++;
                    if (fails > 2)
                        return;
                    to_path.active = true;
                    nav::navTo(GetForwardVector(g_pLocalPlayer->v_Origin, g_pLocalPlayer->v_OrigViewangles, -100.0f));
                }
                else
                {
                    NavBot::task::current_task = NavBot::task::none;
                    return;
                }
            }
            if (!nav::ReadyForCommands)
            {
                timeout.update();
                return;
            }
        }

        if (to_path.active)
        {
            Vector topath = { to_path.x, to_path.y, LOCAL_E->m_vecOrigin().z };
            if (LOCAL_E->m_vecOrigin().AsVector2D().DistTo(topath.AsVector2D()) <= 0.01f || timeout.test_and_set(4000))
            {
                if (LOCAL_E->m_vecOrigin().AsVector2D().DistTo(topath.AsVector2D()) <= 0.01f)
                {
                    if (re::C_BaseCombatWeapon::GetSlot(RAW_ENT(LOCAL_W)) != 5)
                    {
                        yaw_offset     = 0.0f;
                        to_path.active = false;
                        if (to_path.usepitch)
                            current_user_cmd->viewangles.x = to_path.pitch;
                        current_user_cmd->viewangles.y = to_path.yaw;
                        fails                          = 0;
                        logging::Info("Arrived at the destination! offset: %f %f", fabsf(LOCAL_E->m_vecOrigin().x - topath.x), fabsf(LOCAL_E->m_vecOrigin().y - topath.y));
                    }
                    else
                    {
                        timeout.update();
                        if (to_path.usepitch)
                            current_user_cmd->viewangles.x = to_path.pitch;
                        current_user_cmd->viewangles.y = to_path.yaw;
                        int carried_build              = getCarriedBuilding();
                        if (carried_build == -1)
                        {
                            logging::Info("No building held");
                            to_path.active = false;
                            return;
                        }
                        auto ent = ENTITY(carried_build);
                        if (CE_BAD(ent))
                        {
                            logging::Info("No Building held");
                            to_path.active = false;
                            return;
                        }
                        else
                        {
                            current_user_cmd->buttons |= IN_ATTACK;
                            failed = false;
                            if (yaw_offset >= 0.01f)
                            {
                                logging::Info("Failed getting out of bounds, Yaw offset too large");
                                to_path.active = false;
                                failed         = true;
                                yaw_offset     = 0.0f;
                                return;
                            }
                            yaw_offset = -yaw_offset;
                            if (yaw_offset >= 0.0f)
                                yaw_offset += 0.0001f;
                            current_user_cmd->viewangles.y = to_path.yaw + yaw_offset;
                        }
                    }
                }
                else
                {
                    yaw_offset     = 0.0f;
                    to_path.active = false;
                    logging::Info("Timed out trying to get to spot");
                }
            }
            else
            {
                auto move                     = ComputeMovePrecise(LOCAL_E->m_vecOrigin(), topath);
                current_user_cmd->forwardmove = move.first;
                current_user_cmd->sidemove    = move.second;
            }
        }
        else
            timeout.update();
    }
}
#define OOB_ADD(x, y, z, yaw, pitch, name) (oob_list.push_back({ { x, y, z, yaw, pitch, true, true }, name }))
static InitRoutine oob([]() {
    // Badwater
    OOB_ADD(511.943848f, 2783.968750f, 256.029939f, 7.622991f, 89.936729f, "pl_badwater");

    // Borneo
    OOB_ADD(-467.939911f, -6056.031250f, -543.96875f, 9.259290f, 90.082581f, "pl_borneo");

    // Doublecross
    OOB_ADD(-1016.030029f, -2580.031982f, -31.968742f, 9.347898f, 0.041826f, "ctf_doublecross");
    OOB_ADD(1016.001953f, 2580.053223f, -31.968742f, 7.275527f, -179.931656f, "ctf_doublecross");

    // Egypt
    // Stage 1
    OOB_ADD(-1754.255615f, -3344.038574f, -352.409424f, 36.452919f, 0.050812f, "cp_egypt");
    // Stage 2
    OOB_ADD(2919.968750f, 1999.951416f, 256.326172f, 11.952104f, 0.053882f, "cp_egypt");
    OOB_ADD(87.946884f, 1885.851685f, -158.814934f, 34.806473f, 89.951176f, "cp_egypt");
    // Stage 3
    OOB_ADD(1263.968750f, 4495.946289f, 641.140503f, 7.465197f, 0.074329f, "cp_egypt");

    // Turbine
    // Red
    OOB_ADD(1992.028442f, 936.019775f, -255.970061f, 0.272817f, -179.983673f, "ctf_turbine");
    OOB_ADD(1696.029175f, 1008.293091f, -255.970061f, 35.000000f, -90.038498f, "ctf_turbine");
    OOB_ADD(1927.989624f, 936.019775f, -255.970061f, 0.432120f, -0.026141f, "ctf_turbine");
    // Blue
    OOB_ADD(-1992.051514f, -936.055908f, -255.970061f, -0.768594f, 0.064962f, "ctf_turbine");
    OOB_ADD(-1696.021606f, -1008.698181f, -255.970061f, 35.000000f, 89.979446f, "ctf_turbine");
    OOB_ADD(-1927.023193f, -936.055847f, -255.970061f, 2.673917f, 179.936523f, "ctf_turbine");

    // Swiftwater
    OOB_ADD(5543.948730f, -1527.988037f, -1023.96875f, 23.115799f, -0.012952f, "pl_swiftwater_final1");
    OOB_ADD(2636.031250f, -1126.089478f, 13.124457f, -1130.14154f, 179.843811f, "pl_swiftwater_final1");

    // Mossrock
    OOB_ADD(1519.956543f, -1311.202271f, 110.028976f, 29.606899f, -89.951279f, "cp_mossrock");

    // Thundermountain
    // Phase 1
    OOB_ADD(-1068.001587f, -3724.079834f, 132.000061f - 68.000061f, 17.606197f, 0.118330f, "pl_thundermountain");
    OOB_ADD(-655.932190f, -3820.002930f, 132.031311f - 68.000061f, 35.448536f, 90.085930f, "pl_thundermountain");
    // Phase 2
    OOB_ADD(-5088.036133f, 2212.007568f, 516.031311f - 68.000061f, 15.974121f, -89.956055f, "pl_thundermountain");
    // OOB stuff
    OOB_ADD(-2456.001465f, 1055.965454f, 512.031250f, -62.966915f, 0.056807f, "pl_thundermountain");
    OOB_ADD(-2144.050537f, 927.939697f, 512.031250f, -65.708885f, 0.084181f, "pl_thundermountain");
    // Phase 3
    OOB_ADD(2191.972900f, 3353.798340f, 452.031311f - 68.000061f, 12.601318f, 89.972534f, "pl_thundermountain");

    EC::Register(EC::CreateMove, oobcm, "OOB_CM");
});
} // namespace hacks::tf2::OutOfBounds
