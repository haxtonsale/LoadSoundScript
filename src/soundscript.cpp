#include "soundscript.h"

CSoundScript::CSoundScript(ISoundEmitterSystemBase* soundemittersystem, const char* filename) :
	soundemittersystem(soundemittersystem),
	m_szFilename(filename)
{
}

CSoundScript::~CSoundScript()
{
}

void CSoundScript::Refresh()
{
	m_SoundEntries.RemoveAll();

	int iScriptFileIndex = soundemittersystem->FindSoundScript(m_szFilename);
	for (int i = soundemittersystem->First(); i != soundemittersystem->InvalidIndex(); i = soundemittersystem->Next(i))
	{
		// Insert the sound entry in m_SoundEntries if its sound script index is equal to m_iScriptFileIndex.
		if (strcmp(soundemittersystem->GetSourceFileForSound(i), m_szFilename) == 0)
			m_SoundEntries.AddToTail(soundemittersystem->GetSoundName(i));
	}
}

int CSoundScript::Count()
{
	return m_SoundEntries.Count();
}

const char* CSoundScript::GetSound(int index)
{
	if (m_SoundEntries.IsValidIndex(index))
		return m_SoundEntries[index];

	return NULL;
}