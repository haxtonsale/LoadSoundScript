#ifndef _INCLUDE_SOURCEMOD_EXTENSION_PROPER_H_
#define _INCLUDE_SOURCEMOD_EXTENSION_PROPER_H_

/**
 * @file extension.h
 * @brief Sample extension code header.
 */

#include "sdk/smsdk_ext.h"
#include "extensions/iBinTools.h"
#include <sm_argbuffer.h>
#include "soundscript.h"

#ifdef _LINUX
#include <link.h>
#endif

class SoundScriptTypeHandler : public IHandleTypeDispatch
{
public:
	void OnHandleDestroy(HandleType_t type, void* object);
};


/**
 * @brief Sample implementation of the SDK Extension.
 * Note: Uncomment one of the pre-defined virtual functions in order to use it.
 */
class LoadSoundscript : public SDKExtension
{
public:
	/**
	 * @brief This is called after the initial loading sequence has been processed.
	 *
	 * @param error		Error message buffer.
	 * @param maxlen	Size of error message buffer.
	 * @param late		Whether or not the module was loaded after map load.
	 * @return			True to succeed loading, false to fail.
	 */
	virtual bool SDK_OnLoad(char* error, size_t maxlen, bool late);
	
	/**
	 * @brief This is called right before the extension is unloaded.
	 */
	virtual void SDK_OnUnload();

	/**
	 * @brief This is called once all known extensions have been loaded.
	 * Note: It is is a good idea to add natives here, if any are provided.
	 */
	virtual void SDK_OnAllLoaded();

	/**
	 * @brief Called when the pause state is changed.
	 */
	//virtual void SDK_OnPauseChange(bool paused);

	/**
	 * @brief this is called when Core wants to know if your extension is working.
	 *
	 * @param error		Error message buffer.
	 * @param maxlen	Size of error message buffer.
	 * @return			True if working, false otherwise.
	 */
	virtual bool QueryRunning(char* error, size_t maxlength);
public:
#if defined SMEXT_CONF_METAMOD
	/**
	 * @brief Called when Metamod is attached, before the extension version is called.
	 *
	 * @param error			Error buffer.
	 * @param maxlen		Maximum size of error buffer.
	 * @param late			Whether or not Metamod considers this a late load.
	 * @return				True to succeed, false to fail.
	 */
	virtual bool SDK_OnMetamodLoad(ISmmAPI* ismm, char* error, size_t maxlen, bool late);

	/**
	 * @brief Called when Metamod is detaching, after the extension version is called.
	 * NOTE: By default this is blocked unless sent from SourceMod.
	 *
	 * @param error			Error buffer.
	 * @param maxlen		Maximum size of error buffer.
	 * @return				True to succeed, false to fail.
	 */
	//virtual bool SDK_OnMetamodUnload(char *error, size_t maxlen);

	/**
	 * @brief Called when Metamod's pause state is changing.
	 * NOTE: By default this is blocked unless sent from SourceMod.
	 *
	 * @param paused		Pause state being set.
	 * @param error			Error buffer.
	 * @param maxlen		Maximum size of error buffer.
	 * @return				True to succeed, false to fail.
	 */
	//virtual bool SDK_OnMetamodPauseChange(bool paused, char *error, size_t maxlen);
#endif
};

size_t UTIL_DecodeHexString(unsigned char* buffer, size_t maxlength, const char* hexstr);
void* GetAddressFromKeyValues(void* pBaseAddr, IGameConfig* pGameConfig, const char* sKey);
void AddSoundsFromFile(const char* filename, bool bPreload, bool bIsOverride, bool bRefresh);

namespace LoadSoundscriptNative
{
	static cell_t LoadSoundscript(IPluginContext* pContext, const cell_t* params);
	static cell_t GetSoundByName(IPluginContext* pContext, const cell_t* params);

	static cell_t Count(IPluginContext* pContext, const cell_t* params);
	static cell_t GetSound(IPluginContext* pContext, const cell_t* params);

	static cell_t GetName(IPluginContext* pContext, const cell_t* params);
	static cell_t GetChannel(IPluginContext* pContext, const cell_t* params);
	static cell_t GetVolume(IPluginContext* pContext, const cell_t* params);
	static cell_t GetPitch(IPluginContext* pContext, const cell_t* params);
	static cell_t GetSoundLevel(IPluginContext* pContext, const cell_t* params);
	static cell_t GetDelayMsec(IPluginContext* pContext, const cell_t* params);
	static cell_t GetWaveCount(IPluginContext* pContext, const cell_t* params);
	static cell_t GetWavePath(IPluginContext* pContext, const cell_t* params);
	static cell_t OnlyPlayToOwner(IPluginContext* pContext, const cell_t* params);

	sp_nativeinfo_t g_ExtensionNatives[] =
	{
		{ "LoadSoundScript",            LoadSoundscript },
		{ "GetSoundByName",             GetSoundByName },

		{ "SoundScript.Count.get",      Count },
		{ "SoundScript.GetSound",       GetSound },

		{ "SoundEntry.GetName",         GetName },
		{ "SoundEntry.GetChannel",      GetChannel },
		{ "SoundEntry.GetVolume",       GetVolume },
		{ "SoundEntry.GetPitch",        GetPitch },
		{ "SoundEntry.GetSoundLevel",   GetSoundLevel },
		{ "SoundEntry.GetDelayMsec",    GetDelayMsec },
		{ "SoundEntry.GetWaveCount",    GetWaveCount },
		{ "SoundEntry.GetWavePath",     GetWavePath },
		{ "SoundEntry.OnlyPlayToOwner", OnlyPlayToOwner },
		{ NULL,                         NULL }
	};
}

#endif // _INCLUDE_SOURCEMOD_EXTENSION_PROPER_H_
