/*
 * drawing.cpp
 *
 *  Created on: Oct 5, 2016
 *      Author: nullifiedcat
 */

#include "common.hpp"

#include <glez/glez.hpp>
#include <glez/draw.hpp>
#include <GL/glew.h>
#include <SDL2/SDL_video.h>
#include <SDLHooks.hpp>
#include "menu/GuiInterface.hpp"
#include "picopng.hpp"

// String -> Wstring
#include <locale>
#include <codecvt>

#if EXTERNAL_DRAWING
#include "xoverlay.h"
#endif

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
        draw::String(8, y, side_strings_colors[i], side_strings[i].c_str());
        y += fonts::menu->size + 1;
    }
    y = draw::height / 2;
    for (size_t i = 0; i < center_strings_count; ++i)
    {
        float sx, sy;
        fonts::menu->stringSize(center_strings[i], &sx, &sy);
        draw::String((draw::width - sx) / 2, y, center_strings_colors[i], center_strings[i].c_str());
        y += fonts::menu->size + 1;
    }
}

void AddCenterString(const std::string &string, const rgba_t &color)
{
    center_strings[center_strings_count]        = string;
    center_strings_colors[center_strings_count] = color;
    ++center_strings_count;
}

int draw::width  = 0;
int draw::height = 0;
float draw::fov  = 90.0f;
std::mutex draw::draw_mutex;

namespace fonts
{

std::unique_ptr<glez::font> menu{ nullptr };
std::unique_ptr<glez::font> esp{ nullptr };
unsigned long surface_font{ 0 };
} // namespace fonts

namespace draw
{

int texture_white = 0;

void Initialize()
{
    if (!draw::width || !draw::height)
    {
        g_IEngine->GetScreenSize(draw::width, draw::height);
    }
    glez::preInit();
    fonts::menu.reset(new glez::font(DATA_PATH "/fonts/verasans.ttf", 14));
    fonts::esp.reset(new glez::font(DATA_PATH "/fonts/verasans.ttf", 14));
    fonts::surface_font = g_ISurface->CreateFont();
    g_ISurface->SetFontGlyphSet(fonts::surface_font, "Verasans", 15, 500, 0, 0, vgui::ISurface::FONTFLAG_ANTIALIAS | vgui::ISurface::FONTFLAG_ADDITIVE);
    g_ISurface->AddCustomFontFile("Verasans", DATA_PATH "/fonts/verasans.ttf");

    texture_white                = g_ISurface->CreateNewTextureID();
    unsigned char colorBuffer[4] = { 255, 255, 255, 255 };
    g_ISurface->DrawSetTextureRGBA(texture_white, colorBuffer, 1, 1, false, true);
}

void String(int x, int y, rgba_t rgba, const char *text)
{
#if !ENABLE_ENGINE_DRAWING
    glez::draw::outlined_string(x, y, text, *fonts::menu, rgba, colors::black, nullptr, nullptr);
#else
    rgba = rgba * 255.0f;
    g_ISurface->DrawSetTextPos(x, y);
    g_ISurface->DrawSetTextFont(fonts::surface_font);
    g_ISurface->DrawSetTextColor(rgba.r, rgba.g, rgba.b, rgba.a);

    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t> > converter;
    std::wstring ws = converter.from_bytes(text);

    g_ISurface->DrawPrintText(ws.c_str(), ws.size() + 1);
#endif
}

void Line(float x1, float y1, float x2, float y2, rgba_t color, float thickness)
{
#if !ENABLE_ENGINE_DRAWING
    glez::draw::line(x1, y1, x2, y2, color, thickness);
#else
    color = color * 255.0f;
    g_ISurface->DrawSetTexture(texture_white);
    g_ISurface->DrawSetColor(color.r, color.g, color.b, color.a);

    // Dirty
    x1 += 0.5f;
    y1 += 0.5f;

    float length = sqrtf(x2 * x2 + y2 * y2);
    x2 *= (length - 1.0f) / length;
    y2 *= (length - 1.0f) / length;

    float nx = x2;
    float ny = y2;

    float ex = x1 + x2;
    float ey = y1 + y2;

    if (length <= 1.0f)
        return;

    nx /= length;
    ny /= length;

    float th = thickness;

    nx *= th * 0.5f;
    ny *= th * 0.5f;

    float px = ny;
    float py = -nx;

    vgui::Vertex_t vertices[4];

    vertices[2].m_Position = { float(x1) - nx + px, float(y1) - ny + py };
    vertices[1].m_Position = { float(x1) - nx - px, float(y1) - ny - py };
    vertices[3].m_Position = { ex + nx + px, ey + ny + py };
    vertices[0].m_Position = { ex + nx - px, ey + ny - py };

    g_ISurface->DrawTexturedPolygon(4, vertices);
#endif
}

void Rectangle(float x, float y, float w, float h, rgba_t color)
{
#if !ENABLE_ENGINE_DRAWING
    glez::draw::rect(x, y, w, h, color);
#else
    color = color * 255.0f;
    g_ISurface->DrawSetTexture(texture_white);
    g_ISurface->DrawSetColor(color.r, color.g, color.b, color.a);

    vgui::Vertex_t vertices[4];
    vertices[0].m_Position = { x, y };
    vertices[1].m_Position = { x, y + h };
    vertices[2].m_Position = { x + w, y + h };
    vertices[3].m_Position = { x + w, y };

    g_ISurface->DrawTexturedPolygon(4, vertices);
#endif
}

void RectangleOutlined(float x, float y, float w, float h, rgba_t color, float thickness)
{
    Rectangle(x, y, w, 1, color);
    Rectangle(x, y, 1, h, color);
    Rectangle(x + w - 1, y, 1, h, color);
    Rectangle(x, y + h - 1, w, 1, color);
}

void RectangleTextured(float x, float y, float w, float h, rgba_t color, Texture &texture, float tx, float ty, float tw, float th, float angle)
{
#if !ENABLE_ENGINE_DRAWING
    glez::draw::rect_textured(x, y, w, h, color, texture, tx, ty, tw, th, angle);
#else
    color = color * 255.0f;
    vgui::Vertex_t vertices[4];
    g_ISurface->DrawSetColor(color.r, color.g, color.b, color.a);
    g_ISurface->DrawSetTexture(texture.get());

    float tex_width  = texture.getWidth();
    float tex_height = texture.getHeight();

    Vector2D scr_top_left     = { x, y };
    Vector2D scr_top_right    = { x + w, y };
    Vector2D scr_bottom_right = { x + w, y + h };
    Vector2D scr_botton_left  = { x, y + w };

    if (angle != 0.0f)
    {
        float cx = x + float(w) / 2.0f;
        float cy = y + float(h) / 2.0f;

        auto f = [&](Vector2D &v) {
            v.x = cx + cosf(angle) * (v.x - cx) - sinf(angle) * (v.y - cy);
            v.y = cy + sinf(angle) * (v.x - cx) + cosf(angle) * (v.y - cy);
        };
        f(scr_top_left);
        f(scr_top_right);
        f(scr_bottom_right);
        f(scr_bottom_right);
    }

    Vector2D tex_top_left     = { tx / tex_width, ty / tex_height };
    Vector2D tex_top_right    = { (tx + tw) / tex_width, ty / tex_height };
    Vector2D tex_bottom_right = { (tx + tw) / tex_width, (ty + th) / tex_height };
    Vector2D tex_botton_left  = { tx / tex_width, (ty + th) / tex_height };
    // logging::Info("%f,%f %f,%f", tex_top_left.x, tex_top_left.y, tex_top_right.x, tex_top_right.y);

    vertices[0].Init(scr_top_left, tex_top_left);
    vertices[1].Init(scr_top_right, tex_top_right);
    vertices[2].Init(scr_bottom_right, tex_bottom_right);
    vertices[3].Init(scr_botton_left, tex_botton_left);

    g_ISurface->DrawTexturedPolygon(4, vertices);
#endif
}

bool EntityCenterToScreen(CachedEntity *entity, Vector &out)
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

VMatrix wts{};

void UpdateWTS()
{
    memcpy(&wts, &g_IEngine->WorldToScreenMatrix(), sizeof(VMatrix));
}

bool WorldToScreen(const Vector &origin, Vector &screen)
{
    return g_IVDebugOverlay->ScreenPosition(origin, screen) == 0;
}

bool Texture::load()
{
    std::ifstream file(path.c_str(), std::ios::in | std::ios::binary | std::ios::ate);

    std::streamsize size = 0;
    if (file.seekg(0, std::ios::end).good())
        size = file.tellg();
    if (file.seekg(0, std::ios::beg).good())
        size -= file.tellg();

    if (size < 1)
        return false;

    unsigned char *buffer = new unsigned char[(size_t) size + 1];
    file.read((char *) buffer, size);
    file.close();
    int error = decodePNG(data, m_width, m_height, buffer, size);

    // if there's an error, display it and return false to indicate failure
    if (error != 0)
    {
        logging::Info("Error loading texture, error code %i\n", error);
        return false;
    }
    texture_id = g_ISurface->CreateNewTextureID(true);
    // g_ISurface->DrawSetTextureRGBA(texture_id, data, m_width, m_height, false, false);
    g_ISurface->DrawSetTextureRGBAEx(texture_id, data, m_width, m_height, ImageFormat::IMAGE_FORMAT_RGBA8888);
    if (!g_ISurface->IsTextureIDValid(texture_id))
        return false;
    init = true;
    return true;
}

Texture::~Texture()
{
    g_ISurface->DeleteTextureByID(texture_id);
}

unsigned int Texture::get()
{
    if (texture_id == 0)
    {
        if (!load())
            throw std::runtime_error("Couldn't init texture!");
    }
    return texture_id;
}

SDL_GLContext context = nullptr;

void InitGL()
{
    logging::Info("InitGL: %d, %d", draw::width, draw::height);
#if EXTERNAL_DRAWING
    int status = xoverlay_init();
    xoverlay_draw_begin();
    glez::init(xoverlay_library.width, xoverlay_library.height);
    xoverlay_draw_end();
    if (status < 0)
    {
        logging::Info("ERROR: could not initialize Xoverlay: %d", status);
    }
    else
    {
        logging::Info("Xoverlay initialized");
    }
    xoverlay_show();
    context = SDL_GL_CreateContext(sdl_hooks::window);
#else
    glClearColor(1.0, 0.0, 0.0, 0.5);
    glewExperimental = GL_TRUE;
    glewInit();
    glez::init(draw::width, draw::height);
#endif

#if ENABLE_GUI
    gui::init();
#endif
}

void BeginGL()
{
    glColor3f(1, 1, 1);
#if EXTERNAL_DRAWING
    xoverlay_draw_begin();
    {
        PROF_SECTION(draw_begin__SDL_GL_MakeCurrent);
        // SDL_GL_MakeCurrent(sdl_hooks::window, context);
    }
#endif
    {
        glActiveTexture(GL_TEXTURE0);
        PROF_SECTION(draw_begin__glez_begin);
        glez::begin();
        glDisable(GL_FRAMEBUFFER_SRGB);
        PROF_SECTION(DRAWEX_draw_begin);
    }
}

void EndGL()
{
    PROF_SECTION(DRAWEX_draw_end);
    {
        PROF_SECTION(draw_end__glez_end);
        glEnable(GL_FRAMEBUFFER_SRGB);
        glez::end();
    }
#if EXTERNAL_DRAWING
    xoverlay_draw_end();
    {
        PROF_SECTION(draw_end__SDL_GL_MakeCurrent);
        SDL_GL_MakeCurrent(sdl_hooks::window, nullptr);
    }
#endif
}
} // namespace draw
