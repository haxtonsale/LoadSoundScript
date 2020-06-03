#include "soundscript.h"

CSoundScript::CSoundScript(ISoundEmitterSystemBase* soundemittersystem, const char* filename) :
	soundemittersystem(soundemittersystem),
	m_szFilename(filename)
{
#ifdef PLATFORM_WINDOWS
	// ISoundEmitterSystemBase::GetSourceFileForSound returns the path with Windows separators
	// so we gotta make sure we use those m_szFilename as well
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

	LoadedSoundScripts.AddToTail(this);
}

CSoundScript::~CSoundScript()
{
#ifdef PLATFORM_WINDOWS
	delete[] m_szFilename;
#endif

	LoadedSoundScripts.FindAndRemove(this);
}

void CSoundScript::Refresh()
{
	m_SoundEntries.RemoveAll();
	m_iCount = 0;

	for (int i = soundemittersystem->First(); i != soundemittersystem->InvalidIndex(); i = soundemittersystem->Next(i))
	{
		// Insert the sound entry in m_SoundEntries if its sound script index is equal to m_iScriptFileIndex.
		if (strncmp(soundemittersystem->GetSourceFileForSound(i), m_szFilename, PLATFORM_MAX_PATH) == 0)
		{
			m_SoundEntries.AddToTail(soundemittersystem->GetSoundName(i));
			++m_iCount;
		}
	}
}

const char* CSoundScript::GetSound(int index)
{
	if (m_SoundEntries.IsValidIndex(index))
		return m_SoundEntries[index];

	return nullptr;
}