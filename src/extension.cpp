#include "extension.h"

/**
 * @file extension.cpp
 * @brief Implement extension code here.
 */

LoadSoundscript g_LoadSoundscript;		/**< Global singleton for extension's main interface */
SMEXT_LINK(&g_LoadSoundscript);

ISoundEmitterSystemBase *soundemittersystem;
IBinTools *bintools;

void *g_pAddSnd;
ICallWrapper *g_pCallAddSoundsFromFile;

sp_nativeinfo_t g_ExtensionNatives[] =
{
	{ "LoadSoundScript",			Native_LoadSoundscript },
	{ NULL,							NULL }
};

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
	unsigned char signature[200];
	size_t real_bytes = UTIL_DecodeHexString(signature, sizeof(signature), value);
	if (real_bytes < 1)
		return nullptr;

#ifdef _LINUX
	// The pointer returned by dlopen is not inside the loaded library memory region.
	struct link_map* dlmap;
	dlinfo(pBaseAddr, RTLD_DI_LINKMAP, &dlmap);
	pBaseAddr = (void*)dlmap->l_addr;
#endif

	// Find that pattern in the pointed module.
	return memutils->FindPattern(pBaseAddr, (char*)signature, real_bytes);
}

void AddSoundsFromFile(const char* filename, bool bPreload, bool bIsOverride, bool bRefresh)
{
	ArgBuffer<ISoundEmitterSystemBase*, const char*, bool, bool, bool> vstk(soundemittersystem, filename, bPreload, bIsOverride, bRefresh);
	g_pCallAddSoundsFromFile->Execute(vstk, nullptr);
}

bool LoadSoundscript::SDK_OnLoad(char *error, size_t maxlen, bool late)
{
	sharesys->AddDependency(myself, "bintools.ext", true, true);

	IGameConfig* pGameConfig;
	char sError[255];
	if (!gameconfs->LoadGameConfigFile("loadsoundscript.games", &pGameConfig, sError, sizeof(sError)))
	{
		ke::SafeSprintf(error, maxlen, "Could not load loadsoundscript.games gamedata: %s", sError);
		return false;
	}

#ifdef WIN32
	// Find soundemittersystem.dll module in memory
	HMODULE hSndEmitterSys = GetModuleHandleA("soundemittersystem.dll");
	if (!hSndEmitterSys)
	{
		ke::SafeStrcpy(error, maxlen, "Could not find the soundemittersystem library in memory.");
		return false;
	}
#else // WIN32
	void* hSndEmitterSys = dlopen("soundemittersystem_srv.so", RTLD_LAZY);
	if (!hSndEmitterSys)
		hSndEmitterSys = dlopen("soundemittersystem.so", RTLD_LAZY);

	if (!hSndEmitterSys)
	{
		ke::SafeStrcpy(error, maxlen, "Could not find the soundemittersystem library in memory.");
		return false;
	}
#endif

	g_pAddSnd = GetAddressFromKeyValues(hSndEmitterSys, pGameConfig, "CSoundEmitterSystemBase::AddSoundsFromFile");
	if (!g_pAddSnd)
	{
		ke::SafeStrcpy(error, maxlen, "Could not find the CSoundEmitterSystemBase::AddSoundsFromFile signature");
		return false;
	}

	gameconfs->CloseGameConfigFile(pGameConfig);

	return true;
}

void LoadSoundscript::SDK_OnAllLoaded()
{
	SM_GET_LATE_IFACE(BINTOOLS, bintools);

	PassInfo passinfo[] = {
		{PassType_Basic, PASSFLAG_BYVAL, sizeof(char*), NULL, 0},
		{PassType_Basic, PASSFLAG_BYVAL, sizeof(bool), NULL, 0},
		{PassType_Basic, PASSFLAG_BYVAL, sizeof(bool), NULL, 0},
		{PassType_Basic, PASSFLAG_BYVAL, sizeof(bool), NULL, 0}
	};

	g_pCallAddSoundsFromFile = bintools->CreateCall(g_pAddSnd, CallConv_ThisCall, NULL, passinfo, 4);

	sharesys->AddNatives(myself, g_ExtensionNatives);
	sharesys->RegisterLibrary(myself, "LoadSoundscript");
}

bool LoadSoundscript::QueryRunning(char* error, size_t maxlength)
{
	SM_CHECK_IFACE(BINTOOLS, bintools);

	return true;
}

void LoadSoundscript::SDK_OnUnload()
{
	g_pCallAddSoundsFromFile->Destroy();
}

bool LoadSoundscript::SDK_OnMetamodLoad(ISmmAPI* ismm, char* error, size_t maxlen, bool late)
{
	GET_V_IFACE_CURRENT(GetFileSystemFactory, soundemittersystem, ISoundEmitterSystemBase, SOUNDEMITTERSYSTEM_INTERFACE_VERSION);

	return true;
}

static cell_t Native_LoadSoundscript(IPluginContext *pContext, const cell_t *params)
{
	char *sPath;
	pContext->LocalToString(params[1], &sPath);

	AddSoundsFromFile(sPath, params[2], params[3], params[4]);

	return 0;
}