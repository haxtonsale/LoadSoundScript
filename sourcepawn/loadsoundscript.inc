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
	// Retrieves the amount of loaded sound entries.
	property int Count
	{
		public native get();
	}
	
	public native SoundEntry GetSound(int iIndex);
}

/*
 * Loads a custom soundscript and creates a handle to it.
 *
 * @param sPath          Path to the soundscript text file.
 * @param bPreload       Whether sounds should be preloaded.
 */
native SoundScript LoadSoundScript(const char[] sPath, bool bPreload = false);

/*
 * Retrieves sound entries index from its name.
 * Sounds indexes can change on level shutdown, so be careful when caching them.
 *
 * @param sName          Sound name.
 */
native SoundEntry GetSoundByName(const char[] sName);

#if defined _sdktools_stringtables_included
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
	}
}
#endif

#if defined _sdktools_sound_included
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