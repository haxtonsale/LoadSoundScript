#if defined _loadsoundscript_included
#endinput
#endif
#define _loadsoundscript_included

#define INVALID_SOUND_ENTRY view_as<SoundEntry>(-1)

enum SoundInterval
{
	IntervalStart,
	IntervalRange,
}

methodmap SoundEntry
{
	public native void GetName(char[] sBuffer, int iMaxLength);
	
	public native int GetChannel();
	public native float GetVolume(SoundInterval iInterval = IntervalStart);
	public native float GetPitch(SoundInterval iInterval = IntervalStart);
	public native float GetSoundLevel(SoundInterval iInterval = IntervalStart);
	public native int GetDelayMsec();
	
	public native int GetWaveCount();
	public native void GetWavePath(int iIndex, char[] sBuffer, int iMaxLength, bool bSkipSoundChars = true);
	
	public native bool OnlyPlayToOwner();
}

methodmap SoundScript < Handle
{
	/*
	 * Retrieves the amount of loaded sound entries.
	 */
	property int Count
	{
		public native get();
	}
	
	/*
	 * Retrieves the sound from the given index.
	 *
	 * @param iIndex     Sound's index.
	 * @return           Sound's global index on success, INVALID_SOUND_ENTRY otherwise.
	 */
	public native SoundEntry GetSound(int iIndex);
}

/*
 * Loads a custom soundscript and creates a handle to it.
 *
 * @param sPath          Path to the soundscript file.
 * @return               Handle to the soundscript object.
 */
native SoundScript LoadSoundScript(const char[] sPath);

/*
 * Retrieves sound entries index from its name.
 * Sound's index can change on level shutdown, so be careful when caching it.
 *
 * @param sName          Sound name.
 * @return               Sound's global index on success, INVALID_SOUND_ENTRY otherwise.
 */
native SoundEntry GetSoundByName(const char[] sName);

#if defined _sdktools_stringtables_included
/*
 * Adds a sound entry to the downloadables network string table.
 * This forces a client to download the file(s) if they do not already have it.
 *
 * @param sName          Sound entry that will be added to downloadables table.
 * @return               True on success, false otherwise.
 */
stock bool AddScriptSoundToDownloadsTable(const char[] sName)
{
	SoundEntry sound = GetSoundByName(sName);
	if (sound != INVALID_SOUND_ENTRY)
	{
		int iNumSounds = sound.GetWaveCount();
		
		for (int i = 0; i < iNumSounds; i++)
		{
			char sPath[PLATFORM_MAX_PATH];
			sound.GetWavePath(i, sPath, sizeof(sPath));
			
			char sCopy[PLATFORM_MAX_PATH];
			FormatEx(sCopy, sizeof(sCopy), "sound/%s", sPath);
			
			AddFileToDownloadsTable(sCopy);
		}
		
		return true;
	}
	
	return false;
}
#endif

#if defined _sdktools_sound_included
/*
 * Stops a game sound.
 *
 * @param iEntity        Entity index.
 * @param sName          Name of the sound entry that should be stopped.
 */
stock void StopGameSound(int iEntity, const char[] sName)
{
	SoundEntry sound = GetSoundByName(sName);
	if (sound != INVALID_SOUND_ENTRY)
	{
		int iNumSounds = sound.GetWaveCount();
		
		for (int i = 0; i < iNumSounds; i++)
		{
			char sPath[PLATFORM_MAX_PATH];
			sound.GetWavePath(i, sPath, sizeof(sPath), false);
			
			StopSound(iEntity, sound.GetChannel(), sPath);
		}
	}
}
#endif

public Extension __ext_loadsoundscript = 
{
	name = "LoadSoundscript",
	file = "loadsoundscript.ext",
#if defined AUTOLOAD_EXTENSIONS
	autoload = 1,
#else
	autoload = 0,
#endif
#if defined REQUIRE_EXTENSIONS
	required = 1,
#else
	required = 0,
#endif
}