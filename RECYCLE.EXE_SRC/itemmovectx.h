#ifndef __ITEMMOVECTX_H
#define __ITEMMOVECTX_H

#include <windows.h>
#include <commctrl.h>

typedef struct tagLVITEMMOVECONTEXT {
    HANDLE hListViewProcess;
    HWND hListView;
    LPPOINT lpptRemote;
    LPRECT lprcRemote;
} LVITEMMOVECONTEXT, *PLVITEMMOVECONTEXT;

VOID LvItemMoveContext_Delete(PLVITEMMOVECONTEXT pContext);

PLVITEMMOVECONTEXT LvItemMoveContext_CreateFor(HWND hListView);

WINBOOL LvItemMoveContext_GetItemPosition(
    PLVITEMMOVECONTEXT pContext,
    INT i,
    LPPOINT lppt
);

WINBOOL LvItemMoveContext_SetItemPosition(
    PLVITEMMOVECONTEXT pContext,
    INT i,
    INT x, INT y
);

WINBOOL LvItemMoveContext_SetItemPositionPoint(
    PLVITEMMOVECONTEXT pContext,
    INT i,
    LPPOINT lppt
);

BOOL LvItemMoveContext_GetItem(PLVITEMMOVECONTEXT pContext, LPLVITEM lplvi);

INT LvItemMoveContext_GetItemCount(PLVITEMMOVECONTEXT pContext);

VOID LvItemMoveContext_SetExtendedListViewStyle(
    PLVITEMMOVECONTEXT pContext,
    DWORD dwStyle
);

DWORD LvItemMoveContext_GetExtendedListViewStyle(PLVITEMMOVECONTEXT pContext);

static inline VOID LvItemMoveContext_DisableSnapToGrid(
    PLVITEMMOVECONTEXT pContext
) {
    DWORD dwStyle;

    dwStyle = LvItemMoveContext_GetExtendedListViewStyle(pContext);
    dwStyle &= ~LVS_EX_SNAPTOGRID;
    
    LvItemMoveContext_SetExtendedListViewStyle(pContext, dwStyle);
}

#endif /* __ITEMMOVECTX_H */