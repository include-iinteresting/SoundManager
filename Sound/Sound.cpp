// Sound.cpp : アプリケーションのエントリ ポイントを定義します。
//

#include "stdafx.h"

#include "SoundManager.h"
#include <dsound.h>

int main()
{
	wchar_t cBuff[256];
	GetConsoleTitle(cBuff, 256);
	HWND hWnd = FindWindow(NULL, cBuff);

	CSoundManager::Initialize(hWnd);

	CSoundManager::LoadStreamingSound("res/test.mp3", true);

	CSoundManager::StreamPlay(0, 0, DSBPLAY_LOOPING);

	while (true) {

		
		if (GetAsyncKeyState(VK_ESCAPE))
			break;
	}

	CSoundManager::Finalize();

	return 0;
}

