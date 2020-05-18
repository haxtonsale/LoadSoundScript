#ifndef SOUNDSCRIPT_H
#define SOUNDSCRIPT_H
#ifdef _WIN32
#pragma once
#endif

#include "sdk/smsdk_ext.h"
#include "SoundEmitterSystem/isoundemittersystembase.h"

class CSoundScript
{
public:
	CSoundScript(ISoundEmitterSystemBase* soundemittersystem, const char* filename);
	~CSoundScript();

	// Iterates through all sound entries and gets all sounds that belong to our script file.
	void Refresh();
	int Count();
	const char* GetSound(int index);

private:
	ISoundEmitterSystemBase* soundemittersystem;
	CUtlVector<const char*> m_SoundEntries;
	const char* m_szFilename;
};

#endif // SOUNDSCRIPT_H