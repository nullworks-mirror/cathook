#include "common.hpp"
#include "navparser.hpp"
#include "settings/Bool.hpp"
#include "HookedMethods.hpp"

// Rvars
static settings::Bool enable{ "navbot.enable", "false" };
static settings::Bool heavy_mode{ "navbot.heavy-mode", "false" };
static settings::Bool spy_mode{ "navbot.spy-mode", "false" };

// Timers
static Timer general_cooldown{};
static Timer init_cooldown{};

// Vectors
static std::vector<Vector *> default_spots{};
static std::vector<Vector *> preferred_spots{};

// Unordered_maps
static std::unordered_map<int, int> preferred_spots_priority{};

static bool inited = false;

namespace hacks::shared::NavBot
{

// Main Functions
void Init(bool from_LevelInit)
{
    if (from_LevelInit)
    {
        inited = false;
        default_spots.clear();
        preferred_spots.clear();
        preferred_spots_priority.clear();
    }
    if (!nav::prepare())
        return;
    if (!from_LevelInit)
    {
        default_spots.clear();
        preferred_spots.clear();
        preferred_spots_priority.clear();
    }
    inited = true;
    for (auto &i : nav::navfile.get()->m_areas)
    {
        if (!i.m_hidingSpots.empty())
            for (auto &j : i.m_hidingSpots)
                default_spots.push_back(&j.m_pos);
        if (i.m_attributeFlags & NAV_MESH_NO_HOSTAGES)
            preferred_spots.push_back(&i.m_center);
    }
}

static HookedFunction CreateMove(HookedFunctions_types::HF_CreateMove, "NavBot", 10, [](){
    if (!*enable)
        return;
    if (!inited)
    {
        if (init_cooldown.test_and_set(10000))
            Init(false);
        return;
    }
    if (!nav::prepare())
        return;

});

// Helpers

bool CanPath()
{
    if ((*heavy_mode || *spy_mode) && general_cooldown.test_and_set(100))
        return true;
    else
    {
        if (nav::ReadyForCommands && general_cooldown.test_and_set(100))
            return true;
        return false;
    }
}

bool NavToSniperSpot(int priority)
{
    // Already pathing currently and priority is below the wanted, so just return
    if (priority < nav::curr_priority)
        return true;
    // Preferred yay or nay?
    bool use_preferred = preferred_spots.empty() ? false : true;
    if (!use_preferred && default_spots.empty())
        return false;

    std::vector<Vector *> *sniper_spots = use_preferred ? &preferred_spots : &default_spots;

    // Wtf Nullptr!
    if (!sniper_spots)
        return false;
    if (use_preferred)
    {
        // Store Lowest Matches for later, will be useful
        std::vector<unsigned> matches{};

        // Priority INT_MAX, not like you'll exceed it anyways lol
        int lowest_priority = INT_MAX;
        for (unsigned i = 0; i < sniper_spots->size(); i++)
        {
            if (preferred_spots_priority[i] < lowest_priority)
            {
                lowest_priority = preferred_spots_priority[i];
                matches.clear();
                matches.push_back(i);
            }
            else if (preferred_spots_priority[i] == lowest_priority)
                matches.push_back(i);
        }
        if (!matches.empty())
        {
            // Cant exceed.club
            float min_dist = FLT_MAX;

            // Best Spot to nav to
            int best_spot = -1;

            for (auto idx : matches)
            {
                Vector *match = sniper_spots->at(matches.at(0));

                // Score of the match
                float score = match->DistTo(LOCAL_E->m_vecOrigin());

                if ( score < min_dist)
                {
                    min_dist = score;
                    best_spot = idx;
                }
            }

            // return if no spot found
            if (best_spot == -1)
                return false;
            // Make spot less important
            preferred_spots_priority[best_spot]++;
            // Nav to Spot
            return nav::navTo(*sniper_spots->at(best_spot), priority);
        }
    }
    else
    {
        // Get Random Sniper Spot
        unsigned index = unsigned(std::rand()) % sniper_spots->size();
        Vector *random_spot = sniper_spots->at(index);

        // Nav to Spot
        return nav::navTo(*random_spot, priority);
    }
}
}
