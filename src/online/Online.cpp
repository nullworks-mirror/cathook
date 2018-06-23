/*
  Created on 23.06.18.
*/

#include <online/Online.hpp>

#include <unordered_map>
#include <optional>
#include <timer.hpp>
#include <sstream>

namespace online
{

static std::unordered_map<unsigned, std::optional<user_data>> data{};
static std::unordered_map<unsigned, bool> identify_queue{};
static Timer identify_timer{};
static bool identify_stale{ false };

// INTERNAL METHODS

void queueUserForIdentification(unsigned steamId)
{

    identify_queue.insert(steamId);
    identify_timer.update();
    identify_stale = true;
}

void processIdentifyResponse()
{

}

void sendIdentifyRequest()
{
    std::vector<unsigned> steamIds{};
    auto it = identify_queue.begin();
    // Create a list of up to 32 steamId's
    for (int i = 0; i < 32 && it != identify_queue.end(); ++i, ++it)
    {
        if (!it->second)
            it->second = true;
        steamIds.push_back(it->first);
    }

}

// EXTERNAL METHODS

void init()
{

}

void update()
{
    // Only send a request after 3 seconds passed since last unknown steamId was added to the queue
    if (identify_stale && identify_timer.check(3000))
    {
        sendIdentifyRequest();
        identify_stale = false;
    }
}

user_data *getUserData(unsigned steamId)
{
    auto it = data.find(steamId);
    // User not identified
    if (it == data.end())
    {
        // Queue user for identification
        if (identify_queue.find(steamId) == identify_queue.end())
            queueUserForIdentification(steamId);
        return nullptr;
    }
    // SteamID belongs to online user
    if (it->second.has_value())
        return &*it->second;
    // SteamID does not belong to online user
    return nullptr;
}

}