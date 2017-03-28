/*
 * conditions.cpp
 *
 *  Created on: Jan 15, 2017
 *      Author: nullifiedcat
 */

#include "conditions.h"
#include "common.h"

static CatCommand dump_conditions("debug_conditions", "Shows conditions for entity #", [](const CCommand& args) {
	int id = atoi(args.Arg(1));
	if (!id) id = LOCAL_E->m_IDX;
	CachedEntity* ent = ENTITY(id);
	if (CE_BAD(ent)) return;
	condition_data_s old_data = FromOldNetvars(ent);
	logging::Info(format("old conds: ", std::bitset<32>(old_data.cond_0), ' ', std::bitset<32>(old_data.cond_1), ' ', std::bitset<32>(old_data.cond_2), ' ', std::bitset<32>(old_data.cond_3)).c_str());
	condition_data_s new_data = CE_VAR(ent, netvar._condition_bits, condition_data_s);
	logging::Info(format("new conds: ", std::bitset<32>(new_data.cond_0), ' ', std::bitset<32>(new_data.cond_1), ' ', std::bitset<32>(new_data.cond_2), ' ', std::bitset<32>(new_data.cond_3)).c_str());
});

bool BitCheck(condition_data_s data, condition cond) {
	if (cond >= 32 * 3) {
		return data.cond_3 & (1 << (cond % 32));
	}
	if (cond >= 32 * 2) {
		return data.cond_2 & (1 << (cond % 32));
	}
	if (cond >= 32 * 1) {
		return data.cond_1 & (1 << (cond % 32));
	}
	return data.cond_0 & (1 << (cond));
}

void CondBitSet(condition_data_s data, condition cond, bool state) {
	if (state) {
		if (cond > 32 * 3) {
			data.cond_3 |= (1 << (cond % 32));
		} else	if (cond > 32 * 2) {
			data.cond_2 |= (1 << (cond % 32));
		} else if (cond > 32 * 1) {
			data.cond_1 |= (1 << (cond % 32));
		} else {
			data.cond_0 |= (1 << (cond));
		}
	} else {
		if (cond > 32 * 3) {
			data.cond_3 &= ~(1 << (cond % 32));
		} else	if (cond > 32 * 2) {
			data.cond_2 &= ~(1 << (cond % 32));
		} else if (cond > 32 * 1) {
			data.cond_1 &= ~(1 << (cond % 32));
		} else {
			data.cond_0 &= ~(1 << (cond));
		}
	}
}

void OldCondSet(CachedEntity* ent, condition cond, bool state) {
	if (state) {
		if (cond > 32 * 3) {
			CE_INT(ent, netvar.iCond3) |= (1 << (cond % 32));
		} else	if (cond > 32 * 2) {
			CE_INT(ent, netvar.iCond2) |= (1 << (cond % 32));
		} else if (cond > 32 * 1) {
			CE_INT(ent, netvar.iCond1) |= (1 << (cond % 32));
		} else {
			CE_INT(ent, netvar.iCond) |= (1 << (cond));
		}
	} else {
		if (cond > 32 * 3) {
			CE_INT(ent, netvar.iCond3) &= ~(1 << (cond % 32));
		} else	if (cond > 32 * 2) {
			CE_INT(ent, netvar.iCond2) &= ~(1 << (cond % 32));
		} else if (cond > 32 * 1) {
			CE_INT(ent, netvar.iCond1) &= ~(1 << (cond % 32));
		} else {
			CE_INT(ent, netvar.iCond) &= ~(1 << (cond));
		}
	}
}

condition_data_s FromOldNetvars(CachedEntity* ent) {
	condition_data_s result;
	result.cond_0 = CE_INT(ent, netvar.iCond);
	result.cond_1 = CE_INT(ent, netvar.iCond1);
	result.cond_2 = CE_INT(ent, netvar.iCond2);
	result.cond_3 = CE_INT(ent, netvar.iCond3);
	return result;
}

bool HasCondition(CachedEntity* ent, condition cond) {
	if (!TF) return false;
	if (TF2) return BitCheck(CE_VAR(ent, netvar._condition_bits, condition_data_s), cond) || BitCheck(FromOldNetvars(ent), cond);
	return BitCheck(FromOldNetvars(ent), cond);
}

void AddCondition(CachedEntity* ent, condition cond) {
	if (!TF) return;
	if (TF2) CondBitSet(CE_VAR(ent, netvar._condition_bits, condition_data_s), cond, true);
	OldCondSet(ent, cond, true);
}

void RemoveCondition(CachedEntity* ent, condition cond) {
	if (!TF) return;
	if (TF2) CondBitSet(CE_VAR(ent, netvar._condition_bits, condition_data_s), cond, false);
	OldCondSet(ent, cond, false);
}
