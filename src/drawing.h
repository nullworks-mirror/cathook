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
#include "aftercheaders.h"

class CachedEntity;
class Vector;
class IClientEntity;

namespace fonts {

extern unsigned long ESP;
extern unsigned long MENU;
extern unsigned long MENU_BIG;

const int ESP_HEIGHT = 14;
const int MENU_HEIGHT = 12;
const int MENU_BIG_HEIGHT = 30;

}

namespace colors {
extern int pink;

extern int white;
extern int black;

extern int red,    blu;
extern int red_b,  blu_b;  // Background
extern int red_v,  blu_v;  // Vaccinator
extern int red_u,  blu_u;  // Ubercharged
extern int yellow; // Deprecated
extern int orange;
extern int green;

void Init();

constexpr int Create(int r, int g, int b, int a) {
	unsigned char _r = (r) & 0xFF;
	unsigned char _g = (g) & 0xFF;
	unsigned char _b = (b) & 0xFF;
	unsigned char _a = (a) & 0xFF;
	return (int)(_r << 24) | (int)(_g << 16) | (int)(_b << 8) | (int)_a;
}

constexpr int Transparent(int base, float mod = 0.5f) {
	unsigned char _r = (base >> 24) & 0xFF;
	unsigned char _g = (base >> 16) & 0xFF;
	unsigned char _b = (base >> 8)  & 0xFF;
	unsigned char _a = (base) & 0xFF;
	_a *= mod;
	return Create(_r, _g, _b, _a * mod);
}

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

#endif /* DRAWING_H_ */
