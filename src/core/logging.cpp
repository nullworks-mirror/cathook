/*
 * logging.cpp
 *
 *  Created on: Oct 3, 2016
 *      Author: nullifiedcat
 */

#include <stdarg.h>
#include <string.h>

#include <pwd.h>
#include <settings/Bool.hpp>

#include "common.hpp"
#include "hack.hpp"

settings::Bool log_to_console{ "hack.log-console", "false" };

std::ofstream logging::handle;

void logging::Initialize()
{
    // FIXME other method of naming the file?
    passwd *pwd     = getpwuid(getuid());
    logging::handle = std::ofstream(strfmt("/tmp/cathook-%s-%d.log", pwd->pw_name, getpid()).get());
    if (!logging::handle.is_open())
        throw std::runtime_error("Can't open logging file");
}

void logging::Info(const char *fmt, ...)
{
    if (!logging::handle.is_open())
        logging::Initialize();
    auto time = std::time(nullptr);
    auto tm   = *std::localtime(&time);

    // Argument list
    va_list list;
    va_start(list, fmt);
    // Allocate buffer
    auto result = std::make_unique<char[]>(512);
    // Fill buffer
    if (vsnprintf(result.get(), 512, fmt, list) < 0)
        return;
    va_end(list);

    // Print to file
    logging::handle << std::put_time(&tm, "%H:%M:%S ") << result.get() << std::endl;
    // Print to console
#if ENABLE_VISUALS
    if (!hack::shutdown)
    {
        if (*log_to_console)
            g_ICvar->ConsolePrintf("CAT: %s \n", result.get());
    }
#endif
}

void logging::Shutdown()
{
    logging::handle.close();
}
