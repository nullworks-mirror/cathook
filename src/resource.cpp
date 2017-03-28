/*
 * resource.cpp
 *
 *  Created on: Mar 28, 2017
 *      Author: nullifiedcat
 */

#include "resource.hpp"
#include "common.h"

Texture::Texture(unsigned char* start, unsigned w, unsigned h)
	: start_addr(start), w(w), h(h) {}

Texture::~Texture() {
	if (id) {
		g_ISurface->DeleteTextureByID(id);
	}
}

void Texture::Load() {
	id = g_ISurface->CreateNewTextureID(true);
	logging::Info("Loading %ix%i texture from 0x%08x: got id %i", w, h, start_addr, id);
	//g_ISurface->DrawSetTextureRGBA(id, start_addr, w, h, 0, 0);
	g_ISurface->DrawSetTextureRGBAEx(id, start_addr, w, h, ImageFormat::IMAGE_FORMAT_RGBA8888);
}

void Texture::Draw(int x, int y, int sw, int sh, int color) {
	if (!g_ISurface->IsTextureIDValid(id)) throw std::runtime_error("Invalid texture ID!");
	g_ISurface->DrawSetColor(*reinterpret_cast<Color*>(&color));
	g_ISurface->DrawSetTexture(id);
	g_ISurface->DrawTexturedRect(x, y, x + sw, y + sh);
}
