#include "extension.h"
#include "soundscript.h"
#include <soundchars.h>

SH_DECL_HOOK6(IServerGameDLL, LevelInit, SH_NOATTRIB, false, bool, const char*, const char*, const char*, const char*, bool, bool);

LoadSoundscript g_LoadSoundscript; // Global singleton for extension's main interface.
SMEXT_LINK(&g_LoadSoundscript);

ISoundEmitterSystemBase* soundemittersystem = 0;

HandleType_t g_SoundScriptHandleType;
SoundScriptTypeHandler g_SoundScriptHandler;
CUtlVector<CSoundScript*> CSoundScript::LoadedSoundScripts;

//-----------------------------------------------------------------------------

void AddSoundOverrides(const char* filename, bool bPreload)
{
#ifndef SOUNDEMITTERSYSTEM_INTERFACE_VERSION_3
#ifndef NO_REFRESH_PARAM
	soundemittersystem->AddSoundOverrides(filename, bPreload);
#else
	soundemittersystem->AddSoundOverrides(filename);
#endif // NO_REFRESH_PARAM
#else
	soundemittersystem->AddSoundsFromFile(filename, bPreload, false, true);
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

bool OnLevelInit(char const* pMapName, char const* pMapEntities, char const* pOldLevel, char const* pLandmarkName, bool loadGame, bool background)
{
	// Since all loaded sounds should be deleted by now we have to load them again!
	int iCount = CSoundScript::LoadedSoundScripts.Count();
	for (int i = 0; i < iCount; i++)
	{
		CSoundScript* script = CSoundScript::LoadedSoundScripts[i];
		AddSoundOverrides(script->GetFilename(), script->ShouldPreload());
	}

	RETURN_META_VALUE(MRES_IGNORED, true);
}

//-----------------------------------------------------------------------------

bool LoadSoundscript::SDK_OnLoad(char* error, size_t maxlen, bool late)
{
	SH_ADD_HOOK(IServerGameDLL, LevelInit, gamedll, &OnLevelInit, false);

	return true;
}

void LoadSoundscript::SDK_OnAllLoaded()
{
	if (!handlesys->FindHandleType("SoundScriptType", &g_SoundScriptHandleType))
	{
		sharesys->AddNatives(myself, LoadSoundscriptNative::g_ExtensionNatives);
		sharesys->RegisterLibrary(myself, "LoadSoundscript");
		g_SoundScriptHandleType = handlesys->CreateType("SoundScriptType", &g_SoundScriptHandler, 0, NULL, NULL, myself->GetIdentity(), NULL);
	}
}

void LoadSoundscript::SDK_OnUnload()
{
	handlesys->RemoveType(g_SoundScriptHandleType, myself->GetIdentity());
	SH_REMOVE_HOOK(IServerGameDLL, LevelInit, gamedll, &OnLevelInit, false);
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
			AddSoundOverrides(sPath, params[2]);
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