#include "keyblock.h"

struct {
    CONST DWORD* aKeys;
    SHORT        cKeys;
    HHOOK        hHook;
} static g_keyBlockState = {0};

static LRESULT CALLBACK LowLevelKeyboardProc(
    INT     nCode,
    WPARAM  wParam,
    LPARAM  lParam)
{
    KBDLLHOOKSTRUCT*    pHookStruct;
    INT                 i;

    pHookStruct = (KBDLLHOOKSTRUCT*) lParam;

    if (nCode < 0 || (wParam != WM_KEYDOWN && wParam != WM_SYSKEYDOWN)) {
        goto next;
    }

    for (i = 0; i < g_keyBlockState.cKeys; i++) {
        if (pHookStruct->vkCode == g_keyBlockState.aKeys[i]) {
            return 1;
        }
    }

next:
    return CallNextHookEx(0, nCode, wParam, lParam);
}

BOOL KeyBlock_Block(HINSTANCE hInstance, CONST DWORD* aKeys, SHORT cKeys)
{
    if (aKeys == NULL || cKeys <= 0) {
        return FALSE;
    }

    if (g_keyBlockState.hHook != NULL) {
        KeyBlock_Release();
    }

    g_keyBlockState.hHook = SetWindowsHookEx(
        WH_KEYBOARD_LL,
        LowLevelKeyboardProc,
        hInstance,
        0);
    
    if (g_keyBlockState.hHook == NULL) {
        return FALSE;
    }

    g_keyBlockState.aKeys = aKeys;
    g_keyBlockState.cKeys = cKeys;

    return TRUE;
}

VOID KeyBlock_Release(VOID)
{
    if (g_keyBlockState.hHook != NULL) {
        UnhookWindowsHookEx(g_keyBlockState.hHook);
    }

    ZeroMemory(&g_keyBlockState, sizeof g_keyBlockState);
}