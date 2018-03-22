/*
 * resource.hpp
 *
 *  Created on: Mar 28, 2017
 *      Author: nullifiedcat
 */

#ifndef RESOURCE_HPP_
#define RESOURCE_HPP_

#include "common.hpp"

class Texture
{
public:
    Texture(unsigned char *start, unsigned w, unsigned h);
    ~Texture();
    void Load();
    void Draw(int x, int y, int w, int h,
              int color = colorsint::Create(255, 255, 255, 255));

public:
    int id{ 0 };
    const unsigned char *const start_addr;
    const unsigned w;
    const unsigned h;
};

#endif /* RESOURCE_HPP_ */
