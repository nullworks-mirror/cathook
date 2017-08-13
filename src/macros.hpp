/*
 * macros.hpp
 *
 *  Created on: May 25, 2017
 *      Author: nullifiedcat
 */

#pragma once

#define DATA_PATH "/opt/cathook-data"

#if defined(NOGUI) and NOGUI == 1 or defined(TEXTMODE)
#define ENABLE_GUI false
#else
#define ENABLE_GUI true
#endif

