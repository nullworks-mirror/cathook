/*
 * logging.cpp
 *
 *  Created on: Oct 3, 2016
 *      Author: nullifiedcat
 */

#include <stdarg.h>
#include <string>

#include <pwd.h>
#include <settings/Bool.hpp>
#include "logging.hpp"
#include "helpers.hpp"
#include "MiscTemporary.hpp"
#include "hack.hpp"

static settings::Boolean log_to_console{ "hack.log-console", "false" };

static bool shut_down = false;
std::ofstream logging::handle{ nullptr };

#if ENABLE_LOGGING
void logging::Initialize()
{
    // FIXME other method of naming the file?
    static passwd *pwd = getpwuid(getuid());
    logging::handle.open(strfmt("/tmp/cathook-%s-%d.log", pwd->pw_name, getpid()).get(), std::ios::out | std::ios::app);
}
#endif

static inline void Log(std::unique_ptr<char[]> &result, bool file_only)
{
#if ENABLE_LOGGING
    if (!logging::handle.is_open())
        logging::Initialize();
    time_t current_time;
    struct tm *time_info = nullptr;
    char timeString[10];
    time(&current_time);
    time_info = localtime(&current_time);
    strftime(timeString, sizeof(timeString), "%H:%M:%S", time_info);

    std::string to_log = result.get();
    to_log             = strfmt("[%s] ", timeString).get() + to_log + "\n";
    logging::handle << to_log;
    logging::handle.flush();
#if ENABLE_VISUALS
    if (!hack::shutdown)
    {
        if (!file_only && *log_to_console)
            g_ICvar->ConsoleColorPrintf(MENU_COLOR, "CAT: %s\n", result.get());
    }
#endif
#endif
}

void logging::Info(const char *fmt, ...)
{
#if ENABLE_LOGGING
    if (shut_down)
        return;
    // Argument list
    va_list list;
    va_start(list, fmt);
    // Allocate buffer
    char result[512];
    // Fill buffer
    if (vsnprintf(result, 512, fmt, list) < 0)
        return;
    va_end(list);

    Log(result, false);
#endif
}

void logging::File(const char *fmt, ...)
{
#if ENABLE_LOGGING
    if (shut_down)
        return;
    // Argument list
    va_list list;
    va_start(list, fmt);
    // Allocate buffer
    char result[512];
    // Fill buffer
    if (vsnprintf(result, 512, fmt, list) < 0)
        return;
    va_end(list);

    Log(result, true);
#endif
}

void logging::Shutdown()
{
#if ENABLE_LOGGING
    logging::handle.close();
    shut_down = true;
#endif
}
