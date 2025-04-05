#ifndef __VIDEOPLAYER_H
#define __VIDEOPLAYER_H

#include <Windows.h>

typedef struct _VIDEOPLAYER VIDEOPLAYER, *PVIDEOPLAYER;

PVIDEOPLAYER VideoPlayer_FromResource(
    HWND    hOutput,
    LPCTSTR lpszName,
    LPCTSTR lpszType);

VOID VideoPlayer_SetLoop(PVIDEOPLAYER pVideoPlayer, BOOL bState);

VOID VideoPlayer_Play(PVIDEOPLAYER pVideoPlayer);
VOID VideoPlayer_Stop(PVIDEOPLAYER pVideoPlayer);

VOID VideoPlayer_Release(PVIDEOPLAYER pVideoPlayer);

#endif /* __VIDEOPLAYER_H */