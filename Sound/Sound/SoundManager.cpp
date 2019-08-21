#include "stdafx.h"
#include "SoundManager.h"
#include "DirectSound.h"
#include "OggSound.h"
#include <list>
#include <process.h>

//	macro
#undef SAFE_RELEASE
#undef SAFE_DELETE
#undef SAFE_DELETE_ARRAY
#define SAFE_RELEASE(o)	if(o) { (o)->Release(); o = NULL; };
#define SAFE_DELETE(o) if(o) { delete(o); o = NULL; };
#define SAFE_DELETE_ARRAY(o) if(o) { delete[](o); o = NULL; };

//	ThreadPhase
enum ThreadPhase : short {
	THREAD_INIT = 0x00,
	THREAD_RUN = 0x01,
	THREAD_WAIT = 0x02,
	THREAD_DONE = 0x03
};


//	SoundCommand
enum SoundCommand
{
	NO_COMMAND = 0x00,
	SOUND_PLAY = 0x01,
	SOUND_STOP = 0x02,
	SOUND_DONE = 0x03
};

#define QUEUE_SIZE 100
#define QUEUE_NEXT(o) (((o) + 1) % QUEUE_SIZE)
//	RingBuffer
typedef struct {
	SoundCommand Command[QUEUE_SIZE];
	unsigned int numSound[QUEUE_SIZE];
	DWORD dwPriority[QUEUE_SIZE];
	DWORD dwFlag[QUEUE_SIZE];
	short head;
	short tail;
}CommandQueue;


/**
* @class	SoundManagerImpl
* @brief	サウンド管理クラスの中身
*/
class SoundManagerImpl
{
	friend CSoundManager;
private:
	SoundManagerImpl();
	~SoundManagerImpl();

	static	void	Initialize();
	static	void	Finalize();
	static	SoundManagerImpl	*GetInstance();

	void	LoadStreamingSound(const char* pFilename, bool bLoopFlag);
	void	UnloadStreamingSound(unsigned int numSound);
	void	Play(unsigned int numSound, DWORD dwPriority, DWORD dwFlag);
	void	Stop(unsigned int numSound);
	void	Done(unsigned int numSound);
	
	void	Command();
	void	PlayCommand(unsigned int numSound, DWORD dwPriority, DWORD dwFlag);
	void	StopCommand(unsigned int numSound);
	void	DoneCommand(unsigned int numSound);

	void	Lock();
	void	Unlock();

	static	void	ThreadProcLauncher(void* arg);
	void			ThreadProc();
	
	void			EnQueue(unsigned int numSound, SoundCommand Command, DWORD dwPriority, DWORD dwFlag);
	void			DeQueue(unsigned int *pNumSound, SoundCommand *pCommand, DWORD *pPriority, DWORD *pFlag);
private:
	static	SoundManagerImpl	*m_pInstance;
	std::list<IStreamingSound*>	m_pSounds;

	CRITICAL_SECTION	m_CriticalSection;

	volatile	CommandQueue	m_Queue;			//!	CommandQueueRingBuffer

	volatile	ThreadPhase		m_ePhase;
	volatile struct
	{
		volatile	BOOL m_bThreadActive;
		volatile	BOOL m_bThreadDone;
		volatile	BOOL m_bStopCommand;
	}ThreadFlags;
};

SoundManagerImpl	*SoundManagerImpl::m_pInstance = NULL;

/**
* @brief	コンストラクタ
*/
SoundManagerImpl::SoundManagerImpl()
{
	InitializeCriticalSection(&m_CriticalSection);
	
	m_ePhase = ThreadPhase::THREAD_INIT;
	ThreadFlags.m_bThreadActive = true;
	ThreadFlags.m_bStopCommand = false;
	ThreadFlags.m_bThreadDone = false;

	_beginthread(ThreadProcLauncher, 0, this);
}


/**
* @brief	デストラクタ
*/
SoundManagerImpl::~SoundManagerImpl()
{
	while (!ThreadFlags.m_bThreadDone) {
		if (!ThreadFlags.m_bStopCommand) {
			Lock();
			ThreadFlags.m_bStopCommand = true;
			Unlock();
		}
	}
	DeleteCriticalSection(&m_CriticalSection);
}


/**
* @brief	初期化
*/
void SoundManagerImpl::Initialize()
{
	if (m_pInstance)
		return;

	m_pInstance = new SoundManagerImpl();
}


/**
* @brief	終了処理
*/
void SoundManagerImpl::Finalize()
{
	SAFE_DELETE(m_pInstance);
}


/**
* @brief	インスタンス取得
*/
SoundManagerImpl * SoundManagerImpl::GetInstance()
{
	if (!m_pInstance)
		SoundManagerImpl::Initialize();

	return m_pInstance;
}


/**
* @brief	ストリーミング再生する音声ファイルを読み込む
* @param	[in]	pFilename	ファイル名
* @param	[in]	bLoopFlag	ループフラグ
*/
void SoundManagerImpl::LoadStreamingSound(const char * pFilename, bool bLoopFlag)
{
	Lock();
	m_pSounds.push_back(new COggSound(pFilename, bLoopFlag));
	Unlock();
}


/**
* @brief	ストリーミング再生する音声ファイルの読み込みを解除する
* @param	[in]	numSound	サウンドオブジェクトの番号
* @details	サウンドオブジェクトの番号が詰められることに注意
*/
void SoundManagerImpl::UnloadStreamingSound(unsigned int numSound)
{
	Lock();
	std::list<IStreamingSound*>::iterator it = m_pSounds.begin();
	for (unsigned int i = 0; i < numSound; ++i) {
		++it;
	}
	(*it)->Stop();
	SAFE_DELETE(*it);
	it = m_pSounds.erase(it);
	Unlock();
}


/**
* @brief	再生
* @param	[in]	numSound	サウンドオブジェクトの番号
* @param	[in]	dwPriority	優先度
* @param	[in]	dwFlag		再生フラグ
*/
void SoundManagerImpl::Play(unsigned int numSound, DWORD dwPriority, DWORD dwFlag)
{
	Lock();
	std::list<IStreamingSound*>::iterator it = m_pSounds.begin();
	for (unsigned int i = 0; i < numSound; ++i) {
		++it;
	}
	(*it)->Play(dwPriority, dwFlag);
	Unlock();
}


/**
* @brief	停止
* @param	[in]	numSound	サウンドオブジェクトの番号
*/
void SoundManagerImpl::Stop(unsigned int numSound)
{
	Lock();
	std::list<IStreamingSound*>::iterator it = m_pSounds.begin();
	for (unsigned int i = 0; i < numSound; ++i) {
		++it;
	}
	(*it)->Stop();
	Unlock();
}


/**
* @brief	終了
* @param	[in]	numSound	サウンドオブジェクトの番号
*/
void SoundManagerImpl::Done(unsigned int numSound)
{
	Lock();
	std::list<IStreamingSound*>::iterator it = m_pSounds.begin();
	for (unsigned int i = 0; i < numSound; ++i) {
		++it;
	}
	(*it)->Done();
	Unlock();
}


/**
* @brief	命令に応じた処理を呼ぶ
*/
void SoundManagerImpl::Command()
{
	SoundCommand command;	//!	Command
	unsigned int numSound;	//!	SoundObjectNumber
	DWORD dwPriority;       //! 優先度（再生用）
	DWORD dwFlag;			//!	再生フラグ（再生用） 
	//!	キューからデータの取り出し
	DeQueue(&numSound, &command, &dwPriority, &dwFlag);

	//	命令に応じた処理
	switch (command) {
	case SOUND_PLAY:
		this->Play(numSound, dwPriority, dwFlag);
		break;
	case SOUND_STOP:
		this->Stop(numSound);
		break;
	case SOUND_DONE:
		this->Done(numSound);
		break;
	default:
		break;
	}
}


/**
* @brief	再生命令
* @param	[in]	numSound	サウンドオブジェクトの番号
* @param	[in]	dwPriority	優先度
* @param	[in]	dwFlag		再生フラグ
*/
void SoundManagerImpl::PlayCommand(unsigned int numSound, DWORD dwPriority, DWORD dwFlag)
{
	EnQueue(numSound, SOUND_PLAY, dwPriority, dwFlag);
}


/**
* @brief	停止命令
* @param	[in]	numSound	サウンドオブジェクトの番号
*/
void SoundManagerImpl::StopCommand(unsigned int numSound)
{
	EnQueue(numSound, SOUND_STOP, 0, 0);
}


/**
* @brief	終了命令
* @param	[in]	numSound	サウンドオブジェクトの番号
*/
void SoundManagerImpl::DoneCommand(unsigned int numSound)
{
	EnQueue(numSound, SOUND_DONE, 0, 0);
}


/**
* @brief	ロック(排他制御)
*/
void SoundManagerImpl::Lock()
{
	EnterCriticalSection(&m_CriticalSection);
}


/**
* @brief	アンロック(排他制御)
*/
void SoundManagerImpl::Unlock()
{
	LeaveCriticalSection(&m_CriticalSection);
}


/**
* @brief	_beginthreadでThreadProcメソッドを使うためのラッパー
*/
void SoundManagerImpl::ThreadProcLauncher(void * arg)
{
	if (!arg)
		return;

	reinterpret_cast<SoundManagerImpl*>(arg)->ThreadProc();
}


/**
* @brief	別スレッドで実際に呼び出されるメソッド
*/
void SoundManagerImpl::ThreadProc()
{
	while (ThreadFlags.m_bThreadActive) {
		switch (m_ePhase)
		{
		case THREAD_INIT:	//	初期化
			m_pSounds.clear();
			m_ePhase = THREAD_RUN;
		case THREAD_RUN:	//	更新

			for (std::list<IStreamingSound*>::iterator it = m_pSounds.begin(); it != m_pSounds.end(); ++it) {
				(*it)->Update();
			}

			if (m_pSounds.size() > 0)
				Command();

			if (ThreadFlags.m_bStopCommand) {
				m_ePhase = THREAD_WAIT;
			}

			break;
		case THREAD_WAIT:	//	終了待ち
			Lock();
			for (std::list<IStreamingSound*>::iterator it = m_pSounds.begin(); it != m_pSounds.end();) {
				SAFE_DELETE(*it);
				it = m_pSounds.erase(it);
			}
			Unlock();
			

			break;
		case THREAD_DONE:	//	終了処理
			ThreadFlags.m_bThreadActive = false;
			break;
		}
		Sleep(16);
	}

	ThreadFlags.m_bThreadDone = true;
}


/**
* @brief	キューにデータを追加する
* @param	[in]	numSound	サウンドオブジェクトの番号
* @param	[in]	Command		追加する命令
* @param	[in]	dwPriority	優先度（再生用）
* @param	[in]	dwFlag		再生フラグ（再生用）
*/
void SoundManagerImpl::EnQueue(unsigned int numSound, SoundCommand Command, DWORD dwPriority, DWORD dwFlag)
{
	//	キューが満杯かどうかの確認
	if (QUEUE_NEXT(m_Queue.tail) == m_Queue.head)
		return;

	//	キューの末尾にデータを挿入する
	m_Queue.Command[m_Queue.tail] = Command;
	m_Queue.numSound[m_Queue.tail] = numSound;
	m_Queue.dwPriority[m_Queue.tail] = dwPriority;
	m_Queue.dwFlag[m_Queue.tail] = dwFlag;

	//	キューの次回挿入位置を決定する
	m_Queue.tail = QUEUE_NEXT(m_Queue.tail);
}


/**
* @brief	キューのデータを取り出す
* @param	[out]	pNumSound	サウンドオブジェクトの番号
* @param	[out]	pCommand	命令
* @param	[out]	pPriority	優先度（再生用）
* @param	[out]	pFlag		再生フラグ（再生用）
*/
void SoundManagerImpl::DeQueue(unsigned int * pNumSound, SoundCommand * pCommand, DWORD * pPriority, DWORD * pFlag)
{
	//	キューに取り出すデータが存在するかを確認する
	if (m_Queue.head == m_Queue.tail)
		return;

	//	キューからデータを取得する
	*pCommand = m_Queue.Command[m_Queue.head];
	*pNumSound = m_Queue.numSound[m_Queue.head];
	*pPriority = m_Queue.dwPriority[m_Queue.head];
	*pFlag = m_Queue.dwFlag[m_Queue.head];

	//	次のデータ取得位置を決定する
	m_Queue.head = QUEUE_NEXT(m_Queue.head);
}



/**
* @class	CSoundManager
*/

/**
* @brief	初期化
*/
void	CSoundManager::Initialize(HWND hWnd)
{
	CDirectSound::Initialize(hWnd);

	SoundManagerImpl::Initialize();
}


/**
* @brief	終了処理
*/
void	CSoundManager::Finalize()
{
	SoundManagerImpl::Finalize();

	CDirectSound::Finalize();
}


/**
* @brief	ストリーミング再生する音声を読み込む
* @param	[in]	pFilename	ファイル名
* @param	[in]	bLoopFlag	ループフラグ
*/
void CSoundManager::LoadStreamingSound(const char * pFilename, bool bLoopFlag)
{
	SoundManagerImpl *pObj = SoundManagerImpl::GetInstance();

	pObj->LoadStreamingSound(pFilename, bLoopFlag);
}


/**
* @brief	ストリーミング再生する音声の読み込みを解除する
* @param	[in]	numSound	サウンドオブジェクトの番号
*/
void CSoundManager::UnloadStreamingSound(unsigned int numSound)
{
	SoundManagerImpl *pObj = SoundManagerImpl::GetInstance();

	pObj->UnloadStreamingSound(numSound);
}


/**
* @brief	再生
* @param	[in]	numSound	サウンドオブジェクトの番号
* @param	[in]	dwPriority	優先度
* @param	[in]	dwFlag		再生フラグ
*/
void CSoundManager::Play(unsigned int numSound, DWORD dwPriority, DWORD dwFlag)
{
	SoundManagerImpl *pObj = SoundManagerImpl::GetInstance();

	pObj->PlayCommand(numSound, dwPriority, dwFlag);
}


/**
* @brief	停止
* @param	[in]	numSound	サウンドオブジェクトの番号
*/
void CSoundManager::Stop(unsigned int numSound)
{
	SoundManagerImpl *pObj = SoundManagerImpl::GetInstance();

	pObj->StopCommand(numSound);
}


/**
* @brief	終了
* @param	[in]	numSound	サウンドオブジェクトの番号
*/
void CSoundManager::Done(unsigned int numSound)
{
	SoundManagerImpl *pObj = SoundManagerImpl::GetInstance();

	pObj->DoneCommand(numSound);
}
