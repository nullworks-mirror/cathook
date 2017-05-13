/*
 * gameinfo.hpp
 *
 *  Created on: May 11, 2017
 *      Author: nullifiedcat
 */

#ifndef GAMEINFO_HPP_
#define GAMEINFO_HPP_

#include "globals.h"

#ifdef BUILD_GAME
constexpr bool IsTF2() { return BUILD_GAME == 440; }
constexpr bool IsTF2C() { return BUILD_GAME == 243750; }
constexpr bool IsHL2DM() { return BUILD_GAME == 320; }
#else
inline bool IsTF2() { return g_AppID == 440; }
inline bool IsTF2C() { return g_AppID == 243750; }
inline bool IsHL2DM() { return g_AppID == 320; }
#endif

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
