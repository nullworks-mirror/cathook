/*
 * catpackets.h
 *
 *  Created on: Nov 12, 2017
 *      Author: nullifiedcat
 */

#pragma once

#include <stdint.h>

enum
{
    CATP_DRAW_BEGIN,
    CATP_DRAW_END,

    CATP_DRAW_RECT,
    CATP_DRAW_RECT_OUTLINE,
    CATP_DRAW_LINE,
    CATP_DRAW_STRING,
    CATP_DRAW_CIRCLE,

/*  CATP_DRAW_ESP_BOX,
    CATP_DRAW_ESP_BONE_LIST, */

    CATP_TOTAL
};

struct catp_draw_begin_t
{
    float world_to_screen[16];
};

struct catp_draw_rect_t
{
    float x;
    float y;
    float w;
    float h;
    float rgba[4];
};

struct catp_draw_rect_outline_t
{
    float x;
    float y;
    float w;
    float h;
    float rgba[4];
    float thickness;
};

struct catp_draw_line_t
{
    float x;
    float y;
    float dx;
    float dy;
    float rgba[4];
    float thickness;
};

struct catp_draw_circle_t
{
    float x;
    float y;
    float radius;
    float rgba[4];
    float thickness;
    int steps;
};

struct catp_draw_string_t
{
    float x;
    float y;
    float rgba[4];
    uint32_t length;
};

/*
struct catp_draw_esp_box_t
{

};
*/
