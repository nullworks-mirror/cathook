/*
 * drawing.h
 *
 *  Created on: Oct 5, 2016
 *      Author: nullifiedcat
 */

#ifndef DRAWING_H_
#define DRAWING_H_

#if defined(RENDERING_ENGINE_OPENGL)
#define DRAW_API "drawgl.hpp"
#elif defined(RENDERING_ENGINE_XOVERLAY)
#define DRAW_API "drawex.hpp"
#endif

#include "common.h"

extern "C" {
#include <texture-font.h>
}

class CachedEntity;
class Vector;
class CatVar;
class IClientEntity;
class CatEnum;
class VMatrix;

namespace fonts {

extern ftgl::texture_font_t* font_main;

}

void InitStrings();
void ResetStrings();
void AddCenterString(const std::string& string, const rgba_t& color = colors::white);
void AddSideString(const std::string& string, const rgba_t& color = colors::white);
void DrawStrings();

namespace draw {

extern std::mutex draw_mutex;
extern VMatrix wts;

extern int width;
extern int height;
extern float fov;

void Initialize();

void UpdateWTS();
bool WorldToScreen(const Vector &origin, Vector &screen);
bool EntityCenterToScreen(CachedEntity* entity, Vector& out);

}

#if not ENABLE_GUI
constexpr rgba_t GUIColor() {
	return colors::white;
}
#endif

#endif /* DRAWING_H_ */
