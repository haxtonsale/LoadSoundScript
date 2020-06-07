#include "extension.h"
#include "soundscript.h"
#include <sm_argbuffer.h>
#include <extensions/IBinTools.h>
#include <soundchars.h>

#ifdef PLATFORM_LINUX
#include <link.h>
#endif

SH_DECL_HOOK6(IServerGameDLL, LevelInit, SH_NOATTRIB, false, bool, const char*, const char*, const char*, const char*, bool, bool);

LoadSoundscript g_LoadSoundscript; // Global singleton for extension's main interface.
SMEXT_LINK(&g_LoadSoundscript);

ISoundEmitterSystemBase* soundemittersystem = 0;

#ifndef SOUNDEMITTERSYSTEM_INTERFACE_VERSION_3
IBinTools* bintools = 0;
ICallWrapper* g_pCallAddSoundsFromFile = 0;
void* g_pAddSnd = 0;
#endif

HandleType_t g_SoundScriptHandleType;
SoundScriptTypeHandler g_SoundScriptHandler;
CUtlVector<CSoundScript*> CSoundScript::LoadedSoundScripts;

//-----------------------------------------------------------------------------

#ifndef SOUNDEMITTERSYSTEM_INTERFACE_VERSION_3
//From SM's stringutil.cpp
size_t UTIL_DecodeHexString(unsigned char* buffer, size_t maxlength, const char* hexstr)
{
	size_t written = 0;
	size_t length = strlen(hexstr);

	for (size_t i = 0; i < length; i++)
	{
		if (written >= maxlength)
			break;

		buffer[written++] = hexstr[i];
		if (hexstr[i] == '\\' && hexstr[i + 1] == 'x')
		{
			if (i + 3 >= length)
				continue;

			/* Get the hex part. */
			char s_byte[3];
			int r_byte;
			s_byte[0] = hexstr[i + 2];
			s_byte[1] = hexstr[i + 3];
			s_byte[2] = '\0';

			/* Read it as an integer */
			sscanf(s_byte, "%x", &r_byte);

			/* Save the value */
			buffer[written - 1] = r_byte;

			/* Adjust index */
			i += 3;
		}
	}

	return written;
}

//By https://github.com/peace-maker
void* GetAddressFromKeyValues(void* pBaseAddr, IGameConfig* pGameConfig, const char* sKey)
{
	const char* value = pGameConfig->GetKeyValue(sKey);
	if (!value)
		return nullptr;

	// Got a symbol here.
	if (value[0] == '@')
		return memutils->ResolveSymbol(pBaseAddr, &value[1]);

	// Convert hex signature to byte pattern
	unsigned char signature[256];
	size_t real_bytes = UTIL_DecodeHexString(signature, sizeof(signature), value);
	if (real_bytes < 1)
		return nullptr;

#ifdef PLATFORM_LINUX
	// The pointer returned by dlopen is not inside the loaded library memory region.
	struct link_map* dlmap;
	dlinfo(pBaseAddr, RTLD_DI_LINKMAP, &dlmap);
	pBaseAddr = (void*)dlmap->l_addr;
#endif

	// Find that pattern in the pointed module.
	return memutils->FindPattern(pBaseAddr, (char*)signature, real_bytes);
}
#endif // SOUNDEMITTERSYSTEM_INTERFACE_VERSION_3

void AddSoundsFromFile(const char* filename, bool bPreload, bool bIsOverride, bool bRefresh)
{
#ifndef SOUNDEMITTERSYSTEM_INTERFACE_VERSION_3
#ifndef NO_REFRESH_PARAM
	ArgBuffer<ISoundEmitterSystemBase*, const char*, bool, bool, bool> vstk(soundemittersystem, filename, bPreload, bIsOverride, bRefresh);
#else
	ArgBuffer<ISoundEmitterSystemBase*, const char*, bool, bool> vstk(soundemittersystem, filename, bPreload, bIsOverride);
#endif // NO_REFRESH_PARAM
	g_pCallAddSoundsFromFile->Execute(vstk, nullptr);
#else
	soundemittersystem->AddSoundsFromFile(filename, bPreload, false, bIsOverride);
#endif // SOUNDEMITTERSYSTEM_INTERFACE_VERSION_3
}

CSoundScript* GetSoundScriptFromHandle(cell_t cellhandle, IPluginContext* pContext)
{
	Handle_t handle = (Handle_t)cellhandle;
	HandleError hndlError;
	HandleSecurity hndlSecurity;

	hndlSecurity.pOwner = nullptr;
	hndlSecurity.pIdentity = myself->GetIdentity();

	CSoundScript* pSoundScript;
	if ((hndlError = g_pHandleSys->ReadHandle(handle, g_SoundScriptHandleType, &hndlSecurity, (void**)&pSoundScript)) != HandleError_None)
	{
		if (!pContext)
			smutils->LogError(myself, "Invalid CSoundScript handle %x (error %d)", handle, hndlError);
		else
			pContext->ThrowNativeError("Invalid CSoundScript handle %x (error %d)", handle, hndlError);

		return nullptr;
	}

	return pSoundScript;
}

void SoundScriptTypeHandler::OnHandleDestroy(HandleType_t type, void *object)
{
	CSoundScript* pSndScript = (CSoundScript*)object;

	if (pSndScript)
		delete pSndScript;
}

//-----------------------------------------------------------------------------

bool LoadSoundscript::SDK_OnLoad(char* error, size_t maxlen, bool late)
{
#ifndef SOUNDEMITTERSYSTEM_INTERFACE_VERSION_3
	sharesys->AddDependency(myself, "bintools.ext", true, true);

	IGameConfig* pGameConfig;
	char sError[256];
	if (!gameconfs->LoadGameConfigFile("loadsoundscript.games", &pGameConfig, sError, sizeof(sError)))
	{
		ke::SafeSprintf(error, maxlen, "Could not load loadsoundscript.games gamedata: %s", sError);
		return false;
	}

#ifdef PLATFORM_WINDOWS
	// Find soundemittersystem.dll module in memory
	HMODULE hSndEmitterSys = GetModuleHandleA("soundemittersystem.dll");
	if (!hSndEmitterSys)
	{
		ke::SafeStrcpy(error, maxlen, "Could not find the soundemittersystem library in memory.");
		return false;
	}
#else
	void* hSndEmitterSys = dlopen("soundemittersystem_srv.so", RTLD_LAZY);
	if (!hSndEmitterSys)
		hSndEmitterSys = dlopen("soundemittersystem.so", RTLD_LAZY);

	if (!hSndEmitterSys)
	{
		ke::SafeStrcpy(error, maxlen, "Could not find the soundemittersystem library in memory.");
		return false;
	}
#endif // PLATFORM_WINDOWS

	g_pAddSnd = GetAddressFromKeyValues(hSndEmitterSys, pGameConfig, "CSoundEmitterSystemBase::AddSoundsFromFile");
	if (!g_pAddSnd)
	{
		ke::SafeStrcpy(error, maxlen, "Could not find the CSoundEmitterSystemBase::AddSoundsFromFile signature.");
		return false;
	}

	gameconfs->CloseGameConfigFile(pGameConfig);
#endif // SOUNDEMITTERSYSTEM_INTERFACE_VERSION_3

	SH_ADD_HOOK(IServerGameDLL, LevelInit, gamedll, SH_MEMBER(this, &LoadSoundscript::LevelInit), false);

	return true;
}

void LoadSoundscript::SDK_OnAllLoaded()
{
	if (!handlesys->FindHandleType("SoundScriptType", &g_SoundScriptHandleType))
	{
#ifndef SOUNDEMITTERSYSTEM_INTERFACE_VERSION_3
		SM_GET_LATE_IFACE(BINTOOLS, bintools);

		PassInfo passinfo[] = {
			{PassType_Basic, PASSFLAG_BYVAL, sizeof(char*), NULL, 0},
			{PassType_Basic, PASSFLAG_BYVAL, sizeof(bool), NULL, 0},
			{PassType_Basic, PASSFLAG_BYVAL, sizeof(bool), NULL, 0},
#ifndef NO_REFRESH_PARAM
			{PassType_Basic, PASSFLAG_BYVAL, sizeof(bool), NULL, 0},
#endif
		};
		g_pCallAddSoundsFromFile = bintools->CreateCall(g_pAddSnd, CallConv_ThisCall, NULL, passinfo, sizeof(passinfo));
#endif

		sharesys->AddNatives(myself, LoadSoundscriptNative::g_ExtensionNatives);
		sharesys->RegisterLibrary(myself, "LoadSoundscript");
		g_SoundScriptHandleType = handlesys->CreateType("SoundScriptType", &g_SoundScriptHandler, 0, NULL, NULL, myself->GetIdentity(), NULL);
	}
}

bool LoadSoundscript::LevelInit(char const* pMapName, char const* pMapEntities, char const* pOldLevel, char const* pLandmarkName, bool loadGame, bool background)
{
	// Since all loaded sounds should be deleted by now we have to load them again!
	int iCount = CSoundScript::LoadedSoundScripts.Count();
	for (int i = 0; i < iCount; i++)
	{
		CSoundScript* script = CSoundScript::LoadedSoundScripts[i];
		AddSoundsFromFile(script->GetFilename(), script->ShouldPreload(), true, false);
	}

	RETURN_META_VALUE(MRES_IGNORED, true);
}

bool LoadSoundscript::QueryRunning(char* error, size_t maxlength)
{
#ifndef SOUNDEMITTERSYSTEM_INTERFACE_VERSION_3
	SM_CHECK_IFACE(BINTOOLS, bintools);
#endif

	return true;
}

void LoadSoundscript::SDK_OnUnload()
{
#ifndef SOUNDEMITTERSYSTEM_INTERFACE_VERSION_3
	g_pCallAddSoundsFromFile->Destroy();
#endif
	handlesys->RemoveType(g_SoundScriptHandleType, myself->GetIdentity());
	SH_REMOVE_HOOK(IServerGameDLL, LevelInit, gamedll, SH_MEMBER(this, &LoadSoundscript::LevelInit), false);
}

bool LoadSoundscript::SDK_OnMetamodLoad(ISmmAPI *ismm, char *error, size_t maxlen, bool late)
{
	GET_V_IFACE_CURRENT(GetFileSystemFactory, soundemittersystem, ISoundEmitterSystemBase, SOUNDEMITTERSYSTEM_INTERFACE_VERSION);

	return true;
}

//-----------------------------------------------------------------------------

namespace LoadSoundscriptNative
{
	enum
	{
		IntervalStart,
		IntervalRange,
	};

	static cell_t LoadSoundscript(IPluginContext* pContext, const cell_t* params)
	{
		char* sPath;
		pContext->LocalToString(params[1], &sPath);

		CSoundScript* pSoundScript = new CSoundScript(soundemittersystem, sPath);

		HandleError hndlError;
		Handle_t handle = g_pHandleSys->CreateHandle(g_SoundScriptHandleType, pSoundScript, pContext->GetIdentity(), myself->GetIdentity(), &hndlError);

		if (!handle)
		{
			delete pSoundScript;
			pContext->ReportError("Could not create handle to CSoundScript! (error %d)", hndlError);
			return 0;
		}
		else
		{
			AddSoundsFromFile(sPath, params[2], true, false);
			pSoundScript->Refresh();
			return handle;
		}
	}

	static cell_t GetSoundByName(IPluginContext* pContext, const cell_t* params)
	{
		char* sName;
		pContext->LocalToString(params[1], &sName);

		int index = soundemittersystem->GetSoundIndex(sName);
		if (!soundemittersystem->IsValidIndex(index))
			return -1;

		return index;
	}

	static cell_t Count(IPluginContext* pContext, const cell_t* params)
	{
		CSoundScript* pSoundScript = GetSoundScriptFromHandle(params[1], pContext);
		if (pSoundScript)
			return pSoundScript->Count();

		return 0;
	}

	static cell_t GetSound(IPluginContext* pContext, const cell_t* params)
	{
		CSoundScript* pSoundScript = GetSoundScriptFromHandle(params[1], pContext);
		if (pSoundScript)
			return soundemittersystem->GetSoundIndex(pSoundScript->GetSound(params[2]));

		return -1;
	}

	static cell_t GetName(IPluginContext* pContext, const cell_t* params)
	{
		if (!soundemittersystem->IsValidIndex(params[1]))
		{
			pContext->ReportError("Invalid SoundEntry index %d!", params[1]);
			return 0;
		}

		const char* pszName = soundemittersystem->GetSoundName(params[1]);
		pContext->StringToLocal(params[2], (size_t)params[3], pszName);

		return 0;
	}

	static cell_t GetChannel(IPluginContext* pContext, const cell_t* params)
	{
		if (!soundemittersystem->IsValidIndex(params[1]))
		{
			pContext->ReportError("Invalid SoundEntry index %d!", params[1]);
			return 0;
		}

		return soundemittersystem->InternalGetParametersForSound(params[1])->GetChannel();
	}
	static cell_t GetVolume(IPluginContext* pContext, const cell_t* params)
	{
		if (!soundemittersystem->IsValidIndex(params[1]))
		{
			pContext->ReportError("Invalid SoundEntry index %d!", params[1]);
			return 0;
		}

		CSoundParametersInternal* sndparams = soundemittersystem->InternalGetParametersForSound(params[1]);

		switch (params[2])
		{
		case IntervalStart:
			return sp_ftoc(sndparams->GetVolume().start);
			break;
		case IntervalRange:
			return sp_ftoc(sndparams->GetVolume().range);
			break;
		default:
			pContext->ReportError("Invalid SoundInterval %d!", params[2]);
		}

		return 0;
	}

	static cell_t GetPitch(IPluginContext* pContext, const cell_t* params)
	{
		if (!soundemittersystem->IsValidIndex(params[1]))
		{
			pContext->ReportError("Invalid SoundEntry index %d!", params[1]);
			return 0;
		}

		CSoundParametersInternal* sndparams = soundemittersystem->InternalGetParametersForSound(params[1]);

		switch (params[2])
		{
		case IntervalStart:
			return sp_ftoc(sndparams->GetPitch().start);
			break;
		case IntervalRange:
			return sp_ftoc(sndparams->GetPitch().range);
			break;
		default:
			pContext->ReportError("Invalid SoundInterval %d!", params[2]);
		}

		return 0;
	}

	static cell_t GetSoundLevel(IPluginContext* pContext, const cell_t* params)
	{
		if (!soundemittersystem->IsValidIndex(params[1]))
		{
			pContext->ReportError("Invalid SoundEntry index %d!", params[1]);
			return 0;
		}

		CSoundParametersInternal* sndparams = soundemittersystem->InternalGetParametersForSound(params[1]);

		switch (params[2])
		{
		case IntervalStart:
			return sp_ftoc(sndparams->GetSoundLevel().start);
			break;
		case IntervalRange:
			return sp_ftoc(sndparams->GetSoundLevel().range);
			break;
		default:
			pContext->ReportError("Invalid SoundInterval %d!", params[2]);
		}

		return 0;
	}

	static cell_t GetDelayMsec(IPluginContext* pContext, const cell_t* params)
	{
		if (!soundemittersystem->IsValidIndex(params[1]))
		{
			pContext->ReportError("Invalid SoundEntry index %d!", params[1]);
			return 0;
		}

		return soundemittersystem->InternalGetParametersForSound(params[1])->GetDelayMsec();
	}

	static cell_t GetWaveCount(IPluginContext* pContext, const cell_t* params)
	{
		if (!soundemittersystem->IsValidIndex(params[1]))
		{
			pContext->ReportError("Invalid SoundEntry index %d!", params[1]);
			return 0;
		}

		return soundemittersystem->InternalGetParametersForSound(params[1])->NumSoundNames();
	}

	static cell_t GetWavePath(IPluginContext* pContext, const cell_t* params)
	{
		if (!soundemittersystem->IsValidIndex(params[1]))
		{
			pContext->ReportError("Invalid SoundEntry index %d!", params[1]);
			return 0;
		}

		CSoundParametersInternal* sndparams = soundemittersystem->InternalGetParametersForSound(params[1]);
		int iMaxCount = sndparams->NumSoundNames();
		const SoundFile* pszWaves = sndparams->GetSoundNames();

		if (params[2] < 0 || params[2] >= iMaxCount)
		{
			pContext->ReportError("Invalid SoundFile index %d! (NumSoundNames is %d)", params[2], iMaxCount);
			return 0;
		}

		CUtlSymbol sym = sndparams->GetSoundNames()[params[2]].symbol;
		const char* name = soundemittersystem->GetWaveName(sym);
		pContext->StringToLocal(params[3], (size_t)params[4], params[5] ? PSkipSoundChars(name) : name);

		return 0;
	}

	static cell_t OnlyPlayToOwner(IPluginContext* pContext, const cell_t* params)
	{
		if (!soundemittersystem->IsValidIndex(params[1]))
		{
			pContext->ReportError("Invalid SoundEntry index %d!", params[1]);
			return 0;
		}

		return soundemittersystem->InternalGetParametersForSound(params[1])->OnlyPlayToOwner();
	}
}