/*
 * PaintTraverse.h
 *
 *  Created on: Jan 8, 2017
 *      Author: nullifiedcat
 */

#pragma once

class CatVar;

extern CatVar no_zoom;
extern CatVar clean_screenshots;
extern CatVar disable_visuals;
void PaintTraverse_hook(void *, unsigned int, bool, bool);
