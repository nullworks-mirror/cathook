/*
 * drawing.h
 *
 *  Created on: Oct 5, 2016
 *      Author: nullifiedcat
 */

#pragma once

#include "common.hpp"

class CachedEntity;
class Vector;
class CatVar;
class IClientEntity;
class CatEnum;
class VMatrix;

namespace fonts
{

extern draw_api::font_handle_t main_font;
extern draw_api::font_handle_t esp_font;
}

namespace fonts
{

// FIXME add menu fonts
extern unsigned long ESP;
extern unsigned long MENU;
extern unsigned long MENU_BIG;

void Update();

extern const std::vector<std::string> fonts;
extern CatEnum family_enum;
}

constexpr rgba_t GUIColor()
{
    return colors::white;
}

void InitStrings();
void ResetStrings();
void AddCenterString(const std::string &string,
                     const rgba_t &color = colors::white);
void AddSideString(const std::string &string,
                   const rgba_t &color = colors::white);
void DrawStrings();

namespace draw
{

extern std::mutex draw_mutex;
extern VMatrix wts;

extern int width;
extern int height;
extern float fov;

void Initialize();

void UpdateWTS();
bool WorldToScreen(const Vector &origin, Vector &screen);
bool EntityCenterToScreen(CachedEntity *entity, Vector &out);
}
