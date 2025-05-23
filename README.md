# LoadSoundScript [![CI](https://github.com/haxtonsale/LoadSoundScript/actions/workflows/main.yml/badge.svg)](https://github.com/haxtonsale/LoadSoundScript/actions/workflows/main.yml)

A SourceMod extension that provides more soundscript functionality.

## Usage
Loading a soundscript and playing one of the loaded sounds to a client:
```cpp
#include <sourcemod>
#include <loadsoundscript>
#include <sdktools_sound>

public void OnPluginStart()
{
	// Path must be relative to the game folder
	LoadSoundScript("somesoundscript.txt");
	RegConsoleCmd("soundscript_test", Cmd_SoundScriptTest);
}

public Action Cmd_SoundScriptTest(int iClient, int iArgs)
{
	// Whichever sounds were in the file can now be emitted via EmitGameSound
	EmitGameSoundToClient(iClient, "LoadSoundScript.NewSound");
}
```

Getting sound entry volume for every sound of a loaded soundscript:
```cpp
#include <sourcemod>
#include <loadsoundscript>

public void OnPluginStart()
{
	SoundScript sndscript = LoadSoundScript("somesoundscript.txt");

	int iCount = sndscript.Count;
	for (int i = 0; i < iCount; i++)
	{
		SoundEntry entry = sndscript.GetSound(i);

		char sName[128];
		entry.GetName(sName, sizeof(sName));
		
		PrintToServer("The volume of \"%s\" is %.2f", sName, entry.GetVolume());
	}
}
```

Getting sound paths of any sound entry:
```cpp
#include <sourcemod>
#include <loadsoundscript>

public void OnPluginStart()
{
	SoundEntry entry = GetSoundByName("MVM.BotStep");

	if (entry != INVALID_SOUND_ENTRY)
	{
		int iNumSounds = entry.GetWaveCount();
		for (int i = 0; i < iNumSounds; i++)
		{
			char sPath[PLATFORM_MAX_PATH];
			entry.GetWavePath(i, sPath, sizeof(sPath));

			PrintToServer(sPath);
		}
	}
}
```

