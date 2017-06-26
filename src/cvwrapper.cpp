/*
 * cvwrapper.cpp
 *
 *  Created on: Dec 18, 2016
 *      Author: nullifiedcat
 */

#include "cvwrapper.h"

#include "common.h"
#include "sdk.h"

int CatVar::last_id { 0 };

// Prevent initialization errors.
std::vector<CatVar*>& registrationArray() {
	static std::vector<CatVar*> vector;
	return vector;
}

std::vector<CatCommand*>& commandRegistrationArray() {
	static std::vector<CatCommand*> vector;
	return vector;
}


CatCommand::CatCommand(std::string _name, std::string _help, FnCommandCallback_t _callback)
	: name(_name), help(_help), callback(_callback) {
	commandRegistrationArray().push_back(this);
}

CatCommand::CatCommand(std::string _name, std::string _help, FnCommandCallbackVoid_t _callback)
	: name(_name), help(_help), callback_void(_callback) {
	commandRegistrationArray().push_back(this);
}

void CatCommand::Register() {
	char* name_c = new char[256];
	char* help_c = new char[256];
	if (name.at(0) == '+' || name.at(0) == '-') {
		strncpy(name_c, (name).c_str(), 255);
	} else {
		strncpy(name_c, (CON_PREFIX + name).c_str(), 255);
	}
	strncpy(help_c, help.c_str(), 255);
	if (callback) cmd = new ConCommand(name_c, callback, help_c);
	else if (callback_void) cmd = new ConCommand(name_c, callback_void, help_c);
	else throw std::logic_error("no callback in CatCommand");
	g_ICvar->RegisterConCommand(cmd);
	RegisteredCommandsList().push_back(cmd);
	// name_c and help_c are not freed because ConCommandBase doesn't copy them
}

void RegisterCatCommands() {
	while (!commandRegistrationArray().empty()) {
		CatCommand* cmd = commandRegistrationArray().back();
		cmd->Register();
		commandRegistrationArray().pop_back();
	}
}

CatVar::CatVar(CatVar_t _type, std::string _name, std::string _defaults, std::string _desc_short, std::string _desc_long)
	: type(_type), name(_name), defaults(_defaults), desc_short(_desc_short), desc_long(_desc_long) {
	registrationArray().push_back(this);
}

CatVar::CatVar(CatVar_t _type, std::string _name, std::string _defaults, std::string _desc_short, std::string _desc_long, float max_val)
	: type(_type), name(_name), defaults(_defaults), desc_short(_desc_short), desc_long(_desc_long), restricted(true) {
	max = max_val;
	registrationArray().push_back(this);
}

CatVar::CatVar(CatVar_t _type, std::string _name, std::string _defaults, std::string _desc_short, std::string _desc_long, float min_val, float max_val)
	: type(_type), name(_name), defaults(_defaults), desc_short(_desc_short), desc_long(_desc_long), restricted(true) {
	min = min_val;
	max = max_val;
	registrationArray().push_back(this);
}

CatVar::CatVar(CatEnum& cat_enum, std::string _name, std::string _defaults, std::string _desc_short, std::string _desc_long)
	: type(CV_ENUM), name(_name), defaults(_defaults), desc_short(_desc_short), desc_long(_desc_long), enum_type(&cat_enum), restricted(true) {
	min = cat_enum.min_value;
	max = cat_enum.max_value;
	registrationArray().push_back(this);
}

void CatVar::OnRegister(CatVar::RegisterCallbackFn fn) {
	if (registered) fn(this);
	else callbacks.push_back(fn);
}

void CatVar::Register() {
	CatVarList().push_back(this);
	id = last_id++;
	convar = CreateConVar(CON_PREFIX + name, defaults, desc_short);
	convar_parent = convar->m_pParent;
	while (!callbacks.empty()) {
		callbacks.back()(this);
		callbacks.pop_back();
	}
	registered = true;
}

void CatVar::InstallChangeCallback(FnChangeCallback_t callback) {
	OnRegister([callback](CatVar* var) {
		var->convar_parent->InstallChangeCallback(callback);
	});
}

void RegisterCatVars() {
	while (registrationArray().size()) {
		CatVar* var = registrationArray().back();
		var->Register();
		registrationArray().pop_back();
	}
}

CatEnum::CatEnum(std::vector<std::string> values, int min) : value_names(values) {
	min_value = min;
	max_value = min + int(values.size()) - 1;
	size = int(values.size());
}

std::string CatEnum::Name(int value) {
	if (value >= min_value && value < max_value) {
		return value_names.at(unsigned(value) - unsigned(min_value));
	}
	return "unknown";
}

std::vector<CatVar*>& CatVarList() {
	static std::vector<CatVar*> object {};
	return object;
}

