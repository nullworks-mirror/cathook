/*
 * CGameRules.h
 *
 *  Created on: Nov 23, 2017
 *      Author: nullifiedcat
 */

#pragma once
class CGameRules
{
public:
    int pad0[17];               // 0    | 68 bytes  | 68
    int roundmode;              // 68   | 4 bytes   | 72
    int m_bInWaitingForPlayers; // 72   | 4 bytes   | 76
    int winning_team;           // 76   | 4 bytes   | 80
    char pad2[974];             // 100  | 974 bytes | 1054
    bool isPVEMode;             // 1054 | 1 byte    | 1055
    char pad3[15];              // 1055 | 15 bytes  | 1070
    int isUsingSpells;          // 1070 | 4 bytes   | 1074
    char pad4[790];             // 1074 | 790 bytes | 1864
    int halloweenScenario;      // 1864 | 4 bytes   | 1868

    bool isUsingSpells_fn()
    {
        auto tf_spells_enabled = g_ICvar->FindVar("tf_spells_enabled");
        if (tf_spells_enabled->GetBool())
            return true;

        // Hightower
        if (halloweenScenario == 4)
            return true;

        return isUsingSpells;
    }
} __attribute__((packed));
