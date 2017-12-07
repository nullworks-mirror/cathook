/*
 * CTFGCClientSystem.hpp
 *
 *  Created on: Dec 7, 2017
 *      Author: nullifiedcat
 */

#pragma once

class CTFGCClientSystem
{
public:
    static CTFGCClientSystem *GTFGCClientSystem();
    static void AbandonCurrentMatch(CTFGCClientSystem *);
};
