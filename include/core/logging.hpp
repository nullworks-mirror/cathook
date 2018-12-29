/*
 * logging.h
 *
 *  Created on: Oct 3, 2016
 *      Author: nullifiedcat
 */

#pragma once

#include <iostream>

#ifdef __cplusplus
namespace logging
{
#endif

extern std::ofstream handle;

void Initialize();
void Shutdown();
void Info(const char *fmt, ...);

#ifdef __cplusplus
}
#endif
