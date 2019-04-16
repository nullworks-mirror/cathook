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
TextFile oob_file;
TextFile clip_file;
static std::vector<MapPosinfo> oob_list;
static std::vector<MapPosinfo> throughwall_list;

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

void NavOOB(bool oob)
{
    std::string lvlname = g_IEngine->GetLevelName();
    logging::Info("Going out of bounds on %s!", lvlname.c_str());
    std::vector<Posinfo> potential_spots{};
    if (oob)
        for (auto &i : oob_list)
        {
            if (lvlname.find(i.lvlname) != lvlname.npos)
                potential_spots.push_back(i.spot);
        }
    else
        for (auto &i : throughwall_list)
            if (lvlname.find(i.lvlname) != lvlname.npos)
                potential_spots.push_back(i.spot);
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
void OutOfBounds_func(const CCommand &args, bool oob)
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
            NavOOB(oob);
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
void OutOfBoundsCommand(const CCommand &args)
{
    OutOfBounds_func(args, true);
}
void ClipFunc(const CCommand &args)
{
    OutOfBounds_func(args, false);
}

static CatCommand auto_outofbounds{ "outofbounds", "Out of bounds", OutOfBoundsCommand };
static CatCommand clip{ "outofbounds_clipdoors", "Clip doors and or walls (No Out of bounds)", ClipFunc };

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
#define OOB_ADD(x, y, z, pitch, yaw, name) (oob_list.push_back({ { x, y, z, pitch, yaw, true, true }, name }))
#define CLIP_ADD(x, y, z, pitch, yaw, name) (throughwall_list.push_back({ { x, y, z, pitch, yaw, true, true }, name }))
// Makes code look better yes
#define STR_FETCH(name)                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                       \
    auto sep##name = line.find(",");                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                          \
    float name     = std::stof(line.substr(0, sep##name));                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                    \
    line           = line.substr(sep##name + 2);
void ParseFile(TextFile file, bool oob)
{
    if (oob)
        oob_file = {};
    else
        clip_file = {};
    auto lines = file.lines;
    for (auto &line : lines)
    {
        if (line.find("//") != line.npos)
            continue;
        if (line.find(",") == line.npos || line.find("\"") == line.npos)
            continue;
        try
        {
            STR_FETCH(x);
            STR_FETCH(y);
            STR_FETCH(z);
            STR_FETCH(pitch);
            STR_FETCH(yaw);

            auto lvlname_sep    = line.find("\"");
            line                = line.substr(lvlname_sep + 1);
            std::string lvlname = line.substr(0, line.find("\""));
            if (oob)
                OOB_ADD(x, y, z, pitch, yaw, lvlname);
            else
                CLIP_ADD(x, y, z, pitch, yaw, lvlname);
        }
        catch (std::invalid_argument)
        {
            continue;
        }
    }
}

static InitRoutine oob([]() {
    if (oob_file.TryLoad("ooblist.txt"))
        ParseFile(oob_file, true);
    if (clip_file.TryLoad("cliplist.txt"))
        ParseFile(clip_file, false);
    EC::Register(EC::CreateMove, oobcm, "OOB_CM");
});

static CatCommand reload_list("oob_reload", "Reload the oob and clip files", []() {
    if (oob_file.TryLoad("ooblist.txt"))
        ParseFile(oob_file, true);
    if (clip_file.TryLoad("cliplist.txt"))
        ParseFile(clip_file, false);
});
} // namespace hacks::tf2::OutOfBounds
