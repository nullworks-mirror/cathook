/*
 * SkinChanger.cpp
 *
 *  Created on: May 4, 2017
 *      Author: nullifiedcat
 */

#include "SkinChanger.hpp"

namespace hacks { namespace tf2 { namespace skinchanger {

CAttribute::CAttribute(uint16_t iAttributeDefinitionIndex, float flValue) {
	data.defidx = iAttributeDefinitionIndex;
	data.value = flValue;
}

void CAttributeList::RemoveAttribute(int index) {
	for (int i = 0; i < m_Attributes.Count(); i++) {
		const auto& a = m_Attributes[i];
		if (a.data.defidx == index) {
			m_Attributes.Remove(i);
			return;
		}
	}
}

void CAttributeList::SetAttribute(int index, float value) {
	// Let's check if attribute exists already. We don't want dupes.
	for (int i = 0; i < m_Attributes.Count(); i++) {
		auto& a = m_Attributes[i];
		if (a.data.defidx == index) {
			a.data.value = value;
			return;
		}
	}

	if (m_Attributes.Count() > 14)
		return;

	m_Attributes.AddToTail(CAttribute { index, value });
}

static CatVar enabled(CV_SWITCH, "skinchanger", "0", "Skin Changer");
static CatCommand set_attr("skinchanger_set", "Set Attribute", [](const CCommand& args) {
	unsigned attrid = strtoul(args.Arg(1), nullptr, 10);
	unsigned attrv = strtoul(args.Arg(2), nullptr, 10);
	GetModifier(LOCAL_W->m_IDX).Set(attrid, attrv);
	InvalidateCookies();
});

void FrameStageNotify(int stage) {
	if (!enabled) return;
	CachedEntity* weapon = LOCAL_W;
	if (CE_BAD(weapon)) return;
	if (!GetCookie(weapon->m_IDX).Check()) {
		logging::Info("Cookie bad for %i", weapon->m_IDX); // FIXME DEBUG LOGS!
		GetModifier(CE_INT(weapon, netvar.iItemDefinitionIndex)).Apply(weapon->m_IDX);
		GetCookie(weapon->m_IDX).Update(weapon->m_IDX);
	}
}

void PaintTraverse() {
	if (!enabled) return;
	// Debug info?
}

void def_attribute_modifier::Set(int id, float value) {
	for (auto& i : modifiers) {
		if (i.defidx == id) {
			i.value = value;
			return;
		}
	}
	attribute_s& attr = modifiers.at(modifiers.size());
	attr.defidx = id;
	attr.value = value;
}

void InvalidateCookies() {
	logging::Info("All cookies invalidated!"); // FIXME DEBUG LOGS!
	for (auto& cookie : cookie_map) {
		cookie.second.valid = false;
	}
}

patched_weapon_cookie::patched_weapon_cookie(int entity) {
	Update(entity);
}

void patched_weapon_cookie::Update(int entity) {
	IClientEntity* ent = g_IEntityList->GetClientEntity(entity);
	if (!ent || ent->IsDormant()) return;
	logging::Info("Updating cookie for %i", entity); // FIXME DEBUG LOGS!
	eidx = entity;
	defidx = NET_INT(ent, netvar.iItemDefinitionIndex);
	eclass = ent->GetClientClass()->m_ClassID;
	valid = true;
}

bool patched_weapon_cookie::Check() {
	if (!valid) return false;
	IClientEntity* ent = g_IEntityList->GetClientEntity(eidx);
	if (!ent || ent->IsDormant()) return false;
	if (eclass != ent->GetClientClass()->m_ClassID) return false;
	if (defidx != NET_INT(ent, netvar.iItemDefinitionIndex)) return false;
}

void def_attribute_modifier::Apply(int entity) {
	IClientEntity* ent = g_IEntityList->GetClientEntity(entity);
	if (!ent || ent->IsDormant()) return;
	CAttributeList* list = NET_VAR(ent, netvar.AttributeList, CAttributeList*);
	logging::Info("Applying modifiers for %i", entity);
	for (const auto& mod : modifiers) {
		if (mod.defidx) {
			logging::Info("Setting %i to %.2f", mod.defidx, mod.value); // FIXME DEBUG LOGS!
			list->SetAttribute(mod.defidx, mod.value);
		}
	}
}

def_attribute_modifier& GetModifier(int idx) {
	try {
		return modifier_map.at(idx);
	} catch (std::out_of_range& oor) {
		modifier_map.emplace(idx, def_attribute_modifier{});
		return modifier_map.at(idx);
	}
}

patched_weapon_cookie& GetCookie(int idx) {
	try {
		return cookie_map.at(idx);
	} catch (std::out_of_range& oor) {
		cookie_map.emplace(idx, patched_weapon_cookie{idx});
		return cookie_map.at(idx);
	}
}

std::unordered_map<int, def_attribute_modifier> modifier_map {};
std::unordered_map<int, patched_weapon_cookie> cookie_map {};

}}}
