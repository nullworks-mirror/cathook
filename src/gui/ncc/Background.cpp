/*
 * Background.cpp
 *
 *  Created on: Apr 28, 2017
 *      Author: nullifiedcat
 */

#include "Menu.hpp"

namespace menu { namespace ncc {

Background::Background() : CBaseWidget("nc_background"), snowflake_texture(&_binary_snowflake_start, 16, 16) {
	SetSize(draw::width, draw::height);
}

static CatVar snowflake_chance(CV_INT, "gui_snowflake_chance", "10", "Snowflake Spawn Rate", "Defines snowflake spawn rate (HAS TO BE NONZERO!)", 1.0f, 100.0f);
static CatVar snowflake_pack_size(CV_INT, "gui_snowflake_pack_size", "10", "Snowflake Max Pack", "Defines max snowflake spawn pack size (HAS TO BE NONZERO!)", 1.0f, 100.0f);
static CatVar snowflake_safe(CV_INT, "gui_snowflake_safe_zone", "100", "Snowflake Safe Zone", "Defines snowflake safe zone (they will decay after reaching that point)", 0.0f, 400.0f);
static CatVar snowflake_gravity(CV_FLOAT, "gui_snowflake_gravity", "0.9", "Snowflake Gravity", "Defines snowflake gravity (HAS TO BE NONZERO!)", 0.01f, 5.0f);
static CatVar snowflake_jittering(CV_INT, "gui_snowflake_jittering", "2", "Snowflake Jittering", "Defines snowflake jittering amount", 0.0f, 10.0f);
static CatVar snowflake_jittering_chance(CV_INT, "gui_snowflake_jittering_chance", "60", "Snowflake Jittering Rate", "Defines snowflake jittering rate (HAS TO BE NONZERO!)", 1.0f, 20.0f);

void Background::Update() {
	Snowflake* current = list;
	while (current) {
		Snowflake* next = current->next;
		current->Update();
		if (current->dead) {
			KillSnowflake(current);
		}
		current = next;
	}
	if (!(rand() % (int)snowflake_chance)) {
		for (int i = 0; i < rand() % (int)snowflake_pack_size; i++) MakeSnowflake();
	}
}

Background::~Background() {
	Snowflake* current = list;
	while (current) {
		Snowflake* next = current->next;
		delete current;
		current = next;
	}
}

void Background::Draw(int x, int y) {
	if (!snowflake_texture.id) {
		snowflake_texture.Load();
	}
	Snowflake* current = list;
	while (current) {
		Snowflake* next = current->next;
		if (!current->show_in) {
			int color = colors::white;
			if (current->y > (int)snowflake_safe) {
				color = colors::Create(255, 255, 255, ((int)snowflake_safe + 255) - current->y);
			}
			snowflake_texture.Draw((int)current->x, (int)current->y, 16, 16, color);
		}
		current = next;
	}
}

void Background::MakeSnowflake() {
	Snowflake* flake = new Snowflake();
	flake->x = RandFloatRange(0, draw::width);
	flake->y = 3 - (rand() % 6);
	flake->dead = false;
	flake->next = nullptr;
	flake->show_in = rand() % 4;
	if (list_tail)
		list_tail->next = flake;
	flake->prev = list_tail;
	list_tail = flake;
	if (!list) {
		list = flake;
	}
}

void Background::KillSnowflake(Snowflake* flake) {
	if (list_tail == flake) {
		list_tail = flake->prev;
	}
	if (list == flake) {
		list = flake->next;
	}
	if (flake->prev) flake->prev->next = flake->next;
	if (flake->next) flake->next->prev = flake->prev;
	delete flake;
}

void Background::Snowflake::Update() {
	if (show_in) show_in--;
	if (!(rand() % (int)(snowflake_jittering_chance))) {
		x += (rand() % 2) ? (int)snowflake_jittering : -(int)snowflake_jittering;
	}
	y += (float)snowflake_gravity;
	if (y > (int)snowflake_safe + 255) dead = true;
}

}}
