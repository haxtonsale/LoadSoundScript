#ifndef SOUNDSCRIPT_H
#define SOUNDSCRIPT_H

#include "sdk/smsdk_ext.h"
#include <SoundEmitterSystem/isoundemittersystembase.h>

class CSoundScript
{
public:
	CSoundScript(ISoundEmitterSystemBase* soundemittersystem, const char* filename);
	~CSoundScript();

	// Iterates through all sound entries and gets all sounds that belong to our script file.
	void Refresh();
	const char* GetFilename() { return m_szFilename; };
	int Count() { return m_iCount; };
	const char* GetSound(int index);

	static CUtlVector<CSoundScript*> LoadedSoundScripts;

private:
	ISoundEmitterSystemBase* soundemittersystem;
	CUtlVector<const char*> m_SoundEntries;
	int m_iCount; // The number of loaded sounds doesn't change, so we can keep it here.
	const char* m_szFilename;
};

#endif // SOUNDSCRIPT_H