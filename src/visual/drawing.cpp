/*
 * drawing.cpp
 *
 *  Created on: Oct 5, 2016
 *      Author: nullifiedcat
 */

#if ENABLE_VISUALS == 1

#include "common.hpp"

#include <GL/gl.h>

std::array<std::string, 32> side_strings;
std::array<std::string, 32> center_strings;
std::array<rgba_t, 32> side_strings_colors{ colors::empty };
std::array<rgba_t, 32> center_strings_colors{ colors::empty };
size_t side_strings_count{ 0 };
size_t center_strings_count{ 0 };

void InitStrings()
{
    ResetStrings();
}

void ResetStrings()
{
    side_strings_count   = 0;
    center_strings_count = 0;
}

void AddSideString(const std::string &string, const rgba_t &color)
{
    side_strings[side_strings_count]        = string;
    side_strings_colors[side_strings_count] = color;
    ++side_strings_count;
}

void DrawStrings()
{
    int y{ 8 };
    for (size_t i = 0; i < side_strings_count; ++i)
    {
        draw_api::draw_string_with_outline(
            8, y, side_strings[i].c_str(), fonts::main_font,
            side_strings_colors[i], colors::black, 1.5f);
        y += /*((int)fonts::font_main->height)*/ 14 + 1;
    }
    y = draw::height / 2;
    for (size_t i = 0; i < center_strings_count; ++i)
    {
        float sx, sy;
        draw_api::get_string_size(center_strings[i].c_str(), fonts::main_font,
                                  &sx, &sy);
        draw_api::draw_string_with_outline(
            (draw::width - sx) / 2, y, center_strings[i].c_str(),
            fonts::main_font, center_strings_colors[i], colors::black, 1.5f);
        y += /*((int)fonts::font_main->height)*/ 14 + 1;
    }
}

void AddCenterString(const std::string &string, const rgba_t &color)
{
    center_strings[center_strings_count]        = string;
    center_strings_colors[center_strings_count] = color;
    ++center_strings_count;
}

// TODO globals
int draw::width  = 0;
int draw::height = 0;
float draw::fov  = 90.0f;
std::mutex draw::draw_mutex;

namespace fonts
{

draw_api::font_handle_t main_font;
}

namespace fonts {
template<typename T>
constexpr T _clamp(T _min, T _max, T _val) {
	return ((_val > _max) ? _max : ((_val < _min) ? _min : _val));
}
unsigned long ESP = 0;
unsigned long MENU = 0;
unsigned long MENU_BIG = 0;
const std::vector<std::string> fonts = {"Tahoma Bold", "Tahoma", "TF2 Build", "Verdana", "Verdana Bold", "Arial", "Courier New", "Ubuntu Mono Bold"};
CatEnum family_enum(fonts);
CatVar esp_family(family_enum, "font_esp_family", "2", "ESP font", "ESP font family");
CatVar esp_height(CV_INT, "font_esp_height", "14", "ESP height", "ESP font height");

void Update() {
	fonts::ESP = g_ISurface->CreateFont();
	g_ISurface->SetFontGlyphSet(fonts::ESP, fonts::fonts[_clamp(0, 7, (int)fonts::esp_family)].c_str(), (int)fonts::esp_height, 0, 0, 0, 0); // or Ubuntu Mono Bold
	//g_ISurface->ResetFontCaches();
}

}

void draw::Initialize()
{
    if (!draw::width || !draw::height)
    {
        g_IEngine->GetScreenSize(draw::width, draw::height);
    }
    fonts::main_font =
        draw_api::create_font(DATA_PATH "/fonts/verasans.ttf", 14);
}

bool draw::EntityCenterToScreen(CachedEntity *entity, Vector &out)
{
    Vector world, min, max;
    bool succ;

    if (CE_BAD(entity))
        return false;
    RAW_ENT(entity)->GetRenderBounds(min, max);
    world = RAW_ENT(entity)->GetAbsOrigin();
    world.z += (min.z + max.z) / 2;
    succ = draw::WorldToScreen(world, out);
    return succ;
}

VMatrix draw::wts{};

void draw::UpdateWTS()
{
    memcpy(&draw::wts, &g_IEngine->WorldToScreenMatrix(), sizeof(VMatrix));
}

bool draw::WorldToScreen(const Vector &origin, Vector &screen)
{
    float w, odw;
    screen.z = 0;
    w = wts[3][0] * origin[0] + wts[3][1] * origin[1] + wts[3][2] * origin[2] +
        wts[3][3];
    if (w > 0.001)
    {
        odw      = 1.0f / w;
        screen.x = (draw::width / 2) +
                   (0.5 * ((wts[0][0] * origin[0] + wts[0][1] * origin[1] +
                            wts[0][2] * origin[2] + wts[0][3]) *
                           odw) *
                        draw::width +
                    0.5);
        screen.y = (draw::height / 2) -
                   (0.5 * ((wts[1][0] * origin[0] + wts[1][1] * origin[1] +
                            wts[1][2] * origin[2] + wts[1][3]) *
                           odw) *
                        draw::height +
                    0.5);
        return true;
    }
    return false;
}


void draw::DrawRect(int x, int y, int w, int h, int color) {
	g_ISurface->DrawSetColor(*reinterpret_cast<Color*>(&color));
	g_ISurface->DrawFilledRect(x, y, x + w, y + h);
}

void draw::DrawLine(int x, int y, int dx, int dy, int color) {
	g_ISurface->DrawSetColor(*reinterpret_cast<Color*>(&color));
	g_ISurface->DrawLine(x, y, x + dx, y + dy);
}

void draw::OutlineRect(int x, int y, int w, int h, int color) {
	g_ISurface->DrawSetColor(*reinterpret_cast<Color*>(&color));
	g_ISurface->DrawOutlinedRect(x, y, x + w, y + h);
}

void draw::DrawCircle(float x, float y, float r, int num_segments, int color) {
	if (num_segments < 3 || r == 0 ) return;
	g_ISurface->DrawSetColor(*reinterpret_cast<Color*>(&color));
	float Step = PI * 2.0 / num_segments;
    for (float a = 0; a < (PI*2.0); a += Step) {
        float x1 = r * cos(a) + x;
        float y1 = r * sin(a) + y;
        float x2 = r * cos(a + Step) + x;
        float y2 = r * sin(a + Step) + y;
		g_ISurface->DrawLine(x1, y1, x2, y2);
    }
}

void draw::GetStringLength(unsigned long font, char* string, int& length, int& height) {
	wchar_t buf[512];
	memset(buf, 0, sizeof(wchar_t) * 512);
	mbstowcs(buf, string, strlen(string));
	g_ISurface->GetTextSize(font, buf, length, height);
}

void draw::String (unsigned long font, int x, int y, int color, int shadow, const char* text) {
	bool newlined;
	int w, h, s, n;
	char ch[512];
	wchar_t string[512];
	size_t len;

	newlined = false;
	len = strlen(text);
	for (int i = 0; i < len; i++) {
		if (text[i] == '\n') {
			newlined = true; break;
		}
	}
	if (newlined) {
		memset(ch, 0, sizeof(char) * 512);
		GetStringLength(font, "W", w, h);
		strncpy(ch, text, 511);
		s = 0;
		n = 0;
		for (int i = 0; i < len; i++) {
			if (ch[i] == '\n') {
				ch[i] = 0;
				draw::String(font, x, y + n * (h), color, shadow, &ch[0] + s);
				n++;
				s = i + 1;
			}
		}
		draw::String(font, x, y + n * (h), color, shadow, &ch[0] + s);
	} else {
		memset(string, 0, sizeof(wchar_t) * 512);
		mbstowcs(string, text, 511);
		draw::WString(font, x, y, color, shadow, string);
	}
}

void draw::String(unsigned long font, int x, int y, int color, int shadow, std::string text) {
	draw::String(font, x, y, color, shadow, text.c_str());
}
CatVar fast_outline(CV_SWITCH, "fast_outline", "0", "Fast font outline", "Use only single repaint to increase performance");
void draw::WString(unsigned long font, int x, int y, int color, int shadow, const wchar_t* text) {
	unsigned char alpha;
	int black_t;

	if (shadow) {
		alpha = (color >> 24);
		black_t = ((alpha == 255) ? colors2::black : colors2::Create(0, 0, 0, alpha / shadow));
		if (shadow > 0) {
			draw::WString(font, x + 1, y + 1, black_t, false, text);
		}
		if (shadow > 1 && !fast_outline) {
			draw::WString(font, x - 1, y + 1, black_t, false, text);
			draw::WString(font, x - 1, y - 1, black_t, false, text);
			draw::WString(font, x + 1, y - 1, black_t, false, text);
			draw::WString(font, x + 1, y, black_t, false, text);
			draw::WString(font, x, y + 1, black_t, false, text);
			draw::WString(font, x, y - 1, black_t, false, text);
			draw::WString(font, x - 1, y, black_t, false, text);
		}
	}
	g_ISurface->DrawSetTextPos(x, y);
	g_ISurface->DrawSetTextColor(*reinterpret_cast<Color*>(&color));
	g_ISurface->DrawSetTextFont(font);
	g_ISurface->DrawUnicodeString(text);
}

void draw::FString(unsigned long font, int x, int y, int color, int shadow, const char* text, ...) {
	va_list list;
	char buffer[2048] = { '\0' };
	va_start(list, text);
	vsprintf(buffer, text, list);
	va_end(list);
	draw::String(font, x, y, color, shadow, buffer);
}

std::pair<int, int> draw::GetStringLength(unsigned long font, std::string string) {
	int l, h;
	draw::GetStringLength(font, (char*)string.c_str(), l, h);
	return std::make_pair(l, h);
}


#endif
