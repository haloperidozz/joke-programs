#include "videoplayer.h"

/***********************************************************************
 * HACK: Temporary attempt to fix
 * "undefined reference to 'IID_IMFPMediaPlayerCallback'" on MinGW
 ***********************************************************************/

#if __MINGW32__
#include <initguid.h>
#endif /* __MINGW32__ */

#include <mfapi.h>
#include <mfplay.h>

#if __MINGW32__
#undef INITGUID
#endif /* __MINGW32__ */

/***********************************************************************/

#include <shlwapi.h>
#include <windowsx.h>

/***********************************************************************
 * MFCreateMFByteStreamOnStream
 *
 * HACK: For reasons unknown to me, the mfidl.h header in MinGW lacks
 * a definition for the MFCreateMFByteStreamOnStream function.
 ***********************************************************************/

#if __MINGW32__

typedef HRESULT (*MFCreateMFByteStreamOnStream_t)(IStream*, IMFByteStream**);

static HRESULT
MFCreateMFByteStreamOnStream(IStream *pStream, IMFByteStream **ppByteStream)
{
    HMODULE hModule = LoadLibraryA("mfplat.dll");
    FARPROC fpProc;

    if (hModule == NULL) {
        return E_FAIL;
    }
    
    fpProc = GetProcAddress(hModule, "MFCreateMFByteStreamOnStream");

    if (fpProc == NULL) {
        return E_FAIL;
    }

    return ((MFCreateMFByteStreamOnStream_t) fpProc)(pStream, ppByteStream);
}

#endif /* __MINGW32__ */

/***********************************************************************/

struct _VIDEOPLAYER {
    IMFPMediaPlayer*    pPlayer;
    BOOL                bLoop;
};

struct _VIDEOPLAYER_CALLBACK {
    IMFPMediaPlayerCallback iface;
    LONG                    lRefCount;
    PVIDEOPLAYER            pVideoPlayer;
};

/***********************************************************************
 * VIDEOPLAYER_CALLBACK
 ***********************************************************************/

static STDMETHODIMP_(ULONG)
VideoPlayerCallback_AddRef(IMFPMediaPlayerCallback *pThis)
{
    struct _VIDEOPLAYER_CALLBACK* pCallback;
    pCallback = (struct _VIDEOPLAYER_CALLBACK*) pThis;
    return InterlockedIncrement(&pCallback->lRefCount);
}

static STDMETHODIMP_(ULONG)
VideoPlayerCallback_Release(IMFPMediaPlayerCallback *pThis)
{
    struct _VIDEOPLAYER_CALLBACK*   pCallback;
    ULONG                           ulCount;

    pCallback = (struct _VIDEOPLAYER_CALLBACK*) pThis;
    ulCount = InterlockedDecrement(&pCallback->lRefCount);

    if (ulCount == 0) {
        HeapFree(GetProcessHeap(), 0, pCallback);
    }

    return ulCount;
}

static STDMETHODIMP
VideoPlayerCallback_QueryInterface(IMFPMediaPlayerCallback *pThis,
                             REFIID riid, VOID **ppv)
{
    if (IsEqualIID(riid, &IID_IMFPMediaPlayerCallback) ||
        IsEqualIID(riid, &IID_IUnknown)) 
    {
        *ppv = pThis;
        VideoPlayerCallback_AddRef(pThis);
        return S_OK;
    }
    
    *ppv = NULL;
    return E_NOINTERFACE;
}

static STDMETHODIMP_(VOID)
VideoPlayerCallback_OnMediaPlayerEvent(
    IMFPMediaPlayerCallback*    pThis,
    MFP_EVENT_HEADER*           pEventHeader)
{
    struct _VIDEOPLAYER_CALLBACK*   pCallback;

    pCallback = (struct _VIDEOPLAYER_CALLBACK*) pThis;

    if (pEventHeader->eEventType == MFP_EVENT_TYPE_PLAYBACK_ENDED) {
        if (pCallback->pVideoPlayer->bLoop == TRUE) {
            VideoPlayer_Play(pCallback->pVideoPlayer);
        }
    }
}

static IMFPMediaPlayerCallback*
VideoPlayerCallback_CreateFor(PVIDEOPLAYER pVideoPlayer)
{
    struct _VIDEOPLAYER_CALLBACK* pCallback = NULL;

    static IMFPMediaPlayerCallbackVtbl vtbl = {
        .AddRef = VideoPlayerCallback_AddRef,
        .OnMediaPlayerEvent = VideoPlayerCallback_OnMediaPlayerEvent,
        .QueryInterface = VideoPlayerCallback_QueryInterface,
        .Release = VideoPlayerCallback_Release
    };

    if (pVideoPlayer == NULL) {
        return NULL;
    }

    pCallback = HeapAlloc(
        GetProcessHeap(),
        HEAP_ZERO_MEMORY,
        sizeof(struct _VIDEOPLAYER_CALLBACK));

    if (pCallback == NULL) {
        return NULL;
    }

    pCallback->iface.lpVtbl = &vtbl;
    pCallback->lRefCount = 1;
    pCallback->pVideoPlayer = pVideoPlayer;

    return (IMFPMediaPlayerCallback*) pCallback;
}

/***********************************************************************
 * VIDEOPLAYER
 ***********************************************************************/

static IStream* CreateIStreamFromResource(
    HINSTANCE   hInstance,
    LPCTSTR     lpszName,
    LPCTSTR     lpszType)
{
    HRSRC   hResource = NULL;
    HGLOBAL hResourceData = NULL;
    BYTE*   pbResourceData = NULL;
    DWORD   dwResourceSize = 0;

    if (lpszName == NULL || lpszType == NULL) {
        return NULL;
    }

    hResource = FindResource(hInstance, lpszName, lpszType);

    if (hResource == NULL) {
        return NULL;
    }

    dwResourceSize = SizeofResource(hInstance, hResource);

    if (dwResourceSize <= 0) {
        return NULL;
    }

    hResourceData = LoadResource(hInstance, hResource);

    if (hResourceData == NULL) {
        return NULL;
    }

    pbResourceData = LockResource(hResourceData);
    
    if (pbResourceData == NULL) {
        return NULL;
    }

    return SHCreateMemStream(pbResourceData, dwResourceSize);
}

PVIDEOPLAYER VideoPlayer_FromResource(
    HWND    hOutput,
    LPCTSTR lpszName,
    LPCTSTR lpszType)
{
    PVIDEOPLAYER        pVideoPlayer = NULL;
    IStream*            pIStream = NULL;
    IMFSourceResolver*  pResolver = NULL;
    IMFPMediaItem*      pMediaItem = NULL;
    IMFByteStream*      pByteStream = NULL;
    IUnknown*           pSourceUnk = NULL;
    MF_OBJECT_TYPE      objectType = MF_OBJECT_INVALID;
    HRESULT             hResult = S_OK;

    pIStream = CreateIStreamFromResource(
        GetWindowInstance(hOutput),
        lpszName,
        lpszType);
    
    if (pIStream == NULL) {
        return NULL;
    }

    pVideoPlayer = HeapAlloc(GetProcessHeap(), 0, sizeof(VIDEOPLAYER));

    if (pVideoPlayer == NULL) {
        return NULL;
    }

    pVideoPlayer->bLoop = FALSE;

    hResult = MFCreateMFByteStreamOnStream(pIStream, &pByteStream);

    if (FAILED(hResult)) {
        goto cleanup;
    }

    hResult = MFCreateSourceResolver(&pResolver);

    if (FAILED(hResult)) {
        goto cleanup;
    }

    hResult = pResolver->lpVtbl->CreateObjectFromByteStream(
        pResolver,
        pByteStream,
        NULL,
        MF_RESOLUTION_MEDIASOURCE |
        MF_RESOLUTION_CONTENT_DOES_NOT_HAVE_TO_MATCH_EXTENSION_OR_MIME_TYPE,
        NULL,
        &objectType,
        &pSourceUnk);
    
    if (FAILED(hResult)) {
        goto cleanup;
    }
    
    hResult = MFPCreateMediaPlayer(
        NULL,
        FALSE,
        0,
        VideoPlayerCallback_CreateFor(pVideoPlayer),
        hOutput,
        &(pVideoPlayer->pPlayer));
    
    if (FAILED(hResult)) {
        goto cleanup;
    }
    
    hResult = pVideoPlayer->pPlayer->lpVtbl->CreateMediaItemFromObject(
        pVideoPlayer->pPlayer,
        pSourceUnk,
        TRUE,
        0,
        &pMediaItem);
    
    if (FAILED(hResult)) {
        goto cleanup;
    }

    hResult = pVideoPlayer->pPlayer->lpVtbl->SetMediaItem(
        pVideoPlayer->pPlayer,
        pMediaItem);

    if (FAILED(hResult)) {
        goto cleanup;
    }

cleanup:
    pResolver->lpVtbl->Release(pResolver);
    pSourceUnk->lpVtbl->Release(pSourceUnk);
    pMediaItem->lpVtbl->Release(pMediaItem);
    pByteStream->lpVtbl->Release(pByteStream);
    pIStream->lpVtbl->Release(pIStream);

    if (FAILED(hResult)) {
        HeapFree(GetProcessHeap(), 0, pVideoPlayer);
        return NULL;
    }

    return pVideoPlayer;
}

VOID VideoPlayer_SetLoop(PVIDEOPLAYER pVideoPlayer, BOOL bState)
{
    if (pVideoPlayer != NULL) {
        pVideoPlayer->bLoop = bState;
    }
}

VOID VideoPlayer_Play(PVIDEOPLAYER pVideoPlayer)
{
    if (pVideoPlayer != NULL) {
        pVideoPlayer->pPlayer->lpVtbl->Play(pVideoPlayer->pPlayer);
    }
}

VOID VideoPlayer_Stop(PVIDEOPLAYER pVideoPlayer)
{
    if (pVideoPlayer != NULL) {
        pVideoPlayer->pPlayer->lpVtbl->Stop(pVideoPlayer->pPlayer);
    }
}

VOID VideoPlayer_Release(PVIDEOPLAYER pVideoPlayer)
{
    if (pVideoPlayer == NULL) {
        return;
    }

    pVideoPlayer->pPlayer->lpVtbl->Shutdown(pVideoPlayer->pPlayer);
    pVideoPlayer->pPlayer->lpVtbl->Release(pVideoPlayer->pPlayer);

    HeapFree(GetProcessHeap(), 0, pVideoPlayer);
}
