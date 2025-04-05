#include <windows.h>
#include <tchar.h>
#include <mfapi.h>

#include "videoplayer.h"
#include "keyblock.h"

#include "resource.h"

#define KING_VON_CLASS_NAME TEXT("KingVonWindow")

static CONST DWORD g_aKeys[] = {
    VK_MENU /* ALT */, VK_F4, VK_ESCAPE, VK_CONTROL, VK_TAB, VK_SHIFT,
    VK_LWIN, VK_RWIN, VK_DELETE
};

static LRESULT CALLBACK WindowProc(
    HWND    hWnd,
    UINT    uMsg,
    WPARAM  wParam,
    LPARAM  lParam)
{
    PVIDEOPLAYER pVideoPlayer = NULL;

    if (uMsg == WM_CREATE) {
        pVideoPlayer = VideoPlayer_FromResource(
            hWnd,
            MAKEINTRESOURCE(IDR_KING_VON_MP4),
            RT_RCDATA);
        
        if (pVideoPlayer == NULL) {
            return -1;
        }

        VideoPlayer_SetLoop(pVideoPlayer, TRUE);
        VideoPlayer_Play(pVideoPlayer);

        SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR) pVideoPlayer);
    } else {
        pVideoPlayer = (PVIDEOPLAYER) GetWindowLongPtr(hWnd, GWLP_USERDATA);
    }

    if (uMsg == WM_DESTROY) {
        VideoPlayer_Release(pVideoPlayer);
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

INT WINAPI _tWinMain(
    HINSTANCE   hInstance,
    HINSTANCE   hPrevInstance,
    LPTSTR      lpCmdLine,
    INT         nCmdShow)
{
    WNDCLASSEX  wClass = {0};
    HWND        hWnd = NULL;
    MSG         msg = {0};

    if (FAILED(MFStartup(MF_VERSION, MFSTARTUP_LITE))) {
        return -1;
    }

    wClass.cbSize         = sizeof(wClass);
    wClass.lpfnWndProc    = WindowProc;
    wClass.hInstance      = hInstance;
    wClass.lpszClassName  = KING_VON_CLASS_NAME;
    wClass.hCursor        = LoadCursor(NULL, IDC_ARROW);

    if (RegisterClassEx(&wClass) == 0) {
        return -1;
    }

    if (KeyBlock_Block(hInstance, g_aKeys, ARRAYSIZE(g_aKeys)) == FALSE) {
        return -1;
    }

    hWnd = CreateWindow(
        KING_VON_CLASS_NAME,
        NULL,
        WS_POPUP,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        GetSystemMetrics(SM_CXSCREEN),
        GetSystemMetrics(SM_CYSCREEN),
        NULL,
        NULL,
        hInstance,
        NULL);

    if (hWnd == NULL) {
        return -1;
    }

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    while (GetMessage(&msg, NULL, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    KeyBlock_Release();
    MFShutdown();

    return (INT) msg.wParam;
}