/*
 * drawing.h
 *
 *  Created on: Oct 5, 2016
 *      Author: nullifiedcat
 */

#pragma once

#include "config.h"
#if !ENABLE_ENGINE_DRAWING
#include <glez/font.hpp>
#include <glez/draw.hpp>
#endif

#include "colors.hpp"
#include <string>
#include <memory>
#include <vector>

class CachedEntity;
class Vector;
class IClientEntity;
class CatEnum;
class VMatrix;

namespace fonts
{
#if ENABLE_ENGINE_DRAWING
struct font
{
    font(std::string path, int fontsize) : size{ fontsize }, path{ path }
    {
    }
    unsigned int id;
    std::string path;
    int size;
    bool init = false;
    operator unsigned int();
    void stringSize(std::string string, float *x, float *y);
    void Init();
};
#else
typedef glez::font font;
#endif

extern std::unique_ptr<font> esp;
extern std::unique_ptr<font> menu;

void Update();

extern const std::vector<std::string> fonts;
} // namespace fonts

constexpr rgba_t GUIColor()
{
    return colors::white;
}

void InitStrings();
void ResetStrings();
void AddCenterString(const std::string &string, const rgba_t &color = colors::white);
void AddSideString(const std::string &string, const rgba_t &color = colors::white);
void DrawStrings();

namespace draw
{

extern VMatrix wts;

extern int width;
extern int height;
extern float fov;

void Initialize();

#if ENABLE_ENGINE_DRAWING
class Texture
{
public:
    explicit Texture(std::string path) : path{ path }
    {
    }
    bool load();
    unsigned int get();
    ~Texture();
    unsigned int getWidth()
    {
        return m_width;
    }
    unsigned int getHeight()
    {
        return m_height;
    }

private:
    std::string path;
    unsigned int texture_id = 0;
    unsigned char *data     = nullptr;
    bool init               = false;
    int m_width, m_height;
};
#else
typedef glez::texture Texture;
#endif

void Line(float x1, float y1, float x2, float y2, rgba_t color, float thickness);
void String(int x, int y, rgba_t rgba, const char *text, fonts::font &font);
void Rectangle(float x, float y, float w, float h, rgba_t color);
void RectangleOutlined(float x, float y, float w, float h, rgba_t color, float thickness);
void RectangleTextured(float x, float y, float w, float h, rgba_t color, Texture &texture, float tx, float ty, float tw, float th, float angle);
void Circle(float x, float y, float radius, rgba_t color, float thickness, int steps);

void UpdateWTS();
bool WorldToScreen(const Vector &origin, Vector &screen);
bool EntityCenterToScreen(CachedEntity *entity, Vector &out);

void InitGL();
void BeginGL();
void EndGL();

} // namespace draw
