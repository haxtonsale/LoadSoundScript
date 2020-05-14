#if defined _loadsoundscript_included
#endinput
#endif
#define _loadsoundscript_included

/*
 * Loads a custom soundscript.
 *
 * @param sPath          Path to the soundscript text file.
 * @param bPreload       Whether sounds should be preloaded.
 * @param bIsOverride    Whether we should override sounds that already exist. Note that in LevelShutdown all overrides are cleared.
 * @param bRefresh       Whether we should update sound parameters of sounds that already exist.
 */
native void LoadSoundScript(const char[] sPath, bool bPreload, bool bIsOverride = false, bool bRefresh = false);

public Extension __ext_loadsoundscript = 
{
	name = "LoadSoundscript",
	file = "loadsoundscript.ext.2.ep2v",
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