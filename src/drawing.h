/*
 * drawing.h
 *
 *  Created on: Oct 5, 2016
 *      Author: nullifiedcat
 */

#ifndef DRAWING_H_
#define DRAWING_H_

#include "beforecheaders.h"
#include <string>
#include <utility>
#include <vector>
#include "aftercheaders.h"

class CachedEntity;
class Vector;
class CatVar;
class IClientEntity;
class CatEnum;

namespace fonts {

// FIXME add menu fonts
extern unsigned long ESP;
extern unsigned long MENU;
extern unsigned long MENU_BIG;

void Update();

extern const std::vector<std::string> fonts;
extern CatEnum family_enum;
extern CatVar esp_family;
extern CatVar esp_height;

}

namespace colors {

constexpr int Create(int r, int g, int b, int a) {
	unsigned char _r = (r) & 0xFF;
	unsigned char _g = (g) & 0xFF;
	unsigned char _b = (b) & 0xFF;
	unsigned char _a = (a) & 0xFF;
	return (int)(_r) | (int)(_g << 8) | (int)(_b << 16) | (int)(_a << 24);
}

constexpr int Transparent(int base, float mod = 0.5f) {
	unsigned char _a = (base >> 24) & 0xFF;
	unsigned char _b = (base >> 16) & 0xFF;
	unsigned char _g = (base >> 8)  & 0xFF;
	unsigned char _r = (base) & 0xFF;
	return Create(_r, _g, _b, (int)((float)(_a) * mod));
}

constexpr int pink = Create(255, 105, 180, 255);

constexpr int white = Create(255, 255, 255, 255);
constexpr int black = Create(0, 0, 0, 255);

constexpr int red = Create(237, 42, 42, 255), blu = Create(28, 108, 237, 255);
constexpr int red_b = Create(64, 32, 32, 178),  blu_b = Create(32, 32, 64, 178);  // Background
constexpr int red_v = Create(196, 102, 108, 255),  blu_v = Create(102, 182, 196, 255);  // Vaccinator
constexpr int red_u = Create(216, 34, 186, 255),  blu_u = Create(167, 75, 252, 255);  // Ubercharged
constexpr int yellow = Create(255, 255, 0, 255);
constexpr int orange = Create(255, 120, 0, 255);
constexpr int green = Create(0, 255, 0, 255);

int FromHSL(float h, float s, float l);
int RainbowCurrent();
int Health(int health, int max);
int EntityF(CachedEntity* ent);

}

void InitStrings();
void ResetStrings();
void AddCenterString(const std::string& string, int color = 0xFFFFFFFF);
void AddSideString(const std::string& string, int color = 0xFFFFFFFF);
void DrawStrings();

namespace draw {

extern int width;
extern int height;

void Initialize();

void String (unsigned long font, int x, int y, int color, int shadow, const char* text);
void String (unsigned long font, int x, int y, int color, int shadow, std::string text);
void WString(unsigned long font, int x, int y, int color, int shadow, const wchar_t* text);
void FString(unsigned long font, int x, int y, int color, int shadow, const char* text, ...);

/*void DrawString(unsigned long font, int x, int y, Color color, const wchar_t* text);
void DrawString(int x, int y, Color color, Color background, bool center, const char* text, ...);
void DrawString(int x, int y, Color color, const char* text, ...);*/
void DrawRect(int x, int y, int w, int h, int color);
void DrawLine(int x, int y, int dx, int dy, int color);
bool WorldToScreen(Vector &origin, Vector &screen);
bool EntityCenterToScreen(CachedEntity* entity, Vector& out);
void OutlineRect(int x, int y, int w, int h, int color);
void GetStringLength(unsigned long font, char* string, int& length, int& height);
std::pair<int, int> GetStringLength(unsigned long font, std::string string);
//void DrawString(unsigned font_handle, int x, int y, Color color, const char* text, ...);

}

#if NOGUI == 1
constexpr int GUIColor() {
	return colors::white;
}
#endif

#endif /* DRAWING_H_ */
