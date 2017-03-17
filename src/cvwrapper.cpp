/*
 * cvwrapper.cpp
 *
 *  Created on: Dec 18, 2016
 *      Author: nullifiedcat
 */

#include "cvwrapper.h"

#include "common.h"
#include "sdk.h"

// Prevent initialization errors.
std::vector<CatVar*>& registrationArray() {
	static std::vector<CatVar*> vector;
	return vector;
}

CatVar::CatVar(CatVar_t type, std::string name, std::string value, std::string help, CatEnum* enum_type, std::string long_description, bool hasminmax, float maxv, float minv)
	: type(type), name(name), defaults(value), desc_short(help), desc_long(long_description), enum_type(enum_type), callbacks{} {
	logging::Info("Var %s", name.c_str());
	min = minv;
	max = maxv;
	restricted = hasminmax;
	registrationArray().push_back(this);
	logging::Info("%d", registrationArray().size());
}

CatVar::CatVar(CatVar_t type, std::string name, std::string defaults, std::string desc_short, std::string desc_long)
	: type(type), name(name), defaults(defaults), desc_short(desc_short), desc_long(desc_long), enum_type(nullptr), restricted(false), callbacks{} {
	// For some reason, adding min(0.0f), max(0.0f) gives a compilation error.
	logging::Info("Var %s", name.c_str());
	min = 0.0f;
	max = 0.0f;
	registrationArray().push_back(this);
	logging::Info("%d", registrationArray().size());
}

CatVar::CatVar(CatVar_t type, std::string name, std::string defaults, std::string desc_short, std::string desc_long, float max_val)
	: type(type), name(name), defaults(defaults), desc_short(desc_short), desc_long(desc_long), enum_type(nullptr), restricted(true), callbacks{} {
	min = 0.0f;
	max = max_val;
	registrationArray().push_back(this);
}

CatVar::CatVar(CatVar_t type, std::string name, std::string defaults, std::string desc_short, std::string desc_long, float min_val, float max_val)
	: type(type), name(name), defaults(defaults), desc_short(desc_short), desc_long(desc_long), enum_type(nullptr), restricted(true), callbacks{} {
	min = min_val;
	max = max_val;
	registrationArray().push_back(this);
}

CatVar::CatVar(CatEnum& cat_enum, std::string name, std::string defaults, std::string desc_short, std::string desc_long)
	: type(CV_ENUM), name(name), defaults(defaults), desc_short(desc_short), desc_long(desc_long), enum_type(&cat_enum), restricted(true), callbacks{} {
	min = cat_enum.Minimum();
	max = cat_enum.Maximum();
	registrationArray().push_back(this);
}

void CatVar::OnRegister(CatVar::RegisterCallbackFn fn) {
	if (registered) fn(this);
	else callbacks.push_back(fn);
}

void CatVar::Register() {
	convar = CreateConVar(CON_PREFIX + name, defaults, desc_short);
	convar_parent = convar->m_pParent;
	while (!callbacks.empty()) {
		callbacks.back()(this);
		callbacks.pop_back();
	}
	registered = true;
}

void RegisterCatVars() {
	logging::Info("%d", registrationArray().size());
	while (registrationArray().size()) {
		CatVar* var = registrationArray().back();
		logging::Info("Registering %s", var->name.c_str());
		var->Register();
		registrationArray().pop_back();
	}
}

CatEnum::CatEnum(std::vector<std::string> values, int min) : m_values(values) {
	m_iMin = min;
	m_iMax = min + values.size() - 1;
	m_iLength = values.size();
}

std::string CatEnum::Name(int value) {
	if (value - m_iMin >= 0 && value - m_iMin <= m_iMax) {
		return m_values.at(value - Minimum());
	}
	return "unknown";
}

int CatEnum::Maximum() const {
	return m_iMax;
}

int CatEnum::Minimum() const {
	return m_iMin;
}

