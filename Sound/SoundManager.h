#pragma once


/**
* @class	CSoundManager
* @brief	�T�E���h�Ǘ��N���X
*/
class CSoundManager
{
public:
	static	void	Initialize(HWND hWnd);
	static	void	Finalize();

	static	void	LoadStreamingSound(const char* pFilename, bool bLoopFlag);
	static	void	UnloadStreamingSound(unsigned int numSound);
	static	void	Play(unsigned int numSound, DWORD dwPriority, DWORD dwFlag);
	static	void	Stop(unsigned int numSound);
	static	void	Done(unsigned int numSound);

};

