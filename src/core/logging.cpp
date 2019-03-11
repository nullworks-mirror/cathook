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
#include "MiscTemporary.hpp"

static settings::Bool log_to_console{ "hack.log-console", "false" };

FILE *logging::handle{ nullptr };

void logging::Initialize()
{
    // FIXME other method of naming the file?
    passwd *pwd     = getpwuid(getuid());
    logging::handle = fopen(strfmt("/tmp/cathook-%s-%d.log", pwd->pw_name, getpid()).get(), "w");
}

void logging::Info(const char *fmt, ...)
{
    if (logging::handle == nullptr)
        logging::Initialize();

    // Argument list
    va_list list;
    va_start(list, fmt);
    // Allocate buffer
    auto result = std::make_unique<char[]>(512);
    // Fill buffer
    if (vsnprintf(result.get(), 512, fmt, list) < 0)
        return;
    va_end(list);

    std::string print_file(result.get());

    time_t current_time;
    struct tm *time_info = nullptr;
    char timeString[10];
    time(&current_time);
    time_info = localtime(&current_time);
    strftime(timeString, sizeof(timeString), "%H:%M:%S", time_info);

    std::string to_log = result.get();
    to_log             = strfmt("[%s] ", timeString).get() + to_log + "\n";
    fprintf(logging::handle, "%s", to_log.c_str());
    fflush(logging::handle);
#if ENABLE_VISUALS
    if (!hack::shutdown)
    {
        if (*log_to_console)
            g_ICvar->ConsoleColorPrintf(Color(*print_r, *print_g, *print_b, 255), "CAT: %s\n", result.get());
    }
#endif
}

void logging::Shutdown()
{
    fclose(logging::handle);
    logging::handle = nullptr;
}
