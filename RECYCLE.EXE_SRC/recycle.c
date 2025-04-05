#include <windows.h>
#include <tchar.h>
#include <math.h>

#include "itemmovectx.h"

#define MOVE_STEP                   5

/***********************************************************************
 * GetDesktopListView ("FolderView" SysListView32)
 ***********************************************************************/

/* https://stackoverflow.com/a/60856252 */
static BOOL CALLBACK FindDefView_EnumWindowsProc(HWND hWnd, LPARAM lParam)
{
    HWND *phShellDefViewWnd = (HWND*) lParam;
    HWND hNextWin = FindWindowExA(hWnd, 0, "SHELLDLL_DefView", 0);
    HWND hNextNextWin, hPrevNextWin;

    if (hNextWin == NULL) {
        return TRUE;
    }

    hNextNextWin = GetNextWindow(hNextWin, GW_HWNDNEXT);
    hPrevNextWin = GetNextWindow(hNextWin, GW_HWNDPREV);

    if ((hNextNextWin != NULL) || (hPrevNextWin != NULL)) {
        return TRUE;
    }

    *phShellDefViewWnd = hNextWin;
    return FALSE;
}

static HWND GetDesktopListView(VOID)
{
    HWND hDefViewWnd = NULL;

    EnumWindows(&FindDefView_EnumWindowsProc, (LPARAM) &hDefViewWnd);

    if (hDefViewWnd == NULL) {
        return NULL;
    }

    return FindWindowExA(hDefViewWnd, NULL, "SysListView32", NULL);
}

/***********************************************************************
 * Useful Functions
 ***********************************************************************/

static INT GetRecycleBinIndex(PLVITEMMOVECONTEXT pContext)
{
    LVITEM lvi = {0};
    TCHAR szBuffer[256] = {0};
    INT iCount;
    INT i;

    if (pContext == NULL) {
        return -1;
    }

    iCount = LvItemMoveContext_GetItemCount(pContext);

    lvi.mask = LVIF_TEXT;
    lvi.cchTextMax = 256;
    lvi.pszText = szBuffer;

    for (i = 0; i < iCount; ++i) {
        lvi.iItem = i;

        if (LvItemMoveContext_GetItem(pContext, &lvi) == FALSE) {
            continue;
        }

        if (_tcscmp(TEXT("Корзина"), szBuffer) == 0) {
            return i;
        }

        if (_tcscmp(TEXT("Recycle Bin"), szBuffer) == 0) {
            return i;
        }
    }

    return -1;
}

static VOID MovePointTowards(LPPOINT lpptCurrent, POINT ptTarget)
{
    DOUBLE dbDistance = 0.0;
    INT dx, dy;

    if (lpptCurrent == NULL) {
        return;
    }

    dx = ptTarget.x - lpptCurrent->x;
    dy = ptTarget.y - lpptCurrent->y;

    if (dx == 0 && dy == 0) {
        return;
    }

    dbDistance = sqrt(dx * dx + dy * dy);

    lpptCurrent->x += (INT) (MOVE_STEP * dx / dbDistance);
    lpptCurrent->y += (INT) (MOVE_STEP * dy / dbDistance);
}

/***********************************************************************
 * Main
 ***********************************************************************/

int WINAPI _tWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                     LPTSTR lpCmdLine, int nCmdShow)
{
    POINT ptItem, ptRecycleBin;
    PLVITEMMOVECONTEXT pContext;
    INT iCount, i;
    INT iRecycleBin, iForDelete = -1;

    pContext = LvItemMoveContext_CreateFor(GetDesktopListView());

    if (pContext == NULL) {
        return -1;
    }

    while (TRUE) {
        LvItemMoveContext_DisableSnapToGrid(pContext);
        
        iCount = LvItemMoveContext_GetItemCount(pContext);
        iRecycleBin = GetRecycleBinIndex(pContext);

        if (iRecycleBin == -1 || iCount <= 1) {
            break;
        }

        LvItemMoveContext_GetItemPosition(
            pContext, iRecycleBin, &ptRecycleBin
        );

        for (i = 0; i < iCount; ++i) {
            if (i == iRecycleBin) {
                continue;
            }

            LvItemMoveContext_GetItemPosition(pContext, i, &ptItem);
            MovePointTowards(&ptItem, ptRecycleBin);
            LvItemMoveContext_SetItemPositionPoint(pContext, i, &ptItem);
        }

        Sleep(5);
    }

    LvItemMoveContext_Delete(pContext);
    return 0;
}
