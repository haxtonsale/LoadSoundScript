#include "soundscript.h"

CSoundScript::CSoundScript(ISoundEmitterSystemBase* soundemittersystem, const char* filename) :
	soundemittersystem(soundemittersystem),
	m_szFilename(filename)
{
#if defined _WIN32
	// haha
	int size = strnlen(filename, PLATFORM_MAX_PATH)+1;
	char* filenamecpy = new char[size];

	ke::SafeStrcpy(filenamecpy, size, filename);

	for (int i = 0; i < size-1; i++)
	{
		if (filenamecpy[i] == '/')
			filenamecpy[i] = '\\';
	}

	m_szFilename = filenamecpy;
#endif
}

CSoundScript::~CSoundScript()
{
#if defined _WIN32
	delete[] m_szFilename;
#endif
}

void CSoundScript::Refresh()
{
	m_SoundEntries.RemoveAll();

	for (int i = soundemittersystem->First(); i != soundemittersystem->InvalidIndex(); i = soundemittersystem->Next(i))
	{
		// Insert the sound entry in m_SoundEntries if its sound script index is equal to m_iScriptFileIndex.
		if (strncmp(soundemittersystem->GetSourceFileForSound(i), m_szFilename, PLATFORM_MAX_PATH) == 0)
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

	return nullptr;
}