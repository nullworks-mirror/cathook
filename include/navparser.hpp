#pragma once

#include <memory>
#include <mutex>
#include <CNavFile.h>

namespace nav
{
// Navfile containing areas and its lock
extern std::unique_ptr<CNavFile> navfile;
extern std::mutex navfile_lock;
bool navTo(Vector destination, int priority = 5, bool should_repath = true, bool nav_to_local = true, bool is_repath = false);
} // namespace nav
