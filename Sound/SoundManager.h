#pragma once


/**
* @class	CSoundManager
* @brief	サウンド管理クラス
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

