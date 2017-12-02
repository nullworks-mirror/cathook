/*
 * sdk.h
 *
 *  Created on: Dec 5, 2016
 *      Author: nullifiedcat
 */

#ifndef SDK_HPP_
#define SDK_HPP_

#include <fixsdk.hpp>

#define private public
#define protected public

#include <Color.h>
#include <KeyValues.h>
#include <VGuiMatSurface/IMatSystemSurface.h>
#include <basehandle.h>
#include <cdll_int.h>
#include <checksum_md5.h>
#include <client_class.h>
#include <cmodel.h>
#include <dbg.h>
#include <engine/ICollideable.h>
#include <engine/IEngineTrace.h>
#include <engine/ivdebugoverlay.h>
#include <engine/ivmodelinfo.h>
#include <gametrace.h>
#include <globalvars_base.h>
#include <iachievementmgr.h>
#include <iclient.h>
#include <icliententity.h>
#include <icliententitylist.h>
#include <icommandline.h>
#include <iconvar.h>
#include <icvar.h>
#include <igameevents.h>
#include <inetchannel.h>
#include <inetchannelinfo.h>
#include <inetmessage.h>
#include <inputsystem/iinputsystem.h>
#include <iprediction.h>
#include <iserver.h>
#include <ivrenderview.h>
#include <materialsystem/imaterialvar.h>
#include <materialsystem/itexture.h>
#include <mathlib/vector.h>
#include <mathlib/vmatrix.h>
#include <netmessage.hpp>
#include <steam/isteamuser.h>
#include <steam/steam_api.h>
#include <studio.h>
#include <vgui/Cursor.h>
#include <vgui/IPanel.h>
#include <vgui/ISurface.h>
#include <view_shared.h>

#include "sdk/CGameRules.h"
#include "sdk/HUD.h"
#include "sdk/ScreenSpaceEffects.h"
#include "sdk/TFGCClientSystem.hpp"
#include "sdk/igamemovement.h"
#include "sdk/iinput.h"
#include "sdk/imaterialsystemfixed.h"
#include "sdk/in_buttons.h"

#endif /* SDK_HPP_ */
