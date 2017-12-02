/*
 * common.h
 *
 *  Created on: Dec 5, 2016
 *      Author: nullifiedcat
 */

#ifndef COMMON_H_
#define COMMON_H_

#if defined(LINUX) and not defined(NO_IPC)
#define ENABLE_IPC 1
#else
#undef ENABLE_IPC
#endif

#include <algorithm>
#include <array>
#include <atomic>
#include <beforecheaders.hpp>
#include <bitset>
#include <cassert>
#include <cmath>
#include <csignal>
#include <emmintrin.h>
#include <fstream>
#include <functional>
#include <iomanip>
#include <list>
#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

#include <assert.h>
#include <dlfcn.h>
#include <elf.h>
#include <fcntl.h>
#include <link.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/prctl.h>
#include <sys/stat.h>
#include <sys/sysinfo.h>
#include <sys/types.h>
#include <unistd.h>

#include "averager.hpp"
#include "timer.hpp"

#include <aftercheaders.hpp>

#include "macros.hpp"
#include <visual/colors.hpp>

#if ENABLE_VISUALS == 1

#include "fidgetspinner.hpp"
#include <visual/EffectChams.hpp>
#include <visual/EffectGlow.hpp>
#include <visual/atlas.hpp>
#include <visual/drawex.hpp>
#include <visual/drawing.hpp>
#include <visual/drawmgr.hpp>

#endif

#include "angles.hpp"
#include "backpacktf.hpp"
#include "classinfo/classinfo.hpp"
#include "crits.hpp"
#include "entityhitboxcache.hpp"
#include "globals.h"
#include "hooks.hpp"
#include "hooks/hookedmethods.hpp"
#include "hoovy.hpp"
#include "init.hpp"
#include "ipc.hpp"
#include "netvars.hpp"
#include "offsets.hpp"
#include "playerlist.hpp"
#include "playerresource.h"
#include "profiler.hpp"
#include "projlogging.hpp"
#include "reclasses/reclasses.hpp"
#include "sharedobj.hpp"
#include "textfile.hpp"
#include "textmode.hpp"
#include "trace.hpp"
#include "ucccccp_cmds.hpp"
#include "usercmd.hpp"
#include "velocity.hpp"
#include "vfunc.hpp"
#include <chatstack.hpp>
#include <conditions.hpp>
#include <cvwrapper.hpp>
#include <entitycache.hpp>
#include <enums.hpp>
#include <helpers.hpp>
#include <interfaces.hpp>
#include <itemtypes.hpp>
#include <localplayer.hpp>
#include <logging.hpp>
#include <prediction.hpp>
#include <targethelper.hpp>

#include "copypasted/CSignature.h"
#include "copypasted/Netvar.h"

#if ENABLE_GUI
#include "gui/GUI.hpp"
#endif

#include <hacks/hacklist.hpp>

#include <sdk.hpp>

template <typename T> constexpr T _clamp(T _min, T _max, T _val)
{
    return ((_val > _max) ? _max : ((_val < _min) ? _min : _val));
}

#define _FASTCALL __attribute__((fastcall))
#define STRINGIFY(x) #x

#include "gameinfo.hpp"

#define SQR(x) (x) * (x)

#define CON_NAME "cat"
#define CON_PREFIX CON_NAME "_"

#define SUPER_VERBOSE_DEBUG false
#if SUPER_VERBOSE_DEBUG == true
#define SVDBG(...) logging::Info(__VA_ARGS__)
#else
#define SVDBG(...)
#endif

#ifndef DEG2RAD
#define DEG2RAD(x) (float) (x) * (PI / 180.0f)
#endif

#define STR(c) #c

#define GET_RENDER_CONTEXT                                                     \
    (IsTF2() ? g_IMaterialSystem->GetRenderContext()                           \
             : g_IMaterialSystemHL->GetRenderContext())

#endif /* COMMON_H_ */
