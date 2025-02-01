#include <windows.h>
#include <tchar.h>

#include "videoplayer.hh"

#include "resource.h"

#define KING_VON_CLASS_NAME     TEXT("KingVonWindow")

#define VK_ALT                  VK_MENU

static const DWORD aBlockedKeys[] = {
    VK_ALT, VK_F4, VK_ESCAPE, VK_CONTROL, VK_TAB, VK_SHIFT, VK_LWIN,
    VK_RWIN, VK_DELETE
};

static LRESULT CALLBACK LowLevelKeyboardProc(INT nCode, WPARAM wParam,
                                             LPARAM lParam)
{
    KBDLLHOOKSTRUCT *pHookStruct = (KBDLLHOOKSTRUCT*) lParam;
    INT i;

    if (nCode < 0 || (wParam != WM_KEYDOWN && wParam != WM_SYSKEYDOWN)) {
        goto next;
    }

    for (i = 0; i < ARRAYSIZE(aBlockedKeys); i++) {
        if (pHookStruct->vkCode == aBlockedKeys[i]) {
            return 1;
        }
    }

next:
    return CallNextHookEx(0, nCode, wParam, lParam);
}

static LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam,
                                   LPARAM lParam)
{
    VideoPlayer *pVideoPlayer = NULL;

    if (uMsg == WM_CREATE) {
        pVideoPlayer = new VideoPlayer(hWnd);

        pVideoPlayer->LoadFromResource(MAKEINTRESOURCE(IDR_KING_VON_MP4));
        pVideoPlayer->SetLoop(true);
        pVideoPlayer->Play();

        SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR) pVideoPlayer);
    } else {
        pVideoPlayer = (VideoPlayer*) GetWindowLongPtr(hWnd, GWLP_USERDATA);
    }

    if (uMsg == WM_DESTROY) {
        delete pVideoPlayer;
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

static ATOM RegisterKingVonWindowClass(HINSTANCE hInstance)
{
    WNDCLASSEX wClass = {0};

    wClass.cbSize         = sizeof(wClass);
    wClass.lpfnWndProc    = WindowProc;
    wClass.hInstance      = hInstance;
    wClass.lpszClassName  = KING_VON_CLASS_NAME;
    wClass.hCursor        = LoadCursor(NULL, IDC_ARROW);

    return RegisterClassEx(&wClass);
}

INT WINAPI _tWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                     LPTSTR lpCmdLine, INT nCmdShow)
{
    HHOOK hKeyboardHook = NULL;
    HWND hWnd = NULL;
    MSG msg = {0};

    if (FAILED(MFStartup(MF_VERSION))) {
        return -1;
    }

    hKeyboardHook = SetWindowsHookEx(
        WH_KEYBOARD_LL, LowLevelKeyboardProc, hInstance, 0);

    if (RegisterKingVonWindowClass(hInstance) == 0) {
        return -1;
    }

    hWnd = CreateWindow(
        KING_VON_CLASS_NAME, NULL, WS_POPUP,
        CW_USEDEFAULT, CW_USEDEFAULT,
        GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN),
        NULL, NULL, hInstance, NULL
    );

    if (hWnd == NULL) {
        return -1;
    }

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    while (GetMessage(&msg, NULL, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    UnhookWindowsHookEx(hKeyboardHook);
    MFShutdown();

    return (INT) msg.wParam;
}