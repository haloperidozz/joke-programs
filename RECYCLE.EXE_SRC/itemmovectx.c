#include "itemmovectx.h"

/***********************************************************************
 * LvItemMoveContext
 ***********************************************************************/

VOID LvItemMoveContext_Delete(PLVITEMMOVECONTEXT pContext)
{
    if (pContext == NULL) {
        return;
    }

    if (pContext->lprcRemote != NULL) {
        VirtualFreeEx(
            pContext->hListViewProcess, pContext->lprcRemote,
            0, MEM_RELEASE
        );
    }

    if (pContext->lpptRemote != NULL) {
        VirtualFreeEx(
            pContext->hListViewProcess, pContext->lpptRemote,
            0, MEM_RELEASE
        );
    }

    if (pContext->hListViewProcess != NULL) {
        CloseHandle(pContext->hListViewProcess);
    }

    HeapFree(GetProcessHeap(), 0, pContext);
}

PLVITEMMOVECONTEXT LvItemMoveContext_CreateFor(HWND hListView)
{
    PLVITEMMOVECONTEXT pContext = NULL;
    DWORD dwProcessId = 0;
    HANDLE hHeap = GetProcessHeap();

    if (hListView == NULL) {
        return NULL;
    }

    if (GetWindowThreadProcessId(hListView, &dwProcessId) == 0) {
        return NULL;
    }

    pContext = HeapAlloc(hHeap, HEAP_ZERO_MEMORY, sizeof *pContext);

    if (pContext == NULL) {
        return NULL;
    }

    pContext->hListViewProcess = OpenProcess(
        PROCESS_QUERY_INFORMATION | PROCESS_VM_OPERATION |
        PROCESS_VM_WRITE | PROCESS_VM_READ,
        FALSE, dwProcessId
    );

    pContext->hListView = hListView;
    
    if (pContext->hListViewProcess == NULL) {
        LvItemMoveContext_Delete(pContext);
        return NULL;
    }

    pContext->lpptRemote = VirtualAllocEx(
        pContext->hListViewProcess,
        NULL, sizeof(POINT), MEM_COMMIT, PAGE_READWRITE
    );

    if (pContext->lpptRemote == NULL) {
        LvItemMoveContext_Delete(pContext);
        return NULL;
    }

    pContext->lprcRemote = VirtualAllocEx(
        pContext->hListViewProcess,
        NULL, sizeof(RECT), MEM_COMMIT, PAGE_READWRITE
    );

    if (pContext->lprcRemote == NULL) {
        LvItemMoveContext_Delete(pContext);
        return NULL;
    }

    return pContext;
}

BOOL LvItemMoveContext_GetItemPosition(
    PLVITEMMOVECONTEXT pContext,
    INT i,
    LPPOINT lppt)
{
    BOOL bResult = FALSE;

    if (pContext == NULL || lppt == NULL) {
        return FALSE;
    }
            
    bResult = (BOOL) SendMessage(
        pContext->hListView, LVM_GETITEMPOSITION,
        (WPARAM) i, (LPARAM) pContext->lpptRemote
    );

    if (bResult == FALSE) {
        return FALSE;
    }

    return ReadProcessMemory(
        pContext->hListViewProcess, pContext->lpptRemote,
        lppt, sizeof(POINT), NULL
    );
}

BOOL LvItemMoveContext_SetItemPosition(
    PLVITEMMOVECONTEXT pContext,
    INT i,
    INT x, INT y)
{
    if (pContext == NULL) {
        return FALSE;
    }

    return (BOOL) SendMessage(                   /* :D */
        pContext->hListView, LVM_SETITEMPOSITION,
        (WPARAM) i, MAKELPARAM(x, y)
    );
}

BOOL LvItemMoveContext_SetItemPositionPoint(
    PLVITEMMOVECONTEXT pContext,
    INT i,
    LPPOINT lppt)
{
    if (lppt == NULL || pContext == NULL) {
        return FALSE;
    }

    return LvItemMoveContext_SetItemPosition(pContext, i, lppt->x, lppt->y);
}

/* unsafe */
BOOL LvItemMoveContext_GetItem(PLVITEMMOVECONTEXT pContext, LPLVITEM lplvi)
{
    BOOL bResult = FALSE;
    LPLVITEM lplviRemote = NULL;
    LPTSTR pszTextRemote = NULL;
    LPTSTR pszTemp;

    if (pContext == NULL || lplvi == NULL) {
        return FALSE;
    }

    if (lplvi->mask & LVIF_TEXT && lplvi->pszText != NULL) {
        pszTextRemote = VirtualAllocEx(
            pContext->hListViewProcess, NULL,
            lplvi->cchTextMax * sizeof(TCHAR), MEM_COMMIT, PAGE_READWRITE
        );

        if (pszTextRemote == NULL) {
            return FALSE;
        }

        WriteProcessMemory(
            pContext->hListViewProcess, pszTextRemote, lplvi->pszText,
            lplvi->cchTextMax * sizeof(TCHAR), NULL
        );

        pszTemp = lplvi->pszText;
        lplvi->pszText = pszTextRemote;
    }

    lplviRemote = VirtualAllocEx(
        pContext->hListViewProcess, NULL,
        sizeof(LVITEM), MEM_COMMIT, PAGE_READWRITE
    );

    if (lplviRemote == NULL) {
        goto cleanup;
    }

    bResult = WriteProcessMemory(
        pContext->hListViewProcess, lplviRemote,
        lplvi, sizeof(LVITEM), NULL
    );

    if (bResult == FALSE) {
        goto cleanup;
    }

    bResult = (BOOL) SendMessage(
        pContext->hListView, LVM_GETITEM, 0, (LPARAM)lplviRemote
    );

    if (bResult == FALSE) {
        goto cleanup;
    }

    bResult = ReadProcessMemory(
        pContext->hListViewProcess, lplviRemote,
        lplvi, sizeof(LVITEM), NULL
    );

    if (bResult == FALSE) {
        goto cleanup;
    }

    if (lplvi->mask & LVIF_TEXT && lplvi->pszText != NULL) {
        ReadProcessMemory(
            pContext->hListViewProcess, pszTextRemote,
            pszTemp, lplvi->cchTextMax * sizeof(TCHAR), NULL
        );

        lplvi->pszText = pszTemp;
    }

cleanup:
    if (pszTextRemote != NULL) {
        VirtualFreeEx(
            pContext->hListViewProcess, pszTextRemote, 0, MEM_RELEASE
        );
    }

    if (lplviRemote != NULL) {
        VirtualFreeEx(
            pContext->hListViewProcess, lplviRemote, 0, MEM_RELEASE
        );
    }

    return bResult;
}

INT LvItemMoveContext_GetItemCount(PLVITEMMOVECONTEXT pContext)
{
    if (pContext == NULL) {
        return 0;
    }

    return (INT) SendMessage(pContext->hListView, LVM_GETITEMCOUNT, 0, 0);
}

VOID LvItemMoveContext_SetExtendedListViewStyle(
    PLVITEMMOVECONTEXT pContext,
    DWORD dwStyle
) {
    if (pContext == NULL) {
        return;
    }

    SendMessage(
        pContext->hListView,
        LVM_SETEXTENDEDLISTVIEWSTYLE, 0, dwStyle
    );
}

DWORD LvItemMoveContext_GetExtendedListViewStyle(PLVITEMMOVECONTEXT pContext)
{
    if (pContext == NULL) {
        return 0;
    }

    return (DWORD) SendMessage(
        pContext->hListView,
        LVM_GETEXTENDEDLISTVIEWSTYLE, 0, 0
    );
}