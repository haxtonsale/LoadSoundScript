# LoadSoundScript

A SourceMod extension that provides a function that allows plugins to load soundscripts.

## Usage
Loading a soundscript and playing one of the loaded sounds to a client:
```cpp
#include <sourcemod>
#include <loadsoundscript>
#include <sdktools_sound>

public void OnPluginStart()
{
	// Path must be relative to the game folder
	LoadSoundScript("somesoundscript.txt", true);
	RegConsoleCmd("soundscript_test", Cmd_SoundScriptTest);
}

void PlaySound(int iClient)
{
	// Whichever sounds were in the file can now be emitted via EmiteGameSound
	EmitGameSoundToClient(iClient, "LoadSoundScript.NewSound");
}

public Action Cmd_SoundScriptTest(int iClient, int iArgs)
{
	PlaySound(iClient);
}
```

