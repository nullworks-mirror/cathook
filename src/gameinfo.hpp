/*
 * gameinfo.hpp
 *
 *  Created on: May 11, 2017
 *      Author: nullifiedcat
 */

#ifndef GAMEINFO_HPP_
#define GAMEINFO_HPP_

#include "globals.h"

#ifndef FORCE_SINGLE_GAME
#define CURRENT_APPID g_AppID
#else
#define CURRENT_APPID (SINGLE_APPID)
#endif

#define TF2C (CURRENT_APPID == 243750)
#define TF2  (CURRENT_APPID == 440)
#define HL2DM (CURRENT_APPID == 320)
#define TF (TF2C || TF2)

#endif /* GAMEINFO_HPP_ */
