#include "videoplayer.hh"

#include <shlwapi.h>
#include <evr.h>

/***********************************************************************
 * HACK: For reasons unknown to me, the mfidl.h header in MinGW lacks
 * a definition for the MFCreateMFByteStreamOnStream function.
 ***********************************************************************/

#if __MINGW32__

typedef HRESULT (*MFCreateMFByteStreamOnStream_t)(IStream*, IMFByteStream**);

static HRESULT MFCreateMFByteStreamOnStream(IStream *pStream,
                                            IMFByteStream **ppByteStream)
{
    HMODULE hMfplat = LoadLibrary(L"mfplat.dll");
    FARPROC fpAddress = NULL;

    if (hMfplat == NULL) {
        return S_FALSE;
    }

    fpAddress = GetProcAddress(hMfplat, "MFCreateMFByteStreamOnStream");

    if (fpAddress == NULL) {
        return S_FALSE;
    }

    return ((MFCreateMFByteStreamOnStream_t) fpAddress)(pStream, ppByteStream);
}

#endif /* __MINGW32__ */

/***********************************************************************
 * SafeRelease
 ***********************************************************************/

template <class T> void SafeRelease(T **ppT) {
    if ((*ppT) != NULL) {
        (*ppT)->Release();
        *ppT = NULL;
    }
}

/***********************************************************************
 * VideoPlayer
 ***********************************************************************/

VideoPlayer::VideoPlayer(HWND hOutputWnd)
    : m_hOutputWnd(hOutputWnd), m_pMediaSession(NULL), m_bLoop(false)
{
}

VideoPlayer::~VideoPlayer()
{
    Release();
}

bool VideoPlayer::LoadFromResource(LPCTSTR lpszName, LPCTSTR lpszType)
{
    VideoPlayerSessionCallback *pSessionCallback = NULL;
    IMFByteStream *pByteStream = NULL;
    IMFSourceResolver *pResolver = NULL;
    IMFMediaSource *pMediaSource = NULL;
    IMFTopology *pTopology = NULL;
    MF_OBJECT_TYPE objectType = MF_OBJECT_INVALID;
    IUnknown *pSourceUnk = NULL;
    HRESULT hRes;

    Release();

    pByteStream = LoadResourceToMemory(lpszName, lpszType);

    if (pByteStream == NULL) {
        return false;
    }

    MFCreateSourceResolver(&pResolver);

    if (pResolver == NULL) {
        SafeRelease(&pByteStream);
        return false;
    }

    hRes = pResolver->CreateObjectFromByteStream(
        pByteStream,
        NULL,
        MF_RESOLUTION_MEDIASOURCE |
        MF_RESOLUTION_CONTENT_DOES_NOT_HAVE_TO_MATCH_EXTENSION_OR_MIME_TYPE,
        NULL,
        &objectType,
        &pSourceUnk
    );

    if (FAILED(hRes) || objectType != MF_OBJECT_MEDIASOURCE) {
        SafeRelease(&pResolver);
        SafeRelease(&pByteStream);
        return false;
    }

    if (FAILED(pSourceUnk->QueryInterface(IID_PPV_ARGS(&pMediaSource)))) {
        SafeRelease(&pSourceUnk);
        SafeRelease(&pResolver);
        SafeRelease(&pByteStream);
        return false;
    }

    pTopology = CreateMediaTopology(pMediaSource);

    if (pTopology == NULL) {
        SafeRelease(&pMediaSource);
        SafeRelease(&pSourceUnk);
        SafeRelease(&pResolver);
        SafeRelease(&pByteStream);
        return false;
    }

    pSessionCallback = new VideoPlayerSessionCallback(this);

    MFCreateMediaSession(NULL, &m_pMediaSession);

    m_pMediaSession->BeginGetEvent(pSessionCallback, NULL);
    m_pMediaSession->SetTopology(0, pTopology);

    SafeRelease(&pTopology);
    SafeRelease(&pMediaSource);
    SafeRelease(&pSourceUnk);
    SafeRelease(&pResolver);
    SafeRelease(&pByteStream);
    return true;
}

void VideoPlayer::SetLoop(bool bState)
{
    m_bLoop = bState;
}

void VideoPlayer::Play()
{
    PROPVARIANT varStart;

    if (m_pMediaSession != NULL) {
        PropVariantInit(&varStart);
        varStart.vt = VT_EMPTY;
        m_pMediaSession->Start(NULL, &varStart);
        PropVariantClear(&varStart);
    }
}

void VideoPlayer::Stop()
{
    if (m_pMediaSession != NULL) {
        m_pMediaSession->Stop();
    }
}

void VideoPlayer::Release()
{
    SafeRelease(&m_pMediaSession);
    m_bLoop = false;
}

IMFByteStream *VideoPlayer::LoadResourceToMemory(LPCTSTR lpszName,
                                                 LPCTSTR lpszType)
{
    IMFByteStream *pByteStream = NULL;
    IStream *pMemoryStream = NULL;
    HINSTANCE hInst = NULL;
    HRSRC hRes = NULL;
    HGLOBAL hResData = NULL;
    DWORD dwResSize;

    hInst = (HINSTANCE) GetWindowLongPtr(m_hOutputWnd, GWLP_HINSTANCE);

    if (hInst == NULL) {
        return NULL;
    }

    hRes = FindResource(hInst, lpszName, lpszType);

    if (hRes == NULL) {
        return NULL;
    }

    hResData = LoadResource(hInst, hRes);
    dwResSize = SizeofResource(hInst, hRes);

    pMemoryStream = SHCreateMemStream((BYTE*) LockResource(hResData),
                                      dwResSize);

    if (pMemoryStream != NULL) {
        MFCreateMFByteStreamOnStream(pMemoryStream, &pByteStream);
        pMemoryStream->Release();
    }

    return pByteStream;
}

IMFTopology *VideoPlayer::CreateMediaTopology(IMFMediaSource *pMediaSource)
{
    IMFTopology *pTopology = NULL;
    IMFPresentationDescriptor *pPD = NULL;
    IMFStreamDescriptor *pSD = NULL;
    IMFMediaTypeHandler *pHandler = NULL;
    IMFTopologyNode *pSourceNode = NULL;
    IMFTopologyNode *pOutputNode = NULL;
    IMFActivate *pActivate = NULL;
    GUID majorType;
    BOOL bSelected = FALSE;
    DWORD dwStreamCount = 0;
    DWORD i;

    MFCreateTopology(&pTopology);

    if (FAILED(pMediaSource->CreatePresentationDescriptor(&pPD))) {
        SafeRelease(&pTopology);
        return NULL;
    }

    if (FAILED(pPD->GetStreamDescriptorCount(&dwStreamCount))) {
        SafeRelease(&pPD);
        SafeRelease(&pTopology);
        return NULL;
    }

    for (i = 0; i < dwStreamCount; i++) {
        if (FAILED(pPD->GetStreamDescriptorByIndex(i, &bSelected, &pSD))) {
            continue;
        }

        if (FAILED(pSD->GetMediaTypeHandler(&pHandler))) {
            SafeRelease(&pSD);
            continue;
        }

        if (FAILED(pHandler->GetMajorType(&majorType))) {
            SafeRelease(&pHandler);
            SafeRelease(&pSD);
            continue;
        }

        MFCreateTopologyNode(MF_TOPOLOGY_SOURCESTREAM_NODE, &pSourceNode);

        pSourceNode->SetUnknown(MF_TOPONODE_SOURCE, pMediaSource);
        pSourceNode->SetUnknown(MF_TOPONODE_PRESENTATION_DESCRIPTOR, pPD);
        pSourceNode->SetUnknown(MF_TOPONODE_STREAM_DESCRIPTOR, pSD);

        if (majorType == MFMediaType_Video) {
            MFCreateVideoRendererActivate(m_hOutputWnd, &pActivate);
            MFCreateTopologyNode(MF_TOPOLOGY_OUTPUT_NODE, &pOutputNode);
        } else if (majorType == MFMediaType_Audio) {
            MFCreateAudioRendererActivate(&pActivate);
            MFCreateTopologyNode(MF_TOPOLOGY_OUTPUT_NODE, &pOutputNode);
        }

        if (pOutputNode != NULL && pActivate != NULL) {
            pOutputNode->SetObject(pActivate);

            pTopology->AddNode(pSourceNode);
            pTopology->AddNode(pOutputNode);
            pSourceNode->ConnectOutput(0, pOutputNode, 0);
        }

        SafeRelease(&pActivate);
        SafeRelease(&pOutputNode);
        SafeRelease(&pHandler);
        SafeRelease(&pSD);
        SafeRelease(&pSourceNode);
    }

    SafeRelease(&pPD);
    return pTopology;
}

/***********************************************************************
 * VideoPlayerSessionCallback
 ***********************************************************************/

VideoPlayerSessionCallback::VideoPlayerSessionCallback(VideoPlayer *pPlayer)
    : m_pPlayer(pPlayer), m_lRefCount(1)
{
}

STDMETHODIMP VideoPlayerSessionCallback::QueryInterface(REFIID riid, void **ppv)
{
    if (riid == IID_IMFAsyncCallback || riid == IID_IUnknown) {
        *ppv = (IMFAsyncCallback*) this;
        AddRef();
        return S_OK;
    }

    *ppv = NULL;

    return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) VideoPlayerSessionCallback::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

STDMETHODIMP_(ULONG) VideoPlayerSessionCallback::Release()
{
    ULONG uCount = InterlockedDecrement(&m_lRefCount);

    if (uCount == 0) {
        delete this;
    }

    return uCount;
}

STDMETHODIMP VideoPlayerSessionCallback::GetParameters(DWORD*, DWORD*)
{
    return E_NOTIMPL;
}

STDMETHODIMP VideoPlayerSessionCallback::Invoke(IMFAsyncResult *pResult)
{
    IMFMediaSession *pMediaSession = m_pPlayer->m_pMediaSession;
    IMFMediaEvent *pEvent = NULL;
    MediaEventType eventType;
    HRESULT hRes;
    
    hRes = pMediaSession->EndGetEvent(pResult, &pEvent);
    
    if (FAILED(hRes)) {
        return hRes;
    }

    pEvent->GetType(&eventType);

    if (eventType == MESessionEnded && m_pPlayer->m_bLoop) {
        m_pPlayer->Play();
    }

    pEvent->Release();

    pMediaSession->BeginGetEvent(this, nullptr);
    return S_OK;
}