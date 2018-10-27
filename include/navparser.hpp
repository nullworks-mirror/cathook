#pragma once

#include <memory>
#include <CNavFile.h>

namespace nav
{

enum init_status : uint8_t
{
    off = 0,
    unavailable,
    initing,
    on
};

// Call prepare first and check its return value
extern std::unique_ptr<CNavFile> navfile;

// Current path priority
extern int curr_priority;
// Check if ready to recieve another NavTo (to avoid overwriting of
// instructions)
extern bool ReadyForCommands;
// Ignore. For level init only
extern std::atomic<init_status> status;

// Nav to vector
bool navTo(Vector destination, int priority = 5, bool should_repath = true,
           bool nav_to_local = true, bool is_repath = false);
// Check and init navparser
bool prepare();
// Clear current path
void clearInstructions();

} // namespace nav
