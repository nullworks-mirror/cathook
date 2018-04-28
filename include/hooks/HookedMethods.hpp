
/*
  Created by Jenny White on 28.04.18.
  Copyright (c) 2018 nullworks. All rights reserved.
*/


#pragma once

#include "common.hpp"

extern bool *bSendPackets;
extern CatVar no_zoom;
extern CatVar clean_screenshots;
extern CatVar disable_visuals;
extern CatVar disconnect_reason;
#if ENABLE_VISUALS
extern int spectator_target;
#endif

#if ENABLE_VISUALS
union SDL_Event;
#endif

namespace types
{
// ClientMode
using CreateMove = bool(*)(void *, float, CUserCmd *);
using LevelInit = void(*)(void *, const char *);
using LevelShutdown = void(*)(void *);
// ClientMode + 4
using FireGameEvent = void(*)(void *_this, IGameEvent *event);
// IBaseClient
using DispatchUserMessage = bool(*)(void *, int, bf_read &);
// IInput
using GetUserCmd = CUserCmd *(*)(IInput *, int);

using CanPacket = bool(*)(void *);
using IN_KeyEvent = int(*)(void *, int, int, const char *);
using SendNetMsg = bool(*)(void *, INetMessage &, bool, bool);
using Shutdown = void(*)(void *, const char *);
using BeginFrame = void(*)(IStudioRender *);
using CanInspect = bool(*)(IClientEntity *);
using GetClientName = const char *(*)(CBaseClientState *);
using ProcessSetConVar = bool(*)(CBaseClientState *, NET_SetConVar *);
using ProcessGetCvarValue = bool(*)(CBaseClientState *, SVC_GetCvarValue *);
// ISteamFriends
using GetFriendPersonaName = const char *(*)(ISteamFriends *, CSteamID);

#if ENABLE_VISUALS
// ClientMode
using OverrideView = void(*)(void *, CViewSetup *);
// IEngineVGui
using Paint = void(*)(IEngineVGui *, PaintMode_t);
//IVModelRender
using DrawModelExecute = void(*)(IVModelRender *, const DrawModelState_t &,
                                 const ModelRenderInfo_t &, matrix3x4_t *);
// IBaseClient
using FrameStageNotify = void(*)(void *, int);

using PaintTraverse = void(*)(void *, unsigned int, bool, bool);
// SDL
using SDL_GL_SwapWindow = void(*)(SDL_Window *window);
using SDL_PollEvent = int(*)(SDL_Event *event);
// Manual
using RandomInt = int(*)(void *, int, int);
#endif
}

namespace methods
{
bool CreateMove(void *, float, CUserCmd *);
void PaintTraverse(void *, unsigned int, bool, bool);
const char *GetClientName(CBaseClientState *_this);
bool ProcessSetConVar(CBaseClientState *_this, NET_SetConVar *msg);
bool ProcessGetCvarValue(CBaseClientState *_this, SVC_GetCvarValue *msg);
const char *GetFriendPersonaName(ISteamFriends *_this, CSteamID steamID);
void FireGameEvent(void *_this, IGameEvent *event);
CUserCmd *GetUserCmd(IInput *, int);
void DrawModelExecute(IVModelRender *_this, const DrawModelState_t &state,
                           const ModelRenderInfo_t &info, matrix3x4_t *matrix);
bool CanPacket(void *);
int IN_KeyEvent(void *, int, int, const char *);
bool SendNetMsg(void *, INetMessage &, bool, bool);
void Shutdown(void *, const char *);
void OverrideView(void *, CViewSetup *);
bool DispatchUserMessage(void *, int, bf_read &);
void FrameStageNotify(void *, int);
void LevelInit(void *, const char *);
void LevelShutdown(void *);
int RandomInt(void *, int, int);
int SDL_PollEvent(SDL_Event *event);
void SDL_GL_SwapWindow(SDL_Window *window);
void Paint(IEngineVGui *_this, PaintMode_t mode);

}



// typedef void(*CInput__CreateMove_t)(void*, int, float, bool);
// void CInput__CreateMove_hook(void*, int sequence_number, float
// input_sample_frametime, bool active);


#if ENABLE_VISUALS

/* SDL HOOKS */
union SDL_Event;
class SDL_Window;

extern SDL_Window *sdl_current_window;



void DoSDLHooking();
void DoSDLUnhooking();
#endif


// TODO!
#if 0

#if ENABLE_NULL_GRAPHICS
typedef ITexture *(*FindTexture_t)(void *, const char *, const char *, bool,
                                   int);
typedef IMaterial *(*FindMaterialEx_t)(void *, const char *, const char *, int,
                                       bool, const char *);
typedef IMaterial *(*FindMaterial_t)(void *, const char *, const char *, bool,
                                     const char *);

/* 70 */ void ReloadTextures_null_hook(void *this_);
/* 71 */ void ReloadMaterials_null_hook(void *this_, const char *pSubString);
/* 73 */ IMaterial *FindMaterial_null_hook(void *this_,
                                           char const *pMaterialName,
                                           const char *pTextureGroupName,
                                           bool complain,
                                           const char *pComplainPrefix);
/* 81 */ ITexture *FindTexture_null_hook(void *this_, char const *pTextureName,
                                         const char *pTextureGroupName,
                                         bool complain,
                                         int nAdditionalCreationFlags);
/* 121 */ void ReloadFilesInList_null_hook(void *this_,
                                           IFileList *pFilesToReload);
/* 123 */ IMaterial *FindMaterialEx_null_hook(void *this_,
                                              char const *pMaterialName,
                                              const char *pTextureGroupName,
                                              int nContext, bool complain,
                                              const char *pComplainPrefix);
#endif

#endif