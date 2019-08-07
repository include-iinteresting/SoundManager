#include "stdafx.h"
#include <dsound.h>
#include "DirectSound.h"

#pragma comment(lib, "dsound.lib")


class DirectSoundImpl 
{
	friend CDirectSound;
private:
	DirectSoundImpl();
	~DirectSoundImpl();

	static	void	Initialize();
	static	void	Finalize();
	static	DirectSoundImpl	*GetInstance();

	HRESULT	CreateDirectSound(HWND hWnd);
	HRESULT	CreatePrimaryBuffer(IDirectSound8 *pDS8);
private:
	static	DirectSoundImpl	*m_pInstance;
	IDirectSound8			*m_pDS8;
	IDirectSoundBuffer		*m_pDSBuffer;
	IDirectSoundBuffer8		*m_pPrimary;
};

DirectSoundImpl	*DirectSoundImpl::m_pInstance = NULL;

/**
* @brief	コンストラクタ
*/
DirectSoundImpl::DirectSoundImpl()
{
	m_pDS8 = NULL;
	m_pDSBuffer = NULL;
	m_pPrimary = NULL;
}


/**
* @brief	デストラクタ
*/
DirectSoundImpl::~DirectSoundImpl()
{
	SAFE_DELETE(m_pPrimary);
	SAFE_DELETE(m_pDSBuffer);
	SAFE_DELETE(m_pDS8);
}


/**
* @brief	初期化
*/
void DirectSoundImpl::Initialize()
{
	if (m_pInstance)
		return;

	m_pInstance = new DirectSoundImpl();
}


/**
* @brief	終了処理
*/
void DirectSoundImpl::Finalize()
{
	SAFE_DELETE(m_pInstance);
}


/**
* @brief	インスタンス取得
* @return	インスタンス
*/
DirectSoundImpl * DirectSoundImpl::GetInstance()
{
	if (!m_pInstance)
		DirectSoundImpl::Initialize();

	return m_pInstance;
}


/**
* @brief	DirectSoundの生成
* @param	[in]	hWnd	WindowHandle
* @return	HRESULT
*/
HRESULT DirectSoundImpl::CreateDirectSound(HWND hWnd)
{
	HRESULT hr = S_OK;
	hr = DirectSoundCreate8(NULL, &m_pDS8, NULL);
	if (FAILED(hr))
		goto ERROR_EXIT;

	hr = m_pDS8->SetCooperativeLevel(hWnd, DSSCL_EXCLUSIVE | DSSCL_PRIORITY);
	if (FAILED(hr))
		goto ERROR_EXIT;

ERROR_EXIT:
	return hr;
}


/**
* @brief	プライマリーバッファの生成
* @param	[in]	pDS8	IDirectSound8
* @return	HRESULT
*/
HRESULT DirectSoundImpl::CreatePrimaryBuffer(IDirectSound8 *pDS8)
{
	HRESULT hr = S_OK;

	WAVEFORMATEX wfx;

	//	プライマリサウンドバッファの作成
	DSBUFFERDESC dsdesc;
	ZeroMemory(&dsdesc, sizeof(dsdesc));
	dsdesc.dwSize = sizeof(DSBUFFERDESC);
	dsdesc.dwFlags = DSBCAPS_PRIMARYBUFFER;
	dsdesc.dwBufferBytes = 0;
	dsdesc.lpwfxFormat = NULL;
	hr = pDS8->CreateSoundBuffer(&dsdesc,)


	return E_NOTIMPL;
}



