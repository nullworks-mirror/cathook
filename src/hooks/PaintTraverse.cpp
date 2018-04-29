/*
 * PaintTraverse.cpp
 *
 *  Created on: Jan 8, 2017
 *      Author: nullifiedcat
 */

#include "common.hpp"

#if ENABLE_GUI
#include "GUI.h"
#endif

CatVar clean_screenshots(CV_SWITCH, "clean_screenshots", "1",
                         "Clean screenshots",
                         "Don't draw visuals while taking a screenshot");
CatVar disable_visuals(CV_SWITCH, "no_visuals", "0", "Disable ALL drawing",
                       "Completely hides cathook");
CatVar no_zoom(CV_SWITCH, "no_zoom", "0", "Disable scope",
               "Disables black scope overlay");
CatVar pure_bypass(CV_SWITCH, "pure_bypass", "0", "Pure Bypass",
                   "Bypass sv_pure");
void *pure_orig  = nullptr;
void **pure_addr = nullptr;

CatEnum software_cursor_enum({ "KEEP", "ALWAYS", "NEVER", "MENU ON",
                               "MENU OFF" });
CatVar
    software_cursor_mode(software_cursor_enum, "software_cursor_mode", "0",
                         "Software cursor",
                         "Try to change this and see what works best for you");

void PaintTraverse_hook(void *_this, unsigned int vp, bool fr, bool ar)
{

}
