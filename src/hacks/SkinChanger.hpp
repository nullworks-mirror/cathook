/*
 * SkinChanger.hpp
 *
 *  Created on: May 4, 2017
 *      Author: nullifiedcat
 */

#ifndef HACKS_SKINCHANGER_HPP_
#define HACKS_SKINCHANGER_HPP_

#include "../common.h"

namespace hacks { namespace tf2 { namespace skinchanger {

// TOTALLY NOT A PASTE.
// Seriously tho, it's modified at least.
// Credits: blackfire62

struct attribute_s {
	uint16_t defidx;
	float value;
};

class CAttribute {
public:
	CAttribute(uint16_t iAttributeDefinitionIndex, float flValue);
public:
	void* vtable;
	uint16_t defidx;
	float value;
	unsigned int pad01;
};

class CAttributeList {
public:
	CAttributeList();
	void SetAttribute(int index, float value);
	void RemoveAttribute(int index);
public:
	uint32_t unknown;
	CUtlVector<CAttribute, CUtlMemory<CAttribute>> m_Attributes;
};

enum class Attributes {
	loot_rarity = 2022,
	is_australium_item = 2027,
	item_style_override = 542,
	sheen = 2014,
	killstreak_tier = 2025
};

enum class UnusualEffects {
	HOT = 701,
	ISOTOPE,
	COOL,
	ENERGY_ORB
};

struct patched_weapon_cookie {
	patched_weapon_cookie(int entity);
	void Update(int entity);
	bool Check();
public:
	int eidx { 0 };
	int defidx { 0 };
	int eclass { 0 };
	int attrs { 0 };
	bool valid { false };
};

struct def_attribute_modifier {
	void Apply(int entity);
	void Set(int id, float value);
	int defidx { 0 };
	int defidx_redirect { 0 };
	std::array<attribute_s, 15> modifiers { attribute_s{ 0, 0 } };
	int first_free_mod { 0 };
};

extern std::unordered_map<int, def_attribute_modifier> modifier_map;
extern std::unordered_map<int, patched_weapon_cookie> cookie_map;

def_attribute_modifier& GetModifier(int idx);
patched_weapon_cookie& GetCookie(int idx);

void InvalidateCookies();
void FrameStageNotify(int stage);
void PaintTraverse();

}}}

#endif /* HACKS_SKINCHANGER_HPP_ */
