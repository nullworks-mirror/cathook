/*
 * SkinChanger.cpp
 *
 *  Created on: May 4, 2017
 *      Author: nullifiedcat
 */

#include "SkinChanger.hpp"

namespace hacks { namespace tf2 { namespace skinchanger {

CAttribute::CAttribute(uint16_t iAttributeDefinitionIndex, float flValue) {
	defidx = iAttributeDefinitionIndex;
	value = flValue;
}

void CAttributeList::RemoveAttribute(int index) {
	for (int i = 0; i < m_Attributes.Count(); i++) {
		const auto& a = m_Attributes[i];
		if (a.defidx == index) {
			m_Attributes.Remove(i);
			return;
		}
	}
}

CAttributeList::CAttributeList() {}

void CAttributeList::SetAttribute(int index, float value) {
	// Let's check if attribute exists already. We don't want dupes.
	for (int i = 0; i < m_Attributes.Count(); i++) {
		auto& a = m_Attributes[i];
		if (a.defidx == index) {
			a.value = value;
			return;
		}
	}

	if (m_Attributes.Count() > 14)
		return;

	//m_Attributes.m_Memory.m_nGrowSize = -1;
	//logging::Info("0x%08x 0x%08x 0x%08x", m_Attributes.m_Memory.m_nAllocationCount, m_Attributes.m_Memory.m_nGrowSize, m_Attributes.m_Memory.m_pMemory);
	//m_Attributes.m_Memory.SetExternalBuffer(m_Attributes.m_Memory.Base(), 15);
	CAttribute attr( index, value );
	m_Attributes.AddToTail(attr);
}

static CatVar enabled(CV_SWITCH, "skinchanger", "0", "Skin Changer");
static CatCommand set_attr("skinchanger_set", "Set Attribute", [](const CCommand& args) {
	unsigned attrid = strtoul(args.Arg(1), nullptr, 10);
	unsigned attrv = strtoul(args.Arg(2), nullptr, 10);
	GetModifier(CE_INT(LOCAL_W, netvar.iItemDefinitionIndex)).Set(attrid, attrv);
	InvalidateCookies();
});
static CatCommand set_redirect("skinchanger_redirect", "Set Redirect", [](const CCommand& args) {
	unsigned redirect = strtoul(args.Arg(1), nullptr, 10);
	GetModifier(CE_INT(LOCAL_W, netvar.iItemDefinitionIndex)).defidx_redirect = redirect;
	InvalidateCookies();
});
static CatCommand dump_attrs("skinchanger_debug_attrs", "Dump attributes", []() {
	CAttributeList* list = CE_VAR(LOCAL_W, netvar.AttributeList, CAttributeList*);
	logging::Info("ATTRIBUTE LIST: %i", list->m_Attributes.Size());
	for (int i = 0; i < 15; i++) {
		logging::Info("%i %.2f", list->m_Attributes[i].defidx, list->m_Attributes[i].value);
	}
});
static CatCommand invalidate_cookies("skinchanger_invalidate_cookies", "Invalidate Cookies", InvalidateCookies);

void FrameStageNotify(int stage) {
	if (!enabled) return;
	if (stage != FRAME_NET_UPDATE_POSTDATAUPDATE_START) return;
	int handle = CE_INT(g_pLocalPlayer->entity, netvar.hActiveWeapon);
	int eid = handle & 0xFFF;
	IClientEntity* entity = g_IEntityList->GetClientEntity(eid);
	GetModifier(NET_INT(entity, netvar.iItemDefinitionIndex)).Apply(eid);
	/*if (!GetCookie(weapon->m_IDX).Check()) {
		logging::Info("Cookie bad for %i", weapon->m_IDX); // FIXME DEBUG LOGS!
		GetModifier(CE_INT(weapon, netvar.iItemDefinitionIndex)).Apply(weapon->m_IDX);
		GetCookie(weapon->m_IDX).Update(weapon->m_IDX);
	}*/
}

void PaintTraverse() {
	if (!enabled) return;
	if (CE_GOOD(LOCAL_W))
		AddSideString(format("dIDX: ", CE_INT(LOCAL_W, netvar.iItemDefinitionIndex)));
	// Debug info?
}

void def_attribute_modifier::Set(int id, float value) {
	for (auto& i : modifiers) {
		if (i.defidx == id) {
			i.value = value;
			return;
		}
	}
	attribute_s& attr = modifiers.at(first_free_mod);
	first_free_mod++;
	attr.defidx = id;
	attr.value = value;
	logging::Info("new attr: %i %.2f %i", attr.defidx, attr.value, first_free_mod);
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
	CAttributeList* list = NET_VAR(ent, 0x9c0, CAttributeList*);
	attrs = list->m_Attributes.Size();
	eidx = entity;
	defidx = NET_INT(ent, netvar.iItemDefinitionIndex);
	eclass = ent->GetClientClass()->m_ClassID;
	valid = true;
}

bool patched_weapon_cookie::Check() {
	if (!valid) return false;
	IClientEntity* ent = g_IEntityList->GetClientEntity(eidx);
	if (!ent || ent->IsDormant()) return false;
	CAttributeList* list = NET_VAR(ent, 0x9c0, CAttributeList*);
	if (attrs != list->m_Attributes.Size()) return false;
	if (eclass != ent->GetClientClass()->m_ClassID) return false;
	if (defidx != NET_INT(ent, netvar.iItemDefinitionIndex)) return false;
	return true;
}

void def_attribute_modifier::Apply(int entity) {
	IClientEntity* ent = g_IEntityList->GetClientEntity(entity);
	if (!ent) return;
	//logging::Info("Applying modifiers for %i %i %i", entity, NET_INT(ent, netvar.iItemDefinitionIndex), defidx_redirect);
	if (defidx_redirect && NET_INT(ent, netvar.iItemDefinitionIndex) != defidx_redirect) {
		NET_INT(ent, netvar.iItemDefinitionIndex) = defidx_redirect;
		logging::Info("Updated DefIDX to %i", NET_INT(ent, netvar.iItemDefinitionIndex));
		GetCookie(entity).valid = false;
		return;
	}
	CAttributeList* list = NET_VAR(ent, netvar.AttributeList, CAttributeList*);
	//::Info("Attribute list: 0x%08x 0x%08x 0x%08x 0x%08x", 0x9c0, ent, list, (uint32_t)list - (uint32_t)ent);
	list->m_Attributes.m_Size = list->m_Attributes.Size();
	//logging::Info("Length: %i", list->m_Attributes.m_Size);
	//logging::Info("Base: 0x%08x", list->m_Attributes.Base());
	for (const auto& mod : modifiers) {
		if (mod.defidx) {
			//logging::Info("Setting %i to %.2f", mod.defidx, mod.value); // FIXME DEBUG LOGS!
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
