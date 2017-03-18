/*
 * entitywrap.cpp
 *
 *  Created on: Mar 18, 2017
 *      Author: nullifiedcat
 */

#include "entitywrap.h"

#include "common.h"

EntityWrapper::EntityWrapper(int idx) : idx(idx) {}

bool EntityWrapper::good() const {
	return idx < g_IEntityList->GetHighestEntityIndex() && g_IEntityList->GetClientEntity(idx);
}

IClientEntity* EntityWrapper::operator ->() const {
	IClientEntity* ret = g_IEntityList->GetClientEntity(idx);
	if (!ret) throw std::runtime_error("referencing null entity");
	return ret;
}
