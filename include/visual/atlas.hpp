/*
 * atlas.hpp
 *
 *  Created on: May 20, 2017
 *      Author: nullifiedcat
 */

#pragma once

#include "common.hpp"
#include "visual/drawex.hpp"

namespace textures
{

class texture_atlas;
class sprite;

class sprite
{
public:
    sprite(float x, float y, float w, float h, texture_atlas &atlas);

public:
    void setsprite(float x, float y, float w, float h);
    void draw(float scrx, float scry, float scrw, float scrh,
              const rgba_t &rgba);

public:
    float nx;
    float ny;
    float nw;
    float nh;

    texture_atlas &atlas;
};

class texture_atlas
{
public:
    texture_atlas(std::string filename, float width, float height);

public:
    sprite create_sprite(float x, float y, float sx, float sy);

public:
    const float width;
    const float height;

    draw_api::texture_handle_t texture;
};

texture_atlas &atlas();
}
