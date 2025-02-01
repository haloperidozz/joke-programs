#ifndef __VIDEO_PLAYER_HH
#define __VIDEO_PLAYER_HH

#include <windows.h>
#include <mfidl.h>
#include <mfapi.h>

class VideoPlayer
{
    HWND m_hOutputWnd;
    IMFMediaSession *m_pMediaSession;
    bool m_bLoop;

public:
    VideoPlayer(HWND hOutputWnd);
    ~VideoPlayer();

    bool LoadFromResource(LPCTSTR lpszName, LPCTSTR lpszType = RT_RCDATA);
    
    void SetLoop(bool bState);

    void Play();
    void Stop();

    void Release();

private:
    IMFByteStream *LoadResourceToMemory(LPCTSTR lpszName, LPCTSTR lpszType);
    IMFTopology *CreateMediaTopology(IMFMediaSource *pMediaSource);

    friend class VideoPlayerSessionCallback;
};

class VideoPlayerSessionCallback : public IMFAsyncCallback
{
    VideoPlayer *m_pPlayer;
    long m_lRefCount;

private:
    VideoPlayerSessionCallback(VideoPlayer *pPlayer);

    STDMETHODIMP QueryInterface(REFIID riid, void **ppv) override;
    STDMETHODIMP_(ULONG) AddRef() override;
    STDMETHODIMP_(ULONG) Release() override;
    STDMETHODIMP GetParameters(DWORD*, DWORD*) override;
    STDMETHODIMP Invoke(IMFAsyncResult *pResult) override;

    friend class VideoPlayer;
};

#endif /* __VIDEO_PLAYER_HH */